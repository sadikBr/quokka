#pragma once

#include <QQmlNetworkAccessManagerFactory>
#include <QString>

class QNetworkAccessManager;

// Gives QML (images) and the API client network managers that share one on-disk HTTP cache
// and send our User-Agent.
class NetworkFactory : public QQmlNetworkAccessManagerFactory
{
public:
    explicit NetworkFactory(QString userAgent);
    QNetworkAccessManager *create(QObject *parent) override;

    static QString cacheDirectory();
    // Checked local copies of animated GIFs (see MediaResolver::prepareAnimation).
    static QString animationDirectory();

private:
    QString m_userAgent;
};
