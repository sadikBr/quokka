#include "apiclient.h"

#include <QCoreApplication>
#include <QSysInfo>

namespace quokka {

ApiReply::ApiReply(ApiClient *client, QUrl url)
    : QObject(client)
    , m_client(client)
    , m_url(std::move(url))
{
}

ApiReply::ApiReply(QObject *parent)
    : QObject(parent)
{
}

void ApiReply::abort()
{
    if (m_aborted)
        return;
    m_aborted = true;
    if (m_client)
        m_client->cancel(this);
    deleteLater();
}

ApiClient::ApiClient(std::unique_ptr<Transport> transport, QObject *parent)
    : QObject(parent)
    , m_transport(std::move(transport))
{
    m_pauseTimer.setSingleShot(true);
    connect(&m_pauseTimer, &QTimer::timeout, this, &ApiClient::pump);
}

ApiClient::~ApiClient() = default;

QString ApiClient::defaultUserAgent()
{
    // A descriptive User-Agent, as both Reddit and Lemmy admins ask for.
    const QString version = QCoreApplication::applicationVersion().isEmpty()
        ? QStringLiteral("0.0.0")
        : QCoreApplication::applicationVersion();
    return QStringLiteral("Quokka/%1 (%2; open-source Qt client; +https://github.com/sadikBr/Reddit-Client)")
        .arg(version, QSysInfo::productType());
}

QString ApiClient::errorMessageFor(int httpStatus, const QString &networkError, const QString &service)
{
    switch (httpStatus) {
    case 0:
        return networkError.isEmpty() ? QStringLiteral("Could not reach %1.").arg(service)
                                      : QStringLiteral("Could not reach %1: %2").arg(service, networkError);
    case 401:
    case 403:
        return QStringLiteral("%1 refused the request (%2). The content may be private, or the server "
                              "requires signing in.").arg(service).arg(httpStatus);
    case 404:
        return QStringLiteral("Nothing here (404). It may not exist, or was removed.");
    case 429:
        return QStringLiteral("%1 says we are going too fast. Try again in a minute.").arg(service);
    default:
        if (httpStatus >= 500)
            return QStringLiteral("%1 is having trouble right now (%2).").arg(service).arg(httpStatus);
        return QStringLiteral("Unexpected response from %1 (%2).").arg(service).arg(httpStatus);
    }
}

ApiReply *ApiClient::get(const QString &path, const QUrlQuery &query)
{
    auto *reply = new ApiReply(this, m_transport->buildUrl(path, query.toString(QUrl::FullyEncoded)));
    enqueue(reply);
    return reply;
}

void ApiClient::enqueue(ApiReply *reply)
{
    m_queue.append(reply);
    // Pump from the event loop so callers can connect to the reply's signals first.
    QMetaObject::invokeMethod(this, &ApiClient::pump, Qt::QueuedConnection);
}

void ApiClient::pump()
{
    if (m_pauseTimer.isActive())
        return;
    while (m_inFlight < m_maxConcurrent && !m_queue.isEmpty()) {
        QPointer<ApiReply> next = m_queue.takeFirst();
        if (!next || next->isAborted())
            continue;
        dispatch(next);
    }
}

void ApiClient::dispatch(ApiReply *reply)
{
    ++m_inFlight;
    ++reply->m_attempts;
    reply->m_inFlight = true;
    QPointer<ApiReply> guard(reply);
    // Transports never invoke the callback after cancel(), so the in-flight count is
    // decremented either here or in cancel(), never both.
    QObject *handle = m_transport->fetch(reply->m_url, [this, guard](const TransportResponse &r) {
        --m_inFlight;
        if (guard) {
            guard->m_inFlight = false;
            guard->m_handle = nullptr;
            if (!guard->isAborted())
                handleResponse(guard, r);
        }
        pump();
    });
    if (guard && guard->m_inFlight)
        guard->m_handle = handle;
}

void ApiClient::handleResponse(ApiReply *reply, const TransportResponse &r)
{
    // Respect the rate-limit budget before sending anything else.
    if (r.rateLimitRemaining >= 0 && r.rateLimitRemaining < 1.0 && r.rateLimitResetSecs > 0)
        pauseFor(r.rateLimitResetSecs * 1000);

    const bool retryable = r.networkError || r.httpStatus == 429 || r.httpStatus >= 500;
    if (retryable && reply->m_attempts < m_maxAttempts) {
        const int delay = r.httpStatus == 429 && r.rateLimitResetSecs > 0
            ? r.rateLimitResetSecs * 1000
            : m_retryDelayMs * reply->m_attempts;
        pauseFor(delay);
        m_queue.prepend(reply);
        return;
    }

    if (!r.networkError && r.httpStatus >= 200 && r.httpStatus < 300)
        emit reply->finished(r.body);
    else
        emit reply->failed(errorMessageFor(r.networkError ? 0 : r.httpStatus, r.errorString, m_serviceName),
                           r.httpStatus);
    reply->deleteLater();
}

void ApiClient::pauseFor(int ms)
{
    if (ms <= 0)
        return;
    if (!m_pauseTimer.isActive() || m_pauseTimer.remainingTime() < ms)
        m_pauseTimer.start(ms);
}

void ApiClient::cancel(ApiReply *reply)
{
    m_queue.removeAll(QPointer<ApiReply>(reply));
    if (reply->m_inFlight) {
        reply->m_inFlight = false;
        --m_inFlight;
        if (reply->m_handle)
            m_transport->cancel(reply->m_handle);
        reply->m_handle = nullptr;
        QMetaObject::invokeMethod(this, &ApiClient::pump, Qt::QueuedConnection);
    }
}

CompositeReply::CompositeReply(const QList<ApiReply *> &parts, Combiner combine, bool requireAll,
                               QObject *parent)
    : ApiReply(parent)
    , m_bodies(parts.size())
    , m_done(parts.size(), false)
    , m_combine(std::move(combine))
    , m_requireAll(requireAll)
{
    for (int i = 0; i < parts.size(); ++i) {
        ApiReply *part = parts.at(i);
        m_parts.append(part);
        connect(part, &ApiReply::finished, this, [this, i](const QByteArray &body) {
            partDone(i, &body, {}, 200);
        });
        connect(part, &ApiReply::failed, this, [this, i](const QString &message, int status) {
            partDone(i, nullptr, message, status);
        });
    }
    if (parts.isEmpty()) {
        QMetaObject::invokeMethod(this, [this] {
            if (m_aborted || m_emitted)
                return;
            m_emitted = true;
            emit finished(m_combine({}));
            deleteLater();
        }, Qt::QueuedConnection);
    }
}

void CompositeReply::abort()
{
    if (m_aborted)
        return;
    m_aborted = true;
    for (const auto &p : std::as_const(m_parts))
        if (p)
            p->abort();
    deleteLater();
}

void CompositeReply::partDone(int index, const QByteArray *body, const QString &error, int status)
{
    if (m_aborted || m_emitted)
        return;
    m_done[index] = true;
    if (body) {
        m_bodies[index] = *body;
    } else {
        ++m_failures;
        if (m_requireAll || m_failures == m_parts.size()) {
            m_emitted = true;
            for (const auto &p : std::as_const(m_parts))
                if (p && p != m_parts.at(index))
                    p->abort();
            emit failed(error, status);
            deleteLater();
            return;
        }
    }
    if (m_done.contains(false))
        return;
    m_emitted = true;
    emit finished(m_combine(m_bodies));
    deleteLater();
}

} // namespace quokka
