#include "account.h"

#include <QTest>

class TestAccount : public QObject
{
    Q_OBJECT

private slots:
    void normalizesLogins_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("expected");
        QTest::addColumn<QString>("otherServer");
        QTest::newRow("name") << " alice " << "alice" << "";
        QTest::newRow("u/") << "u/alice" << "alice" << "";
        QTest::newRow("handle here") << "@alice@lemmy.world" << "alice" << "";
        QTest::newRow("name@here") << "alice@Lemmy.World" << "alice" << "";
        QTest::newRow("email") << "alice@gmail.com" << "alice@gmail.com" << "";
        QTest::newRow("handle elsewhere") << "@alice@lemmy.ml" << "alice@lemmy.ml" << "lemmy.ml";
    }
    void normalizesLogins()
    {
        QFETCH(QString, input);
        QFETCH(QString, expected);
        QFETCH(QString, otherServer);
        QString other;
        QCOMPARE(Account::normalizeLogin(input, "lemmy.world", &other), expected);
        QCOMPARE(other, otherServer);
    }
};

QTEST_GUILESS_MAIN(TestAccount)
#include "tst_account.moc"
