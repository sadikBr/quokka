#include "parsers.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QUrl>

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
        usePreview(preview);
        if (post.previewWidth <= 0) {
            post.previewWidth = video.value(QLatin1String("width")).toInt();
            post.previewHeight = video.value(QLatin1String("height")).toInt();
        }
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
        if (p.selftext.isEmpty())
            p.selftext = parent.selftext;
        p.previewUrl = parent.previewUrl;
        p.previewWidth = parent.previewWidth;
        p.previewHeight = parent.previewHeight;
        p.videoUrl = parent.videoUrl;
        p.galleryCount = parent.galleryCount;
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

} // namespace quokka::parsers
