#pragma once

#include "post.h"
#include "apiclient.h"
#include "backend.h"

#include <QAbstractListModel>
#include <QPointer>
#include <QQmlEngine>
#include <QSet>

// A paged feed of posts: a subreddit ("pics"), a multireddit ("pics+aww"), or "popular"/"all".
class PostListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString feed READ feed WRITE setFeed NOTIFY feedChanged)
    Q_PROPERTY(QString sort READ sort WRITE setSort NOTIFY sortChanged)
    Q_PROPERTY(QString timeRange READ timeRange WRITE setTimeRange NOTIFY timeRangeChanged)
    // When set, lists posts matching this search (inside `feed` unless it is popular/all).
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    // When set, lists the posts submitted by this user instead of a feed.
    Q_PROPERTY(QString userName READ userName WRITE setUserName NOTIFY userNameChanged)
    // When false, nothing is fetched (e.g. a search page with an empty query).
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool showNsfw READ showNsfw WRITE setShowNsfw NOTIFY showNsfwChanged)
    // Only NSFW items (the "Show only NSFW" setting).
    Q_PROPERTY(bool nsfwOnly READ nsfwOnly WRITE setNsfwOnly NOTIFY nsfwOnlyChanged)
    // Only posts with pictures or videos (the swipe-through media feed).
    Q_PROPERTY(bool mediaOnly READ mediaOnly WRITE setMediaOnly NOTIFY mediaOnlyChanged)
    // Only posts with a video or an animated picture (GIF), not plain photos.
    // Check media links in the background and drop posts (or gallery items) whose media is
    // dead; links found dead before are dropped right away (see quokka::MediaProbe).
    Q_PROPERTY(bool hideBroken READ hideBroken WRITE setHideBroken NOTIFY hideBrokenChanged)
    Q_PROPERTY(bool animatedOnly READ animatedOnly WRITE setAnimatedOnly NOTIFY animatedOnlyChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool refreshing READ refreshing NOTIFY loadingChanged)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY hasMoreChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Kind { SelfPost, LinkPost, ImagePost, GalleryPost, VideoPost };
    Q_ENUM(Kind)

    enum Role {
        IdRole = Qt::UserRole + 1,
        FullnameRole,
        TitleRole,
        AuthorRole,
        SubredditRole,
        PermalinkRole,
        UrlRole,
        DomainRole,
        SelftextRole,
        FlairRole,
        FlairColorRole,
        CreatedUtcRole,
        ScoreRole,
        NumCommentsRole,
        NsfwRole,
        SpoilerRole,
        StickiedRole,
        LockedRole,
        KindRole,
        PreviewUrlRole,
        PreviewFallbackUrlRole,
        PreviewWidthRole,
        PreviewHeightRole,
        VideoUrlRole,
        GalleryCountRole,
        MediaRole,
    };

    explicit PostListModel(QObject *parent = nullptr);
    ~PostListModel() override;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Shared with other post models so PostCard works with all of them.
    static QHash<int, QByteArray> postRoleNames();
    static QVariant postRoleData(const quokka::Post &post, int role);

    QString feed() const { return m_feed; }
    void setFeed(const QString &feed);
    QString sort() const { return m_sort; }
    void setSort(const QString &sort);
    QString timeRange() const { return m_timeRange; }
    void setTimeRange(const QString &range);
    bool showNsfw() const { return m_showNsfw; }
    bool nsfwOnly() const { return m_nsfwOnly; }
    void setNsfwOnly(bool on);
    void setShowNsfw(bool show);
    bool mediaOnly() const { return m_mediaOnly; }
    void setMediaOnly(bool on);
    bool hideBroken() const { return m_hideBroken; }
    void setHideBroken(bool on);
    // Media that turned out not to load after all (e.g. a video that won't play): remember the
    // links as dead and drop that media from the post (the post itself if nothing is left).
    Q_INVOKABLE void reportBroken(const QString &postId, const QStringList &urls);
    // Each media item of a post as its alternative links, first the item's own URL.
    static QList<QStringList> mediaGroups(const quokka::Post &post);
    bool animatedOnly() const { return m_animatedOnly; }
    void setAnimatedOnly(bool on);
    static bool isAnimated(const quokka::Post &post);
    QString searchQuery() const { return m_search; }
    void setSearchQuery(const QString &q);
    QString userName() const { return m_user; }
    void setUserName(const QString &name);
    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    bool loading() const { return m_reply != nullptr; }
    bool refreshing() const { return m_reply != nullptr && m_refreshing; }
    bool hasMore() const { return m_hasMore; }
    QString error() const { return m_error; }
    int count() const { return int(m_posts.size()); }

    // Reloads the first page. Existing posts stay visible until the new page arrives.
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void loadMore();
    // Everything about one post as a map (role names as keys), for the post page.
    Q_INVOKABLE QVariantMap get(int row) const;

    // What to ask the backend for, given the current properties (exposed for tests).
    quokka::FeedRequest feedRequest() const;

signals:
    void feedChanged();
    void sortChanged();
    void timeRangeChanged();
    void showNsfwChanged();
    void nsfwOnlyChanged();
    void mediaOnlyChanged();
    void animatedOnlyChanged();
    void hideBrokenChanged();
    void searchQueryChanged();
    void userNameChanged();
    void enabledChanged();
    void loadingChanged();
    void hasMoreChanged();
    void errorChanged();
    void countChanged();

private:
    void request(bool reset);
    void setError(const QString &error);
    void scheduleReload();

    bool m_mediaOnly = false;
    bool m_animatedOnly = false;
    bool m_hideBroken = false;
    // Drops known-dead media from `posts` (and posts left without any).
    void dropKnownDead(QList<quokka::Post> &posts) const;
    void probe(const QList<quokka::Post> &posts);
    void dropMedia(const QString &postId, const QString &itemUrl);
    QString m_feed = QStringLiteral("popular");
    QString m_sort = QStringLiteral("hot");
    QString m_timeRange = QStringLiteral("day");
    bool m_showNsfw = false;
    bool m_nsfwOnly = false;
    QString m_search;
    QString m_user;
    bool m_enabled = true;

    QList<quokka::Post> m_posts;
    QSet<QString> m_seen;
    QString m_after;
    bool m_hasMore = true;
    bool m_refreshing = false;
    bool m_reloadPending = false;
    int m_emptyPages = 0;
    QString m_error;
    QPointer<quokka::ApiReply> m_reply;
};
