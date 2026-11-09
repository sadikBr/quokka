#pragma once

#include "comment.h"
#include "apiclient.h"

#include <QAbstractListModel>
#include <QPointer>
#include <QQmlEngine>

// A user's recent comments (/user/<name>/comments), each with the post it belongs to.
class UserCommentModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString userName READ userName WRITE setUserName NOTIFY userNameChanged)
    Q_PROPERTY(QString sort READ sort WRITE setSort NOTIFY sortChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool showNsfw READ showNsfw WRITE setShowNsfw NOTIFY showNsfwChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY loadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        CommentIdRole = Qt::UserRole + 1,
        BodyHtmlRole,
        ScoreRole,
        CreatedUtcRole,
        SubredditRole,
        LinkTitleRole,
        PermalinkRole,
    };

    explicit UserCommentModel(QObject *parent = nullptr);
    ~UserCommentModel() override;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString userName() const { return m_user; }
    void setUserName(const QString &name);
    QString sort() const { return m_sort; }
    void setSort(const QString &sort);
    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);
    bool showNsfw() const { return m_showNsfw; }
    void setShowNsfw(bool show);
    bool loading() const { return m_reply != nullptr; }
    bool hasMore() const { return !m_after.isEmpty(); }
    QString error() const { return m_error; }
    int count() const { return int(m_items.size()); }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void loadMore();

signals:
    void userNameChanged();
    void sortChanged();
    void enabledChanged();
    void showNsfwChanged();
    void loadingChanged();
    void errorChanged();
    void countChanged();

private:
    void request(bool reset);
    void scheduleReload();

    QString m_user;
    QString m_sort = QStringLiteral("new");
    bool m_enabled = true;
    bool m_showNsfw = false;
    bool m_reloadPending = false;
    QList<quokka::Comment> m_items;
    QString m_after;
    QString m_error;
    QPointer<quokka::ApiReply> m_reply;
};
