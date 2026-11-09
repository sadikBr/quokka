#pragma once

#include "transport.h"

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QUrlQuery>

#include <memory>

namespace quokka {

class RedditClient;

// One API request. Emits exactly one of finished() / failed(), unless aborted, then deletes
// itself. Connect with a context object so a destroyed receiver never gets called.
class ApiReply : public QObject
{
    Q_OBJECT
public:
    void abort();
    bool isAborted() const { return m_aborted; }
    QUrl url() const { return m_url; }

signals:
    void finished(const QByteArray &body);
    void failed(const QString &message, int httpStatus);

private:
    friend class RedditClient;
    ApiReply(RedditClient *client, QUrl url);

    QPointer<RedditClient> m_client;
    QUrl m_url;
    QPointer<QObject> m_handle; // transport handle while in flight
    int m_attempts = 0;
    bool m_inFlight = false;
    bool m_aborted = false;
};

// Entry point for all Reddit API calls. Owns the transport, queues requests, keeps a small
// number in flight, honours Reddit's rate-limit headers and retries on 429 / 5xx.
class RedditClient : public QObject
{
    Q_OBJECT
public:
    explicit RedditClient(std::unique_ptr<Transport> transport, QObject *parent = nullptr);
    ~RedditClient() override;

    // path: "/r/pics/hot" (no ".json"). raw_json=1 is always added.
    ApiReply *get(const QString &path, QUrlQuery query = {});

    void setMaxConcurrent(int n) { m_maxConcurrent = qMax(1, n); }
    void setMaxAttempts(int n) { m_maxAttempts = qMax(1, n); }
    void setRetryDelayMs(int ms) { m_retryDelayMs = qMax(0, ms); }

    static QString defaultUserAgent();
    static QString errorMessageFor(int httpStatus, const QString &networkError);

private:
    friend class ApiReply;
    void enqueue(ApiReply *reply);
    void pump();
    void dispatch(ApiReply *reply);
    void handleResponse(ApiReply *reply, const TransportResponse &response);
    void pauseFor(int ms);
    void cancel(ApiReply *reply);

    std::unique_ptr<Transport> m_transport;
    QList<QPointer<ApiReply>> m_queue;
    int m_inFlight = 0;
    int m_maxConcurrent = 2;
    int m_maxAttempts = 3;
    int m_retryDelayMs = 4000;
    QTimer m_pauseTimer;
};

} // namespace quokka
