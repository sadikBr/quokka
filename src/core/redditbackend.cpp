#include "backend.h"

#include <QUrlQuery>

namespace quokka {

namespace {
constexpr int kMoreBatch = 100; // /api/morechildren accepts up to 100 ids per call

QString redditSort(const QString &sort)
{
    if (sort == QLatin1String("active"))
        return QStringLiteral("rising");
    return sort;
}
} // namespace

RedditBackend::RedditBackend(ApiClient *client)
    : m_client(client)
{
    m_client->setServiceName(QStringLiteral("Reddit"));
}

bool RedditBackend::isPseudoFeed(const QString &name) const
{
    const QString n = name.toLower();
    return n.isEmpty() || n == QLatin1String("popular") || n == QLatin1String("all")
        || n == QLatin1String("local") || n == QLatin1String("home") || n.contains(QLatin1Char('+'));
}

ApiReply *RedditBackend::get(const QString &path, QUrlQuery query)
{
    query.removeAllQueryItems(QStringLiteral("raw_json"));
    query.addQueryItem(QStringLiteral("raw_json"), QStringLiteral("1"));
    return m_client->get(path, query);
}

QString RedditBackend::postsPath(const FeedRequest &r)
{
    if (!r.user.isEmpty())
        return QStringLiteral("/user/%1/submitted").arg(r.user);
    QString feed = r.feed.toLower() == QLatin1String("local") ? QStringLiteral("popular") : r.feed;
    if (!r.search.isEmpty()) {
        const QString f = feed.toLower();
        return (f == QLatin1String("popular") || f == QLatin1String("all"))
            ? QStringLiteral("/search")
            : QStringLiteral("/r/%1/search").arg(feed);
    }
    const QString sort = redditSort(r.sort);
    const bool feedSort = sort != QLatin1String("relevance") && sort != QLatin1String("comments");
    return QStringLiteral("/r/%1/%2").arg(feed, feedSort ? sort : QStringLiteral("hot"));
}

ApiReply *RedditBackend::posts(const FeedRequest &r)
{
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("limit"), QStringLiteral("25"));
    if (!r.after.isEmpty())
        q.addQueryItem(QStringLiteral("after"), r.after);
    const QString sort = redditSort(r.sort);
    const bool timed = sort == QLatin1String("top") || sort == QLatin1String("controversial")
        || (!r.search.isEmpty() && sort != QLatin1String("new"));
    if (timed)
        q.addQueryItem(QStringLiteral("t"), r.timeRange);
    if (!r.user.isEmpty() || !r.search.isEmpty())
        q.addQueryItem(QStringLiteral("sort"), sort);
    const QString path = postsPath(r);
    if (!r.search.isEmpty()) {
        q.addQueryItem(QStringLiteral("q"), r.search);
        q.addQueryItem(QStringLiteral("type"), QStringLiteral("link"));
        if (!path.startsWith(QLatin1String("/search")))
            q.addQueryItem(QStringLiteral("restrict_sr"), QStringLiteral("on"));
    }
    if (r.nsfw)
        q.addQueryItem(QStringLiteral("include_over_18"), QStringLiteral("on"));
    return get(path, q);
}

parsers::PostListing RedditBackend::parsePosts(const QByteArray &body, const FeedRequest &) const
{
    return parsers::parsePostListing(body);
}

ApiReply *RedditBackend::thread(const QString &postId, const QString &focus, const QString &sort)
{
    QString path = QStringLiteral("/comments/") + postId;
    if (!focus.isEmpty())
        path += QStringLiteral("/_/") + focus;
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("sort"), sort);
    q.addQueryItem(QStringLiteral("limit"), QStringLiteral("200"));
    if (!focus.isEmpty())
        q.addQueryItem(QStringLiteral("context"), QStringLiteral("0"));
    return get(path, q);
}

parsers::CommentsPage RedditBackend::parseThread(const QByteArray &body) const
{
    return parsers::parseCommentsPage(body);
}

ApiReply *RedditBackend::moreComments(const QString &postId, const Comment &more, const QString &sort)
{
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("api_type"), QStringLiteral("json"));
    q.addQueryItem(QStringLiteral("link_id"), QStringLiteral("t3_") + postId);
    q.addQueryItem(QStringLiteral("children"), more.moreChildren.mid(0, kMoreBatch).join(QLatin1Char(',')));
    q.addQueryItem(QStringLiteral("sort"), sort);
    q.addQueryItem(QStringLiteral("limit_children"), QStringLiteral("false"));
    return get(QStringLiteral("/api/morechildren"), q);
}

QList<Comment> RedditBackend::parseMoreComments(const QByteArray &body, const Comment &more,
                                                const QString &postAuthor, QString *error) const
{
    QList<Comment> rows = parsers::parseMoreChildren(body, postAuthor, more.depth, error);
    const QStringList rest = more.moreChildren.mid(kMoreBatch);
    if (!rest.isEmpty()) {
        Comment next = more;
        next.moreChildren = rest;
        next.moreCount = int(rest.size());
        rows.append(next);
    }
    return rows;
}

ApiReply *RedditBackend::community(const QString &name)
{
    return get(QStringLiteral("/r/%1/about").arg(name));
}

Subreddit RedditBackend::parseCommunity(const QByteArray &body, QString *error) const
{
    return parsers::parseSubredditAbout(body, error);
}

ApiReply *RedditBackend::communities(const QString &query, bool nsfw, const QString &after)
{
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("limit"), QStringLiteral("25"));
    if (!after.isEmpty())
        q.addQueryItem(QStringLiteral("after"), after);
    if (nsfw)
        q.addQueryItem(QStringLiteral("include_over_18"), QStringLiteral("on"));
    if (query.isEmpty())
        return get(QStringLiteral("/subreddits/popular"), q);
    q.addQueryItem(QStringLiteral("q"), query);
    return get(QStringLiteral("/subreddits/search"), q);
}

QList<Subreddit> RedditBackend::parseCommunities(const QByteArray &body, const QString &,
                                                 QString *next, QString *error) const
{
    return parsers::parseSubredditListing(body, next, error);
}

ApiReply *RedditBackend::user(const QString &name)
{
    return get(QStringLiteral("/user/%1/about").arg(name));
}

UserAccount RedditBackend::parseUser(const QByteArray &body, QString *error) const
{
    return parsers::parseUserAbout(body, error);
}

ApiReply *RedditBackend::userComments(const QString &name, const QString &sort, const QString &after)
{
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("limit"), QStringLiteral("25"));
    q.addQueryItem(QStringLiteral("sort"), sort);
    if (!after.isEmpty())
        q.addQueryItem(QStringLiteral("after"), after);
    return get(QStringLiteral("/user/%1/comments").arg(name), q);
}

QList<Comment> RedditBackend::parseUserComments(const QByteArray &body, const QString &,
                                                QString *next, QString *error) const
{
    return parsers::parseCommentListing(body, next, error);
}

} // namespace quokka
