#include "transport.h"

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace quokka {

PublicJsonTransport::PublicJsonTransport(QNetworkAccessManager *nam, QString userAgent)
    : m_nam(nam)
    , m_userAgent(std::move(userAgent))
{
}

QUrl PublicJsonTransport::buildUrl(const QString &path, const QString &query) const
{
    QString p = path;
    if (!p.startsWith(QLatin1Char('/')))
        p.prepend(QLatin1Char('/'));
    while (p.size() > 1 && p.endsWith(QLatin1Char('/')))
        p.chop(1);
    if (!p.endsWith(QLatin1String(".json")))
        p += QLatin1String(".json");

    QUrl url(QStringLiteral("https://www.reddit.com"));
    url.setPath(p);
    url.setQuery(query);
    return url;
}

QObject *PublicJsonTransport::fetch(const QUrl &url, Callback done)
{
    if (!m_nam) {
        TransportResponse r;
        r.networkError = true;
        r.errorString = QStringLiteral("Network unavailable");
        done(r);
        return nullptr;
    }

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);
    req.setRawHeader("Accept", "application/json");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                     QNetworkRequest::PreferNetwork);
    req.setTransferTimeout(20000);

    QNetworkReply *reply = m_nam->get(req);
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

void PublicJsonTransport::cancel(QObject *handle)
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

QObject *FixtureTransport::fetch(const QUrl &url, Callback done)
{
    QString file = m_dir.filePath(fileNameForPath(url.path()));
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
