#pragma once

#include "backend.h"

#include <QHash>
#include <QList>
#include <QStringList>

namespace quokka {

// A media site browsed as a post feed. Only posts: no comments, communities or accounts (those
// requests fail with a friendly message).
class MediaSiteBackend : public Backend
{
public:
    explicit MediaSiteBackend(ApiClient *client) : m_client(client) {}

    QString communityPrefix() const override { return {}; }
    ApiClient *client() const override { return m_client; }
    bool isPseudoFeed(const QString &) const override { return true; }

    ApiReply *thread(const QString &, const QString &, const QString &) override;
    parsers::CommentsPage parseThread(const QByteArray &) const override { return {}; }
    ApiReply *moreComments(const QString &, const Comment &, const QString &) override;
    QList<Comment> parseMoreComments(const QByteArray &, const Comment &, const QString &, QString *) const override
    {
        return {};
    }
    ApiReply *community(const QString &) override;
    Subreddit parseCommunity(const QByteArray &, QString *) const override { return {}; }
    ApiReply *communities(const QString &, bool, const QString &) override;
    QList<Subreddit> parseCommunities(const QByteArray &, const QString &, QString *, QString *) const override
    {
        return {};
    }
    ApiReply *user(const QString &) override;
    UserAccount parseUser(const QByteArray &, QString *) const override { return {}; }
    ApiReply *userComments(const QString &, const QString &, const QString &) override;
    QList<Comment> parseUserComments(const QByteArray &, const QString &, QString *, QString *) const override
    {
        return {};
    }

protected:
    ApiReply *unsupported() const;
    ApiClient *m_client;
};

// redgifs.com: short NSFW videos. Public API with an anonymous temporary token.
//   search "<Tag>+<Tag>"   -> clips with all these tags (exact tag names, see suggest())
//   feed "niche:<id>"      -> a niche's clips
//   user "<name>"          -> a creator's clips
//   otherwise              -> everything (trending / latest / top)
class RedgifsBackend : public MediaSiteBackend
{
public:
    explicit RedgifsBackend(ApiClient *client) : MediaSiteBackend(client) {}

    QString id() const override { return QStringLiteral("redgifs"); }
    QString displayName() const override { return QStringLiteral("Redgifs"); }
    QString webBase() const override { return QStringLiteral("https://www.redgifs.com"); }

    ApiReply *posts(const FeedRequest &request) override;
    parsers::PostListing parsePosts(const QByteArray &body, const FeedRequest &request) const override;

    // Path and query for a feed request (exposed for tests).
    static QString requestPath(const FeedRequest &request, QUrlQuery *query);
    static parsers::PostListing parse(const QByteArray &body, int page);

    // Search suggestions as you type: tags (with clip counts), creators and niches.
    struct Suggestion {
        QString text;
        QString type;   // "tag", "user" / "creator", "niche"
        int count = 0;  // clips
    };
    ApiReply *suggest(const QString &query);
    static QList<Suggestion> parseSuggestions(const QByteArray &body);
    // The tags trending now.
    ApiReply *trendingTags();
    static QList<Suggestion> parseTags(const QByteArray &body);

private:
    ApiReply *withToken(std::function<ApiReply *()> request);
    QByteArray m_token;
};

// eporner.com: full-length videos through its public JSON API. Playing one needs a lookup of
// the video page first (MediaItem::resolve == "eporner").
class EpornerBackend : public MediaSiteBackend
{
public:
    explicit EpornerBackend(ApiClient *client) : MediaSiteBackend(client) {}

    QString id() const override { return QStringLiteral("eporner"); }
    QString displayName() const override { return QStringLiteral("Eporner"); }
    QString webBase() const override { return QStringLiteral("https://www.eporner.com"); }

    ApiReply *posts(const FeedRequest &request) override;
    parsers::PostListing parsePosts(const QByteArray &body, const FeedRequest &request) const override;

    static QUrlQuery requestQuery(const FeedRequest &request);
    static parsers::PostListing parse(const QByteArray &body, int page);
};

// Several Lemmy servers as one feed: each is asked for the same feed and the pages are mixed.
// Posts carry their server in Post::origin, so their comments load from the right place.
// Comments, communities and users go to the first server.
class MultiLemmyBackend : public Backend
{
public:
    // `servers`: host name -> its backend (not owned), in the order given by `hosts`.
    MultiLemmyBackend(QStringList hosts, QHash<QString, Backend *> servers);

    QString id() const override { return QStringLiteral("lemmy"); }
    QString displayName() const override { return m_hosts.join(QStringLiteral(" + ")); }
    QString webBase() const override { return first()->webBase(); }
    QString communityPrefix() const override { return QStringLiteral("c/"); }
    ApiClient *client() const override { return first()->client(); }
    bool isPseudoFeed(const QString &name) const override { return first()->isPseudoFeed(name); }

    ApiReply *posts(const FeedRequest &request) override;
    parsers::PostListing parsePosts(const QByteArray &body, const FeedRequest &request) const override;

    ApiReply *thread(const QString &postId, const QString &focus, const QString &sort) override
    {
        return first()->thread(postId, focus, sort);
    }
    parsers::CommentsPage parseThread(const QByteArray &body) const override { return first()->parseThread(body); }
    ApiReply *moreComments(const QString &postId, const Comment &more, const QString &sort) override
    {
        return first()->moreComments(postId, more, sort);
    }
    QList<Comment> parseMoreComments(const QByteArray &body, const Comment &more, const QString &author,
                                     QString *error) const override
    {
        return first()->parseMoreComments(body, more, author, error);
    }
    ApiReply *community(const QString &name) override { return first()->community(name); }
    Subreddit parseCommunity(const QByteArray &body, QString *error) const override
    {
        return first()->parseCommunity(body, error);
    }
    ApiReply *communities(const QString &q, bool nsfw, const QString &after) override
    {
        return first()->communities(q, nsfw, after);
    }
    QList<Subreddit> parseCommunities(const QByteArray &body, const QString &after, QString *next,
                                      QString *error) const override
    {
        return first()->parseCommunities(body, after, next, error);
    }
    ApiReply *user(const QString &name) override { return first()->user(name); }
    UserAccount parseUser(const QByteArray &body, QString *error) const override { return first()->parseUser(body, error); }
    ApiReply *userComments(const QString &name, const QString &sort, const QString &after) override
    {
        return first()->userComments(name, sort, after);
    }
    QList<Comment> parseUserComments(const QByteArray &body, const QString &after, QString *next,
                                     QString *error) const override
    {
        return first()->parseUserComments(body, after, next, error);
    }

    // "host=cursor;host2=cursor2" <-> map (exposed for tests).
    static QHash<QString, QString> splitCursor(const QString &after);
    static QString joinCursor(const QStringList &hosts, const QHash<QString, QString> &cursors);

private:
    Backend *first() const { return m_servers.value(m_hosts.first()); }
    QStringList m_hosts;
    QHash<QString, Backend *> m_servers;
};

} // namespace quokka
