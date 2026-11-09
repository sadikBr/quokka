#pragma once

#include <QByteArray>
#include <QDir>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>

#include <functional>

class QNetworkAccessManager;

namespace quokka {

struct TransportResponse
{
    int httpStatus = 0;
    QByteArray body;
    bool networkError = false;
    QString errorString;
    double rateLimitRemaining = -1; // from X-Ratelimit-Remaining, -1 when absent
    int rateLimitResetSecs = -1;    // from X-Ratelimit-Reset, -1 when absent
};

// How requests reach Reddit. The public ".json" transport is the default; an OAuth transport
// (oauth.reddit.com with a bearer token) can be added later behind the same interface.
class Transport
{
public:
    using Callback = std::function<void(const TransportResponse &)>;
    virtual ~Transport() = default;

    // Turns an API path such as "/r/pics/hot" plus a query into a full URL.
    virtual QUrl buildUrl(const QString &path, const QString &query) const = 0;

    // Starts the request. Returns a handle that can be passed to cancel(), or nullptr.
    virtual QObject *fetch(const QUrl &url, Callback done) = 0;
    virtual void cancel(QObject *handle) = 0;
};

// Talks to https://www.reddit.com/<path>.json
class PublicJsonTransport : public Transport
{
public:
    explicit PublicJsonTransport(QNetworkAccessManager *nam, QString userAgent);

    QUrl buildUrl(const QString &path, const QString &query) const override;
    QObject *fetch(const QUrl &url, Callback done) override;
    void cancel(QObject *handle) override;

private:
    QPointer<QNetworkAccessManager> m_nam;
    QString m_userAgent;
};

// Serves recorded responses from a directory, for tests and offline UI development.
// "/r/pics/hot?after=x" maps to "r_pics_hot.json"; when that file does not exist,
// "listing.json" is used for any listing request.
class FixtureTransport : public Transport
{
public:
    explicit FixtureTransport(QString directory);

    QUrl buildUrl(const QString &path, const QString &query) const override;
    QObject *fetch(const QUrl &url, Callback done) override;
    void cancel(QObject *handle) override;

    static QString fileNameForPath(const QString &path);

private:
    QDir m_dir;
};

} // namespace quokka
