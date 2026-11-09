#pragma once

#include "apiclient.h"
#include "parsers.h"

#include <QString>

namespace quokka {

// What a post list shows. `feed` is "all", "local", "popular", "subscribed" (signed in), a
// community name, or several joined with "+". `after` is the backend's pagination token (empty for the first page).
struct FeedRequest
{
    QString feed = QStringLiteral("all");
    QString sort = QStringLiteral("hot");   // hot new top active controversial relevance comments
    QString timeRange = QStringLiteral("day"); // hour day week month year all
    QString search;                          // non-empty: search posts
    QString user;                            // non-empty: posts by this user
    bool nsfw = false;
    QString after;
};

// One content source (Lemmy or Reddit). Builds requests on its ApiClient and parses the
// responses into Quokka's own types, so models and UI never see the wire format.
class Backend
{
public:
    virtual ~Backend() = default;

    virtual QString id() const = 0;              // "lemmy" | "reddit"
    virtual QString displayName() const = 0;     // "lemmy.world" | "Reddit"
    virtual QString webBase() const = 0;         // "https://lemmy.world"
    virtual QString communityPrefix() const = 0; // "c/" | "r/"
    virtual ApiClient *client() const = 0;

    // True for names that are not one real community (all, local, multi feeds...).
    virtual bool isPseudoFeed(const QString &name) const = 0;

    virtual ApiReply *posts(const FeedRequest &request) = 0;
    virtual parsers::PostListing parsePosts(const QByteArray &body, const FeedRequest &request) const = 0;

    virtual ApiReply *thread(const QString &postId, const QString &focusCommentId, const QString &sort) = 0;
    virtual parsers::CommentsPage parseThread(const QByteArray &body) const = 0;

    // Loads what a Kind::More row stands for. The result may include comments already shown;
    // the comment model merges by id.
    virtual ApiReply *moreComments(const QString &postId, const Comment &more, const QString &sort) = 0;
    virtual QList<Comment> parseMoreComments(const QByteArray &body, const Comment &more,
                                             const QString &postAuthor, QString *error) const = 0;

    virtual ApiReply *community(const QString &name) = 0;
    virtual Subreddit parseCommunity(const QByteArray &body, QString *error) const = 0;

    // Search communities, or list popular ones when `query` is empty.
    virtual ApiReply *communities(const QString &query, bool nsfw, const QString &after) = 0;
    virtual QList<Subreddit> parseCommunities(const QByteArray &body, const QString &after,
                                              QString *next, QString *error) const = 0;

    virtual ApiReply *user(const QString &name) = 0;
    virtual UserAccount parseUser(const QByteArray &body, QString *error) const = 0;

    virtual ApiReply *userComments(const QString &name, const QString &sort, const QString &after) = 0;
    virtual QList<Comment> parseUserComments(const QByteArray &body, const QString &after,
                                             QString *next, QString *error) const = 0;

    // ---- Optional accounts. Requests need ApiClient::setAuthToken() after login. ----
    virtual bool supportsAccounts() const { return false; }
    virtual ApiReply *login(const QString &user, const QString &password, const QString &totp)
    {
        Q_UNUSED(user) Q_UNUSED(password) Q_UNUSED(totp) return nullptr;
    }
    // Returns the session token, or sets *error.
    virtual QString parseLogin(const QByteArray &body, QString *error) const
    {
        Q_UNUSED(body) if (error) *error = QStringLiteral("Signing in is not supported here.");
        return {};
    }
    virtual ApiReply *logout() { return nullptr; }
    virtual ApiReply *account() { return nullptr; }
    virtual AccountInfo parseAccount(const QByteArray &body) const { Q_UNUSED(body) return {}; }
    virtual ApiReply *follow(qint64 communityId, bool follow)
    {
        Q_UNUSED(communityId) Q_UNUSED(follow) return nullptr;
    }
    virtual ApiReply *setAccountNsfw(bool show) { Q_UNUSED(show) return nullptr; }
};

// Reddit's public JSON. Kept for when API access is available again; Reddit now answers
// anonymous requests with 403.
class RedditBackend : public Backend
{
public:
    explicit RedditBackend(ApiClient *client);

    QString id() const override { return QStringLiteral("reddit"); }
    QString displayName() const override { return QStringLiteral("Reddit"); }
    QString webBase() const override { return QStringLiteral("https://www.reddit.com"); }
    QString communityPrefix() const override { return QStringLiteral("r/"); }
    ApiClient *client() const override { return m_client; }
    bool isPseudoFeed(const QString &name) const override;

    static QString postsPath(const FeedRequest &request);

    ApiReply *posts(const FeedRequest &request) override;
    parsers::PostListing parsePosts(const QByteArray &body, const FeedRequest &request) const override;
    ApiReply *thread(const QString &postId, const QString &focusCommentId, const QString &sort) override;
    parsers::CommentsPage parseThread(const QByteArray &body) const override;
    ApiReply *moreComments(const QString &postId, const Comment &more, const QString &sort) override;
    QList<Comment> parseMoreComments(const QByteArray &body, const Comment &more,
                                     const QString &postAuthor, QString *error) const override;
    ApiReply *community(const QString &name) override;
    Subreddit parseCommunity(const QByteArray &body, QString *error) const override;
    ApiReply *communities(const QString &query, bool nsfw, const QString &after) override;
    QList<Subreddit> parseCommunities(const QByteArray &body, const QString &after, QString *next,
                                      QString *error) const override;
    ApiReply *user(const QString &name) override;
    UserAccount parseUser(const QByteArray &body, QString *error) const override;
    ApiReply *userComments(const QString &name, const QString &sort, const QString &after) override;
    QList<Comment> parseUserComments(const QByteArray &body, const QString &after, QString *next,
                                     QString *error) const override;

private:
    ApiReply *get(const QString &path, QUrlQuery query = {});
    ApiClient *m_client;
};

// A Lemmy instance (e.g. lemmy.world) through its v3 HTTP API. Everything public is readable
// without an account.
class LemmyBackend : public Backend
{
public:
    // `instance` is a host name such as "lemmy.world".
    LemmyBackend(ApiClient *client, QString instance);

    QString id() const override { return QStringLiteral("lemmy"); }
    QString displayName() const override { return m_instance; }
    QString webBase() const override { return QStringLiteral("https://") + m_instance; }
    QString communityPrefix() const override { return QStringLiteral("c/"); }
    ApiClient *client() const override { return m_client; }
    bool isPseudoFeed(const QString &name) const override;
    QString instance() const { return m_instance; }

    static constexpr int kPageSize = 25;
    static constexpr int kMaxMultiCommunities = 25;
    static QString sortType(const QString &sort, const QString &timeRange);
    static QString commentSortType(const QString &sort);

    ApiReply *posts(const FeedRequest &request) override;
    parsers::PostListing parsePosts(const QByteArray &body, const FeedRequest &request) const override;
    ApiReply *thread(const QString &postId, const QString &focusCommentId, const QString &sort) override;
    parsers::CommentsPage parseThread(const QByteArray &body) const override;
    ApiReply *moreComments(const QString &postId, const Comment &more, const QString &sort) override;
    QList<Comment> parseMoreComments(const QByteArray &body, const Comment &more,
                                     const QString &postAuthor, QString *error) const override;
    ApiReply *community(const QString &name) override;
    Subreddit parseCommunity(const QByteArray &body, QString *error) const override;
    ApiReply *communities(const QString &query, bool nsfw, const QString &after) override;
    QList<Subreddit> parseCommunities(const QByteArray &body, const QString &after, QString *next,
                                      QString *error) const override;
    ApiReply *user(const QString &name) override;
    UserAccount parseUser(const QByteArray &body, QString *error) const override;
    ApiReply *userComments(const QString &name, const QString &sort, const QString &after) override;
    QList<Comment> parseUserComments(const QByteArray &body, const QString &after, QString *next,
                                     QString *error) const override;

    bool supportsAccounts() const override { return true; }
    ApiReply *login(const QString &user, const QString &password, const QString &totp) override;
    QString parseLogin(const QByteArray &body, QString *error) const override;
    ApiReply *logout() override;
    ApiReply *account() override;
    AccountInfo parseAccount(const QByteArray &body) const override;
    ApiReply *follow(qint64 communityId, bool follow) override;
    ApiReply *setAccountNsfw(bool show) override;

    // Merges several GetPostsResponse bodies into one, ordered for `sort` (exposed for tests).
    static QByteArray mergePostLists(const QList<QByteArray> &bodies, const QString &sort);

private:
    ApiClient *m_client;
    QString m_instance;
};

} // namespace quokka
