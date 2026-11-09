#include "parsers.h"
#include "transport.h"

#include <QFile>
#include <QTest>

using quokka::Post;
using namespace quokka::parsers;

class TestParsers : public QObject
{
    Q_OBJECT

private:
    static QByteArray fixture(const char *name)
    {
        QFile f(QStringLiteral(QUOKKA_FIXTURES_DIR "/") + QLatin1String(name));
        if (!f.open(QIODevice::ReadOnly))
            qFatal("missing fixture %s", name);
        return f.readAll();
    }

    static const Post &byId(const PostListing &l, const QString &id)
    {
        for (const Post &p : l.posts)
            if (p.id == id)
                return p;
        qFatal("post %s not found", qPrintable(id));
        return l.posts.first();
    }

    PostListing listing;

private slots:
    void initTestCase()
    {
        listing = parsePostListing(fixture("listing.json"));
        QVERIFY2(listing.ok, qPrintable(listing.error));
    }

    void listingBasics()
    {
        QCOMPARE(listing.posts.size(), 9);
        QCOMPARE(listing.after, QStringLiteral("t3_pin001"));
        const auto last = parsePostListing(fixture("listing_last_page.json"));
        QVERIFY(last.ok);
        QVERIFY(last.after.isEmpty());
    }

    void imagePost()
    {
        const Post &p = byId(listing, "img001");
        QCOMPARE(p.kind, Post::Kind::Image);
        QCOMPARE(p.fullname, QStringLiteral("t3_img001"));
        QCOMPARE(p.flair, QStringLiteral("OC"));
        QCOMPARE(p.score, 48213);
        // Smallest resolution >= 640 px wide
        QCOMPARE(p.previewWidth, 640);
        QCOMPARE(p.previewHeight, 853);
        QVERIFY(p.previewUrl.contains(QLatin1String("width=640")));
    }

    void selfPost()
    {
        const Post &p = byId(listing, "self01");
        QCOMPARE(p.kind, Post::Kind::Self);
        QVERIFY(p.selftext.contains(QLatin1String("Rottnest")));
    }

    void galleryPost()
    {
        const Post &p = byId(listing, "gal001");
        QCOMPARE(p.kind, Post::Kind::Gallery);
        QCOMPARE(p.galleryCount, 6);
        QCOMPARE(p.previewWidth, 720);
        QCOMPARE(p.previewHeight, 540);
    }

    void videoPost()
    {
        const Post &p = byId(listing, "vid001");
        QCOMPARE(p.kind, Post::Kind::Video);
        QVERIFY(p.videoUrl.endsWith(QLatin1String("HLSPlaylist.m3u8")));
        QVERIFY(!p.previewUrl.isEmpty());
        QVERIFY(p.previewHeight > p.previewWidth);
    }

    void linkPostFallsBackToThumbnail()
    {
        const Post &p = byId(listing, "lnk001");
        QCOMPARE(p.kind, Post::Kind::Link);
        QCOMPARE(p.domain, QStringLiteral("phoronix.com"));
        QCOMPARE(p.previewUrl, QStringLiteral("https://b.thumbs.redditmedia.com/t.jpg"));
    }

    void flagsAndCrossposts()
    {
        QVERIFY(byId(listing, "nsfw01").nsfw);
        const Post &spoiler = byId(listing, "spl001");
        QVERIFY(spoiler.spoiler);
        QVERIFY(spoiler.locked);
        QVERIFY(byId(listing, "pin001").stickied);

        const Post &x = byId(listing, "xpost1");
        QCOMPARE(x.subreddit, QStringLiteral("Otters"));
        QCOMPARE(x.kind, Post::Kind::Video);
        QVERIFY(!x.videoUrl.isEmpty());
    }

    void rejectsGarbage()
    {
        QVERIFY(!parsePostListing("not json").ok);
        QVERIFY(!parsePostListing(R"({"kind":"t3","data":{}})").ok);
    }

    void fixtureFileNames()
    {
        using quokka::FixtureTransport;
        QCOMPARE(FixtureTransport::fileNameForPath("/r/Pics/hot"), QStringLiteral("r_pics_hot.json"));
        QCOMPARE(FixtureTransport::fileNameForPath("/r/pics/hot.json"), QStringLiteral("r_pics_hot.json"));
    }
};

QTEST_GUILESS_MAIN(TestParsers)
#include "tst_parsers.moc"
