#pragma once

#include "post.h"

#include <QList>
#include <QString>
#include <optional>

namespace quokka::media {

// What a URL points at, if it is something Quokka can show in place:
// - direct files: .jpg .png .webp .gif .avif (images), .mp4 .webm .mov .m4v (videos)
// - imgur pages and .gifv, rewritten to their direct file
// - redgifs and streamable pages, which need a lookup before playing (MediaItem::resolve)
std::optional<MediaItem> classify(const QString &url);

// All showable media linked from a text (Markdown or HTML), in order, without duplicates.
QList<MediaItem> extractFromText(const QString &text, int max = 12);

} // namespace quokka::media
