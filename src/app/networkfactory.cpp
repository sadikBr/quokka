#include "networkfactory.h"

#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QLoggingCategory>
#include <QStandardPaths>

Q_LOGGING_CATEGORY(lcNetwork, "quokka.network")

namespace {

// Adds our User-Agent to every request (QML Image requests included).
class UserAgentNam : public QNetworkAccessManager
{
public:
    UserAgentNam(QString ua, QObject *parent)
        : QNetworkAccessManager(parent)
        , m_ua(std::move(ua))
    {
    }

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &original,
                                 QIODevice *outgoingData) override
    {
        QNetworkRequest req(original);
        if (!req.hasRawHeader("User-Agent"))
            req.setHeader(QNetworkRequest::UserAgentHeader, m_ua);
        QNetworkReply *reply = QNetworkAccessManager::createRequest(op, req, outgoingData);
        // Name the URL when something fails, so problems can be traced (QNetworkAccessManager's
        // own messages don't say which request it was). Tokens are in headers, never in URLs.
        connect(reply, &QNetworkReply::finished, reply, [reply] {
            const auto error = reply->error();
            if (error == QNetworkReply::NoError || error == QNetworkReply::OperationCanceledError)
                return;
            // Images that fail are already reported by QML (and fall back to another copy).
            if (!reply->url().path().contains(QLatin1String("/api/")))
                return;
            qCWarning(lcNetwork).noquote()
                << "Request failed:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
                << reply->operation() << reply->url().toString(QUrl::RemoveUserInfo)
                << "-" << reply->errorString();
        });
        return reply;
    }

private:
    QString m_ua;
};

} // namespace

NetworkFactory::NetworkFactory(QString userAgent)
    : m_userAgent(std::move(userAgent))
{
}

QString NetworkFactory::cacheDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
        + QStringLiteral("/http");
}

QString NetworkFactory::animationDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
        + QStringLiteral("/animations");
}

QString NetworkFactory::frameDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
        + QStringLiteral("/frames");
}

QNetworkAccessManager *NetworkFactory::create(QObject *parent)
{
    auto *nam = new UserAgentNam(m_userAgent, parent);
    auto *cache = new QNetworkDiskCache(nam);
    cache->setCacheDirectory(cacheDirectory());
    cache->setMaximumCacheSize(150 * 1024 * 1024);
    nam->setCache(cache);
    return nam;
}
