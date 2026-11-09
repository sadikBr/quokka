#pragma once

#include "transport.h"

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QUrlQuery>

#include <functional>
#include <memory>

namespace quokka {

class ApiClient;

// One API request. Emits exactly one of finished() / failed(), unless aborted, then deletes
// itself. Connect with a context object so a destroyed receiver never gets called.
class ApiReply : public QObject
{
    Q_OBJECT
public:
    virtual void abort();
    bool isAborted() const { return m_aborted; }
    QUrl url() const { return m_request.url; }

signals:
    void finished(const QByteArray &body);
    // `code` is the server's machine-readable error (e.g. "incorrect_login"), when it sent one.
    void failed(const QString &message, int httpStatus, const QString &code = {});

protected:
    explicit ApiReply(QObject *parent); // for replies not tied to a single request
    bool m_aborted = false;

private:
    friend class ApiClient;
    ApiReply(ApiClient *client, TransportRequest request);

    QPointer<ApiClient> m_client;
    TransportRequest m_request;
    QPointer<QObject> m_handle; // transport handle while in flight
    int m_attempts = 0;
    bool m_inFlight = false;
};

// Waits for several requests and emits one combined body, e.g. a post plus its comments, or
// the pages of several communities merged into one feed.
class CompositeReply : public ApiReply
{
    Q_OBJECT
public:
    // `bodies` has one entry per part, in order; failed parts are empty.
    using Combiner = std::function<QByteArray(const QList<QByteArray> &bodies)>;

    // requireAll: fail as soon as any part fails. Otherwise fail only if every part fails.
    CompositeReply(const QList<ApiReply *> &parts, Combiner combine, bool requireAll,
                   QObject *parent = nullptr);
    void abort() override;

private:
    void partDone(int index, const QByteArray *body, const QString &error, int status);

    QList<QPointer<ApiReply>> m_parts;
    QList<QByteArray> m_bodies;
    QList<bool> m_done;
    Combiner m_combine;
    bool m_requireAll;
    int m_failures = 0;
    bool m_emitted = false;
};

// One request, then a follow-up built from its answer (e.g. fetch a token, then the data).
// Finishes or fails with the follow-up; fails if the first one does.
class ChainReply : public ApiReply
{
    Q_OBJECT
public:
    // `next` may return nullptr to finish with the first body as is.
    using Next = std::function<ApiReply *(const QByteArray &firstBody)>;
    ChainReply(ApiReply *first, Next next, QObject *parent = nullptr);
    void abort() override;

private:
    void forward(ApiReply *reply);
    QPointer<ApiReply> m_current;
    Next m_next;
};

// A reply that fails right away (asynchronously), for things a source doesn't offer.
class FailedReply : public ApiReply
{
    Q_OBJECT
public:
    explicit FailedReply(const QString &message, QObject *parent = nullptr);
};

// Entry point for all API calls. Owns the transport, queues requests, keeps a small
// number in flight, honours Reddit's rate-limit headers and retries on 429 / 5xx.
class ApiClient : public QObject
{
    Q_OBJECT
public:
    explicit ApiClient(std::unique_ptr<Transport> transport, QObject *parent = nullptr);
    ~ApiClient() override;

    // path: "/api/v3/post/list" or "/r/pics/hot"; the transport adds the base URL.
    ApiReply *get(const QString &path, const QUrlQuery &query = {});
    // JSON body requests. These are not retried (they may change something on the server).
    ApiReply *post(const QString &path, const QByteArray &json);
    ApiReply *put(const QString &path, const QByteArray &json);

    // Signed-in requests carry this token ("Authorization: Bearer ..."). Empty = anonymous.
    void setAuthToken(const QString &token) { m_token = token.toUtf8(); }
    bool hasAuthToken() const { return !m_token.isEmpty(); }

    // Readable text for a server error code such as Lemmy's {"error":"incorrect_login"}.
    static QString serverErrorMessage(const QString &code);

    // Name used in error messages ("lemmy.world", "Reddit").
    void setServiceName(const QString &name) { m_serviceName = name; }
    QString serviceName() const { return m_serviceName; }

    void setMaxConcurrent(int n) { m_maxConcurrent = qMax(1, n); }
    void setMaxAttempts(int n) { m_maxAttempts = qMax(1, n); }
    void setRetryDelayMs(int ms) { m_retryDelayMs = qMax(0, ms); }

    static QString defaultUserAgent();
    static QString errorMessageFor(int httpStatus, const QString &networkError,
                                   const QString &service = QStringLiteral("the server"));

signals:
    // A signed-in request was refused because the token is no longer valid.
    void authenticationExpired();

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
    QString m_serviceName = QStringLiteral("the server");
    QByteArray m_token;
    QTimer m_pauseTimer;
};

} // namespace quokka
