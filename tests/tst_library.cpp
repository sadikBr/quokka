#include "library.h"
#include "postmap.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class TestLibrary : public QObject
{
    Q_OBJECT

    static QVariantMap post(const QString &id, const QString &title = QStringLiteral("A title"))
    {
        quokka::Post p;
        p.id = id;
        p.title = title;
        p.subreddit = QStringLiteral("aww");
        p.kind = quokka::Post::Kind::Gallery;
        p.media.append({QStringLiteral("https://i.redd.it/x.jpg"), 10, 20, QStringLiteral("cap"), false});
        return postToVariantMap(p);
    }

private slots:
    void normalizesNames()
    {
        QCOMPARE(Library::normalizeName(" r/Pics/ "), QStringLiteral("Pics"));
        QCOMPARE(Library::normalizeName("https://www.reddit.com/r/linux"), QStringLiteral("linux"));
        QCOMPARE(Library::normalizeName("!linux@Lemmy.ML"), QStringLiteral("linux@lemmy.ml"));
        QCOMPARE(Library::normalizeName("c/technology@lemmy.world"), QStringLiteral("technology@lemmy.world"));
        QCOMPARE(Library::normalizeName("https://lemmy.ml/c/linux"), QStringLiteral("linux@lemmy.ml"));
        QCOMPARE(Library::normalizeName("linux@"), QString());
        QCOMPARE(Library::normalizeName("bad name!"), QString());
        QCOMPARE(Library::normalizeName("x"), QString());
    }

    void subscriptionsPersist()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("q.db");
        {
            Library lib(path);
            QSignalSpy spy(&lib, &Library::subscriptionsChanged);
            lib.subscribe("r/pics");
            lib.subscribe("PICS"); // duplicate, case-insensitive
            lib.subscribe("aww");
            QCOMPARE(lib.subscriptions(), QStringList({"aww", "pics"}));
            QCOMPARE(lib.homeFeed(), QStringLiteral("aww+pics"));
            QVERIFY(lib.isSubscribed("Pics"));
            QCOMPARE(spy.count(), 2);
            QCOMPARE(lib.importSubscriptions("r/linux, aww\nr/Android + !!"), 2);
        }
        Library reopened(path);
        QCOMPARE(reopened.subscriptions(), QStringList({"Android", "aww", "linux", "pics"}));
        reopened.toggleSubscription("aww");
        QVERIFY(!reopened.isSubscribed("aww"));
        QVERIFY(reopened.exportSubscriptions().contains("r/linux"));
    }

    void savedPostsRoundTrip()
    {
        Library lib(":memory:");
        lib.toggleSaved(post("p1", "First"));
        lib.toggleSaved(post("p2", "Second"));
        QVERIFY(lib.isSaved("p1"));
        QCOMPARE(lib.savedCount(), 2);

        const auto saved = lib.savedPosts();
        QCOMPARE(saved.size(), 2);
        QCOMPARE(saved.first().id, QStringLiteral("p2")); // newest first
        QCOMPARE(saved.first().kind, quokka::Post::Kind::Gallery);
        QCOMPARE(saved.first().media.size(), 1);
        QCOMPARE(saved.first().media.first().caption, QStringLiteral("cap"));

        lib.toggleSaved(post("p1"));
        QVERIFY(!lib.isSaved("p1"));
        QCOMPARE(lib.savedPosts().size(), 1);
    }

    void historyIsCappedAndClearable()
    {
        Library lib(":memory:");
        for (int i = 0; i < Library::kMaxHistory + 5; ++i)
            lib.markVisited(post(QStringLiteral("h%1").arg(i)));
        QCOMPARE(lib.historyCount(), Library::kMaxHistory);
        QVERIFY(lib.isVisited(QStringLiteral("h%1").arg(Library::kMaxHistory + 4)));
        QVERIFY(!lib.isVisited("h0"));
        lib.markVisited(QVariantMap{{"postId", "notitle"}}); // ignored
        QVERIFY(!lib.isVisited("notitle"));
        lib.clearHistory();
        QCOMPARE(lib.historyCount(), 0);
        QVERIFY(lib.historyPosts().isEmpty());
    }

    void localModelFollowsLibrary()
    {
        Library lib(":memory:");
        LocalPostModel model;
        model.setLibrary(&lib);
        model.setSource("saved");
        QCOMPARE(model.rowCount(), 0);
        lib.save(post("s1"));
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.get(0).value("postId").toString(), QStringLiteral("s1"));
    }
};

QTEST_GUILESS_MAIN(TestLibrary)
#include "tst_library.moc"
