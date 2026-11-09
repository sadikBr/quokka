#include "postmap.h"

#include "postlistmodel.h"

#include <QVariantList>

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

QVariantMap postToVariantMap(const quokka::Post &p)
{
    QVariantList media;
    for (const auto &m : p.media) {
        media.append(QVariantMap{
            {QStringLiteral("url"), m.url},
            {QStringLiteral("width"), m.width},
            {QStringLiteral("height"), m.height},
            {QStringLiteral("caption"), m.caption},
            {QStringLiteral("animated"), m.animated},
        });
    }
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
        {QStringLiteral("previewWidth"), p.previewWidth},
        {QStringLiteral("previewHeight"), p.previewHeight},
        {QStringLiteral("videoUrl"), p.videoUrl},
        {QStringLiteral("videoFallbackUrl"), p.videoFallbackUrl},
        {QStringLiteral("galleryCount"), p.galleryCount},
        {QStringLiteral("media"), media},
    };
}
