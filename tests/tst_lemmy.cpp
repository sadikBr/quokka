#include "apiclient.h"
#include "backend.h"
#include "commenttreemodel.h"
#include "lemmyparsers.h"
#include "markdown.h"
#include "services.h"

#include <QAbstractItemModelTester>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>

using namespace quokka;

class TestLemmy : public QObject
{
    Q_OBJECT

    static QByteArray fixture(const char *name)
    {
        QFile f(QStringLiteral(QUOKKA_FIXTURES_DIR "/lemmy/") + QLatin1String(name));
        if (!f.open(QIODevice::ReadOnly))
            qFatal("missing fixture %s", name);
        return f.readAll();
    }

    std::unique_ptr<ApiClient> client;
    std::unique_ptr<LemmyBackend> backend;

    static const Post &byId(const QList<Post> &posts, const QString &id)
    {
        for (const Post &p : posts)
            if (p.id == id)
                return p;
        qFatal("post %s not found", qPrintable(id));
        return posts.first();
    }

    static QStringList names(const QList<Comment> &rows)
    {
        QStringList out;
        for (const Comment &c : rows)
            out << QStringLiteral("%1:%2").arg(c.fullname).arg(c.depth);
        return out;
    }

private slots:
    void initTestCase()
    {
        client = std::make_unique<ApiClient>(
            std::make_unique<FixtureTransport>(QStringLiteral(QUOKKA_FIXTURES_DIR "/lemmy")));
        backend = std::make_unique<LemmyBackend>(client.get(), QStringLiteral("lemmy.world"));
        services::setBackend(backend.get());
    }

    void cleanupTestCase() { services::setBackend(nullptr); }

    void sorts()
    {
        QCOMPARE(LemmyBackend::sortType("hot", "day"), QStringLiteral("Hot"));
        QCOMPARE(LemmyBackend::sortType("top", "week"), QStringLiteral("TopWeek"));
        QCOMPARE(LemmyBackend::sortType("top", "all"), QStringLiteral("TopAll"));
        QCOMPARE(LemmyBackend::sortType("active", "day"), QStringLiteral("Active"));
        QCOMPARE(LemmyBackend::sortType("comments", "day"), QStringLiteral("MostComments"));
        QCOMPARE(LemmyBackend::commentSortType("confidence"), QStringLiteral("Hot"));
        QCOMPARE(LemmyBackend::commentSortType("old"), QStringLiteral("Old"));
    }

    void times()
    {
        QCOMPARE(lemmy::parseTime("2024-01-01T00:00:00.123456Z"), 1704067200);
        QCOMPARE(lemmy::parseTime("2024-01-01T00:00:00.123456"), 1704067200);   // pre-0.19: UTC
        QCOMPARE(lemmy::parseTime("2024-01-01T02:00:00+02:00"), 1704067200);
        QCOMPARE(lemmy::parseTime("garbage"), 0);
    }

    void posts()
    {
        FeedRequest r;
        r.after = QStringLiteral("3");
        const auto listing = backend->parsePosts(fixture("api_v3_post_list.json"), r);
        QVERIFY2(listing.ok, qPrintable(listing.error));
        QCOMPARE(listing.posts.size(), 5);
        QCOMPARE(listing.after, QStringLiteral("4"));

        const Post &link = byId(listing.posts, "101");
        QCOMPARE(link.kind, Post::Kind::Link);
        QCOMPARE(link.subreddit, QStringLiteral("linux@lemmy.ml"));
        QCOMPARE(link.author, QStringLiteral("bob@lemmy.ml"));
        QCOMPARE(link.domain, QStringLiteral("kernel.org"));
        QVERIFY(link.stickied);
        QVERIFY(link.previewUrl.contains("thumbnail=320"));
        QCOMPARE(link.permalink, QStringLiteral("https://lemmy.world/post/101"));
        QCOMPARE(link.score, 500);
        QCOMPARE(link.numComments, 42);

        const Post &image = byId(listing.posts, "102");
        QCOMPARE(image.kind, Post::Kind::Image);
        QCOMPARE(image.previewWidth, 1600);
        QCOMPARE(image.previewHeight, 1200);
        // The reading server's thumbnail first, the origin server's image as the fallback.
        QVERIFY(image.previewUrl.startsWith("https://lemmy.world/pictrs/image/desk_thumb.webp"));
        QVERIFY(image.previewUrl.contains("thumbnail=640"));
        QVERIFY(image.previewFallbackUrl.startsWith("https://lemmynsfw.example/pictrs/image/desk.jpeg"));
        QCOMPARE(image.media.size(), 1);
        QCOMPARE(image.media.first().url, QStringLiteral("https://lemmynsfw.example/pictrs/image/desk.jpeg"));
        QCOMPARE(image.media.first().fallbackUrl, QStringLiteral("https://lemmy.world/pictrs/image/desk_thumb.webp"));
        QCOMPARE(image.media.first().caption, QStringLiteral("A tidy desk"));

        const Post &text = byId(listing.posts, "103");
        QCOMPARE(text.kind, Post::Kind::Self);
        QVERIFY(text.selftextHtml.contains("QK_SPOILER"));
        QVERIFY(text.selftextHtml.contains("why"));
        QCOMPARE(text.subreddit, QStringLiteral("technology@lemmy.world"));

        const Post &video = byId(listing.posts, "104");
        QCOMPARE(video.kind, Post::Kind::Video);
        QCOMPARE(video.videoUrl, QStringLiteral("https://files.example/cat.mp4"));

        QVERIFY(byId(listing.posts, "105").nsfw);
        QVERIFY(!byId(listing.posts, "102").nsfw);
    }

    void emptyPageEndsPaging()
    {
        const auto listing = backend->parsePosts(R"({"posts":[]})", FeedRequest{});
        QVERIFY(listing.ok);
        QVERIFY(listing.after.isEmpty());
    }

    void mergesCommunities()
    {
        auto feed = [](std::initializer_list<std::pair<int, const char *>> posts) {
            QJsonArray arr;
            for (const auto &[score, published] : posts)
                arr.append(QJsonObject{{"post", QJsonObject{{"id", score}, {"published", QString::fromLatin1(published)}}},
                                       {"counts", QJsonObject{{"score", score}, {"comments", 0}}}});
            return QJsonDocument(QJsonObject{{"posts", arr}}).toJson();
        };
        const QByteArray a = feed({{1, "2025-01-03T00:00:00Z"}, {2, "2025-01-01T00:00:00Z"}});
        const QByteArray b = feed({{30, "2025-01-02T00:00:00Z"}});
        auto ids = [](const QByteArray &merged) {
            QList<int> out;
            for (const auto &v : QJsonDocument::fromJson(merged).object().value("posts").toArray())
                out << v.toObject().value("post").toObject().value("id").toInt();
            return out;
        };
        QCOMPARE(ids(LemmyBackend::mergePostLists({a, b}, "hot")), QList<int>({1, 30, 2}));  // interleaved
        QCOMPARE(ids(LemmyBackend::mergePostLists({a, b}, "new")), QList<int>({1, 30, 2}));  // by date
        QCOMPARE(ids(LemmyBackend::mergePostLists({a, b}, "top")), QList<int>({30, 2, 1}));  // by score
        QCOMPARE(ids(LemmyBackend::mergePostLists({a, {}}, "hot")), QList<int>({1, 2}));     // failed part
    }

    void multiFeedRequest()
    {
        FeedRequest r;
        r.feed = QStringLiteral("technology@lemmy.world+linux@lemmy.ml");
        ApiReply *reply = backend->posts(r);
        QSignalSpy done(reply, &ApiReply::finished);
        QVERIFY(done.wait(3000));
        const auto listing = backend->parsePosts(done.first().first().toByteArray(), r);
        QVERIFY(listing.ok);
        QCOMPARE(listing.posts.size(), 10); // the fixture answers both; the model removes duplicates
    }

    void thread()
    {
        ApiReply *reply = backend->thread(QStringLiteral("103"), {}, QStringLiteral("confidence"));
        QSignalSpy done(reply, &ApiReply::finished);
        QVERIFY(done.wait(3000));
        const auto page = backend->parseThread(done.first().first().toByteArray());
        QVERIFY2(page.ok, qPrintable(page.error));
        QCOMPARE(page.post.id, QStringLiteral("103"));
        QCOMPARE(names(page.comments),
                 QStringList({"lc_1:0", "lc_2:1", "lc_3:2", "lm_3:3", "lc_5:0", "lc_6:0"}));

        const Comment &top = page.comments[0];
        QCOMPARE(top.distinguished, QStringLiteral("moderator"));
        QVERIFY(top.bodyHtml.contains("href=\"https://lemmy.ml/c/linux\""));
        QVERIFY(page.comments[1].isSubmitter);                 // carol wrote the post
        QVERIFY(page.comments[1].bodyHtml.contains("italic"));
        QVERIFY(page.comments[2].bodyHtml.contains("<sup>sup</sup>"));
        QCOMPARE(page.comments[3].kind, Comment::Kind::More);
        QCOMPARE(page.comments[3].moreCount, 1);                // shown once, not on every ancestor
        QCOMPARE(page.comments[3].moreParentId, QStringLiteral("3"));
        QVERIFY(page.comments[5].bodyHtml.contains("deleted"));
    }

    void loadMoreMergesUnderParent()
    {
        const auto page = backend->parseThread([&] {
            QJsonObject combined{{"post", QJsonDocument::fromJson(fixture("api_v3_post.json")).object()},
                                 {"comments", QJsonDocument::fromJson(fixture("api_v3_comment_list.json")).object()}};
            return QJsonDocument(combined).toJson();
        }());
        CommentTreeModel model;
        QAbstractItemModelTester tester(&model);
        model.setComments(page.comments, page.post.author);

        Comment more = page.comments[3];
        QString err;
        const auto rows = backend->parseMoreComments(fixture("more_comments.json"), more, page.post.author, &err);
        QVERIFY(err.isEmpty());
        model.insertMore(more.fullname, rows);

        QStringList shown;
        for (int r = 0; r < model.rowCount(); ++r)
            shown << QStringLiteral("%1:%2").arg(model.data(model.index(r), CommentTreeModel::FullnameRole).toString())
                         .arg(model.data(model.index(r), CommentTreeModel::DepthRole).toInt());
        QCOMPARE(shown, QStringList({"lc_1:0", "lc_2:1", "lc_3:2", "lc_4:3", "lc_5:0", "lc_6:0"}));
    }

    void community()
    {
        QString err;
        const Subreddit s = backend->parseCommunity(fixture("api_v3_community.json"), &err);
        QVERIFY2(err.isEmpty(), qPrintable(err));
        QCOMPARE(s.name, QStringLiteral("technology@lemmy.world"));
        QCOMPARE(s.subscribers, 75000);
        QCOMPARE(s.activeUsers, 1500);
        QCOMPARE(s.publicDescription, QStringLiteral("News about technology and gadgets."));
        QVERIFY(s.descriptionHtml.contains("Rules"));
        QVERIFY(s.iconUrl.contains("thumbnail=128"));

        backend->parseCommunity(R"({"error":"couldnt_find_community"})", &err);
        QVERIFY(!err.isEmpty());
    }

    void communitiesAndUsers()
    {
        QString next, err;
        const auto list = backend->parseCommunities(fixture("api_v3_community_list.json"), {}, &next, &err);
        QCOMPARE(list.size(), 3);
        QCOMPARE(next, QStringLiteral("2"));
        QVERIFY(list[2].nsfw);

        const UserAccount u = backend->parseUser(fixture("api_v3_user.json"), &err);
        QCOMPARE(u.name, QStringLiteral("alice@lemmy.world"));
        QVERIFY(!u.hasKarma);
        QCOMPARE(u.postCount, 12);
        QCOMPARE(u.commentCount, 340);
        QCOMPARE(u.description, QStringLiteral("I like quokkas."));

        const auto comments = backend->parseUserComments(fixture("api_v3_user.json"), {}, &next, &err);
        QCOMPARE(comments.size(), 1);
        QCOMPARE(comments[0].linkTitle, QStringLiteral("Ask: favourite distro?"));
        QCOMPARE(comments[0].permalink, QStringLiteral("https://lemmy.world/post/103/1"));
    }

    void accounts()
    {
        QVERIFY(backend->supportsAccounts());
        QString err;
        QCOMPARE(backend->parseLogin(R"({"jwt":"abc.def","registration_created":false,"verify_email_sent":false})", &err),
                 QStringLiteral("abc.def"));
        QVERIFY(err.isEmpty());
        QVERIFY(backend->parseLogin(R"({"registration_created":false,"verify_email_sent":true})", &err).isEmpty());
        QVERIFY(err.contains("email"));

        const AccountInfo me = backend->parseAccount(fixture("api_v3_site.json"));
        QVERIFY(me.ok);
        QCOMPARE(me.name, QStringLiteral("quokka_fan@lemmy.world"));
        QVERIFY(!me.showNsfw);
        QCOMPARE(me.follows.size(), 2);
        QCOMPARE(me.follows[0].name, QStringLiteral("linux@lemmy.ml")); // sorted
        QCOMPARE(me.follows[0].id, 42);

        const AccountInfo anonymous = backend->parseAccount(R"({"site_view":{}})");
        QVERIFY(!anonymous.ok);

        QString err2;
        const Subreddit s = backend->parseCommunity(fixture("api_v3_community.json"), &err2);
        QVERIFY(s.id > 0); // needed to follow
    }

    void subscribedFeedIsPseudo()
    {
        QVERIFY(backend->isPseudoFeed("subscribed"));
        FeedRequest r;
        r.feed = QStringLiteral("subscribed");
        ApiReply *reply = backend->posts(r);
        QVERIFY(reply->url().query().contains("type_=Subscribed"));
        reply->abort();
    }

    void markdownRendering()
    {
        const QString html = markdown::toHtml(
            "Look ![a cat](https://x.y/cat.png) and ~~old~~ news\n\n> quoted\n\n@bob@lemmy.ml hi");
        QVERIFY(html.contains("href=\"https://x.y/cat.png\""));
        QVERIFY(html.contains("Image: a cat"));
        QVERIFY(!html.contains("<img"));
        QVERIFY(html.contains("line-through"));
        QVERIFY(html.contains("href=\"https://lemmy.ml/u/bob\""));
        QVERIFY(!html.contains("<body"));
        QVERIFY(!html.contains("#0000ff"));
        QVERIFY(markdown::toHtml("   ").isEmpty());
        QCOMPARE(markdown::toPlainExcerpt("What do you use and **why**?\n\n::: spoiler my answer\nArch\n:::\n\n- [a link](x)"),
                 QStringLiteral("What do you use and why? [spoiler] a link"));
        QCOMPARE(markdown::toPlainSummary("## Title\n\nFirst **para** [x](y).\n\nSecond"),
                 QStringLiteral("First para x."));
    }
};

QTEST_MAIN(TestLemmy)
#include "tst_lemmy.moc"
