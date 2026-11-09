#include "mediaresolver.h"

#include "apiclient.h"
#include "medialinks.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

MediaResolver::MediaResolver(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

QNetworkReply *MediaResolver::get(const QUrl &url, const QByteArray &bearer)
{
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, quokka::ApiClient::defaultUserAgent());
    req.setRawHeader("Accept", "application/json");
    req.setTransferTimeout(15000);
    if (!bearer.isEmpty())
        req.setRawHeader("Authorization", "Bearer " + bearer);
    return m_nam->get(req);
}

QString MediaResolver::redgifsId(const QString &pageUrl)
{
    // https://www.redgifs.com/watch/<id>  or  /ifr/<id>; ids are lower-case words.
    const QStringList parts = QUrl(pageUrl).path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    const int i = int(parts.indexOf(QStringLiteral("watch"))) >= 0 ? int(parts.indexOf(QStringLiteral("watch")))
                                                                     : int(parts.indexOf(QStringLiteral("ifr")));
    if (i < 0 || i + 1 >= parts.size())
        return {};
    QString id = parts.at(i + 1).toLower();
    const int dash = id.indexOf(QLatin1Char('-')); // "<id>-some-title"
    return dash > 0 ? id.left(dash) : id;
}

QString MediaResolver::streamableCode(const QString &pageUrl)
{
    const QStringList parts = QUrl(pageUrl).path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    return parts.isEmpty() ? QString() : parts.last();
}

QString MediaResolver::parseRedgifs(const QByteArray &json)
{
    const QJsonObject urls = QJsonDocument::fromJson(json).object().value(QLatin1String("gif")).toObject()
                                 .value(QLatin1String("urls")).toObject();
    const QString hd = urls.value(QLatin1String("hd")).toString();
    return hd.isEmpty() ? urls.value(QLatin1String("sd")).toString() : hd;
}

QString MediaResolver::parseStreamable(const QByteArray &json)
{
    const QJsonObject files = QJsonDocument::fromJson(json).object().value(QLatin1String("files")).toObject();
    QString url = files.value(QLatin1String("mp4")).toObject().value(QLatin1String("url")).toString();
    if (url.isEmpty())
        url = files.value(QLatin1String("mp4-mobile")).toObject().value(QLatin1String("url")).toString();
    if (url.startsWith(QLatin1String("//")))
        url.prepend(QLatin1String("https:"));
    return url;
}

void MediaResolver::finish(const QString &pageUrl, const QString &mediaUrl, const QString &error)
{
    if (!mediaUrl.isEmpty()) {
        m_cache.insert(pageUrl, mediaUrl);
        emit resolved(pageUrl, mediaUrl);
    } else {
        emit failed(pageUrl, error.isEmpty() ? tr("Couldn't find the video on that page.") : error);
    }
}

void MediaResolver::resolve(const QString &pageUrl, const QString &kind)
{
    if (const QString hit = m_cache.value(pageUrl); !hit.isEmpty()) {
        QMetaObject::invokeMethod(this, [this, pageUrl, hit] { emit resolved(pageUrl, hit); }, Qt::QueuedConnection);
        return;
    }
    if (kind == QLatin1String("redgifs")) {
        resolveRedgifs(pageUrl, false);
    } else if (kind == QLatin1String("streamable")) {
        const QString code = streamableCode(pageUrl);
        QNetworkReply *reply = get(QUrl(QStringLiteral("https://api.streamable.com/videos/") + code));
        connect(reply, &QNetworkReply::finished, this, [this, reply, pageUrl] {
            reply->deleteLater();
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            finish(pageUrl, reply->error() == QNetworkReply::NoError ? parseStreamable(reply->readAll()) : QString(),
                   describeFailure(status, pageUrl));
        });
    } else {
        QMetaObject::invokeMethod(this, [this, pageUrl] { finish(pageUrl, {}, tr("Unsupported video page.")); },
                                  Qt::QueuedConnection);
    }
}

void MediaResolver::resolveRedgifs(const QString &pageUrl, bool retried)
{
    const QString id = redgifsId(pageUrl);
    if (id.isEmpty()) {
        finish(pageUrl, {}, {});
        return;
    }
    auto fetchGif = [this, pageUrl, id, retried] {
        QNetworkReply *reply = get(QUrl(QStringLiteral("https://api.redgifs.com/v2/gifs/") + id), m_redgifsToken);
        connect(reply, &QNetworkReply::finished, this, [this, reply, pageUrl, retried] {
            reply->deleteLater();
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if ((status == 401 || status == 403) && !retried) {
                m_redgifsToken.clear(); // expired: get a new one and try once more
                resolveRedgifs(pageUrl, true);
                return;
            }
            finish(pageUrl, reply->error() == QNetworkReply::NoError ? parseRedgifs(reply->readAll()) : QString(),
                   describeFailure(status, pageUrl));
        });
    };
    if (!m_redgifsToken.isEmpty()) {
        fetchGif();
        return;
    }
    // redgifs hands out anonymous temporary tokens for its public API.
    QNetworkReply *auth = get(QUrl(QStringLiteral("https://api.redgifs.com/v2/auth/temporary")));
    connect(auth, &QNetworkReply::finished, this, [this, auth, pageUrl, fetchGif] {
        auth->deleteLater();
        m_redgifsToken = QJsonDocument::fromJson(auth->readAll()).object()
                             .value(QLatin1String("token")).toString().toUtf8();
        if (m_redgifsToken.isEmpty()) {
            finish(pageUrl, {}, tr("redgifs is not answering right now."));
            return;
        }
        fetchGif();
    });
}

namespace {
// A server whose images failed this many times without a single success is treated as down.
constexpr int kDeadHostFailures = 6;
}

QStringList MediaResolver::imageSources(const QString &primary, const QString &fallback) const
{
    QStringList out;
    for (const QString &u : quokka::media::imageSources(primary, fallback)) {
        const QString host = QUrl(u).host();
        if (m_deadUrls.contains(u)
            || (!m_hostsOk.contains(host) && m_hostFailures.value(host) >= kDeadHostFailures))
            continue;
        out.append(u);
    }
    return out;
}

void MediaResolver::reportImage(const QString &url, bool ok)
{
    const QString host = QUrl(url).host();
    if (url.isEmpty() || host.isEmpty())
        return;
    if (ok) {
        m_hostsOk.insert(host);
        m_deadUrls.remove(url);
    } else if (!m_deadUrls.contains(url)) {
        m_deadUrls.insert(url);
        m_hostFailures[host] += 1;
    }
}

QString MediaResolver::describeFailure(int status, const QString &url)
{
    QString host = QUrl(url).host();
    if (host.startsWith(QLatin1String("www.")) || host.startsWith(QLatin1String("api.")))
        host = host.mid(4);
    if (status == 404 || status == 410)
        return tr("This video was deleted from %1.").arg(host);
    if (status == 401 || status == 403)
        return tr("%1 refused to share this video.").arg(host);
    if (status == 429)
        return tr("%1 is limiting requests. Try again in a minute.").arg(host);
    if (status >= 500)
        return tr("%1 is having trouble right now (error %2). Try again later.").arg(host).arg(status);
    return {};
}

QString MediaResolver::describePlaybackError(const QString &message, const QString &url)
{
    const QString host = QUrl(url).host();
    const QString m = message.toLower();
    if (m.contains(QLatin1String("5xx")) || m.contains(QLatin1String("server error"))
        || m.contains(QLatin1String("bad gateway")))
        return tr("The server hosting this video (%1) is having trouble right now. Try again later.").arg(host);
    if (m.contains(QLatin1String("404")) || m.contains(QLatin1String("not found")) || m.contains(QLatin1String("410")))
        return tr("This video was deleted from %1.").arg(host);
    if (m.contains(QLatin1String("403")) || m.contains(QLatin1String("forbidden")))
        return tr("%1 refused to share this video.").arg(host);
    if (m.isEmpty() || m.contains(QLatin1String("could not open")) || m.contains(QLatin1String("resource"))
        || m.contains(QLatin1String("enough data")))
        return tr("Couldn't load this video from %1. The server may be down, or the video deleted.").arg(host);
    return tr("Can't play this video: %1").arg(message);
}
