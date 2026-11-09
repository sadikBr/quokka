#include "usercommentmodel.h"

#include "backend.h"
#include "postmap.h"
#include "services.h"


UserCommentModel::UserCommentModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

UserCommentModel::~UserCommentModel()
{
    if (m_reply)
        m_reply->abort();
}

int UserCommentModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_items.size());
}

QVariant UserCommentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};
    const auto &c = m_items.at(index.row());
    switch (role) {
    case CommentIdRole: return c.id;
    case BodyHtmlRole: return c.bodyHtml;
    case ScoreRole: return c.score;
    case CreatedUtcRole: return c.createdUtc;
    case SubredditRole: return c.subreddit;
    case LinkTitleRole: return c.linkTitle;
    case PermalinkRole: return c.permalink;
    case MediaRole: return mediaToVariantList(c.media);
    default: return {};
    }
}

QHash<int, QByteArray> UserCommentModel::roleNames() const
{
    return {{CommentIdRole, "commentId"}, {BodyHtmlRole, "bodyHtml"},   {ScoreRole, "score"},
            {CreatedUtcRole, "createdUtc"}, {SubredditRole, "subreddit"}, {LinkTitleRole, "linkTitle"},
            {PermalinkRole, "permalink"}, {MediaRole, "media"}};
}

void UserCommentModel::setUserName(const QString &name)
{
    if (name == m_user)
        return;
    m_user = name;
    emit userNameChanged();
    scheduleReload();
}

void UserCommentModel::setSort(const QString &sort)
{
    if (sort == m_sort)
        return;
    m_sort = sort;
    emit sortChanged();
    scheduleReload();
}

void UserCommentModel::setNsfwOnly(bool on)
{
    if (on == m_nsfwOnly)
        return;
    m_nsfwOnly = on;
    emit nsfwOnlyChanged();
    scheduleReload();
}

void UserCommentModel::setShowNsfw(bool show)
{
    if (show == m_showNsfw)
        return;
    m_showNsfw = show;
    emit showNsfwChanged();
    scheduleReload();
}

void UserCommentModel::setEnabled(bool enabled)
{
    if (enabled == m_enabled)
        return;
    m_enabled = enabled;
    emit enabledChanged();
    if (m_enabled && m_items.isEmpty())
        scheduleReload();
}

void UserCommentModel::scheduleReload()
{
    if (m_reloadPending)
        return;
    m_reloadPending = true;
    QMetaObject::invokeMethod(this, [this] {
        m_reloadPending = false;
        request(true);
    }, Qt::QueuedConnection);
}

void UserCommentModel::refresh()
{
    request(true);
}

void UserCommentModel::loadMore()
{
    if (!m_reply && !m_after.isEmpty() && m_error.isEmpty())
        request(false);
}

void UserCommentModel::request(bool reset)
{
    auto *backend = services::backend();
    if (!backend || m_user.isEmpty() || !m_enabled)
        return;
    if (m_reply)
        m_reply->abort();
    if (reset)
        m_after.clear();
    if (!m_error.isEmpty()) {
        m_error.clear();
        emit errorChanged();
    }

    const QString after = m_after;
    auto *reply = backend->userComments(m_user, m_sort, after);
    m_reply = reply;
    emit loadingChanged();
    connect(reply, &quokka::ApiReply::finished, this, [this, reply, reset, after](const QByteArray &body) {
        if (reply != m_reply)
            return;
        m_reply = nullptr;
        auto *backend = services::backend();
        if (!backend)
            return;
        QString err;
        QList<quokka::Comment> items = backend->parseUserComments(body, after, &m_after, &err);
        if (!err.isEmpty()) {
            m_error = err;
            emit errorChanged();
        }
        if (!m_showNsfw)
            items.removeIf([](const quokka::Comment &c) { return c.nsfw; });
        if (m_nsfwOnly)
            items.removeIf([](const quokka::Comment &x) { return !x.nsfw; });
        if (reset) {
            beginResetModel();
            m_items = items;
            endResetModel();
        } else if (!items.isEmpty()) {
            beginInsertRows({}, int(m_items.size()), int(m_items.size() + items.size() - 1));
            m_items.append(items);
            endInsertRows();
        }
        emit countChanged();
        emit loadingChanged();
    });
    connect(reply, &quokka::ApiReply::failed, this, [this, reply](const QString &message) {
        if (reply != m_reply)
            return;
        m_reply = nullptr;
        m_error = message;
        emit errorChanged();
        emit loadingChanged();
    });
}
