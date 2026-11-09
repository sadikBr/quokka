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
