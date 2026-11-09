#include "postmap.h"

#include "medialinks.h"
#include "postlistmodel.h"

#include <QVariantList>

#include <algorithm>

int postKindValue(quokka::Post::Kind kind)
{
    using K = quokka::Post::Kind;
    switch (kind) {
    case K::Self: return PostListModel::SelfPost;
    case K::Image: return PostListModel::ImagePost;
    case K::Gallery: return PostListModel::GalleryPost;
    case K::Video: return PostListModel::VideoPost;
    case K::Link: break;
    }
    return PostListModel::LinkPost;
}

QVariantList mediaToVariantList(const QList<quokka::MediaItem> &items)
{
    QVariantList out;
    for (const auto &m : items) {
        QVariantList alternates; // {url, resolve} for each other link to the same video
        for (const QString &a : m.alternates) {
            const auto c = quokka::media::classify(a);
            alternates.append(QVariantMap{{QStringLiteral("url"), a},
                                          {QStringLiteral("resolve"), c ? c->resolve : QString()}});
        }
        out.append(QVariantMap{
            {QStringLiteral("url"), m.url},
            {QStringLiteral("width"), m.width},
            {QStringLiteral("height"), m.height},
            {QStringLiteral("caption"), m.caption},
            {QStringLiteral("animated"), m.animated},
            {QStringLiteral("fallbackUrl"), m.fallbackUrl},
            {QStringLiteral("video"), m.video},
            {QStringLiteral("resolve"), m.resolve},
            {QStringLiteral("poster"), m.poster},
            {QStringLiteral("alternates"), alternates},
        });
    }
    return out;
}

QVariantMap postToVariantMap(const quokka::Post &p)
{
    const QVariantList media = mediaToVariantList(p.media);
    // Media linked in the text of posts that already have a link of their own (and that is not
    // already part of the post's own media).
    QList<quokka::MediaItem> extra;
    if (!p.url.isEmpty()) {
        for (const auto &m : quokka::media::extractFromText(p.selftext)) {
            const bool known = std::any_of(p.media.begin(), p.media.end(), [&m](const quokka::MediaItem &o) {
                return o.url == m.url || o.alternates.contains(m.url);
            });
            if (!known)
                extra.append(m);
        }
    }
    const QVariantList bodyMedia = mediaToVariantList(extra);
    return {
        {QStringLiteral("postId"), p.id},
        {QStringLiteral("fullname"), p.fullname},
        {QStringLiteral("title"), p.title},
        {QStringLiteral("author"), p.author},
        {QStringLiteral("subreddit"), p.subreddit},
        {QStringLiteral("permalink"), p.permalink},
        {QStringLiteral("url"), p.url},
        {QStringLiteral("domain"), p.domain},
        {QStringLiteral("selftext"), p.selftext},
        {QStringLiteral("excerpt"), p.excerpt},
        {QStringLiteral("selftextHtml"), p.selftextHtml},
        {QStringLiteral("flair"), p.flair},
        {QStringLiteral("flairColor"), p.flairColor},
        {QStringLiteral("createdUtc"), p.createdUtc},
        {QStringLiteral("score"), p.score},
        {QStringLiteral("numComments"), p.numComments},
        {QStringLiteral("upvoteRatio"), p.upvoteRatio},
        {QStringLiteral("nsfw"), p.nsfw},
        {QStringLiteral("spoiler"), p.spoiler},
        {QStringLiteral("stickied"), p.stickied},
        {QStringLiteral("locked"), p.locked},
        {QStringLiteral("kind"), postKindValue(p.kind)},
        {QStringLiteral("previewUrl"), p.previewUrl},
        {QStringLiteral("previewFallbackUrl"), p.previewFallbackUrl},
        {QStringLiteral("previewWidth"), p.previewWidth},
        {QStringLiteral("previewHeight"), p.previewHeight},
        {QStringLiteral("videoUrl"), p.videoUrl},
        {QStringLiteral("videoFallbackUrl"), p.videoFallbackUrl},
        {QStringLiteral("galleryCount"), p.galleryCount},
        {QStringLiteral("media"), media},
        {QStringLiteral("bodyMedia"), bodyMedia},
    };
}

quokka::Post postFromVariantMap(const QVariantMap &m)
{
    using K = quokka::Post::Kind;
    quokka::Post p;
    p.id = m.value(QStringLiteral("postId")).toString();
    p.fullname = m.value(QStringLiteral("fullname")).toString();
    p.title = m.value(QStringLiteral("title")).toString();
    p.author = m.value(QStringLiteral("author")).toString();
    p.subreddit = m.value(QStringLiteral("subreddit")).toString();
    p.permalink = m.value(QStringLiteral("permalink")).toString();
    p.url = m.value(QStringLiteral("url")).toString();
    p.domain = m.value(QStringLiteral("domain")).toString();
    p.selftext = m.value(QStringLiteral("selftext")).toString();
    p.excerpt = m.value(QStringLiteral("excerpt")).toString();
    p.selftextHtml = m.value(QStringLiteral("selftextHtml")).toString();
    p.flair = m.value(QStringLiteral("flair")).toString();
    p.flairColor = m.value(QStringLiteral("flairColor")).toString();
    p.createdUtc = m.value(QStringLiteral("createdUtc")).toLongLong();
    p.score = m.value(QStringLiteral("score")).toInt();
    p.numComments = m.value(QStringLiteral("numComments")).toInt();
    p.upvoteRatio = m.value(QStringLiteral("upvoteRatio")).toDouble();
    p.nsfw = m.value(QStringLiteral("nsfw")).toBool();
    p.spoiler = m.value(QStringLiteral("spoiler")).toBool();
    p.stickied = m.value(QStringLiteral("stickied")).toBool();
    p.locked = m.value(QStringLiteral("locked")).toBool();
    switch (m.value(QStringLiteral("kind")).toInt()) {
    case PostListModel::SelfPost: p.kind = K::Self; break;
    case PostListModel::ImagePost: p.kind = K::Image; break;
    case PostListModel::GalleryPost: p.kind = K::Gallery; break;
    case PostListModel::VideoPost: p.kind = K::Video; break;
    default: p.kind = K::Link; break;
    }
    p.previewUrl = m.value(QStringLiteral("previewUrl")).toString();
    p.previewFallbackUrl = m.value(QStringLiteral("previewFallbackUrl")).toString();
    p.previewWidth = m.value(QStringLiteral("previewWidth")).toInt();
    p.previewHeight = m.value(QStringLiteral("previewHeight")).toInt();
    p.videoUrl = m.value(QStringLiteral("videoUrl")).toString();
    p.videoFallbackUrl = m.value(QStringLiteral("videoFallbackUrl")).toString();
    p.galleryCount = m.value(QStringLiteral("galleryCount")).toInt();
    for (const QVariant &v : m.value(QStringLiteral("media")).toList()) {
        const QVariantMap mm = v.toMap();
        quokka::MediaItem item{mm.value(QStringLiteral("url")).toString(),
                               mm.value(QStringLiteral("width")).toInt(),
                               mm.value(QStringLiteral("height")).toInt(),
                               mm.value(QStringLiteral("caption")).toString(),
                               mm.value(QStringLiteral("animated")).toBool(),
                               mm.value(QStringLiteral("fallbackUrl")).toString()};
        item.video = mm.value(QStringLiteral("video")).toBool();
        item.resolve = mm.value(QStringLiteral("resolve")).toString();
        item.poster = mm.value(QStringLiteral("poster")).toString();
        for (const QVariant &a : mm.value(QStringLiteral("alternates")).toList())
            item.alternates.append(a.toMap().value(QStringLiteral("url")).toString());
        p.media.append(item);
    }
    return p;
}
