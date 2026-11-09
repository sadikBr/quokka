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

// Plain HTTPS GET against a base URL, e.g. https://lemmy.world + "/api/v3/post/list", or
// https://www.reddit.com + "/r/pics/hot" + ".json".
class HttpTransport : public Transport
{
public:
    HttpTransport(QNetworkAccessManager *nam, QString userAgent, QUrl baseUrl,
                  QString pathSuffix = {});

    QUrl buildUrl(const QString &path, const QString &query) const override;
    QObject *fetch(const QUrl &url, Callback done) override;
    void cancel(QObject *handle) override;

private:
    QPointer<QNetworkAccessManager> m_nam;
    QString m_userAgent;
    QUrl m_base;
    QString m_suffix;
};

// Serves recorded responses from a directory, for tests and offline UI development.
// "/api/v3/post/list?x=y" maps to "api_v3_post_list.json"; when that file does not exist the
// last path segment is dropped and tried again ("api_v3_post.json", ...), then "listing.json".
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
