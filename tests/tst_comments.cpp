#include "commenttreemodel.h"
#include "html.h"
#include "postlistmodel.h"
#include "parsers.h"

#include <QAbstractItemModelTester>
#include <QFile>
#include <QTest>

using quokka::Comment;
using namespace quokka::parsers;

class TestComments : public QObject
{
    Q_OBJECT

    static QByteArray fixture(const char *name)
    {
        QFile f(QStringLiteral(QUOKKA_FIXTURES_DIR "/") + QLatin1String(name));
        if (!f.open(QIODevice::ReadOnly))
            qFatal("missing fixture %s", name);
        return f.readAll();
    }

    static QStringList ids(const QAbstractItemModel &m)
    {
        QStringList out;
        for (int r = 0; r < m.rowCount(); ++r)
            out << m.data(m.index(r, 0), CommentTreeModel::FullnameRole).toString();
        return out;
    }

private slots:
    void cleansHtml()
    {
        using namespace quokka::html;
        const QString out = cleanRedditHtml(
            "<!-- SC_OFF --><div class=\"md\"><p>a <a href=\"/r/x\">x</a> "
            "<span class=\"md-spoiler-text\">s</span><del>d</del></p>\n</div><!-- SC_ON -->");
        QVERIFY(!out.contains("SC_OFF"));
        QVERIFY(!out.contains("class=\"md\""));
        QVERIFY(out.contains("href=\"https://www.reddit.com/r/x\""));
        QVERIFY(hasSpoilers(out));
        QVERIFY(out.contains("<s>d</s>"));
        QVERIFY(cleanRedditHtml({}).isEmpty());
    }

    void parsesThread()
    {
        const auto page = parseCommentsPage(fixture("comments_abc.json"));
        QVERIFY2(page.ok, qPrintable(page.error));
        QCOMPARE(page.post.id, QStringLiteral("abc"));
        QVERIFY(page.post.selftextHtml.contains("<strong>world</strong>"));

        QStringList order;
        QList<int> depths;
        for (const Comment &c : page.comments) {
            order << c.fullname;
            depths << c.depth;
        }
        QCOMPARE(order, QStringList({"t1_c1", "t1_c2", "t1_c3", "t1__", "t1_m1", "t1_c6", "t1_m2"}));
        QCOMPARE(depths, QList<int>({0, 1, 2, 3, 1, 0, 0}));

        QCOMPARE(page.comments[0].distinguished, QStringLiteral("moderator"));
        QVERIFY(page.comments[1].isSubmitter); // op_user wrote the post
        QCOMPARE(page.comments[3].kind, Comment::Kind::Continue);
        QCOMPARE(page.comments[4].kind, Comment::Kind::More);
        QCOMPARE(page.comments[4].moreCount, 2);
        QVERIFY(page.comments[5].bodyHtml.contains("border=\"1\""));
    }

    void parsesMoreChildrenAsTree()
    {
        const auto rows = parseMoreChildren(fixture("morechildren.json"), "op_user", 1);
        QCOMPARE(rows.size(), 3);
        QCOMPARE(rows[0].fullname, QStringLiteral("t1_c4"));
        QCOMPARE(rows[0].depth, 1);
        QCOMPARE(rows[2].fullname, QStringLiteral("t1_c10"));
        QCOMPARE(rows[2].depth, 2);
    }

    void collapseAndExpand()
    {
        CommentTreeModel model;
        QAbstractItemModelTester tester(&model);
        const auto page = parseCommentsPage(fixture("comments_abc.json"));
        model.setComments(page.comments, page.post.author);
        QCOMPARE(model.rowCount(), 7);

        model.toggleCollapsed(1); // c2 hides c3 and the continue row
        QCOMPARE(ids(model), QStringList({"t1_c1", "t1_c2", "t1_m1", "t1_c6", "t1_m2"}));
        QCOMPARE(model.data(model.index(1), CommentTreeModel::HiddenCountRole).toInt(), 2);

        model.toggleCollapsed(0); // c1 hides everything under it
        QCOMPARE(ids(model), QStringList({"t1_c1", "t1_c6", "t1_m2"}));

        model.toggleCollapsed(0); // expanding c1 keeps c2 collapsed
        QCOMPARE(ids(model), QStringList({"t1_c1", "t1_c2", "t1_m1", "t1_c6", "t1_m2"}));

        model.toggleCollapsed(1);
        QCOMPARE(model.rowCount(), 7);
        QCOMPARE(model.parentRow(2), 1);
    }

    void insertsMoreInPlace()
    {
        CommentTreeModel model;
        QAbstractItemModelTester tester(&model);
        const auto page = parseCommentsPage(fixture("comments_abc.json"));
        model.setComments(page.comments, page.post.author);

        const auto rows = parseMoreChildren(fixture("morechildren.json"), "op_user", 1);
        model.insertMore("t1_m1", rows);
        QCOMPARE(ids(model), QStringList({"t1_c1", "t1_c2", "t1_c3", "t1__", "t1_c4", "t1_c5",
                                          "t1_c10", "t1_c6", "t1_m2"}));

        // Collapsing c1 now hides the newly loaded rows too.
        model.toggleCollapsed(0);
        QCOMPARE(ids(model), QStringList({"t1_c1", "t1_c6", "t1_m2"}));
    }

    void postListPaths()
    {
        PostListModel m;
        QCOMPARE(m.apiPath(), QStringLiteral("/r/popular/hot"));
        m.setFeed("r/Pics/");
        m.setSort("top");
        QCOMPARE(m.apiPath(), QStringLiteral("/r/Pics/top"));
        m.setSearchQuery("  quokka ");
        QCOMPARE(m.apiPath(), QStringLiteral("/r/Pics/search"));
        m.setFeed("all");
        QCOMPARE(m.apiPath(), QStringLiteral("/search"));
        m.setSearchQuery("");
        m.setSort("relevance"); // search-only sort falls back to hot for feeds
        QCOMPARE(m.apiPath(), QStringLiteral("/r/all/hot"));
        m.setUserName("u/someone");
        QCOMPARE(m.apiPath(), QStringLiteral("/user/someone/submitted"));
    }
};

QTEST_GUILESS_MAIN(TestComments)
#include "tst_comments.moc"
