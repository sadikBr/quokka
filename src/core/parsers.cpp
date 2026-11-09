#include "parsers.h"

#include "html.h"
#include "markdown.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QHash>
#include <QSet>
#include <QUrl>

#include <functional>

namespace quokka::parsers {

namespace {

struct ImageChoice
{
    QString url;
    int width = 0;
    int height = 0;
    bool isValid() const { return !url.isEmpty() && width > 0 && height > 0; }
};

// Picks the smallest image that is at least kPreferredPreviewWidth wide, falling back to the
// largest one available. `candidates` must use the given key names (Reddit uses both
// {url,width,height} and {u,x,y} depending on the endpoint).
ImageChoice pickImage(const QJsonArray &candidates, const QJsonObject &source,
                      const QString &urlKey, const QString &wKey, const QString &hKey)
{
    ImageChoice best;
    for (const auto &v : candidates) {
        const QJsonObject o = v.toObject();
        ImageChoice c{o.value(urlKey).toString(), o.value(wKey).toInt(), o.value(hKey).toInt()};
        if (!c.isValid())
            continue;
        best = c;
        if (c.width >= kPreferredPreviewWidth)
            return best;
    }
    ImageChoice src{source.value(urlKey).toString(), source.value(wKey).toInt(),
                    source.value(hKey).toInt()};
    if (src.isValid() && src.width > best.width)
        return src;
    return best;
}

ImageChoice previewImage(const QJsonObject &data)
{
    const QJsonArray images = data.value(QLatin1String("preview")).toObject()
                                  .value(QLatin1String("images")).toArray();
    if (images.isEmpty())
        return {};
    const QJsonObject first = images.first().toObject();
    return pickImage(first.value(QLatin1String("resolutions")).toArray(),
                     first.value(QLatin1String("source")).toObject(),
                     QStringLiteral("url"), QStringLiteral("width"), QStringLiteral("height"));
}

bool looksLikeImageUrl(const QString &url)
{
    const QString path = QUrl(url).path().toLower();
    return path.endsWith(QLatin1String(".jpg")) || path.endsWith(QLatin1String(".jpeg"))
        || path.endsWith(QLatin1String(".png")) || path.endsWith(QLatin1String(".webp"))
        || path.endsWith(QLatin1String(".gif"));
}

QJsonObject redditVideo(const QJsonObject &data)
{
    for (const auto key : {"secure_media", "media"}) {
        const QJsonObject v = data.value(QLatin1String(key)).toObject()
                                  .value(QLatin1String("reddit_video")).toObject();
        if (!v.isEmpty())
            return v;
    }
    // GIF-like posts (imgur gifv, giphy, ...) expose a transcoded preview video.
    return data.value(QLatin1String("preview")).toObject()
        .value(QLatin1String("reddit_video_preview")).toObject();
}

void resolveMedia(Post &post, const QJsonObject &data)
{
    const ImageChoice preview = previewImage(data);
    auto usePreview = [&](const ImageChoice &img) {
        post.previewUrl = img.url;
        post.previewWidth = img.width;
        post.previewHeight = img.height;
    };

    // 1. Gallery
    if (data.value(QLatin1String("is_gallery")).toBool()) {
        const QJsonArray items = data.value(QLatin1String("gallery_data")).toObject()
                                     .value(QLatin1String("items")).toArray();
        const QJsonObject meta = data.value(QLatin1String("media_metadata")).toObject();
        if (!items.isEmpty()) {
            post.kind = Post::Kind::Gallery;
            post.galleryCount = int(items.size());
            for (const auto &it : items) {
                const QJsonObject item = it.toObject();
                const QJsonObject mm = meta.value(item.value(QLatin1String("media_id")).toString())
                                           .toObject();
                if (mm.value(QLatin1String("status")).toString() != QLatin1String("valid"))
                    continue;
                const QJsonObject src = mm.value(QLatin1String("s")).toObject();
                MediaItem mi;
                mi.width = src.value(QLatin1String("x")).toInt();
                mi.height = src.value(QLatin1String("y")).toInt();
                mi.caption = item.value(QLatin1String("caption")).toString();
                if (src.contains(QLatin1String("mp4"))) {
                    mi.url = src.value(QLatin1String("mp4")).toString();
                    mi.animated = true;
                } else if (src.contains(QLatin1String("gif"))) {
                    mi.url = src.value(QLatin1String("gif")).toString();
                    mi.animated = true;
                } else {
                    mi.url = src.value(QLatin1String("u")).toString();
                }
                if (!mi.url.isEmpty())
                    post.media.append(mi);
            }
            const QString firstId = items.first().toObject()
                                        .value(QLatin1String("media_id")).toString();
            const QJsonObject m = meta.value(firstId).toObject();
            usePreview(pickImage(m.value(QLatin1String("p")).toArray(),
                                 m.value(QLatin1String("s")).toObject(),
                                 QStringLiteral("u"), QStringLiteral("x"), QStringLiteral("y")));
            if (post.previewUrl.isEmpty())
                usePreview(preview);
            return;
        }
    }

    // 2. Video (v.redd.it or a GIF preview video)
    const QJsonObject video = redditVideo(data);
    const QString hls = video.value(QLatin1String("hls_url")).toString();
    if (!hls.isEmpty()) {
        post.kind = Post::Kind::Video;
        post.videoUrl = hls;
        post.videoFallbackUrl = video.value(QLatin1String("fallback_url")).toString();
        usePreview(preview);
        const int vw = video.value(QLatin1String("width")).toInt();
        const int vh = video.value(QLatin1String("height")).toInt();
        if (post.previewWidth <= 0) {
            post.previewWidth = vw;
            post.previewHeight = vh;
        }
        post.media.append(MediaItem{hls, vw > 0 ? vw : post.previewWidth,
                                    vh > 0 ? vh : post.previewHeight, {}, true});
        return;
    }

    // 3. Self post (may still carry an inline preview image, which we keep for the card)
    if (post.kind == Post::Kind::Self) {
        if (preview.isValid())
            usePreview(preview);
        return;
    }

    // 4. Image
    const QString hint = data.value(QLatin1String("post_hint")).toString();
    if (hint == QLatin1String("image") || looksLikeImageUrl(post.url)) {
        post.kind = Post::Kind::Image;
        if (preview.isValid())
            usePreview(preview);
        else
            post.previewUrl = post.url; // size unknown until it loads

        const QJsonObject img = data.value(QLatin1String("preview")).toObject()
                                    .value(QLatin1String("images")).toArray().first().toObject();
        const QJsonObject source = img.value(QLatin1String("source")).toObject();
        const QJsonObject mp4 = img.value(QLatin1String("variants")).toObject()
                                    .value(QLatin1String("mp4")).toObject()
                                    .value(QLatin1String("source")).toObject();
        MediaItem mi;
        if (!mp4.isEmpty()) { // GIFs: Reddit offers an MP4 rendition
            mi = {mp4.value(QLatin1String("url")).toString(), mp4.value(QLatin1String("width")).toInt(),
                  mp4.value(QLatin1String("height")).toInt(), {}, true};
        } else if (QUrl(post.url).path().endsWith(QLatin1String(".gif"), Qt::CaseInsensitive)) {
            mi = {post.url, source.value(QLatin1String("width")).toInt(),
                  source.value(QLatin1String("height")).toInt(), {}, true};
        } else if (post.url.contains(QLatin1String("i.redd.it")) || source.isEmpty()) {
            mi = {post.url, source.value(QLatin1String("width")).toInt(),
                  source.value(QLatin1String("height")).toInt(), {}, false};
        } else {
            mi = {source.value(QLatin1String("url")).toString(),
                  source.value(QLatin1String("width")).toInt(),
                  source.value(QLatin1String("height")).toInt(), {}, false};
        }
        if (!mi.url.isEmpty())
            post.media.append(mi);
        return;
    }

    // 5. External link: thumbnail-sized preview if we have one
    post.kind = Post::Kind::Link;
    if (preview.isValid()) {
        usePreview(preview);
    } else {
        const QString thumb = data.value(QLatin1String("thumbnail")).toString();
        if (thumb.startsWith(QLatin1String("http"))) {
            post.previewUrl = thumb;
            post.previewWidth = data.value(QLatin1String("thumbnail_width")).toInt();
            post.previewHeight = data.value(QLatin1String("thumbnail_height")).toInt();
        }
    }
}

} // namespace

Post parsePost(const QJsonObject &data)
{
    Post p;
    p.id = data.value(QLatin1String("id")).toString();
    p.fullname = data.value(QLatin1String("name")).toString();
    if (p.fullname.isEmpty() && !p.id.isEmpty())
        p.fullname = QStringLiteral("t3_") + p.id;
    p.title = data.value(QLatin1String("title")).toString();
    p.author = data.value(QLatin1String("author")).toString();
    p.subreddit = data.value(QLatin1String("subreddit")).toString();
    p.permalink = data.value(QLatin1String("permalink")).toString();
    p.url = data.value(QLatin1String("url_overridden_by_dest")).toString();
    if (p.url.isEmpty())
        p.url = data.value(QLatin1String("url")).toString();
    p.domain = data.value(QLatin1String("domain")).toString();
    p.selftext = data.value(QLatin1String("selftext")).toString();
    p.selftextHtml = html::cleanRedditHtml(data.value(QLatin1String("selftext_html")).toString());
    p.excerpt = markdown::toPlainExcerpt(p.selftext);
    p.flair = data.value(QLatin1String("link_flair_text")).toString();
    p.flairColor = data.value(QLatin1String("link_flair_background_color")).toString();

    p.createdUtc = qint64(data.value(QLatin1String("created_utc")).toDouble());
    p.score = data.value(QLatin1String("score")).toInt();
    p.numComments = data.value(QLatin1String("num_comments")).toInt();
    p.upvoteRatio = data.value(QLatin1String("upvote_ratio")).toDouble();

    p.nsfw = data.value(QLatin1String("over_18")).toBool();
    p.spoiler = data.value(QLatin1String("spoiler")).toBool();
    p.stickied = data.value(QLatin1String("stickied")).toBool();
    p.locked = data.value(QLatin1String("locked")).toBool();

    p.kind = data.value(QLatin1String("is_self")).toBool() ? Post::Kind::Self : Post::Kind::Link;

    // Crossposts carry their media on the original post.
    const QJsonArray parents = data.value(QLatin1String("crosspost_parent_list")).toArray();
    const QJsonObject mediaSource = parents.isEmpty() ? data : parents.first().toObject();
    if (!parents.isEmpty()) {
        const Post parent = parsePost(mediaSource);
        p.kind = parent.kind;
        p.url = parent.url;
        p.domain = parent.domain;
        if (p.selftext.isEmpty()) {
            p.selftext = parent.selftext;
            p.selftextHtml = parent.selftextHtml;
        }
        p.previewUrl = parent.previewUrl;
        p.previewWidth = parent.previewWidth;
        p.previewHeight = parent.previewHeight;
        p.videoUrl = parent.videoUrl;
        p.videoFallbackUrl = parent.videoFallbackUrl;
        p.galleryCount = parent.galleryCount;
        p.media = parent.media;
    } else {
        resolveMedia(p, data);
    }
    return p;
}

PostListing parsePostListing(const QByteArray &json)
{
    PostListing result;
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError) {
        result.error = QStringLiteral("Invalid JSON: %1").arg(err.errorString());
        return result;
    }
    const QJsonObject root = doc.object();
    if (root.value(QLatin1String("kind")).toString() != QLatin1String("Listing")) {
        result.error = QStringLiteral("Unexpected response (not a Listing)");
        return result;
    }
    const QJsonObject data = root.value(QLatin1String("data")).toObject();
    result.after = data.value(QLatin1String("after")).toString();
    const QJsonArray children = data.value(QLatin1String("children")).toArray();
    result.posts.reserve(children.size());
    for (const auto &child : children) {
        const QJsonObject thing = child.toObject();
        if (thing.value(QLatin1String("kind")).toString() != QLatin1String("t3"))
            continue;
        result.posts.append(parsePost(thing.value(QLatin1String("data")).toObject()));
    }
    result.ok = true;
    return result;
}

namespace {

Comment parseCommentData(const QJsonObject &d, const QString &linkAuthor)
{
    Comment c;
    c.kind = Comment::Kind::Comment;
    c.id = d.value(QLatin1String("id")).toString();
    c.fullname = d.value(QLatin1String("name")).toString();
    if (c.fullname.isEmpty() && !c.id.isEmpty())
        c.fullname = QStringLiteral("t1_") + c.id;
    c.parentId = d.value(QLatin1String("parent_id")).toString();
    c.depth = d.value(QLatin1String("depth")).toInt();
    c.author = d.value(QLatin1String("author")).toString();
    c.bodyHtml = html::cleanRedditHtml(d.value(QLatin1String("body_html")).toString());
    if (c.bodyHtml.isEmpty())
        c.bodyHtml = d.value(QLatin1String("body")).toString().toHtmlEscaped();
    c.flair = d.value(QLatin1String("author_flair_text")).toString();
    c.distinguished = d.value(QLatin1String("distinguished")).toString();
    c.score = d.value(QLatin1String("score")).toInt();
    c.createdUtc = qint64(d.value(QLatin1String("created_utc")).toDouble());
    c.isSubmitter = d.value(QLatin1String("is_submitter")).toBool()
        || (!linkAuthor.isEmpty() && c.author == linkAuthor && c.author != QLatin1String("[deleted]"));
    c.stickied = d.value(QLatin1String("stickied")).toBool();
    c.scoreHidden = d.value(QLatin1String("score_hidden")).toBool();
    const QJsonValue edited = d.value(QLatin1String("edited"));
    c.edited = edited.isDouble() || (edited.isBool() && edited.toBool());
    c.subreddit = d.value(QLatin1String("subreddit")).toString();
    c.linkTitle = d.value(QLatin1String("link_title")).toString();
    c.permalink = d.value(QLatin1String("permalink")).toString();
    return c;
}

Comment parseMoreData(const QJsonObject &d)
{
    Comment c;
    c.id = d.value(QLatin1String("id")).toString();
    c.fullname = d.value(QLatin1String("name")).toString();
    c.parentId = d.value(QLatin1String("parent_id")).toString();
    c.depth = d.value(QLatin1String("depth")).toInt();
    c.moreCount = d.value(QLatin1String("count")).toInt();
    for (const auto &v : d.value(QLatin1String("children")).toArray())
        c.moreChildren.append(v.toString());
    // An empty "more" (id "_") means the thread continues on its own page.
    c.kind = (c.moreChildren.isEmpty() || c.id == QLatin1String("_")) ? Comment::Kind::Continue
                                                                      : Comment::Kind::More;
    return c;
}

void flattenThings(const QJsonArray &things, const QString &linkAuthor, int depth,
                   QList<Comment> &out)
{
    for (const auto &v : things) {
        const QJsonObject thing = v.toObject();
        const QString kind = thing.value(QLatin1String("kind")).toString();
        const QJsonObject d = thing.value(QLatin1String("data")).toObject();
        if (kind == QLatin1String("t1")) {
            Comment c = parseCommentData(d, linkAuthor);
            c.depth = depth;
            out.append(c);
            const QJsonValue replies = d.value(QLatin1String("replies"));
            if (replies.isObject()) { // "" when there are none
                flattenThings(replies.toObject().value(QLatin1String("data")).toObject()
                                  .value(QLatin1String("children")).toArray(),
                              linkAuthor, depth + 1, out);
            }
        } else if (kind == QLatin1String("more")) {
            Comment c = parseMoreData(d);
            c.depth = depth;
            // A childless "more" directly under the post carries no information.
            if (c.kind == Comment::Kind::More || !c.parentId.startsWith(QLatin1String("t3_")))
                out.append(c);
        }
    }
}

QJsonDocument parseDoc(const QByteArray &json, QString *error)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError)
        *error = QStringLiteral("Invalid JSON: %1").arg(err.errorString());
    return doc;
}

} // namespace

CommentsPage parseCommentsPage(const QByteArray &json)
{
    CommentsPage page;
    const QJsonDocument doc = parseDoc(json, &page.error);
    if (!page.error.isEmpty())
        return page;
    const QJsonArray parts = doc.array();
    if (parts.size() < 2) {
        page.error = QStringLiteral("Unexpected response (no comment listing)");
        return page;
    }
    const QJsonArray postChildren = parts.at(0).toObject().value(QLatin1String("data")).toObject()
                                        .value(QLatin1String("children")).toArray();
    if (postChildren.isEmpty()) {
        page.error = QStringLiteral("Post not found");
        return page;
    }
    page.post = parsePost(postChildren.first().toObject().value(QLatin1String("data")).toObject());
    flattenThings(parts.at(1).toObject().value(QLatin1String("data")).toObject()
                      .value(QLatin1String("children")).toArray(),
                  page.post.author, 0, page.comments);
    page.ok = true;
    return page;
}

QList<Comment> parseMoreChildren(const QByteArray &json, const QString &linkAuthor, int baseDepth,
                                 QString *error)
{
    QString err;
    const QJsonDocument doc = parseDoc(json, &err);
    QList<Comment> out;
    if (!err.isEmpty()) {
        if (error)
            *error = err;
        return out;
    }
    const QJsonArray things = doc.object().value(QLatin1String("json")).toObject()
                                  .value(QLatin1String("data")).toObject()
                                  .value(QLatin1String("things")).toArray();

    // Things come back flat; rebuild the tree from parent ids, keeping Reddit's order.
    QList<Comment> flat;
    for (const auto &v : things) {
        const QJsonObject thing = v.toObject();
        const QString kind = thing.value(QLatin1String("kind")).toString();
        const QJsonObject d = thing.value(QLatin1String("data")).toObject();
        if (kind == QLatin1String("t1"))
            flat.append(parseCommentData(d, linkAuthor));
        else if (kind == QLatin1String("more"))
            flat.append(parseMoreData(d));
    }
    QSet<QString> present;
    for (const Comment &c : flat)
        present.insert(c.fullname);
    QHash<QString, QList<int>> childrenOf;
    QList<int> roots;
    for (int i = 0; i < flat.size(); ++i) {
        if (present.contains(flat[i].parentId))
            childrenOf[flat[i].parentId].append(i);
        else
            roots.append(i);
    }
    std::function<void(int, int)> visit = [&](int idx, int depth) {
        Comment c = flat[idx];
        c.depth = depth;
        out.append(c);
        for (int child : childrenOf.value(c.fullname))
            visit(child, depth + 1);
    };
    for (int r : roots)
        visit(r, baseDepth);
    return out;
}

QList<Comment> parseCommentListing(const QByteArray &json, QString *after, QString *error)
{
    QString err;
    const QJsonDocument doc = parseDoc(json, &err);
    QList<Comment> out;
    if (!err.isEmpty() || doc.object().value(QLatin1String("kind")).toString() != QLatin1String("Listing")) {
        if (error)
            *error = err.isEmpty() ? QStringLiteral("Unexpected response (not a Listing)") : err;
        return out;
    }
    const QJsonObject data = doc.object().value(QLatin1String("data")).toObject();
    if (after)
        *after = data.value(QLatin1String("after")).toString();
    for (const auto &v : data.value(QLatin1String("children")).toArray()) {
        const QJsonObject thing = v.toObject();
        if (thing.value(QLatin1String("kind")).toString() == QLatin1String("t1"))
            out.append(parseCommentData(thing.value(QLatin1String("data")).toObject(), {}));
    }
    return out;
}

namespace {

// Reddit image URLs sometimes come back empty, as "self"/"default", or with escaped &amp;.
QString imageUrl(const QJsonValue &v)
{
    QString u = v.toString();
    if (!u.startsWith(QLatin1String("http")))
        return {};
    u.replace(QLatin1String("&amp;"), QLatin1String("&"));
    return u;
}

QString firstImage(const QJsonObject &d, std::initializer_list<const char *> keys)
{
    for (const char *k : keys) {
        const QString u = imageUrl(d.value(QLatin1String(k)));
        if (!u.isEmpty())
            return u;
    }
    return {};
}

Subreddit subredditFromData(const QJsonObject &d)
{
    Subreddit s;
    s.name = d.value(QLatin1String("display_name")).toString();
    s.title = d.value(QLatin1String("title")).toString();
    s.publicDescription = d.value(QLatin1String("public_description")).toString().trimmed();
    s.descriptionHtml = html::cleanRedditHtml(d.value(QLatin1String("description_html")).toString());
    s.subscribers = qint64(d.value(QLatin1String("subscribers")).toDouble());
    const QJsonValue active = d.value(QLatin1String("active_user_count"));
    s.activeUsers = active.isDouble() ? qint64(active.toDouble()) : -1;
    s.iconUrl = firstImage(d, {"community_icon", "icon_img"});
    s.bannerUrl = firstImage(d, {"banner_background_image", "mobile_banner_image", "banner_img"});
    s.color = d.value(QLatin1String("primary_color")).toString();
    if (s.color.isEmpty())
        s.color = d.value(QLatin1String("key_color")).toString();
    s.nsfw = d.value(QLatin1String("over18")).toBool();
    s.createdUtc = qint64(d.value(QLatin1String("created_utc")).toDouble());
    return s;
}

} // namespace

Subreddit parseSubredditAbout(const QByteArray &json, QString *error)
{
    QString err;
    const QJsonDocument doc = parseDoc(json, &err);
    const QJsonObject root = doc.object();
    if (err.isEmpty() && root.value(QLatin1String("kind")).toString() != QLatin1String("t5"))
        err = QStringLiteral("Community not found");
    if (!err.isEmpty()) {
        if (error)
            *error = err;
        return {};
    }
    return subredditFromData(root.value(QLatin1String("data")).toObject());
}

QList<Subreddit> parseSubredditListing(const QByteArray &json, QString *after, QString *error)
{
    QString err;
    const QJsonDocument doc = parseDoc(json, &err);
    QList<Subreddit> out;
    if (!err.isEmpty() || doc.object().value(QLatin1String("kind")).toString() != QLatin1String("Listing")) {
        if (error)
            *error = err.isEmpty() ? QStringLiteral("Unexpected response (not a Listing)") : err;
        return out;
    }
    const QJsonObject data = doc.object().value(QLatin1String("data")).toObject();
    if (after)
        *after = data.value(QLatin1String("after")).toString();
    for (const auto &v : data.value(QLatin1String("children")).toArray()) {
        const QJsonObject thing = v.toObject();
        if (thing.value(QLatin1String("kind")).toString() == QLatin1String("t5"))
            out.append(subredditFromData(thing.value(QLatin1String("data")).toObject()));
    }
    return out;
}

UserAccount parseUserAbout(const QByteArray &json, QString *error)
{
    QString err;
    const QJsonDocument doc = parseDoc(json, &err);
    const QJsonObject root = doc.object();
    if (err.isEmpty() && root.value(QLatin1String("kind")).toString() != QLatin1String("t2"))
        err = QStringLiteral("User not found");
    if (!err.isEmpty()) {
        if (error)
            *error = err;
        return {};
    }
    const QJsonObject d = root.value(QLatin1String("data")).toObject();
    UserAccount u;
    u.name = d.value(QLatin1String("name")).toString();
    u.suspended = d.value(QLatin1String("is_suspended")).toBool();
    u.iconUrl = firstImage(d, {"snoovatar_img", "icon_img"});
    const QJsonObject profile = d.value(QLatin1String("subreddit")).toObject();
    u.bannerUrl = imageUrl(profile.value(QLatin1String("banner_img")));
    u.description = profile.value(QLatin1String("public_description")).toString().trimmed();
    u.linkKarma = qint64(d.value(QLatin1String("link_karma")).toDouble());
    u.commentKarma = qint64(d.value(QLatin1String("comment_karma")).toDouble());
    u.totalKarma = qint64(d.value(QLatin1String("total_karma")).toDouble());
    if (u.totalKarma == 0)
        u.totalKarma = u.linkKarma + u.commentKarma;
    u.createdUtc = qint64(d.value(QLatin1String("created_utc")).toDouble());
    return u;
}

} // namespace quokka::parsers
