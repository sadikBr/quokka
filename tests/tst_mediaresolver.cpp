#include "mediaresolver.h"

#include <QTest>

class TestMediaResolver : public QObject
{
    Q_OBJECT

private slots:
    void ids()
    {
        QCOMPARE(MediaResolver::redgifsId("https://www.redgifs.com/watch/SteelBothPeafowl"), QStringLiteral("steelbothpeafowl"));
        QCOMPARE(MediaResolver::redgifsId("https://www.redgifs.com/ifr/steelbothpeafowl"), QStringLiteral("steelbothpeafowl"));
        QCOMPARE(MediaResolver::redgifsId("https://v3.redgifs.com/watch/abc-some-title"), QStringLiteral("abc"));
        QVERIFY(MediaResolver::redgifsId("https://www.redgifs.com/").isEmpty());
        QCOMPARE(MediaResolver::streamableCode("https://streamable.com/xyz12"), QStringLiteral("xyz12"));
    }

    void skipsDeadImages()
    {
        MediaResolver r;
        r.setConverterEnabled(false);
        const QString a = "https://down.example/pictrs/image/a.jpeg?thumbnail=640&format=jpg";
        QCOMPARE(r.imageSources(a).size(), 4);
        r.reportImage(a, false);
        QCOMPARE(r.imageSources(a).size(), 3); // that copy is skipped from now on
        // Six failures and no success: the whole server is treated as down.
        for (int i = 0; i < 6; ++i)
            r.reportImage(QStringLiteral("https://down.example/pictrs/image/%1.png").arg(i), false);
        QVERIFY(r.imageSources(a).isEmpty());
        QCOMPARE(r.imageSources(a, "https://up.example/b.jpg"), QStringList{"https://up.example/b.jpg"});
        // A server that worked once is never written off.
        r.reportImage("https://ok.example/x.png", true);
        for (int i = 0; i < 8; ++i)
            r.reportImage(QStringLiteral("https://ok.example/%1.png").arg(i), false);
        QCOMPARE(r.imageSources("https://ok.example/new.png").size(), 1);
    }

    void converter()
    {
        QCOMPARE(MediaResolver::convertedUrl("https://i.ibb.co/cS9JXXCG/1000032992.png"),
                 QStringLiteral("https://wsrv.nl/?url=https%3A%2F%2Fi.ibb.co%2FcS9JXXCG%2F1000032992.png&w=1600&we&output=jpg"));
        // pict-rs resizing options are dropped: the service converts the original.
        QVERIFY(MediaResolver::convertedUrl("https://a.org/pictrs/image/x.png?thumbnail=640&format=jpg")
                    .startsWith("https://wsrv.nl/?url=https%3A%2F%2Fa.org%2Fpictrs%2Fimage%2Fx.png&"));

        MediaResolver r;
        const bool was = r.converterEnabled();
        r.setConverterEnabled(true);
        const QStringList s = r.imageSources("https://i.ibb.co/a.png");
        QCOMPARE(s.size(), 2);
        QVERIFY(s.last().startsWith("https://wsrv.nl/"));
        // Conversion failures never write the service off.
        for (int i = 0; i < 10; ++i)
            r.reportImage(MediaResolver::convertedUrl(QStringLiteral("https://gone.example/%1.png").arg(i)), false);
        QCOMPARE(r.imageSources("https://i.ibb.co/b.png").size(), 2);
        r.setConverterEnabled(was);
    }

    void messages()
    {
        QVERIFY(MediaResolver::describeFailure(410, "https://www.redgifs.com/watch/x").contains("deleted from redgifs.com"));
        QVERIFY(MediaResolver::describeFailure(502, "https://x.org/a").contains("trouble"));
        QVERIFY(MediaResolver::describePlaybackError("Could not open file", "https://lemmynsfw.com/a.mp4").contains("lemmynsfw.com"));
    }

    void parsing()
    {
        QCOMPARE(MediaResolver::parseRedgifs("{\"gif\":{\"urls\":{\"sd\":\"https://media.redgifs.com/a-mobile.mp4\",\"hd\":\"https://media.redgifs.com/a.mp4\"}}}"),
                 QStringLiteral("https://media.redgifs.com/a.mp4"));
        QCOMPARE(MediaResolver::parseRedgifs("{\"gif\":{\"urls\":{\"sd\":\"https://media.redgifs.com/a-mobile.mp4\"}}}"),
                 QStringLiteral("https://media.redgifs.com/a-mobile.mp4"));
        QVERIFY(MediaResolver::parseRedgifs("{\"error\":{}}").isEmpty());
        QCOMPARE(MediaResolver::parseStreamable("{\"files\":{\"mp4\":{\"url\":\"//cdn-cf-east.streamable.com/video/mp4/x.mp4?token=1\"}}}"),
                 QStringLiteral("https://cdn-cf-east.streamable.com/video/mp4/x.mp4?token=1"));
    }
};

QTEST_GUILESS_MAIN(TestMediaResolver)
#include "tst_mediaresolver.moc"
