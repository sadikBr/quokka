#pragma once

#include "comment.h"
#include "apiclient.h"

#include <QAbstractListModel>
#include <QPointer>
#include <QQmlEngine>
#include <QVariantMap>

// The comment thread of one post, flattened for a ListView. Supports collapsing subtrees,
// "load more comments" and focusing on a single comment's thread.
class CommentTreeModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString postId READ postId WRITE setPostId NOTIFY postIdChanged)
    Q_PROPERTY(QString focusCommentId READ focusCommentId WRITE setFocusCommentId NOTIFY focusCommentIdChanged)
    Q_PROPERTY(QString sort READ sort WRITE setSort NOTIFY sortChanged)
    // The server the post lives on, when it isn't the one being read (combined feed).
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QVariantMap post READ post NOTIFY postChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum RowKind { CommentRow, MoreRow, ContinueRow };
    Q_ENUM(RowKind)

    enum Role {
        KindRole = Qt::UserRole + 1,
        CommentIdRole,
        FullnameRole,
        ParentIdRole,
        DepthRole,
        AuthorRole,
        BodyHtmlRole,
        HasSpoilersRole,
        FlairRole,
        DistinguishedRole,
        ScoreRole,
        ScoreHiddenRole,
        CreatedUtcRole,
        IsSubmitterRole,
        StickiedRole,
        EditedRole,
        CollapsedRole,
        HiddenCountRole,
        MoreCountRole,
        LoadingMoreRole,
        MediaRole,
    };

    explicit CommentTreeModel(QObject *parent = nullptr);
    ~CommentTreeModel() override;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString postId() const { return m_postId; }
    void setPostId(const QString &id);
    QString focusCommentId() const { return m_focus; }
    void setFocusCommentId(const QString &id);
    QString source() const { return m_source; }
    void setSource(const QString &source);
    QString sort() const { return m_sort; }
    void setSort(const QString &sort);
    QVariantMap post() const { return m_post; }
    bool loading() const { return m_reply != nullptr; }
    bool loaded() const { return m_loaded; }
    QString error() const { return m_error; }
    int count() const { return int(m_visible.size()); }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void toggleCollapsed(int row);
    Q_INVOKABLE void loadMore(int row);
    // Row of the parent comment, or -1 (used for "jump to parent").
    Q_INVOKABLE int parentRow(int row) const;
    // Next (direction > 0) or previous top-level comment row from `row`, or `row` if none.
    Q_INVOKABLE int nextTopLevel(int row, int direction) const;

    // Exposed for tests.
    void setComments(const QList<quokka::Comment> &comments, const QString &linkAuthor = {});
    // Replaces a "load more" row with what it stood for: each new comment goes under its parent,
    // comments already on screen are skipped.
    void insertMore(const QString &moreFullname, const QList<quokka::Comment> &rows);

signals:
    void postIdChanged();
    void focusCommentIdChanged();
    void sortChanged();
    void sourceChanged();
    void postChanged();
    void loadingChanged();
    void errorChanged();
    void countChanged();

private:
    struct Node
    {
        quokka::Comment c;
        bool collapsed = false;
        bool loadingMore = false;
    };

    void scheduleReload();
    void setError(const QString &e);
    int subtreeEnd(int index) const;     // one past the last descendant in m_all
    int indexOfFullname(const QString &fullname, quokka::Comment::Kind kind) const;
    bool isVisibleChildOf(int parent) const;
    void insertNode(int at, const quokka::Comment &c, bool visible);
    void setMoreLoading(const QString &moreFullname, bool loading);
    QList<int> visibleDescendants(int index) const;
    int hiddenCount(int index) const;

    QString m_postId;
    QString m_focus;
    QString m_sort = QStringLiteral("confidence");
    QString m_source;
    QString m_linkAuthor;
    QVariantMap m_post;
    QList<Node> m_all;
    QList<int> m_visible; // indices into m_all
    bool m_loaded = false;
    bool m_reloadPending = false;
    QString m_error;
    QPointer<quokka::ApiReply> m_reply;
    QList<QPointer<quokka::ApiReply>> m_moreReplies;
};
