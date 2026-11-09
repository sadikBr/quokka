#pragma once

#include "post.h"
#include "redditclient.h"

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
    Q_PROPERTY(bool showNsfw READ showNsfw WRITE setShowNsfw NOTIFY showNsfwChanged)
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
        PreviewWidthRole,
        PreviewHeightRole,
        VideoUrlRole,
        GalleryCountRole,
    };

    explicit PostListModel(QObject *parent = nullptr);
    ~PostListModel() override;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString feed() const { return m_feed; }
    void setFeed(const QString &feed);
    QString sort() const { return m_sort; }
    void setSort(const QString &sort);
    QString timeRange() const { return m_timeRange; }
    void setTimeRange(const QString &range);
    bool showNsfw() const { return m_showNsfw; }
    void setShowNsfw(bool show);

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

    // Builds the API path for the current feed + sort, e.g. "/r/pics/top".
    QString apiPath() const;

signals:
    void feedChanged();
    void sortChanged();
    void timeRangeChanged();
    void showNsfwChanged();
    void loadingChanged();
    void hasMoreChanged();
    void errorChanged();
    void countChanged();

private:
    void request(bool reset);
    void setError(const QString &error);
    void scheduleReload();

    QString m_feed = QStringLiteral("popular");
    QString m_sort = QStringLiteral("hot");
    QString m_timeRange = QStringLiteral("day");
    bool m_showNsfw = false;

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
