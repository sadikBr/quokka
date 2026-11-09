#include "redditclient.h"

#include <QCoreApplication>
#include <QSysInfo>

namespace quokka {

ApiReply::ApiReply(RedditClient *client, QUrl url)
    : QObject(client)
    , m_client(client)
    , m_url(std::move(url))
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

RedditClient::RedditClient(std::unique_ptr<Transport> transport, QObject *parent)
    : QObject(parent)
    , m_transport(std::move(transport))
{
    m_pauseTimer.setSingleShot(true);
    connect(&m_pauseTimer, &QTimer::timeout, this, &RedditClient::pump);
}

RedditClient::~RedditClient() = default;

QString RedditClient::defaultUserAgent()
{
    // Reddit asks for "<platform>:<app id>:<version> (by /u/<user>)". We have no user, so we
    // point at the project instead. Generic user agents get throttled hard.
    const QString version = QCoreApplication::applicationVersion().isEmpty()
        ? QStringLiteral("0.0.0")
        : QCoreApplication::applicationVersion();
    return QStringLiteral("%1:io.github.sadikbr.quokka:v%2 (open-source Qt client; "
                          "+https://github.com/sadikBr/Reddit-Client)")
        .arg(QSysInfo::productType(), version);
}

QString RedditClient::errorMessageFor(int httpStatus, const QString &networkError)
{
    switch (httpStatus) {
    case 0:
        return networkError.isEmpty() ? QStringLiteral("Could not reach Reddit.")
                                      : QStringLiteral("Could not reach Reddit: %1").arg(networkError);
    case 403:
        return QStringLiteral("Reddit refused the request (403). The community may be private, "
                              "or Reddit is blocking anonymous access from this network.");
    case 404:
        return QStringLiteral("Nothing here (404). The community may not exist or was banned.");
    case 429:
        return QStringLiteral("Reddit says we are going too fast. Try again in a minute.");
    default:
        if (httpStatus >= 500)
            return QStringLiteral("Reddit is having trouble right now (%1).").arg(httpStatus);
        return QStringLiteral("Unexpected response from Reddit (%1).").arg(httpStatus);
    }
}

ApiReply *RedditClient::get(const QString &path, QUrlQuery query)
{
    query.removeAllQueryItems(QStringLiteral("raw_json"));
    query.addQueryItem(QStringLiteral("raw_json"), QStringLiteral("1"));
    auto *reply = new ApiReply(this, m_transport->buildUrl(path, query.toString(QUrl::FullyEncoded)));
    enqueue(reply);
    return reply;
}

void RedditClient::enqueue(ApiReply *reply)
{
    m_queue.append(reply);
    // Pump from the event loop so callers can connect to the reply's signals first.
    QMetaObject::invokeMethod(this, &RedditClient::pump, Qt::QueuedConnection);
}

void RedditClient::pump()
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

void RedditClient::dispatch(ApiReply *reply)
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

void RedditClient::handleResponse(ApiReply *reply, const TransportResponse &r)
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
        emit reply->failed(errorMessageFor(r.networkError ? 0 : r.httpStatus, r.errorString),
                           r.httpStatus);
    reply->deleteLater();
}

void RedditClient::pauseFor(int ms)
{
    if (ms <= 0)
        return;
    if (!m_pauseTimer.isActive() || m_pauseTimer.remainingTime() < ms)
        m_pauseTimer.start(ms);
}

void RedditClient::cancel(ApiReply *reply)
{
    m_queue.removeAll(QPointer<ApiReply>(reply));
    if (reply->m_inFlight) {
        reply->m_inFlight = false;
        --m_inFlight;
        if (reply->m_handle)
            m_transport->cancel(reply->m_handle);
        reply->m_handle = nullptr;
        QMetaObject::invokeMethod(this, &RedditClient::pump, Qt::QueuedConnection);
    }
}

} // namespace quokka
