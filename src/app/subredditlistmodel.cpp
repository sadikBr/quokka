#include "subredditlistmodel.h"

#include "parsers.h"
#include "services.h"

#include <QUrlQuery>

SubredditListModel::SubredditListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    scheduleReload();
}

SubredditListModel::~SubredditListModel()
{
    if (m_reply)
        m_reply->abort();
}

int SubredditListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_items.size());
}

QVariant SubredditListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};
    const auto &s = m_items.at(index.row());
    switch (role) {
    case NameRole: return s.name;
    case TitleRole: return s.title;
    case DescriptionRole: return s.publicDescription;
    case SubscribersRole: return s.subscribers;
    case IconUrlRole: return s.iconUrl;
    case ColorRole: return s.color;
    case NsfwRole: return s.nsfw;
    default: return {};
    }
}

QHash<int, QByteArray> SubredditListModel::roleNames() const
{
    return {{NameRole, "name"},          {TitleRole, "title"},     {DescriptionRole, "description"},
            {SubscribersRole, "subscribers"}, {IconUrlRole, "iconUrl"}, {ColorRole, "color"},
            {NsfwRole, "nsfw"}};
}

void SubredditListModel::setQuery(const QString &q)
{
    const QString v = q.trimmed();
    if (v == m_query)
        return;
    m_query = v;
    emit queryChanged();
    scheduleReload();
}

void SubredditListModel::setShowNsfw(bool show)
{
    if (show == m_showNsfw)
        return;
    m_showNsfw = show;
    emit showNsfwChanged();
    scheduleReload();
}

void SubredditListModel::scheduleReload()
{
    if (m_reloadPending)
        return;
    m_reloadPending = true;
    QMetaObject::invokeMethod(this, [this] {
        m_reloadPending = false;
        request(true);
    }, Qt::QueuedConnection);
}

void SubredditListModel::refresh()
{
    request(true);
}

void SubredditListModel::loadMore()
{
    if (!m_reply && !m_after.isEmpty() && m_error.isEmpty())
        request(false);
}

void SubredditListModel::request(bool reset)
{
    auto *client = services::reddit();
    if (!client)
        return;
    if (m_reply)
        m_reply->abort();
    if (reset)
        m_after.clear();
    if (!m_error.isEmpty()) {
        m_error.clear();
        emit errorChanged();
    }

    QUrlQuery q;
    q.addQueryItem(QStringLiteral("limit"), QStringLiteral("25"));
    if (!reset && !m_after.isEmpty())
        q.addQueryItem(QStringLiteral("after"), m_after);
    if (m_showNsfw)
        q.addQueryItem(QStringLiteral("include_over_18"), QStringLiteral("on"));
    QString path = QStringLiteral("/subreddits/popular");
    if (!m_query.isEmpty()) {
        path = QStringLiteral("/subreddits/search");
        q.addQueryItem(QStringLiteral("q"), m_query);
    }

    auto *reply = client->get(path, q);
    m_reply = reply;
    emit loadingChanged();
    connect(reply, &quokka::ApiReply::finished, this, [this, reply, reset](const QByteArray &body) {
        if (reply != m_reply)
            return;
        m_reply = nullptr;
        QString err;
        QList<quokka::Subreddit> items = quokka::parsers::parseSubredditListing(body, &m_after, &err);
        if (!err.isEmpty()) {
            m_error = err;
            emit errorChanged();
        }
        if (!m_showNsfw)
            items.removeIf([](const quokka::Subreddit &s) { return s.nsfw; });
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
