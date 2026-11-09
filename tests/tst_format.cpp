#include "format.h"

#include <QTest>

class TestFormat : public QObject
{
    Q_OBJECT

private slots:
    void relativeTime_data()
    {
        QTest::addColumn<qint64>("age");
        QTest::addColumn<QString>("expected");
        QTest::newRow("now") << qint64(5) << "now";
        QTest::newRow("minutes") << qint64(5 * 60) << "5m";
        QTest::newRow("hours") << qint64(3 * 3600 + 10) << "3h";
        QTest::newRow("days") << qint64(2 * 86400) << "2d";
        QTest::newRow("months") << qint64(65 * 86400) << "2mo";
        QTest::newRow("years") << qint64(800 * 86400) << "2y";
        QTest::newRow("future") << qint64(-100) << "now";
    }
    void relativeTime()
    {
        QFETCH(qint64, age);
        QFETCH(QString, expected);
        const qint64 now = 1'700'000'000;
        QCOMPARE(Format::relativeTime(now - age, now), expected);
    }

    void age()
    {
        const qint64 now = 1'700'000'000;
        QCOMPARE(Format::age(now - 86400, now), QStringLiteral("1 day"));
        QCOMPARE(Format::age(now - 5 * 86400, now), QStringLiteral("5 days"));
        QCOMPARE(Format::age(now - 70 * 86400, now), QStringLiteral("2 months"));
        QCOMPARE(Format::age(now - 400 * 86400, now), QStringLiteral("1 year"));
    }

    void compact_data()
    {
        QTest::addColumn<qint64>("n");
        QTest::addColumn<QString>("expected");
        QTest::newRow("small") << qint64(999) << "999";
        QTest::newRow("k") << qint64(1234) << "1.2k";
        QTest::newRow("round k") << qint64(2000) << "2k";
        QTest::newRow("tens of k") << qint64(45678) << "45.7k";
        QTest::newRow("hundreds of k") << qint64(456789) << "457k";
        QTest::newRow("M") << qint64(1234567) << "1.2M";
        QTest::newRow("edge") << qint64(999999) << "1M";
        QTest::newRow("negative") << qint64(-1500) << "-1.5k";
    }
    void compact()
    {
        QFETCH(qint64, n);
        QFETCH(QString, expected);
        QCOMPARE(Format::compact(n), expected);
    }

    void nameColorIsStable()
    {
        QCOMPARE(Format::nameColor("pics"), Format::nameColor("PICS"));
        QVERIFY(Format::nameColor("pics").isValid());
    }

    void webUrl()
    {
        QCOMPARE(Format::webUrl("/r/a/comments/x/"), QStringLiteral("https://www.reddit.com/r/a/comments/x/"));
        QCOMPARE(Format::webUrl("https://x.y"), QStringLiteral("https://x.y"));
    }
};

QTEST_GUILESS_MAIN(TestFormat)
#include "tst_format.moc"
