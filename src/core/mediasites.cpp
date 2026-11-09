#include "mediasites.h"

#include "apiclient.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>
#include <QUrl>
#include <QUrlQuery>

namespace quokka {

namespace {

int pageOf(const QString &after)
{
    const int p = after.toInt();
    return p > 0 ? p : 1;
}

QString nextPage(int page, int pages, qsizetype got)
{
    if (got == 0)
        return {};
    if (pages > 0 && page >= pages)
        return {};
    return QString::number(page + 1);
}

QString tagFrom(const QString &feed)
{
    return feed.startsWith(QLatin1String("tag:")) ? feed.mid(4).trimmed() : QString();
}

bool isImageUrl(const QString &url)
{
    const QString path = QUrl(url).path().toLower();
    return path.endsWith(QLatin1String(".jpg")) || path.endsWith(QLatin1String(".jpeg"))
        || path.endsWith(QLatin1String(".png")) || path.endsWith(QLatin1String(".webp"));
}

} // namespace

// ---- MediaSiteBackend -------------------------------------------------------------------

ApiReply *MediaSiteBackend::unsupported() const
{
    return new FailedReply(QObject::tr("%1 has no comments or communities.").arg(displayName()));
}

ApiReply *MediaSiteBackend::thread(const QString &, const QString &, const QString &) { return unsupported(); }
ApiReply *MediaSiteBackend::moreComments(const QString &, const Comment &, const QString &) { return unsupported(); }
ApiReply *MediaSiteBackend::community(const QString &) { return unsupported(); }
ApiReply *MediaSiteBackend::communities(const QString &, bool, const QString &) { return unsupported(); }
ApiReply *MediaSiteBackend::user(const QString &) { return unsupported(); }
ApiReply *MediaSiteBackend::userComments(const QString &, const QString &, const QString &) { return unsupported(); }

// ---- Redgifs ----------------------------------------------------------------------------

QString RedgifsBackend::requestPath(const FeedRequest &r, QUrlQuery *query)
{
    const int page = pageOf(r.after);
    QString order;
    if (!r.user.isEmpty()) {
        order = r.sort == QLatin1String("top") ? QStringLiteral("top") : QStringLiteral("new");
    } else if (r.sort == QLatin1String("new")) {
        order = QStringLiteral("latest");
    } else if (r.sort == QLatin1String("top")) {
        order = r.timeRange == QLatin1String("week")    ? QStringLiteral("top7")
              : r.timeRange == QLatin1String("month")   ? QStringLiteral("top28")
                                                        : QStringLiteral("top");
    } else {
        order = QStringLiteral("trending");
    }
    // Built as text: several tags are joined with a literal '+', which must reach the server
    // encoded (%2B), not as a space.
    QString q = QStringLiteral("order=%1&count=40&page=%2").arg(order).arg(page);

    QString path;
    if (!r.user.isEmpty()) {
        path = QStringLiteral("/v2/users/%1/search").arg(QString::fromUtf8(QUrl::toPercentEncoding(r.user.toLower())));
        q += QStringLiteral("&type=g");
    } else if (r.feed.startsWith(QLatin1String("niche:"))) {
        path = QStringLiteral("/v2/niches/%1/gifs").arg(QString::fromUtf8(QUrl::toPercentEncoding(r.feed.mid(6))));
    } else {
        path = QStringLiteral("/v2/gifs/search");
        q += QStringLiteral("&type=g");
        const QString tags = !r.search.isEmpty() ? r.search : tagFrom(r.feed);
        if (!tags.isEmpty())
            q += QStringLiteral("&tags=") + QString::fromUtf8(QUrl::toPercentEncoding(tags));
    }
    if (query)
        *query = QUrlQuery(q);
    return path;
}

ApiReply *RedgifsBackend::suggest(const QString &text)
{
    return withToken([this, text] {
        return m_client->get(QStringLiteral("/v2/search/suggest"),
                             QUrlQuery(QStringLiteral("query=") + QString::fromUtf8(QUrl::toPercentEncoding(text))));
    });
}

ApiReply *RedgifsBackend::trendingTags()
{
    return withToken([this] { return m_client->get(QStringLiteral("/v2/search/trending")); });
}

namespace {
// The suggestion / tag lists come as a bare array or wrapped in an object; accept both.
QJsonArray listIn(const QByteArray &body)
{
    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (doc.isArray())
        return doc.array();
    const QJsonObject o = doc.object();
    for (const char *key : {"tags", "items", "suggestions", "results", "niches"})
        if (o.value(QLatin1String(key)).isArray())
            return o.value(QLatin1String(key)).toArray();
    return {};
}
} // namespace

QList<RedgifsBackend::Suggestion> RedgifsBackend::parseSuggestions(const QByteArray &body)
{
    QList<Suggestion> out;
    for (const QJsonValue &v : listIn(body)) {
        const QJsonObject o = v.toObject();
        Suggestion s;
        s.text = o.value(QLatin1String("text")).toString(o.value(QLatin1String("name")).toString());
        s.type = o.value(QLatin1String("type")).toString(QStringLiteral("tag")).toLower();
        s.count = o.value(QLatin1String("gifs")).toInt(o.value(QLatin1String("count")).toInt());
        if (!s.text.isEmpty())
            out.append(s);
    }
    return out;
}

QList<RedgifsBackend::Suggestion> RedgifsBackend::parseTags(const QByteArray &body)
{
    QList<Suggestion> out;
    for (const QJsonValue &v : listIn(body)) {
        Suggestion s;
        if (v.isString()) {
            s.text = v.toString();
        } else {
            const QJsonObject o = v.toObject();
            s.text = o.value(QLatin1String("name")).toString(o.value(QLatin1String("text")).toString());
            s.count = o.value(QLatin1String("count")).toInt(o.value(QLatin1String("gifs")).toInt());
        }
        s.type = QStringLiteral("tag");
        if (!s.text.isEmpty())
            out.append(s);
    }
    return out;
}

ApiReply *RedgifsBackend::withToken(std::function<ApiReply *()> request)
{
    auto wrap = [this](ApiReply *reply) {
        // An expired token: get a new one next time.
        QObject::connect(reply, &ApiReply::failed, reply, [this](const QString &, int status) {
            if (status == 401 || status == 403) {
                m_token.clear();
                m_client->setAuthToken({});
            }
        });
        return reply;
    };
    if (!m_token.isEmpty())
        return wrap(request());
    // redgifs hands out anonymous temporary tokens for its public API.
    ApiReply *auth = m_client->get(QStringLiteral("/v2/auth/temporary"));
    return wrap(new ChainReply(auth, [this, request](const QByteArray &body) -> ApiReply * {
        m_token = QJsonDocument::fromJson(body).object().value(QLatin1String("token")).toString().toUtf8();
        m_client->setAuthToken(QString::fromUtf8(m_token));
        return request();
    }));
}

ApiReply *RedgifsBackend::posts(const FeedRequest &request)
{
    return withToken([this, request] {
        QUrlQuery q;
        const QString path = requestPath(request, &q);
        return m_client->get(path, q);
    });
}

parsers::PostListing RedgifsBackend::parsePosts(const QByteArray &body, const FeedRequest &request) const
{
    return parse(body, pageOf(request.after));
}

parsers::PostListing RedgifsBackend::parse(const QByteArray &body, int page)
{
    parsers::PostListing out;
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (!doc.isObject()) {
        out.error = QObject::tr("Unexpected answer from Redgifs.");
        return out;
    }
    const QJsonObject root = doc.object();
    if (root.contains(QLatin1String("error"))) {
        const QJsonObject e = root.value(QLatin1String("error")).toObject();
        out.error = e.value(QLatin1String("message")).toString(QObject::tr("Redgifs refused the request."));
        return out;
    }
    const QJsonArray gifs = root.value(QLatin1String("gifs")).toArray();
    for (const QJsonValue &v : gifs) {
        const QJsonObject g = v.toObject();
        const QJsonObject urls = g.value(QLatin1String("urls")).toObject();
        const QString hd = urls.value(QLatin1String("hd")).toString();
        const QString sd = urls.value(QLatin1String("sd")).toString();
        const QString file = hd.isEmpty() ? sd : hd;
        if (file.isEmpty())
            continue;
        Post p;
        p.id = g.value(QLatin1String("id")).toString();
        p.fullname = QStringLiteral("redgifs_") + p.id;
        p.source = QStringLiteral("redgifs");
        p.author = g.value(QLatin1String("userName")).toString();
        p.permalink = QStringLiteral("https://www.redgifs.com/watch/") + p.id;
        p.url = p.permalink;
        p.domain = QStringLiteral("redgifs.com");
        for (const QJsonValue &t : g.value(QLatin1String("tags")).toArray())
            p.tags.append(t.toString());
        const QString description = g.value(QLatin1String("description")).toString().trimmed();
        if (!description.isEmpty()) {
            p.title = description;
        } else if (!p.tags.isEmpty()) {
            QStringList hashed;
            for (const QString &t : p.tags.mid(0, 6))
                hashed.append(QLatin1Char('#') + t);
            p.title = hashed.join(QLatin1Char(' '));
        } else {
            p.title = p.id;
        }
        p.createdUtc = qint64(g.value(QLatin1String("createDate")).toDouble());
        p.score = g.value(QLatin1String("likes")).toInt();
        p.nsfw = true;
        p.previewWidth = g.value(QLatin1String("width")).toInt();
        p.previewHeight = g.value(QLatin1String("height")).toInt();
        p.previewUrl = urls.value(QLatin1String("poster")).toString();
        if (p.previewUrl.isEmpty())
            p.previewUrl = urls.value(QLatin1String("thumbnail")).toString();

        MediaItem m;
        m.url = file;
        m.width = p.previewWidth;
        m.height = p.previewHeight;
        m.poster = p.previewUrl;
        if (isImageUrl(file)) {
            p.kind = Post::Kind::Image;
            if (p.previewUrl.isEmpty())
                p.previewUrl = file;
        } else {
            p.kind = Post::Kind::Video;
            m.video = true;
            m.animated = !g.value(QLatin1String("hasAudio")).toBool();
            if (!sd.isEmpty() && sd != file)
                m.alternates.append(sd);
            p.videoUrl = file;
        }
        p.media.append(m);
        out.posts.append(p);
    }
    out.after = nextPage(page, root.value(QLatin1String("pages")).toInt(), out.posts.size());
    out.ok = true;
    return out;
}

// ---- Eporner ----------------------------------------------------------------------------

QUrlQuery EpornerBackend::requestQuery(const FeedRequest &r)
{
    QUrlQuery q;
    QString text = !r.search.isEmpty() ? r.search : tagFrom(r.feed);
    if (!r.user.isEmpty())
        text = r.user; // no creators: search their name
    q.addQueryItem(QStringLiteral("query"), text.isEmpty() ? QStringLiteral("all") : text);
    q.addQueryItem(QStringLiteral("per_page"), QStringLiteral("30"));
    q.addQueryItem(QStringLiteral("page"), QString::number(pageOf(r.after)));
    q.addQueryItem(QStringLiteral("thumbsize"), QStringLiteral("big"));
    QString order;
    if (r.sort == QLatin1String("new"))
        order = QStringLiteral("latest");
    else if (r.sort == QLatin1String("top"))
        order = r.timeRange == QLatin1String("month") ? QStringLiteral("top-monthly")
              : (r.timeRange == QLatin1String("year") || r.timeRange == QLatin1String("all")) ? QStringLiteral("top-rated")
                                                                                             : QStringLiteral("top-weekly");
    else
        order = QStringLiteral("most-popular");
    q.addQueryItem(QStringLiteral("order"), order);
    q.addQueryItem(QStringLiteral("gay"), QStringLiteral("0"));
    q.addQueryItem(QStringLiteral("lq"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    return q;
}

ApiReply *EpornerBackend::posts(const FeedRequest &request)
{
    return m_client->get(QStringLiteral("/api/v2/video/search/"), requestQuery(request));
}

parsers::PostListing EpornerBackend::parsePosts(const QByteArray &body, const FeedRequest &request) const
{
    return parse(body, pageOf(request.after));
}

parsers::PostListing EpornerBackend::parse(const QByteArray &body, int page)
{
    parsers::PostListing out;
    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        out.error = QObject::tr("Unexpected answer from Eporner.");
        return out;
    }
    const QJsonObject root = doc.object();
    for (const QJsonValue &v : root.value(QLatin1String("videos")).toArray()) {
        const QJsonObject o = v.toObject();
        Post p;
        p.id = o.value(QLatin1String("id")).toString();
        p.url = o.value(QLatin1String("url")).toString();
        if (p.id.isEmpty() || p.url.isEmpty())
            continue;
        p.fullname = QStringLiteral("eporner_") + p.id;
        p.source = QStringLiteral("eporner");
        p.title = o.value(QLatin1String("title")).toString();
        p.permalink = p.url;
        p.domain = QStringLiteral("eporner.com");
        p.nsfw = true;
        p.kind = Post::Kind::Video;
        // "2024-03-01 10:20:30" (server time; close enough for "3 days ago").
        const QDateTime added = QDateTime::fromString(o.value(QLatin1String("added")).toString(),
                                                      QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        p.createdUtc = added.isValid() ? added.toSecsSinceEpoch() : 0;
        const QString keywords = o.value(QLatin1String("keywords")).toString();
        for (const QString &k : keywords.split(QLatin1Char(','), Qt::SkipEmptyParts))
            if (p.tags.size() < 12)
                p.tags.append(k.trimmed());
        const int seconds = o.value(QLatin1String("length_sec")).toInt();
        if (seconds > 0)
            p.flair = QStringLiteral("%1:%2").arg(seconds / 60).arg(seconds % 60, 2, 10, QLatin1Char('0'));
        const QJsonObject thumb = o.value(QLatin1String("default_thumb")).toObject();
        p.previewUrl = thumb.value(QLatin1String("src")).toString();
        p.previewWidth = thumb.value(QLatin1String("width")).toInt();
        p.previewHeight = thumb.value(QLatin1String("height")).toInt();

        MediaItem m;
        m.url = p.url;           // the video page: looked up when played
        m.video = true;
        m.resolve = QStringLiteral("eporner");
        m.poster = p.previewUrl;
        m.width = p.previewWidth;
        m.height = p.previewHeight;
        p.media.append(m);
        out.posts.append(p);
    }
    out.after = nextPage(page, root.value(QLatin1String("total_pages")).toInt(), out.posts.size());
    out.ok = true;
    return out;
}

// ---- Several Lemmy servers --------------------------------------------------------------

MultiLemmyBackend::MultiLemmyBackend(QStringList hosts, QHash<QString, Backend *> servers)
    : m_hosts(std::move(hosts))
    , m_servers(std::move(servers))
{
}

QHash<QString, QString> MultiLemmyBackend::splitCursor(const QString &after)
{
    QHash<QString, QString> out;
    for (const QString &part : after.split(QLatin1Char(';'), Qt::SkipEmptyParts)) {
        const int eq = int(part.indexOf(QLatin1Char('=')));
        if (eq > 0)
            out.insert(part.left(eq), part.mid(eq + 1));
    }
    return out;
}

QString MultiLemmyBackend::joinCursor(const QStringList &hosts, const QHash<QString, QString> &cursors)
{
    QStringList parts;
    for (const QString &h : hosts)
        if (cursors.contains(h))
            parts.append(h + QLatin1Char('=') + cursors.value(h));
    return parts.join(QLatin1Char(';'));
}

ApiReply *MultiLemmyBackend::posts(const FeedRequest &request)
{
    // First page: every server. Later pages: the servers that still have more.
    const QHash<QString, QString> cursors = splitCursor(request.after);
    QList<ApiReply *> parts;
    QStringList asked;
    for (const QString &h : m_hosts) {
        if (!request.after.isEmpty() && !cursors.contains(h))
            continue;
        Backend *b = m_servers.value(h);
        if (!b)
            continue;
        FeedRequest r = request;
        r.after = cursors.value(h);
        parts.append(b->posts(r));
        asked.append(h);
    }
    // One body for all: [{"host": ..., "body": "<that server's JSON>"}, ...]
    return new CompositeReply(parts, [asked](const QList<QByteArray> &bodies) {
        QJsonArray arr;
        for (int i = 0; i < bodies.size() && i < asked.size(); ++i)
            arr.append(QJsonObject{{QStringLiteral("host"), asked.at(i)},
                                   {QStringLiteral("body"), QString::fromUtf8(bodies.at(i))}});
        return QJsonDocument(arr).toJson(QJsonDocument::Compact);
    }, false);
}

parsers::PostListing MultiLemmyBackend::parsePosts(const QByteArray &body, const FeedRequest &request) const
{
    parsers::PostListing out;
    const QHash<QString, QString> cursors = splitCursor(request.after);
    QHash<QString, QString> next;
    QList<QList<Post>> lists;
    for (const QJsonValue &v : QJsonDocument::fromJson(body).array()) {
        const QJsonObject part = v.toObject();
        const QString host = part.value(QLatin1String("host")).toString();
        const QByteArray partBody = part.value(QLatin1String("body")).toString().toUtf8();
        Backend *b = m_servers.value(host);
        if (!b)
            continue;
        if (partBody.isEmpty()) {
            next.insert(host, cursors.value(host)); // failed this time: try the same page again
            continue;
        }
        FeedRequest r = request;
        r.after = cursors.value(host);
        parsers::PostListing l = b->parsePosts(partBody, r);
        if (!l.ok) {
            next.insert(host, cursors.value(host));
            continue;
        }
        for (Post &p : l.posts)
            p.origin = host;
        lists.append(l.posts);
        if (!l.after.isEmpty())
            next.insert(host, l.after);
    }
    // Mix the servers' pages: one post from each in turn.
    for (int i = 0;; ++i) {
        bool any = false;
        for (const QList<Post> &l : std::as_const(lists))
            if (i < l.size()) {
                out.posts.append(l.at(i));
                any = true;
            }
        if (!any)
            break;
    }
    out.after = joinCursor(m_hosts, next);
    out.ok = true;
    return out;
}

} // namespace quokka
