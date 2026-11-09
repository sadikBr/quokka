#include "apiclient.h"
#include "mediasites.h"

#include <QFile>
#include <QSignalSpy>
#include <QTest>

using namespace quokka;

class TestMediaSites : public QObject
{
    Q_OBJECT

    static QByteArray fixture(const char *name)
    {
        QFile f(QStringLiteral(QUOKKA_FIXTURES_DIR "/sites/") + QLatin1String(name));
        if (!f.open(QIODevice::ReadOnly))
            qFatal("missing fixture %s", name);
        return f.readAll();
    }

private slots:
    void redgifsRequests()
    {
        FeedRequest r;
        r.sort = "hot";
        QUrlQuery q;
        QCOMPARE(RedgifsBackend::requestPath(r, &q), QStringLiteral("/v2/gifs/search"));
        QCOMPARE(q.queryItemValue("order"), QStringLiteral("trending"));
        QCOMPARE(q.queryItemValue("type"), QStringLiteral("g"));
        QVERIFY(!q.hasQueryItem("tags"));

        // Exact tag names, several joined with '+', sent encoded.
        r.search = "Big Tits+Blonde";
        r.sort = "new";
        r.after = "3";
        RedgifsBackend::requestPath(r, &q);
        QCOMPARE(q.queryItemValue("tags", QUrl::FullyDecoded), QStringLiteral("Big Tits+Blonde"));
        QVERIFY(q.query(QUrl::FullyEncoded).contains("tags=Big%20Tits%2BBlonde"));
        QCOMPARE(q.queryItemValue("order"), QStringLiteral("latest"));
        QCOMPARE(q.queryItemValue("page"), QStringLiteral("3"));

        FeedRequest u;
        u.user = "SomeCreator";
        QCOMPARE(RedgifsBackend::requestPath(u, &q), QStringLiteral("/v2/users/somecreator/search"));
        QCOMPARE(q.queryItemValue("order"), QStringLiteral("new"));

        FeedRequest n;
        n.feed = "niche:outdoor";
        n.sort = "top";
        n.timeRange = "week";
        QCOMPARE(RedgifsBackend::requestPath(n, &q), QStringLiteral("/v2/niches/outdoor/gifs"));
        QCOMPARE(q.queryItemValue("order"), QStringLiteral("top7"));
    }

    void redgifsSuggestions()
    {
        const auto s = RedgifsBackend::parseSuggestions(
            "[{\"gifs\":12034,\"text\":\"Blonde\",\"type\":\"tag\"},{\"gifs\":40,\"text\":\"blondebabe\",\"type\":\"user\"}]");
        QCOMPARE(s.size(), 2);
        QCOMPARE(s[0].text, QStringLiteral("Blonde"));
        QCOMPARE(s[0].count, 12034);
        QCOMPARE(s[1].type, QStringLiteral("user"));
        const auto t = RedgifsBackend::parseTags("{\"tags\":[{\"name\":\"Amateur\",\"count\":900000},{\"name\":\"Asian\",\"count\":5}]}");
        QCOMPARE(t.size(), 2);
        QCOMPARE(t[0].text, QStringLiteral("Amateur"));
        QCOMPARE(t[0].count, 900000);
    }

    void redgifsParse()
    {
        const auto l = RedgifsBackend::parse(fixture("redgifs_search.json"), 1);
        QVERIFY(l.ok);
        QCOMPARE(l.posts.size(), 2); // the one without files is skipped
        QCOMPARE(l.after, QStringLiteral("2"));
        const Post &v = l.posts[0];
        QCOMPARE(v.source, QStringLiteral("redgifs"));
        QCOMPARE(v.kind, Post::Kind::Video);
        QCOMPARE(v.author, QStringLiteral("somecreator"));
        QCOMPARE(v.title, QStringLiteral("#Amateur #Blonde #Outdoor"));
        QCOMPARE(v.tags, (QStringList{"Amateur", "Blonde", "Outdoor"}));
        QVERIFY(v.nsfw);
        QCOMPARE(v.media.size(), 1);
        QVERIFY(v.media[0].video);
        QCOMPARE(v.media[0].url, QStringLiteral("https://media.redgifs.com/SteelBothPeafowl.mp4"));
        QCOMPARE(v.media[0].alternates, QStringList{"https://media.redgifs.com/SteelBothPeafowl-mobile.mp4"});
        QCOMPARE(v.previewUrl, QStringLiteral("https://media.redgifs.com/SteelBothPeafowl-poster.jpg"));
        QCOMPARE(v.permalink, QStringLiteral("https://www.redgifs.com/watch/steelbothpeafowl"));

        const Post &img = l.posts[1];
        QCOMPARE(img.kind, Post::Kind::Image);
        QCOMPARE(img.title, QStringLiteral("A still one"));

        // Last page: no next.
        QVERIFY(RedgifsBackend::parse(fixture("redgifs_search.json"), 3).after.isEmpty());
        QVERIFY(!RedgifsBackend::parse("not json", 1).ok);
    }

    void epornerRequests()
    {
        FeedRequest r;
        QUrlQuery q = EpornerBackend::requestQuery(r);
        QCOMPARE(q.queryItemValue("query"), QStringLiteral("all"));
        QCOMPARE(q.queryItemValue("order"), QStringLiteral("most-popular"));
        r.search = "beach";
        r.sort = "new";
        q = EpornerBackend::requestQuery(r);
        QCOMPARE(q.queryItemValue("query"), QStringLiteral("beach"));
        QCOMPARE(q.queryItemValue("order"), QStringLiteral("latest"));
    }

    void epornerParse()
    {
        const auto l = EpornerBackend::parse(fixture("eporner_search.json"), 1);
        QVERIFY(l.ok);
        QCOMPARE(l.posts.size(), 1);
        QVERIFY(l.after.isEmpty()); // total_pages 1
        const Post &p = l.posts[0];
        QCOMPARE(p.source, QStringLiteral("eporner"));
        QCOMPARE(p.kind, Post::Kind::Video);
        QCOMPARE(p.flair, QStringLiteral("12:34"));
        QCOMPARE(p.tags, (QStringList{"one", "two", "three"}));
        QCOMPARE(p.media[0].resolve, QStringLiteral("eporner"));
        QCOMPARE(p.media[0].url, QStringLiteral("https://www.eporner.com/video-AbCdEf12345/some-video-title/"));
        QVERIFY(p.createdUtc > 0);
    }

    void multiCursor()
    {
        const QHash<QString, QString> c = MultiLemmyBackend::splitCursor("lemmy.world=3;lemmynsfw.com=2");
        QCOMPARE(c.value("lemmy.world"), QStringLiteral("3"));
        QCOMPARE(c.value("lemmynsfw.com"), QStringLiteral("2"));
        QCOMPARE(MultiLemmyBackend::joinCursor({"lemmynsfw.com", "lemmy.world", "gone.example"}, c),
                 QStringLiteral("lemmynsfw.com=2;lemmy.world=3"));
        QVERIFY(MultiLemmyBackend::splitCursor({}).isEmpty());
    }

    void failedReply()
    {
        auto *r = new FailedReply("nope");
        QSignalSpy spy(r, &ApiReply::failed);
        QVERIFY(spy.wait(1000));
        QCOMPARE(spy.first().first().toString(), QStringLiteral("nope"));
    }
};

QTEST_GUILESS_MAIN(TestMediaSites)
#include "tst_mediasites.moc"
