#pragma once

#include <QString>

namespace quokka::html {

// Placeholder put in the style attribute of spoiler spans. The UI swaps it for real CSS so it
// can hide or reveal spoilers and follow the theme.
inline constexpr char kSpoilerToken[] = "QK_SPOILER";

// Turns Reddit's rendered HTML (body_html / selftext_html with raw_json=1) into something
// QML's Text.RichText renders well: strips wrappers and comments, absolutises links,
// marks spoilers, and tidies tables and code blocks.
QString cleanRedditHtml(const QString &html);

// True when cleanRedditHtml() output contains spoilers.
bool hasSpoilers(const QString &cleaned);

} // namespace quokka::html
