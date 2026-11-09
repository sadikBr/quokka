#include "transport.h"

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace quokka {

HttpTransport::HttpTransport(QNetworkAccessManager *nam, QString userAgent, QUrl baseUrl,
                             QString pathSuffix)
    : m_nam(nam)
    , m_userAgent(std::move(userAgent))
    , m_base(std::move(baseUrl))
    , m_suffix(std::move(pathSuffix))
{
}

QUrl HttpTransport::buildUrl(const QString &path, const QString &query) const
{
    QString p = path;
    if (!p.startsWith(QLatin1Char('/')))
        p.prepend(QLatin1Char('/'));
    while (p.size() > 1 && p.endsWith(QLatin1Char('/')))
        p.chop(1);
    if (!m_suffix.isEmpty() && !p.endsWith(m_suffix))
        p += m_suffix;

    QUrl url(m_base);
    url.setPath(m_base.path() + p);
    url.setQuery(query);
    return url;
}

QObject *HttpTransport::fetch(const TransportRequest &request, Callback done)
{
    if (!m_nam) {
        TransportResponse r;
        r.networkError = true;
        r.errorString = QStringLiteral("Network unavailable");
        done(r);
        return nullptr;
    }

    QNetworkRequest req(request.url);
    req.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);
    req.setRawHeader("Accept", "application/json");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setTransferTimeout(20000);
    const bool personal = !request.bearerToken.isEmpty() || request.method != "GET";
    if (!request.bearerToken.isEmpty())
        req.setRawHeader("Authorization", "Bearer " + request.bearerToken);
    // Never cache signed-in or state-changing responses on disk.
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                     personal ? QNetworkRequest::AlwaysNetwork : QNetworkRequest::PreferNetwork);
    req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, !personal);

    QNetworkReply *reply = nullptr;
    if (request.method == "GET") {
        reply = m_nam->get(req);
    } else {
        req.setHeader(QNetworkRequest::ContentTypeHeader, QByteArrayLiteral("application/json"));
        reply = m_nam->sendCustomRequest(req, request.method, request.body);
    }
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, done = std::move(done)] {
        TransportResponse r;
        r.httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        r.body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError && r.httpStatus == 0) {
            r.networkError = true;
            r.errorString = reply->errorString();
        }
        bool ok = false;
        const double remaining = reply->rawHeader("x-ratelimit-remaining").toDouble(&ok);
        if (ok)
            r.rateLimitRemaining = remaining;
        const int reset = reply->rawHeader("x-ratelimit-reset").toInt(&ok);
        if (ok)
            r.rateLimitResetSecs = reset;
        reply->deleteLater();
        if (reply->error() != QNetworkReply::OperationCanceledError)
            done(r);
    });
    return reply;
}

void HttpTransport::cancel(QObject *handle)
{
    if (auto *reply = qobject_cast<QNetworkReply *>(handle))
        reply->abort();
}

FixtureTransport::FixtureTransport(QString directory)
    : m_dir(std::move(directory))
{
}

QUrl FixtureTransport::buildUrl(const QString &path, const QString &query) const
{
    QUrl url;
    url.setScheme(QStringLiteral("fixture"));
    url.setPath(path);
    url.setQuery(query);
    return url;
}

QString FixtureTransport::fileNameForPath(const QString &path)
{
    QString name = path;
    if (name.endsWith(QLatin1String(".json")))
        name.chop(5);
    while (name.startsWith(QLatin1Char('/')))
        name.remove(0, 1);
    while (name.endsWith(QLatin1Char('/')))
        name.chop(1);
    name.replace(QLatin1Char('/'), QLatin1Char('_'));
    return name.toLower() + QLatin1String(".json");
}

QObject *FixtureTransport::fetch(const TransportRequest &request, Callback done)
{
    const QUrl &url = request.url;
    QString path = url.path();
    QString file = m_dir.filePath(fileNameForPath(path));
    while (!QFile::exists(file) && path.count(QLatin1Char('/')) > 1) {
        path = path.left(path.lastIndexOf(QLatin1Char('/')));
        file = m_dir.filePath(fileNameForPath(path));
    }
    if (!QFile::exists(file))
        file = m_dir.filePath(QStringLiteral("listing.json"));

    TransportResponse r;
    QFile f(file);
    if (f.open(QIODevice::ReadOnly)) {
        r.httpStatus = 200;
        r.body = f.readAll();
    } else {
        r.httpStatus = 404;
        r.errorString = QStringLiteral("No fixture for %1").arg(url.path());
    }
    // Deliver asynchronously, like a real network reply.
    auto *handle = new QTimer;
    handle->setSingleShot(true);
    handle->setInterval(150);
    QObject::connect(handle, &QTimer::timeout, handle, [handle, r, done = std::move(done)] {
        done(r);
        handle->deleteLater();
    });
    handle->start();
    return handle;
}

void FixtureTransport::cancel(QObject *handle)
{
    if (auto *timer = qobject_cast<QTimer *>(handle)) {
        timer->stop();
        timer->deleteLater();
    }
}

} // namespace quokka
