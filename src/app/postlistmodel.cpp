#include "postlistmodel.h"

#include "parsers.h"
#include "postmap.h"
#include "backend.h"
#include "services.h"

#include <QUrlQuery>

using quokka::Post;

namespace {

const QStringList kSorts = {QStringLiteral("hot"), QStringLiteral("new"), QStringLiteral("top"),
                            QStringLiteral("rising"), QStringLiteral("active"), QStringLiteral("controversial"),
                            QStringLiteral("relevance"), QStringLiteral("comments")};
const QStringList kRanges = {QStringLiteral("hour"), QStringLiteral("day"), QStringLiteral("week"),
                             QStringLiteral("month"), QStringLiteral("year"), QStringLiteral("all")};
constexpr int kMaxEmptyPagesInARow = 3;

QString normalizeFeed(QString feed)
{
    feed = feed.trimmed();
    for (const auto prefix : {"/r/", "r/", "/c/", "c/", "!", "/"}) {
        if (feed.startsWith(QLatin1String(prefix), Qt::CaseInsensitive)) {
            feed = feed.mid(int(qstrlen(prefix)));
            break;
        }
    }
    while (feed.endsWith(QLatin1Char('/')))
        feed.chop(1);
    feed.remove(QLatin1Char(' '));
    return feed.isEmpty() ? QStringLiteral("popular") : feed;
}

} // namespace

PostListModel::PostListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    scheduleReload();
}

PostListModel::~PostListModel()
{
    if (m_reply)
        m_reply->abort();
}

int PostListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_posts.size());
}

QVariant PostListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_posts.size())
        return {};
    return postRoleData(m_posts.at(index.row()), role);
}

QVariant PostListModel::postRoleData(const Post &p, int role)
{
    switch (role) {
    case IdRole: return p.id;
    case FullnameRole: return p.fullname;
    case TitleRole: return p.title;
    case AuthorRole: return p.author;
    case SubredditRole: return p.subreddit;
    case PermalinkRole: return p.permalink;
    case UrlRole: return p.url;
    case DomainRole: return p.domain;
    case SelftextRole: return p.selftext.left(600);
    case FlairRole: return p.flair;
    case FlairColorRole: return p.flairColor;
    case CreatedUtcRole: return p.createdUtc;
    case ScoreRole: return p.score;
    case NumCommentsRole: return p.numComments;
    case NsfwRole: return p.nsfw;
    case SpoilerRole: return p.spoiler;
    case StickiedRole: return p.stickied;
    case LockedRole: return p.locked;
    case KindRole: return postKindValue(p.kind);
    case PreviewUrlRole: return p.previewUrl;
    case PreviewFallbackUrlRole: return p.previewFallbackUrl;
    case PreviewWidthRole: return p.previewWidth;
    case PreviewHeightRole: return p.previewHeight;
    case VideoUrlRole: return p.videoUrl;
    case GalleryCountRole: return p.galleryCount;
    default: return {};
    }
}

QHash<int, QByteArray> PostListModel::roleNames() const
{
    return postRoleNames();
}

QHash<int, QByteArray> PostListModel::postRoleNames()
{
    return {
        {IdRole, "postId"},
        {FullnameRole, "fullname"},
        {TitleRole, "title"},
        {AuthorRole, "author"},
        {SubredditRole, "subreddit"},
        {PermalinkRole, "permalink"},
        {UrlRole, "url"},
        {DomainRole, "domain"},
        {SelftextRole, "selftext"},
        {FlairRole, "flair"},
        {FlairColorRole, "flairColor"},
        {CreatedUtcRole, "createdUtc"},
        {ScoreRole, "score"},
        {NumCommentsRole, "numComments"},
        {NsfwRole, "nsfw"},
        {SpoilerRole, "spoiler"},
        {StickiedRole, "stickied"},
        {LockedRole, "locked"},
        {KindRole, "kind"},
        {PreviewUrlRole, "previewUrl"},
        {PreviewFallbackUrlRole, "previewFallbackUrl"},
        {PreviewWidthRole, "previewWidth"},
        {PreviewHeightRole, "previewHeight"},
        {VideoUrlRole, "videoUrl"},
        {GalleryCountRole, "galleryCount"},
    };
}

QVariantMap PostListModel::get(int row) const
{
    if (row < 0 || row >= m_posts.size())
        return {};
    return postToVariantMap(m_posts.at(row));
}

void PostListModel::setFeed(const QString &feed)
{
    const QString f = normalizeFeed(feed);
    if (f == m_feed)
        return;
    m_feed = f;
    emit feedChanged();
    scheduleReload();
}

void PostListModel::setSort(const QString &sort)
{
    const QString s = kSorts.contains(sort.toLower()) ? sort.toLower() : QStringLiteral("hot");
    if (s == m_sort)
        return;
    m_sort = s;
    emit sortChanged();
    scheduleReload();
}

void PostListModel::setTimeRange(const QString &range)
{
    const QString r = kRanges.contains(range.toLower()) ? range.toLower() : QStringLiteral("day");
    if (r == m_timeRange)
        return;
    m_timeRange = r;
    emit timeRangeChanged();
    if (m_sort == QLatin1String("top") || m_sort == QLatin1String("controversial") || !m_search.isEmpty())
        scheduleReload();
}

void PostListModel::setShowNsfw(bool show)
{
    if (show == m_showNsfw)
        return;
    m_showNsfw = show;
    emit showNsfwChanged();
    scheduleReload();
}

void PostListModel::setSearchQuery(const QString &q)
{
    const QString v = q.trimmed();
    if (v == m_search)
        return;
    m_search = v;
    emit searchQueryChanged();
    scheduleReload();
}

void PostListModel::setUserName(const QString &name)
{
    QString v = name.trimmed();
    for (const auto prefix : {"/u/", "u/", "/user/", "user/"})
        if (v.startsWith(QLatin1String(prefix), Qt::CaseInsensitive))
            v = v.mid(int(qstrlen(prefix)));
    if (v == m_user)
        return;
    m_user = v;
    emit userNameChanged();
    scheduleReload();
}

void PostListModel::setEnabled(bool enabled)
{
    if (enabled == m_enabled)
        return;
    m_enabled = enabled;
    emit enabledChanged();
    scheduleReload();
}

quokka::FeedRequest PostListModel::feedRequest() const
{
    quokka::FeedRequest r;
    r.feed = m_feed;
    r.sort = m_sort;
    r.timeRange = m_timeRange;
    r.search = m_search;
    r.user = m_user;
    r.nsfw = m_showNsfw;
    r.after = m_after;
    return r;
}

void PostListModel::scheduleReload()
{
    // Coalesce several property changes made in the same event-loop turn into one request.
    if (m_reloadPending)
        return;
    m_reloadPending = true;
    QMetaObject::invokeMethod(this, [this] {
        m_reloadPending = false;
        request(true);
    }, Qt::QueuedConnection);
}

void PostListModel::refresh()
{
    request(true);
}

void PostListModel::loadMore()
{
    if (m_reply || !m_hasMore || !m_error.isEmpty())
        return;
    request(false);
}

void PostListModel::request(bool reset)
{
    auto *backend = services::backend();
    if (!backend)
        return;

    if (m_reply) {
        m_reply->abort();
        m_reply = nullptr;
    }
    if (!m_enabled) {
        if (reset && !m_posts.isEmpty()) {
            beginResetModel();
            m_posts.clear();
            m_seen.clear();
            endResetModel();
            emit countChanged();
        }
        emit loadingChanged();
        return;
    }
    if (reset) {
        m_after.clear();
        m_hasMore = true;
        emit hasMoreChanged();
    }
    m_refreshing = reset;
    setError({});

    quokka::FeedRequest feedRequest = this->feedRequest();
    if (reset)
        feedRequest.after.clear();
    quokka::ApiReply *reply = backend->posts(feedRequest);
    m_reply = reply;
    emit loadingChanged();

    if (reset)
        m_emptyPages = 0;

    connect(reply, &quokka::ApiReply::finished, this, [this, reply, reset, feedRequest](const QByteArray &body) {
        if (reply != m_reply)
            return;
        m_reply = nullptr;
        auto *backend = services::backend();
        if (!backend)
            return;
        const auto listing = backend->parsePosts(body, feedRequest);
        if (!listing.ok) {
            setError(listing.error);
            emit loadingChanged();
            return;
        }

        QList<Post> fresh;
        if (reset)
            m_seen.clear();
        for (const Post &p : listing.posts) {
            if (p.nsfw && !m_showNsfw)
                continue;
            if (m_seen.contains(p.id))
                continue;
            m_seen.insert(p.id);
            fresh.append(p);
        }

        if (reset) {
            beginResetModel();
            m_posts = fresh;
            endResetModel();
        } else if (!fresh.isEmpty()) {
            beginInsertRows({}, int(m_posts.size()), int(m_posts.size() + fresh.size() - 1));
            m_posts.append(fresh);
            endInsertRows();
        }
        m_after = listing.after;
        const bool more = !m_after.isEmpty();
        if (more != m_hasMore) {
            m_hasMore = more;
            emit hasMoreChanged();
        }
        emit countChanged();
        emit loadingChanged();

        // Skip ahead when a whole page was filtered out (e.g. NSFW), but not forever.
        m_emptyPages = fresh.isEmpty() ? m_emptyPages + 1 : 0;
        if (fresh.isEmpty() && more && m_emptyPages < kMaxEmptyPagesInARow)
            request(false);
    });
    connect(reply, &quokka::ApiReply::failed, this, [this, reply](const QString &message, int) {
        if (reply != m_reply)
            return;
        m_reply = nullptr;
        setError(message);
        emit loadingChanged();
    });
}

void PostListModel::setError(const QString &error)
{
    if (error == m_error)
        return;
    m_error = error;
    emit errorChanged();
}
