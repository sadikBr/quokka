#include "commenttreemodel.h"

#include "backend.h"
#include "parsers.h"
#include "postmap.h"
#include "services.h"

#include <algorithm>

using quokka::Comment;

namespace {
const QStringList kSorts = {QStringLiteral("confidence"), QStringLiteral("top"),
                            QStringLiteral("new"), QStringLiteral("controversial"),
                            QStringLiteral("old"), QStringLiteral("qa")};

int rowKind(Comment::Kind k)
{
    switch (k) {
    case Comment::Kind::More: return CommentTreeModel::MoreRow;
    case Comment::Kind::Continue: return CommentTreeModel::ContinueRow;
    case Comment::Kind::Comment: break;
    }
    return CommentTreeModel::CommentRow;
}
} // namespace

CommentTreeModel::CommentTreeModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

CommentTreeModel::~CommentTreeModel()
{
    if (m_reply)
        m_reply->abort();
    for (const auto &r : std::as_const(m_moreReplies))
        if (r)
            r->abort();
}

int CommentTreeModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_visible.size());
}

QVariant CommentTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_visible.size())
        return {};
    const int i = m_visible.at(index.row());
    const Node &n = m_all.at(i);
    const Comment &c = n.c;
    switch (role) {
    case KindRole: return rowKind(c.kind);
    case CommentIdRole: return c.id;
    case FullnameRole: return c.fullname;
    case ParentIdRole: return c.parentId;
    case DepthRole: return c.depth;
    case AuthorRole: return c.author;
    case BodyHtmlRole: return c.bodyHtml;
    case HasSpoilersRole: return c.bodyHtml.contains(QLatin1String("QK_SPOILER"));
    case FlairRole: return c.flair;
    case DistinguishedRole: return c.distinguished;
    case ScoreRole: return c.score;
    case ScoreHiddenRole: return c.scoreHidden;
    case CreatedUtcRole: return c.createdUtc;
    case IsSubmitterRole: return c.isSubmitter;
    case StickiedRole: return c.stickied;
    case EditedRole: return c.edited;
    case CollapsedRole: return n.collapsed;
    case HiddenCountRole: return n.collapsed ? hiddenCount(i) : 0;
    case MoreCountRole: return c.moreCount;
    case LoadingMoreRole: return n.loadingMore;
    case MediaRole: return mediaToVariantList(c.media);
    default: return {};
    }
}

QHash<int, QByteArray> CommentTreeModel::roleNames() const
{
    return {
        {KindRole, "kind"},
        {CommentIdRole, "commentId"},
        {FullnameRole, "fullname"},
        {ParentIdRole, "parentId"},
        {DepthRole, "depth"},
        {AuthorRole, "author"},
        {BodyHtmlRole, "bodyHtml"},
        {HasSpoilersRole, "hasSpoilers"},
        {FlairRole, "flair"},
        {DistinguishedRole, "distinguished"},
        {ScoreRole, "score"},
        {ScoreHiddenRole, "scoreHidden"},
        {CreatedUtcRole, "createdUtc"},
        {IsSubmitterRole, "isSubmitter"},
        {StickiedRole, "stickied"},
        {EditedRole, "edited"},
        {CollapsedRole, "collapsed"},
        {HiddenCountRole, "hiddenCount"},
        {MoreCountRole, "moreCount"},
        {LoadingMoreRole, "loadingMore"},
        {MediaRole, "media"},
    };
}

void CommentTreeModel::setPostId(const QString &id)
{
    QString v = id.trimmed();
    if (v.startsWith(QLatin1String("t3_")))
        v = v.mid(3);
    if (v == m_postId)
        return;
    m_postId = v;
    emit postIdChanged();
    scheduleReload();
}

void CommentTreeModel::setFocusCommentId(const QString &id)
{
    QString v = id.trimmed();
    if (v.startsWith(QLatin1String("t1_")))
        v = v.mid(3);
    if (v == m_focus)
        return;
    m_focus = v;
    emit focusCommentIdChanged();
    scheduleReload();
}

void CommentTreeModel::setSort(const QString &sort)
{
    const QString s = kSorts.contains(sort) ? sort : QStringLiteral("confidence");
    if (s == m_sort)
        return;
    m_sort = s;
    emit sortChanged();
    scheduleReload();
}

void CommentTreeModel::scheduleReload()
{
    if (m_reloadPending)
        return;
    m_reloadPending = true;
    QMetaObject::invokeMethod(this, [this] {
        m_reloadPending = false;
        refresh();
    }, Qt::QueuedConnection);
}

void CommentTreeModel::setError(const QString &e)
{
    if (e == m_error)
        return;
    m_error = e;
    emit errorChanged();
}

void CommentTreeModel::refresh()
{
    auto *backend = services::backend();
    if (!backend || m_postId.isEmpty())
        return;
    if (m_reply)
        m_reply->abort();
    setError({});

    auto *reply = backend->thread(m_postId, m_focus, m_sort);
    m_reply = reply;
    emit loadingChanged();
    connect(reply, &quokka::ApiReply::finished, this, [this, reply](const QByteArray &body) {
        if (reply != m_reply)
            return;
        m_reply = nullptr;
        auto *backend = services::backend();
        if (!backend)
            return;
        const auto page = backend->parseThread(body);
        if (!page.ok) {
            setError(page.error);
        } else {
            m_post = postToVariantMap(page.post);
            emit postChanged();
            setComments(page.comments, page.post.author);
            m_loaded = true;
        }
        emit loadingChanged();
    });
    connect(reply, &quokka::ApiReply::failed, this, [this, reply](const QString &message) {
        if (reply != m_reply)
            return;
        m_reply = nullptr;
        setError(message);
        emit loadingChanged();
    });
}

void CommentTreeModel::setComments(const QList<Comment> &comments, const QString &linkAuthor)
{
    beginResetModel();
    m_linkAuthor = linkAuthor;
    m_all.clear();
    m_visible.clear();
    m_all.reserve(comments.size());
    for (const Comment &c : comments)
        m_all.append(Node{c});
    for (int i = 0; i < m_all.size(); ++i)
        m_visible.append(i);
    endResetModel();
    emit countChanged();
}

int CommentTreeModel::subtreeEnd(int index) const
{
    const int depth = m_all.at(index).c.depth;
    int j = index + 1;
    while (j < m_all.size() && m_all.at(j).c.depth > depth)
        ++j;
    return j;
}

QList<int> CommentTreeModel::visibleDescendants(int index) const
{
    QList<int> out;
    const int end = subtreeEnd(index);
    for (int j = index + 1; j < end;) {
        out.append(j);
        j = m_all.at(j).collapsed ? subtreeEnd(j) : j + 1;
    }
    return out;
}

int CommentTreeModel::hiddenCount(int index) const
{
    int n = 0;
    const int end = subtreeEnd(index);
    for (int j = index + 1; j < end; ++j)
        n += m_all.at(j).c.kind == Comment::Kind::More ? m_all.at(j).c.moreCount : 1;
    return n;
}

void CommentTreeModel::toggleCollapsed(int row)
{
    if (row < 0 || row >= m_visible.size())
        return;
    const int i = m_visible.at(row);
    Node &n = m_all[i];
    if (n.c.kind != Comment::Kind::Comment)
        return;

    if (!n.collapsed) {
        const int end = subtreeEnd(i);
        int k = 0;
        while (row + 1 + k < m_visible.size() && m_visible.at(row + 1 + k) < end)
            ++k;
        n.collapsed = true;
        if (k > 0) {
            beginRemoveRows({}, row + 1, row + k);
            m_visible.remove(row + 1, k);
            endRemoveRows();
            emit countChanged();
        }
    } else {
        n.collapsed = false;
        const QList<int> rows = visibleDescendants(i);
        if (!rows.isEmpty()) {
            beginInsertRows({}, row + 1, row + int(rows.size()));
            for (int k = 0; k < rows.size(); ++k)
                m_visible.insert(row + 1 + k, rows.at(k));
            endInsertRows();
            emit countChanged();
        }
    }
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {CollapsedRole, HiddenCountRole});
}

int CommentTreeModel::parentRow(int row) const
{
    if (row < 0 || row >= m_visible.size())
        return -1;
    const QString parent = m_all.at(m_visible.at(row)).c.parentId;
    for (int r = row - 1; r >= 0; --r)
        if (m_all.at(m_visible.at(r)).c.fullname == parent)
            return r;
    return -1;
}

int CommentTreeModel::nextTopLevel(int row, int direction) const
{
    const int step = direction > 0 ? 1 : -1;
    for (int r = row + step; r >= 0 && r < m_visible.size(); r += step)
        if (m_all.at(m_visible.at(r)).c.depth == 0)
            return r;
    return row;
}

void CommentTreeModel::loadMore(int row)
{
    auto *backend = services::backend();
    if (!backend || row < 0 || row >= m_visible.size())
        return;
    const int i = m_visible.at(row);
    Node &n = m_all[i];
    if (n.c.kind != Comment::Kind::More || n.loadingMore)
        return;

    const Comment more = n.c;
    n.loadingMore = true;
    emit dataChanged(index(row), index(row), {LoadingMoreRole});

    auto *reply = backend->moreComments(m_postId, more, m_sort);
    m_moreReplies.append(reply);

    const QString moreName = more.fullname;
    connect(reply, &quokka::ApiReply::finished, this, [this, more](const QByteArray &body) {
        auto *backend = services::backend();
        QString err;
        const QList<Comment> rows = backend ? backend->parseMoreComments(body, more, m_linkAuthor, &err)
                                            : QList<Comment>{};
        if (!err.isEmpty() || !backend)
            setMoreLoading(more.fullname, false);
        else
            insertMore(more.fullname, rows);
    });
    connect(reply, &quokka::ApiReply::failed, this, [this, moreName] { setMoreLoading(moreName, false); });
}

void CommentTreeModel::setMoreLoading(const QString &moreFullname, bool loading)
{
    for (int j = 0; j < m_all.size(); ++j) {
        if (m_all[j].c.fullname == moreFullname && m_all[j].c.kind == Comment::Kind::More) {
            m_all[j].loadingMore = loading;
            const int r = int(m_visible.indexOf(j));
            if (r >= 0)
                emit dataChanged(index(r), index(r), {LoadingMoreRole});
        }
    }
}

int CommentTreeModel::indexOfFullname(const QString &fullname, Comment::Kind kind) const
{
    for (int j = 0; j < m_all.size(); ++j)
        if (m_all[j].c.fullname == fullname && m_all[j].c.kind == kind)
            return j;
    return -1;
}

bool CommentTreeModel::isVisibleChildOf(int parent) const
{
    // A new child of `parent` is visible if the parent is visible and expanded.
    return parent >= 0 && m_visible.contains(parent) && !m_all.at(parent).collapsed;
}

void CommentTreeModel::insertNode(int at, const Comment &c, bool visible)
{
    for (int &v : m_visible)
        if (v >= at)
            ++v;
    m_all.insert(at, Node{c});
    if (!visible)
        return;
    const int row = int(std::lower_bound(m_visible.begin(), m_visible.end(), at) - m_visible.begin());
    beginInsertRows({}, row, row);
    m_visible.insert(row, at);
    endInsertRows();
}

void CommentTreeModel::insertMore(const QString &moreFullname, const QList<Comment> &rows)
{
    // 1. Remove the placeholder, remembering where it was.
    int anchor = indexOfFullname(moreFullname, Comment::Kind::More);
    if (anchor < 0)
        return;
    const QString anchorParent = m_all.at(anchor).c.parentId;
    const int anchorRow = int(m_visible.indexOf(anchor));
    if (anchorRow >= 0) {
        beginRemoveRows({}, anchorRow, anchorRow);
        m_visible.removeAt(anchorRow);
        endRemoveRows();
    }
    m_all.removeAt(anchor);
    for (int &v : m_visible)
        if (v > anchor)
            --v;

    // 2. Merge each new row under its parent (after the parent's existing replies). Rows whose
    //    parent is not on screen (e.g. more top-level comments) go where the placeholder was,
    //    each after the previous one's replies.
    const int anchorParentIndex = indexOfFullname(anchorParent, Comment::Kind::Comment);
    QString lastOrphan;
    Comment::Kind lastOrphanKind = Comment::Kind::Comment;
    for (const Comment &incoming : rows) {
        if (indexOfFullname(incoming.fullname, incoming.kind) >= 0)
            continue; // already shown
        Comment c = incoming;
        const int parent = indexOfFullname(c.parentId, Comment::Kind::Comment);
        int at;
        bool visible;
        if (parent >= 0) {
            c.depth = m_all.at(parent).c.depth + 1;
            at = subtreeEnd(parent);
            visible = isVisibleChildOf(parent);
        } else {
            const int parentOfAnchor = indexOfFullname(anchorParent, Comment::Kind::Comment);
            c.depth = parentOfAnchor >= 0 ? m_all.at(parentOfAnchor).c.depth + 1 : 0;
            const int prev = lastOrphan.isEmpty() ? -1 : indexOfFullname(lastOrphan, lastOrphanKind);
            at = prev >= 0 ? subtreeEnd(prev) : qMin(anchor, int(m_all.size()));
            visible = anchorParentIndex < 0 || isVisibleChildOf(parentOfAnchor);
            lastOrphan = c.fullname;
            lastOrphanKind = c.kind;
        }
        if (at <= anchor)
            ++anchor;
        insertNode(at, c, visible);
    }
    emit countChanged();
}
