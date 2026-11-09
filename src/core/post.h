#pragma once

#include <QList>
#include <QString>
#include <QtGlobal>

namespace quokka {

// One image or video inside a gallery (or the single image of an image post).
struct MediaItem
{
    QString url;      // full-size image, or an MP4/GIF for animated items
    int width = 0;
    int height = 0;
    QString caption;
    bool animated = false;
};

// A Reddit "t3" thing (a link / post), reduced to what the UI needs.
struct Post
{
    enum class Kind { Self, Link, Image, Gallery, Video };

    QString id;          // "abc123"
    QString fullname;    // "t3_abc123"
    QString title;
    QString author;
    QString subreddit;   // without the "r/" prefix
    QString permalink;   // "/r/sub/comments/abc123/slug/"
    QString url;         // target URL for link posts
    QString domain;
    QString selftext;    // Markdown body for self posts
    QString selftextHtml; // Reddit-rendered HTML, cleaned for Qt rich text
    QString flair;
    QString flairColor;  // "#rrggbb" or empty

    qint64 createdUtc = 0;
    int score = 0;
    int numComments = 0;
    double upvoteRatio = 0.0;

    bool nsfw = false;
    bool spoiler = false;
    bool stickied = false;
    bool locked = false;

    Kind kind = Kind::Link;
    QString previewUrl;  // best-fit preview image (or video poster)
    int previewWidth = 0;
    int previewHeight = 0;
    QString videoUrl;    // HLS stream when kind == Video
    QString videoFallbackUrl; // MP4 (no audio) when available
    int galleryCount = 0;

    // Full-size media for the viewer: one item for images/videos, all items for galleries.
    QList<MediaItem> media;
};

} // namespace quokka
