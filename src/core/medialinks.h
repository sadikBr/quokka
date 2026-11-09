#pragma once

#include "post.h"

#include <QList>
#include <QString>
#include <QStringList>
#include <optional>

namespace quokka::media {

// What a URL points at, if it is something Quokka can show in place:
// - direct files: .jpg .png .webp .gif .avif (images), .mp4 .webm .mov .m4v (videos)
// - imgur pages and .gifv, rewritten to their direct file
// - v.redd.it and giphy pages, rewritten to their video file
// - redgifs and streamable pages, which need a lookup before playing (MediaItem::resolve)
std::optional<MediaItem> classify(const QString &url);

// All showable media linked from a text (Markdown or HTML), in order, without duplicates.
QList<MediaItem> extractFromText(const QString &text, int max = 12);

// Every copy of a picture worth trying, best first, without duplicates: the given URLs, then
// for Lemmy (pict-rs) images the same thumbnail as PNG, the thumbnail in its own format, and the
// untouched original. Servers sometimes fail to resize (HTTP 502) or send a resized file Qt
// cannot read; one of the other copies usually works.
QStringList imageSources(const QString &primary, const QString &fallback = {});

// The video id of a YouTube watch / short / youtu.be link, or empty.
QString youtubeId(const QString &url);

} // namespace quokka::media
