#include "lemmyparsers.h"

#include <algorithm>

#include "markdown.h"
#include "medialinks.h"

#include <QDateTime>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QUrl>

#include <functional>

namespace quokka::lemmy {

namespace {

QString str(const QJsonObject &o, const char *key)
{
    return o.value(QLatin1String(key)).toString();
}

qint64 num(const QJsonObject &o, const char *key)
{
    return qint64(o.value(QLatin1String(key)).toDouble());
}

QString idString(const QJsonObject &o)
{
    return QString::number(num(o, "id"));
}

bool hasExtension(const QString &url, std::initializer_list<const char *> exts)
{
    const QString path = QUrl(url).path().toLower();
    for (const char *e : exts)
        if (path.endsWith(QLatin1String(e)))
            return true;
    return false;
}

// pict-rs (Lemmy's image server) resizes on request; ask for a card-sized thumbnail. JPEG, not
// WebP: Qt only decodes WebP when the optional qtimageformats plugin is installed. If a server
// cannot produce it, media::imageSources() lists the other copies the UI falls back to.
QString sizedImage(const QString &url, int width)
{
    if (!url.contains(QLatin1String("/pictrs/image/")) || url.contains(QLatin1String("thumbnail=")))
        return url;
    return url + (url.contains(QLatin1Char('?')) ? QLatin1Char('&') : QLatin1Char('?'))
        + QStringLiteral("thumbnail=%1&format=jpg").arg(width);
}

} // namespace

qint64 parseTime(const QString &iso)
{
    if (iso.isEmpty())
        return 0;
    // Lemmy sends microseconds and, before 0.19, no time zone (always UTC).
    static const QRegularExpression re(
        QStringLiteral("^(\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2})(\\.\\d+)?(Z|[+-]\\d{2}:?\\d{2})?$"));
    const auto m = re.match(iso.trimmed());
    if (!m.hasMatch())
        return 0;
    QString zone = m.captured(3);
    if (zone.isEmpty() || zone == QLatin1String("Z"))
        zone = QStringLiteral("+00:00");
    else if (!zone.contains(QLatin1Char(':')))
        zone.insert(3, QLatin1Char(':'));
    const QDateTime dt = QDateTime::fromString(m.captured(1) + zone, Qt::ISODate);
    return dt.isValid() ? dt.toSecsSinceEpoch() : 0;
}

QString hostOf(const QString &url)
{
    return QUrl(url).host().toLower();
}

QString qualifiedName(const QJsonObject &actor)
{
    const QString name = str(actor, "name");
    const QString host = hostOf(str(actor, "actor_id"));
    return host.isEmpty() ? name : name + QLatin1Char('@') + host;
}

Post postFromView(const QJsonObject &pv, const QString &webBase)
{
    const QJsonObject p = pv.value(QLatin1String("post")).toObject();
    const QJsonObject creator = pv.value(QLatin1String("creator")).toObject();
    const QJsonObject community = pv.value(QLatin1String("community")).toObject();
    const QJsonObject counts = pv.value(QLatin1String("counts")).toObject();
    const QJsonObject image = pv.value(QLatin1String("image_details")).toObject();

    Post post;
    post.id = idString(p);
    post.fullname = QStringLiteral("lp_") + post.id;
    post.title = str(p, "name");
    post.author = qualifiedName(creator);
    post.subreddit = qualifiedName(community);
    post.permalink = webBase + QStringLiteral("/post/") + post.id;
    post.url = str(p, "url");
    post.domain = post.url.isEmpty() ? hostOf(webBase) : hostOf(post.url);
    post.selftext = str(p, "body");
    post.selftextHtml = markdown::toHtml(post.selftext);
    post.excerpt = markdown::toPlainExcerpt(post.selftext);
    post.createdUtc = parseTime(str(p, "published"));
    post.score = int(num(counts, "score"));
    post.numComments = int(num(counts, "comments"));
    const double up = counts.value(QLatin1String("upvotes")).toDouble();
    const double down = counts.value(QLatin1String("downvotes")).toDouble();
    post.upvoteRatio = up + down > 0 ? up / (up + down) : 0;
    post.nsfw = p.value(QLatin1String("nsfw")).toBool() || community.value(QLatin1String("nsfw")).toBool();
    post.stickied = p.value(QLatin1String("featured_community")).toBool()
        || p.value(QLatin1String("featured_local")).toBool();
    post.locked = p.value(QLatin1String("locked")).toBool();

    const QString contentType = str(p, "url_content_type").toLower();
    const QString thumb = str(p, "thumbnail_url");
    const QString embedVideo = str(p, "embed_video_url");
    const bool directVideo = contentType.startsWith(QLatin1String("video/"))
        || hasExtension(post.url, {".mp4", ".webm", ".mov", ".m4v"});
    const bool embedIsFile = hasExtension(embedVideo, {".mp4", ".webm", ".mov", ".m4v", ".m3u8"});
    const bool isImage = contentType.startsWith(QLatin1String("image/"))
        || hasExtension(post.url, {".jpg", ".jpeg", ".png", ".webp", ".gif", ".avif"});
    const bool gif = contentType == QLatin1String("image/gif") || hasExtension(post.url, {".gif"});
    if (!image.isEmpty()) {
        post.previewWidth = int(num(image, "width"));
        post.previewHeight = int(num(image, "height"));
    }

    const auto linked = media::classify(post.url); // imgur, redgifs, streamable, ...

    if (post.url.isEmpty()) {
        post.kind = Post::Kind::Self;
        post.previewUrl = sizedImage(thumb, 640);
        // A text post that is really media: show the files linked in its body.
        const QList<MediaItem> bodyMedia = media::extractFromText(post.selftext);
        if (!bodyMedia.isEmpty()) {
            post.media = bodyMedia;
            // Only videos? Then they're nearly always the same clip on several hosts: one video
            // that tries each link in turn.
            if (bodyMedia.size() > 1
                && std::all_of(bodyMedia.begin(), bodyMedia.end(), [](const MediaItem &m) { return m.video; })) {
                post.media = {bodyMedia.first()};
                for (qsizetype i = 1; i < bodyMedia.size(); ++i)
                    post.media.first().alternates.append(bodyMedia.at(i).url);
            }
            post.galleryCount = int(post.media.size());
            const MediaItem &first = bodyMedia.first();
            if (post.media.size() > 1)
                post.kind = Post::Kind::Gallery;
            else
                post.kind = first.video ? Post::Kind::Video : Post::Kind::Image;
            if (post.kind == Post::Kind::Video)
                post.videoUrl = first.url;
            if (first.video && !post.previewUrl.isEmpty())
                post.media.first().poster = post.previewUrl; // the post's thumbnail shows its first video
            for (const MediaItem &m : bodyMedia) {
                if (!m.video && post.previewUrl.isEmpty()) {
                    post.previewUrl = m.animated ? m.url : sizedImage(m.url, 640);
                    break;
                }
            }
        }
    } else if (directVideo || embedIsFile || (linked && linked->video)) {
        post.kind = Post::Kind::Video;
        post.videoUrl = directVideo ? post.url : linked && linked->video ? linked->url : embedVideo;
        post.previewUrl = sizedImage(thumb, 640);
        MediaItem m{post.videoUrl, post.previewWidth, post.previewHeight, {}, true};
        m.video = true;
        m.poster = post.previewUrl;
        if (linked && linked->video)
            m.resolve = linked->resolve;
        post.media.append(m);
    } else if (isImage) {
        post.kind = Post::Kind::Image;
        // Prefer the thumbnail: for posts from other servers it is usually a copy kept by the
        // server we read from, which is faster and more reliable than the post's origin server.
        // The original is the fallback (and the other way round for GIFs, to keep them animated).
        const QString original = gif ? post.url : sizedImage(post.url, 640);
        const QString local = thumb.isEmpty() ? QString() : sizedImage(thumb, 640);
        if (!local.isEmpty() && !gif) {
            post.previewUrl = local;
            post.previewFallbackUrl = original;
        } else {
            post.previewUrl = original;
            post.previewFallbackUrl = local;
        }
        post.media.append({post.url, post.previewWidth, post.previewHeight, str(p, "alt_text"), gif, thumb});
    } else if (linked) {
        // An image page we can show directly (e.g. imgur).
        post.kind = Post::Kind::Image;
        post.previewUrl = thumb.isEmpty() ? linked->url : sizedImage(thumb, 640);
        post.previewFallbackUrl = thumb.isEmpty() ? QString() : linked->url;
        post.media.append({linked->url, post.previewWidth, post.previewHeight, str(p, "alt_text"),
                           linked->animated, thumb});
    } else if (const QString yt = media::youtubeId(post.url); !yt.isEmpty()) {
        // YouTube can't play inside the app, but gets a full-size poster that opens it.
        post.kind = Post::Kind::Link;
        // The server's thumbnail is normally YouTube's full 16:9 picture; theirs (4:3) is the backup.
        const QString poster = QStringLiteral("https://i.ytimg.com/vi/%1/hqdefault.jpg").arg(yt);
        post.previewUrl = thumb.isEmpty() ? poster : sizedImage(thumb, 640);
        post.previewFallbackUrl = thumb.isEmpty() ? QString() : poster;
        post.previewWidth = 16;
        post.previewHeight = 9;
    } else {
        post.kind = Post::Kind::Link;
        post.previewUrl = sizedImage(thumb, 320);
        // No thumbnail? Linked pictures in the post's text still give it one.
        if (post.previewUrl.isEmpty())
            for (const MediaItem &m : media::extractFromText(post.selftext, 4))
                if (!m.video) { post.previewUrl = m.animated ? m.url : sizedImage(m.url, 320); break; }
    }
    if (!post.url.isEmpty() && (post.kind == Post::Kind::Image || post.kind == Post::Kind::Video)) {
        // Pictures (and, for picture posts, videos) linked in the text join the post's own: one
        // gallery, nothing missed. Videos linked from a video post are nearly always the same clip
        // from another host ("source", "HD version"); those stay on the post page.
        const bool videoPost = post.kind == Post::Kind::Video;
        for (const MediaItem &m : media::extractFromText(post.selftext)) {
            const bool known = std::any_of(post.media.begin(), post.media.end(),
                                           [&m](const MediaItem &o) { return o.url == m.url; });
            if (known || m.url == post.url)
                continue;
            if (videoPost && m.video) {
                if (!post.media.first().alternates.contains(m.url))
                    post.media.first().alternates.append(m.url);
            } else {
                post.media.append(m);
            }
        }
        if (post.media.size() > 1) {
            post.kind = Post::Kind::Gallery;
            post.galleryCount = int(post.media.size());
        }
    }
    return post;
}

Subreddit communityFromView(const QJsonObject &cv)
{
    const QJsonObject c = cv.value(QLatin1String("community")).toObject();
    const QJsonObject counts = cv.value(QLatin1String("counts")).toObject();
    Subreddit s;
    s.id = num(c, "id");
    s.name = qualifiedName(c);
    s.title = str(c, "title");
    const QString description = str(c, "description");
    s.publicDescription = markdown::toPlainSummary(description);
    s.descriptionHtml = markdown::toHtml(description);
    s.subscribers = num(counts, "subscribers");
    s.activeUsers = counts.contains(QLatin1String("users_active_day")) ? num(counts, "users_active_day") : -1;
    s.iconUrl = sizedImage(str(c, "icon"), 128);
    s.bannerUrl = str(c, "banner");
    s.nsfw = c.value(QLatin1String("nsfw")).toBool();
    s.createdUtc = parseTime(str(c, "published"));
    return s;
}

UserAccount userFromView(const QJsonObject &pv)
{
    const QJsonObject p = pv.value(QLatin1String("person")).toObject();
    const QJsonObject counts = pv.value(QLatin1String("counts")).toObject();
    UserAccount u;
    u.name = qualifiedName(p);
    u.iconUrl = sizedImage(str(p, "avatar"), 160);
    u.bannerUrl = str(p, "banner");
    u.description = markdown::toPlainSummary(str(p, "bio"), 300);
    u.createdUtc = parseTime(str(p, "published"));
    u.suspended = p.value(QLatin1String("banned")).toBool();
    u.hasKarma = false;
    u.postCount = num(counts, "post_count");
    u.commentCount = num(counts, "comment_count");
    return u;
}

Comment commentFromView(const QJsonObject &cv, const QString &webBase)
{
    const QJsonObject c = cv.value(QLatin1String("comment")).toObject();
    const QJsonObject creator = cv.value(QLatin1String("creator")).toObject();
    const QJsonObject post = cv.value(QLatin1String("post")).toObject();
    const QJsonObject community = cv.value(QLatin1String("community")).toObject();
    const QJsonObject counts = cv.value(QLatin1String("counts")).toObject();

    Comment out;
    out.id = idString(c);
    out.fullname = QStringLiteral("lc_") + out.id;
    const QStringList path = str(c, "path").split(QLatin1Char('.'), Qt::SkipEmptyParts);
    out.depth = qMax(0, int(path.size()) - 2);
    out.parentId = path.size() >= 3 ? QStringLiteral("lc_") + path.at(path.size() - 2)
                                    : QStringLiteral("lp_") + QString::number(num(post, "id"));
    out.author = qualifiedName(creator);
    if (c.value(QLatin1String("deleted")).toBool()) {
        out.bodyHtml = QStringLiteral("<i>[deleted]</i>");
        out.author = QStringLiteral("[deleted]");
    } else if (c.value(QLatin1String("removed")).toBool()) {
        out.bodyHtml = QStringLiteral("<i>[removed by a moderator]</i>");
    } else {
        out.bodyHtml = markdown::toHtml(str(c, "content"));
    }
    if (c.value(QLatin1String("distinguished")).toBool())
        out.distinguished = cv.value(QLatin1String("creator_is_admin")).toBool() ? QStringLiteral("admin")
                                                                               : QStringLiteral("moderator");
    out.score = int(num(counts, "score"));
    out.createdUtc = parseTime(str(c, "published"));
    out.isSubmitter = num(c, "creator_id") != 0 && num(c, "creator_id") == num(post, "creator_id");
    out.edited = !str(c, "updated").isEmpty();
    out.subreddit = qualifiedName(community);
    out.linkTitle = str(post, "name");
    out.permalink = webBase + QStringLiteral("/post/%1/%2").arg(num(post, "id")).arg(out.id);
    out.nsfw = community.value(QLatin1String("nsfw")).toBool() || post.value(QLatin1String("nsfw")).toBool();
    if (!c.value(QLatin1String("deleted")).toBool() && !c.value(QLatin1String("removed")).toBool())
        out.media = media::extractFromText(str(c, "content"), 6);
    out.moreCount = int(num(counts, "child_count")); // total replies; used by buildCommentTree
    return out;
}

QList<Comment> buildCommentTree(const QJsonArray &views, const QString &webBase, const QString &postAuthor)
{
    QList<Comment> all;
    QSet<QString> seen;
    for (const auto &v : views) {
        Comment c = commentFromView(v.toObject(), webBase);
        if (seen.contains(c.fullname))
            continue;
        seen.insert(c.fullname);
        if (!postAuthor.isEmpty() && c.author == postAuthor)
            c.isSubmitter = true;
        all.append(c);
    }

    QHash<QString, QList<int>> childrenOf;
    QList<int> roots;
    for (int i = 0; i < all.size(); ++i) {
        if (seen.contains(all[i].parentId))
            childrenOf[all[i].parentId].append(i);
        else
            roots.append(i);
    }

    QList<Comment> out;
    // Lemmy's child_count counts all descendants. A missing reply is shown once, under the
    // deepest loaded comment that knows about it: visit() returns how many descendants are
    // accounted for (loaded, or already behind a "load more" row further down).
    std::function<int(int)> visit = [&](int i) -> int {
        Comment c = all[i];
        const int totalReplies = c.moreCount;
        c.moreCount = 0;
        out.append(c);
        int below = 0;
        for (int child : childrenOf.value(c.fullname))
            below += 1 + visit(child);
        const int missing = totalReplies - below;
        if (missing > 0) {
            Comment more;
            more.kind = Comment::Kind::More;
            more.fullname = QStringLiteral("lm_") + c.id;
            more.parentId = c.fullname;
            more.depth = c.depth + 1;
            more.moreCount = missing;
            more.moreParentId = c.id;
            out.append(more);
            below += missing;
        }
        return below;
    };
    for (int r : roots)
        visit(r);
    return out;
}

} // namespace quokka::lemmy
