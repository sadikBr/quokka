#include "html.h"

#include <QRegularExpression>

namespace quokka::html {

QString cleanRedditHtml(const QString &input)
{
    if (input.isEmpty())
        return {};
    QString s = input;

    static const QRegularExpression comments(QStringLiteral("<!--.*?-->"),
                                             QRegularExpression::DotMatchesEverythingOption);
    s.remove(comments);

    static const QRegularExpression mdDiv(QStringLiteral("^\\s*<div class=\"md\">(.*)</div>\\s*$"),
                                          QRegularExpression::DotMatchesEverythingOption);
    const auto m = mdDiv.match(s);
    if (m.hasMatch())
        s = m.captured(1);

    // Relative links -> absolute, so the UI can route them.
    static const QRegularExpression relLink(QStringLiteral("href=\"/"));
    s.replace(relLink, QStringLiteral("href=\"https://www.reddit.com/"));

    static const QRegularExpression spoiler(QStringLiteral("<span class=\"md-spoiler-text\"[^>]*>"));
    s.replace(spoiler, QStringLiteral("<span style=\"%1\">").arg(QLatin1String(kSpoilerToken)));

    // Qt rich text ignores CSS borders on tables; the HTML attributes work.
    s.replace(QStringLiteral("<table>"),
              QStringLiteral("<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\">"));

    s.replace(QStringLiteral("<del>"), QStringLiteral("<s>"));
    s.replace(QStringLiteral("</del>"), QStringLiteral("</s>"));

    return s.trimmed();
}

bool hasSpoilers(const QString &cleaned)
{
    return cleaned.contains(QLatin1String(kSpoilerToken));
}

} // namespace quokka::html
