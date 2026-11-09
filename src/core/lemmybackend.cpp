#include "backend.h"
#include "lemmyparsers.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

#include <algorithm>

namespace quokka {

namespace {

QJsonObject object(const QByteArray &body, QString *error)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error)
            *error = QStringLiteral("Unexpected response from the server");
        return {};
    }
    return doc.object();
}

int pageOf(const QString &after)
{
    bool ok = false;
    const int p = after.toInt(&ok);
    return ok && p > 0 ? p : 1;
}

QString nextPage(int page, qsizetype received)
{
    return received > 0 ? QString::number(page + 1) : QString();
}

QString personSort(const QString &sort)
{
    if (sort == QLatin1String("top"))
        return QStringLiteral("TopAll");
    if (sort == QLatin1String("hot"))
        return QStringLiteral("Hot");
    if (sort == QLatin1String("controversial"))
        return QStringLiteral("Controversial");
    return QStringLiteral("New");
}

} // namespace

LemmyBackend::LemmyBackend(ApiClient *client, QString instance)
    : m_client(client)
    , m_instance(std::move(instance))
{
    m_client->setServiceName(m_instance);
    m_client->setMaxConcurrent(4); // Home merges several communities at once
}

bool LemmyBackend::isPseudoFeed(const QString &name) const
{
    const QString n = name.toLower();
    return n.isEmpty() || n == QLatin1String("all") || n == QLatin1String("popular")
        || n == QLatin1String("local") || n == QLatin1String("home") || n.contains(QLatin1Char('+'));
}

QString LemmyBackend::sortType(const QString &sort, const QString &timeRange)
{
    if (sort == QLatin1String("new"))
        return QStringLiteral("New");
    if (sort == QLatin1String("active") || sort == QLatin1String("rising"))
        return QStringLiteral("Active");
    if (sort == QLatin1String("controversial"))
        return QStringLiteral("Controversial");
    if (sort == QLatin1String("comments"))
        return QStringLiteral("MostComments");
    if (sort == QLatin1String("top") || sort == QLatin1String("relevance")) {
        const QString t = sort == QLatin1String("relevance") ? QStringLiteral("all") : timeRange;
        if (t == QLatin1String("hour")) return QStringLiteral("TopHour");
        if (t == QLatin1String("day")) return QStringLiteral("TopDay");
        if (t == QLatin1String("week")) return QStringLiteral("TopWeek");
        if (t == QLatin1String("month")) return QStringLiteral("TopMonth");
        if (t == QLatin1String("year")) return QStringLiteral("TopYear");
        return QStringLiteral("TopAll");
    }
    return QStringLiteral("Hot");
}

QString LemmyBackend::commentSortType(const QString &sort)
{
    if (sort == QLatin1String("top")) return QStringLiteral("Top");
    if (sort == QLatin1String("new")) return QStringLiteral("New");
    if (sort == QLatin1String("old")) return QStringLiteral("Old");
    if (sort == QLatin1String("controversial")) return QStringLiteral("Controversial");
    return QStringLiteral("Hot");
}

ApiReply *LemmyBackend::posts(const FeedRequest &r)
{
    const int page = pageOf(r.after);
    const QString sort = sortType(r.sort, r.timeRange);
    const QString nsfw = r.nsfw ? QStringLiteral("true") : QStringLiteral("false");

    if (!r.user.isEmpty()) {
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("username"), r.user);
        q.addQueryItem(QStringLiteral("sort"), personSort(r.sort));
        q.addQueryItem(QStringLiteral("page"), QString::number(page));
        q.addQueryItem(QStringLiteral("limit"), QString::number(kPageSize));
        return m_client->get(QStringLiteral("/api/v3/user"), q);
    }

    if (!r.search.isEmpty()) {
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("q"), r.search);
        q.addQueryItem(QStringLiteral("type_"), QStringLiteral("Posts"));
        q.addQueryItem(QStringLiteral("sort"), sort);
        q.addQueryItem(QStringLiteral("listing_type"), QStringLiteral("All"));
        q.addQueryItem(QStringLiteral("page"), QString::number(page));
        q.addQueryItem(QStringLiteral("limit"), QString::number(kPageSize));
        if (!isPseudoFeed(r.feed))
            q.addQueryItem(QStringLiteral("community_name"), r.feed);
        return m_client->get(QStringLiteral("/api/v3/search"), q);
    }

    auto listQuery = [&](int limit) {
        QUrlQuery q;
        q.addQueryItem(QStringLiteral("sort"), sort);
        q.addQueryItem(QStringLiteral("page"), QString::number(page));
        q.addQueryItem(QStringLiteral("limit"), QString::number(limit));
        q.addQueryItem(QStringLiteral("show_nsfw"), nsfw);
        return q;
    };

    // Several communities (the Home feed): one request each, merged into one list.
    if (r.feed.contains(QLatin1Char('+'))) {
        const QStringList names = r.feed.split(QLatin1Char('+'), Qt::SkipEmptyParts).mid(0, kMaxMultiCommunities);
        const int perCommunity = qMax(4, 50 / qMax(1, int(names.size())));
        QList<ApiReply *> parts;
        for (const QString &name : names) {
            QUrlQuery q = listQuery(perCommunity);
            q.addQueryItem(QStringLiteral("community_name"), name);
            parts.append(m_client->get(QStringLiteral("/api/v3/post/list"), q));
        }
        const QString s = r.sort;
        return new CompositeReply(parts, [s](const QList<QByteArray> &bodies) {
            return mergePostLists(bodies, s);
        }, /*requireAll=*/false, m_client);
    }

    QUrlQuery q = listQuery(kPageSize);
    const QString f = r.feed.toLower();
    if (f == QLatin1String("local"))
        q.addQueryItem(QStringLiteral("type_"), QStringLiteral("Local"));
    else if (isPseudoFeed(r.feed))
        q.addQueryItem(QStringLiteral("type_"), QStringLiteral("All"));
    else
        q.addQueryItem(QStringLiteral("community_name"), r.feed);
    return m_client->get(QStringLiteral("/api/v3/post/list"), q);
}

QByteArray LemmyBackend::mergePostLists(const QList<QByteArray> &bodies, const QString &sort)
{
    QList<QJsonArray> lists;
    for (const QByteArray &b : bodies) {
        const QJsonArray posts = QJsonDocument::fromJson(b).object().value(QLatin1String("posts")).toArray();
        if (!posts.isEmpty())
            lists.append(posts);
    }
    QList<QJsonObject> merged;
    if (sort == QLatin1String("new") || sort == QLatin1String("top") || sort == QLatin1String("comments")) {
        for (const QJsonArray &l : lists)
            for (const auto &v : l)
                merged.append(v.toObject());
        auto key = [&sort](const QJsonObject &pv) -> double {
            const QJsonObject counts = pv.value(QLatin1String("counts")).toObject();
            if (sort == QLatin1String("new"))
                return double(lemmy::parseTime(pv.value(QLatin1String("post")).toObject()
                                                   .value(QLatin1String("published")).toString()));
            if (sort == QLatin1String("comments"))
                return counts.value(QLatin1String("comments")).toDouble();
            return counts.value(QLatin1String("score")).toDouble();
        };
        std::stable_sort(merged.begin(), merged.end(),
                         [&](const QJsonObject &a, const QJsonObject &b) { return key(a) > key(b); });
    } else {
        // Hot / active: each list is already ranked; interleave them so no community dominates.
        for (int i = 0;; ++i) {
            bool any = false;
            for (const QJsonArray &l : lists) {
                if (i < l.size()) {
                    merged.append(l.at(i).toObject());
                    any = true;
                }
            }
            if (!any)
                break;
        }
    }
    QJsonArray out;
    for (const QJsonObject &o : merged)
        out.append(o);
    return QJsonDocument(QJsonObject{{QStringLiteral("posts"), out}}).toJson(QJsonDocument::Compact);
}

parsers::PostListing LemmyBackend::parsePosts(const QByteArray &body, const FeedRequest &r) const
{
    parsers::PostListing listing;
    const QJsonObject root = object(body, &listing.error);
    if (!listing.error.isEmpty())
        return listing;
    const QJsonArray posts = root.value(QLatin1String("posts")).toArray();
    for (const auto &v : posts)
        listing.posts.append(lemmy::postFromView(v.toObject(), webBase()));
    listing.after = nextPage(pageOf(r.after), posts.size());
    listing.ok = true;
    return listing;
}

ApiReply *LemmyBackend::thread(const QString &postId, const QString &focus, const QString &sort)
{
    QUrlQuery pq;
    pq.addQueryItem(QStringLiteral("id"), postId);
    QList<ApiReply *> parts{m_client->get(QStringLiteral("/api/v3/post"), pq)};

    QUrlQuery cq;
    cq.addQueryItem(QStringLiteral("type_"), QStringLiteral("All"));
    cq.addQueryItem(QStringLiteral("sort"), commentSortType(sort));
    cq.addQueryItem(QStringLiteral("max_depth"), QStringLiteral("8"));
    if (focus.isEmpty())
        cq.addQueryItem(QStringLiteral("post_id"), postId);
    else
        cq.addQueryItem(QStringLiteral("parent_id"), focus);
    parts.append(m_client->get(QStringLiteral("/api/v3/comment/list"), cq));

    if (!focus.isEmpty()) {
        QUrlQuery fq;
        fq.addQueryItem(QStringLiteral("id"), focus);
        parts.append(m_client->get(QStringLiteral("/api/v3/comment"), fq));
    }
    return new CompositeReply(parts, [](const QList<QByteArray> &bodies) {
        QJsonObject combined;
        combined.insert(QStringLiteral("post"), QJsonDocument::fromJson(bodies.value(0)).object());
        combined.insert(QStringLiteral("comments"), QJsonDocument::fromJson(bodies.value(1)).object());
        if (bodies.size() > 2)
            combined.insert(QStringLiteral("focus"), QJsonDocument::fromJson(bodies.value(2)).object());
        return QJsonDocument(combined).toJson(QJsonDocument::Compact);
    }, /*requireAll=*/true, m_client);
}

parsers::CommentsPage LemmyBackend::parseThread(const QByteArray &body) const
{
    parsers::CommentsPage page;
    const QJsonObject root = object(body, &page.error);
    if (!page.error.isEmpty())
        return page;
    const QJsonObject postView = root.value(QLatin1String("post")).toObject()
                                     .value(QLatin1String("post_view")).toObject();
    if (postView.isEmpty()) {
        page.error = QStringLiteral("Post not found");
        return page;
    }
    page.post = lemmy::postFromView(postView, webBase());

    QJsonArray views;
    const QJsonObject focus = root.value(QLatin1String("focus")).toObject()
                                  .value(QLatin1String("comment_view")).toObject();
    if (!focus.isEmpty())
        views.append(focus);
    for (const auto &v : root.value(QLatin1String("comments")).toObject()
                             .value(QLatin1String("comments")).toArray())
        views.append(v);
    page.comments = lemmy::buildCommentTree(views, webBase(), page.post.author);

    // In a focused thread, show the focused comment at the top level.
    if (!focus.isEmpty() && !page.comments.isEmpty()) {
        const int base = page.comments.first().depth;
        for (Comment &c : page.comments)
            c.depth = qMax(0, c.depth - base);
    }
    page.ok = true;
    return page;
}

ApiReply *LemmyBackend::moreComments(const QString &, const Comment &more, const QString &sort)
{
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("type_"), QStringLiteral("All"));
    q.addQueryItem(QStringLiteral("sort"), commentSortType(sort));
    q.addQueryItem(QStringLiteral("max_depth"), QStringLiteral("8"));
    q.addQueryItem(QStringLiteral("parent_id"), more.moreParentId);
    return m_client->get(QStringLiteral("/api/v3/comment/list"), q);
}

QList<Comment> LemmyBackend::parseMoreComments(const QByteArray &body, const Comment &more,
                                               const QString &postAuthor, QString *error) const
{
    const QJsonObject root = object(body, error);
    QJsonArray views;
    for (const auto &v : root.value(QLatin1String("comments")).toArray()) {
        // The parent itself may come back too; it is already on screen.
        const QString id = QString::number(qint64(v.toObject().value(QLatin1String("comment")).toObject()
                                                      .value(QLatin1String("id")).toDouble()));
        if (id != more.moreParentId)
            views.append(v);
    }
    return lemmy::buildCommentTree(views, webBase(), postAuthor);
}

ApiReply *LemmyBackend::community(const QString &name)
{
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("name"), name);
    return m_client->get(QStringLiteral("/api/v3/community"), q);
}

Subreddit LemmyBackend::parseCommunity(const QByteArray &body, QString *error) const
{
    const QJsonObject view = object(body, error).value(QLatin1String("community_view")).toObject();
    if (view.isEmpty()) {
        if (error && error->isEmpty())
            *error = QStringLiteral("Community not found");
        return {};
    }
    return lemmy::communityFromView(view);
}

ApiReply *LemmyBackend::communities(const QString &query, bool nsfw, const QString &after)
{
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("page"), QString::number(pageOf(after)));
    q.addQueryItem(QStringLiteral("limit"), QString::number(kPageSize));
    if (query.isEmpty()) {
        q.addQueryItem(QStringLiteral("type_"), QStringLiteral("All"));
        q.addQueryItem(QStringLiteral("sort"), QStringLiteral("TopMonth"));
        q.addQueryItem(QStringLiteral("show_nsfw"), nsfw ? QStringLiteral("true") : QStringLiteral("false"));
        return m_client->get(QStringLiteral("/api/v3/community/list"), q);
    }
    q.addQueryItem(QStringLiteral("q"), query);
    q.addQueryItem(QStringLiteral("type_"), QStringLiteral("Communities"));
    q.addQueryItem(QStringLiteral("sort"), QStringLiteral("TopAll"));
    q.addQueryItem(QStringLiteral("listing_type"), QStringLiteral("All"));
    return m_client->get(QStringLiteral("/api/v3/search"), q);
}

QList<Subreddit> LemmyBackend::parseCommunities(const QByteArray &body, const QString &after,
                                                QString *next, QString *error) const
{
    QList<Subreddit> out;
    const QJsonArray list = object(body, error).value(QLatin1String("communities")).toArray();
    for (const auto &v : list)
        out.append(lemmy::communityFromView(v.toObject()));
    if (next)
        *next = nextPage(pageOf(after), list.size());
    return out;
}

ApiReply *LemmyBackend::user(const QString &name)
{
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("username"), name);
    q.addQueryItem(QStringLiteral("limit"), QStringLiteral("1"));
    return m_client->get(QStringLiteral("/api/v3/user"), q);
}

UserAccount LemmyBackend::parseUser(const QByteArray &body, QString *error) const
{
    const QJsonObject view = object(body, error).value(QLatin1String("person_view")).toObject();
    if (view.isEmpty()) {
        if (error && error->isEmpty())
            *error = QStringLiteral("User not found");
        return {};
    }
    return lemmy::userFromView(view);
}

ApiReply *LemmyBackend::userComments(const QString &name, const QString &sort, const QString &after)
{
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("username"), name);
    q.addQueryItem(QStringLiteral("sort"), personSort(sort));
    q.addQueryItem(QStringLiteral("page"), QString::number(pageOf(after)));
    q.addQueryItem(QStringLiteral("limit"), QString::number(kPageSize));
    return m_client->get(QStringLiteral("/api/v3/user"), q);
}

QList<Comment> LemmyBackend::parseUserComments(const QByteArray &body, const QString &after,
                                               QString *next, QString *error) const
{
    QList<Comment> out;
    const QJsonArray list = object(body, error).value(QLatin1String("comments")).toArray();
    for (const auto &v : list) {
        Comment c = lemmy::commentFromView(v.toObject(), webBase());
        c.moreCount = 0;
        c.depth = 0;
        out.append(c);
    }
    if (next)
        *next = nextPage(pageOf(after), list.size());
    return out;
}

} // namespace quokka
