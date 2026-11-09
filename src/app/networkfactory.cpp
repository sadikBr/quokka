#include "networkfactory.h"

#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>

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
        return QNetworkAccessManager::createRequest(op, req, outgoingData);
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

QNetworkAccessManager *NetworkFactory::create(QObject *parent)
{
    auto *nam = new UserAgentNam(m_userAgent, parent);
    auto *cache = new QNetworkDiskCache(nam);
    cache->setCacheDirectory(cacheDirectory());
    cache->setMaximumCacheSize(150 * 1024 * 1024);
    nam->setCache(cache);
    return nam;
}
