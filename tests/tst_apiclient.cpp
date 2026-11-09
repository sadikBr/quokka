#include "apiclient.h"

#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <QUrlQuery>

using namespace quokka;

// Answers requests from a scripted list of responses, asynchronously.
class FakeTransport : public Transport
{
public:
    QList<TransportResponse> script;
    QList<QUrl> requested;
    QList<TransportRequest> requests;
    int cancelled = 0;

    QUrl buildUrl(const QString &path, const QString &query) const override
    {
        QUrl u(QStringLiteral("https://example.test"));
        u.setPath(path + QStringLiteral(".json"));
        u.setQuery(query);
        return u;
    }

    QObject *fetch(const TransportRequest &request, Callback done) override
    {
        requested.append(request.url);
        requests.append(request);
        const TransportResponse r = script.isEmpty() ? TransportResponse{200, "{}"} : script.takeFirst();
        auto *t = new QTimer;
        t->setSingleShot(true);
        QObject::connect(t, &QTimer::timeout, t, [t, r, done] { done(r); t->deleteLater(); });
        t->start(5);
        return t;
    }

    void cancel(QObject *handle) override
    {
        ++cancelled;
        if (auto *t = qobject_cast<QTimer *>(handle)) {
            t->stop();
            t->deleteLater();
        }
    }
};

class TestApiClient : public QObject
{
    Q_OBJECT

private slots:
    void sendsQueryAndSucceeds()
    {
        auto *fake = new FakeTransport;
        fake->script = {TransportResponse{200, R"({"ok":true})"}};
        ApiClient client{std::unique_ptr<Transport>(fake)};

        QUrlQuery q;
        q.addQueryItem("limit", "5");
        ApiReply *reply = client.get("/r/pics/hot", q);
        QSignalSpy ok(reply, &ApiReply::finished);
        QVERIFY(ok.wait(1000));
        QCOMPARE(ok.first().first().toByteArray(), QByteArray(R"({"ok":true})"));

        QCOMPARE(fake->requested.size(), 1);
        const QUrlQuery sent(fake->requested.first());
        QCOMPARE(sent.queryItemValue("limit"), QStringLiteral("5"));
    }

    void retriesServerErrorsThenSucceeds()
    {
        auto *fake = new FakeTransport;
        fake->script = {TransportResponse{503, {}}, TransportResponse{200, "{}"}};
        ApiClient client{std::unique_ptr<Transport>(fake)};
        client.setRetryDelayMs(10);

        ApiReply *reply = client.get("/r/pics/hot");
        QSignalSpy ok(reply, &ApiReply::finished);
        QVERIFY(ok.wait(2000));
        QCOMPARE(fake->requested.size(), 2);
    }

    void givesUpAfterMaxAttempts()
    {
        auto *fake = new FakeTransport;
        fake->script = {TransportResponse{500, {}}, TransportResponse{500, {}}};
        ApiClient client{std::unique_ptr<Transport>(fake)};
        client.setRetryDelayMs(5);
        client.setMaxAttempts(2);

        ApiReply *reply = client.get("/r/pics/hot");
        QSignalSpy failed(reply, &ApiReply::failed);
        QVERIFY(failed.wait(2000));
        QCOMPARE(failed.first().at(1).toInt(), 500);
        QCOMPARE(fake->requested.size(), 2);
    }

    void doesNotRetryClientErrors()
    {
        auto *fake = new FakeTransport;
        fake->script = {TransportResponse{404, {}}};
        ApiClient client{std::unique_ptr<Transport>(fake)};

        ApiReply *reply = client.get("/r/doesnotexist/hot");
        QSignalSpy failed(reply, &ApiReply::failed);
        QVERIFY(failed.wait(1000));
        QCOMPARE(failed.first().at(1).toInt(), 404);
        QVERIFY(failed.first().at(0).toString().contains("404"));
        QCOMPARE(fake->requested.size(), 1);
    }

    void limitsConcurrency()
    {
        auto *fake = new FakeTransport;
        ApiClient client{std::unique_ptr<Transport>(fake)};
        client.setMaxConcurrent(2);

        QList<ApiReply *> replies;
        for (int i = 0; i < 5; ++i)
            replies.append(client.get(QStringLiteral("/r/s%1/hot").arg(i)));
        QSignalSpy last(replies.last(), &ApiReply::finished);
        QCoreApplication::processEvents();
        QCOMPARE(fake->requested.size(), 2);
        QVERIFY(last.wait(2000));
        QCOMPARE(fake->requested.size(), 5);
    }

    void abortCancelsAndFreesSlot()
    {
        auto *fake = new FakeTransport;
        ApiClient client{std::unique_ptr<Transport>(fake)};
        client.setMaxConcurrent(1);

        ApiReply *first = client.get("/r/a/hot");
        QSignalSpy firstOk(first, &ApiReply::finished);
        ApiReply *second = client.get("/r/b/hot");
        QSignalSpy secondOk(second, &ApiReply::finished);
        QCoreApplication::processEvents(); // first is now in flight
        QCOMPARE(fake->requested.size(), 1);

        first->abort();
        QVERIFY(secondOk.wait(1000));
        QCOMPARE(firstOk.count(), 0);
        QCOMPARE(fake->cancelled, 1);
    }

    void sendsTokenAndJsonBodies()
    {
        auto *fake = new FakeTransport;
        ApiClient client{std::unique_ptr<Transport>(fake)};

        ApiReply *anon = client.get("/api/v3/site");
        QSignalSpy anonDone(anon, &ApiReply::finished);
        QVERIFY(anonDone.wait(1000));
        QVERIFY(fake->requests.last().bearerToken.isEmpty());

        client.setAuthToken("secret-token");
        ApiReply *post = client.post("/api/v3/community/follow", R"({"community_id":5,"follow":true})");
        QSignalSpy postDone(post, &ApiReply::finished);
        QVERIFY(postDone.wait(1000));
        QCOMPARE(fake->requests.last().method, QByteArray("POST"));
        QCOMPARE(fake->requests.last().bearerToken, QByteArray("secret-token"));
        QCOMPARE(fake->requests.last().body, QByteArray(R"({"community_id":5,"follow":true})"));

        ApiReply *put = client.put("/api/v3/user/save_user_settings", R"({"show_nsfw":true})");
        QSignalSpy putDone(put, &ApiReply::finished);
        QVERIFY(putDone.wait(1000));
        QCOMPARE(fake->requests.last().method, QByteArray("PUT"));
    }

    void doesNotRetryPosts()
    {
        auto *fake = new FakeTransport;
        fake->script = {TransportResponse{503, {}}, TransportResponse{200, "{}"}};
        ApiClient client{std::unique_ptr<Transport>(fake)};
        client.setRetryDelayMs(5);
        ApiReply *reply = client.post("/api/v3/user/login", "{}");
        QSignalSpy failed(reply, &ApiReply::failed);
        QVERIFY(failed.wait(1000));
        QCOMPARE(fake->requested.size(), 1);
    }

    void readsServerErrorCodes()
    {
        auto *fake = new FakeTransport;
        fake->script = {TransportResponse{400, R"({"error":"incorrect_login"})"}};
        ApiClient client{std::unique_ptr<Transport>(fake)};
        ApiReply *reply = client.post("/api/v3/user/login", "{}");
        QSignalSpy failed(reply, &ApiReply::failed);
        QVERIFY(failed.wait(1000));
        QCOMPARE(failed.first().at(0).toString(), QStringLiteral("Wrong username or password."));
        QCOMPARE(failed.first().at(2).toString(), QStringLiteral("incorrect_login"));
    }

    void reportsExpiredSessions()
    {
        auto *fake = new FakeTransport;
        fake->script = {TransportResponse{400, R"({"error":"not_logged_in"})"},
                        TransportResponse{401, {}}};
        ApiClient client{std::unique_ptr<Transport>(fake)};
        QSignalSpy expired(&client, &ApiClient::authenticationExpired);

        // Anonymous requests never count as an expired session.
        ApiReply *anon = client.get("/api/v3/post/list");
        QSignalSpy anonFailed(anon, &ApiReply::failed);
        QVERIFY(anonFailed.wait(1000));
        QCOMPARE(expired.count(), 0);

        client.setAuthToken("old");
        ApiReply *reply = client.get("/api/v3/site");
        QSignalSpy failed(reply, &ApiReply::failed);
        QVERIFY(failed.wait(1000));
        QCOMPARE(expired.count(), 1);
    }

    void pausesWhenRateLimitIsExhausted()
    {
        auto *fake = new FakeTransport;
        TransportResponse exhausted{200, "{}"};
        exhausted.rateLimitRemaining = 0;
        exhausted.rateLimitResetSecs = 1;
        fake->script = {exhausted};
        ApiClient client{std::unique_ptr<Transport>(fake)};

        ApiReply *a = client.get("/r/a/hot");
        QSignalSpy aOk(a, &ApiReply::finished);
        QVERIFY(aOk.wait(1000));

        ApiReply *b = client.get("/r/b/hot");
        QSignalSpy bOk(b, &ApiReply::finished);
        QTest::qWait(300);
        QCOMPARE(fake->requested.size(), 1); // still paused
        QVERIFY(bOk.wait(2000));
        QCOMPARE(fake->requested.size(), 2);
    }
};

QTEST_GUILESS_MAIN(TestApiClient)
#include "tst_apiclient.moc"
