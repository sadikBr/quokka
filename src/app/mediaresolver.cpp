#include "mediaresolver.h"

#include "apiclient.h"

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
            finish(pageUrl, reply->error() == QNetworkReply::NoError ? parseStreamable(reply->readAll()) : QString(),
                   reply->errorString());
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
                   reply->errorString());
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
