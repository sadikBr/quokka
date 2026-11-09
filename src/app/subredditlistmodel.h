#pragma once

#include "community.h"
#include "apiclient.h"

#include <QAbstractListModel>
#include <QPointer>
#include <QQmlEngine>

// Communities: search results for `query`, or popular communities when the query is empty.
class SubredditListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(bool showNsfw READ showNsfw WRITE setShowNsfw NOTIFY showNsfwChanged)
    // Only NSFW items (the "Show only NSFW" setting).
    Q_PROPERTY(bool nsfwOnly READ nsfwOnly WRITE setNsfwOnly NOTIFY nsfwOnlyChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY loadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        TitleRole,
        DescriptionRole,
        SubscribersRole,
        IconUrlRole,
        ColorRole,
        NsfwRole,
        CommunityIdRole,
        ActiveMonthRole,
        PostsRole,
    };

    explicit SubredditListModel(QObject *parent = nullptr);
    ~SubredditListModel() override;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString query() const { return m_query; }
    void setQuery(const QString &q);
    bool showNsfw() const { return m_showNsfw; }
    bool nsfwOnly() const { return m_nsfwOnly; }
    void setNsfwOnly(bool on);
    void setShowNsfw(bool show);
    bool loading() const { return m_reply != nullptr; }
    bool hasMore() const { return !m_after.isEmpty(); }
    QString error() const { return m_error; }
    int count() const { return int(m_items.size()); }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void loadMore();

signals:
    void queryChanged();
    void showNsfwChanged();
    void nsfwOnlyChanged();
    void loadingChanged();
    void errorChanged();
    void countChanged();

private:
    void request(bool reset);
    void scheduleReload();

    QString m_query;
    bool m_showNsfw = false;
    bool m_nsfwOnly = false;
    int m_skippedPages = 0;   // NSFW only: pages fetched in a row to fill the list
    QList<quokka::Subreddit> m_items;
    QString m_after;
    QString m_error;
    bool m_reloadPending = false;
    QPointer<quokka::ApiReply> m_reply;
};
