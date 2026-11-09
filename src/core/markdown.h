#pragma once

#include <QString>

namespace quokka::markdown {

// Converts Lemmy-flavoured Markdown (CommonMark + GitHub tables/strikethrough, ^superscript^,
// "::: spoiler" blocks, !community@host and @user@host mentions) into HTML for QML's
// Text.RichText. Spoilers are marked with html::kSpoilerToken like Reddit's, and images become
// links (the media viewer opens them) so comments never pull in huge pictures inline.
// `instanceHost` is used to link mentions; needs a QGuiApplication (it uses QTextDocument).
QString toHtml(const QString &markdown, const QString &instanceHost = {});

// Plain text for previews: markup removed, spoilers hidden, paragraphs joined, cut at maxLength.
QString toPlainExcerpt(const QString &markdown, int maxLength = 600);

// A short plain-text summary (first paragraph, markup removed), e.g. for community headers.
QString toPlainSummary(const QString &markdown, int maxLength = 220);

} // namespace quokka::markdown
