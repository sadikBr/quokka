#pragma once

#include <QString>
#include <QtGlobal>

namespace quokka {

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
    int galleryCount = 0;
};

} // namespace quokka
