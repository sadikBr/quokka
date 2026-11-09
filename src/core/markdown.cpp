#include "markdown.h"

#include "html.h"

#include <QRegularExpression>
#include <QTextDocument>

namespace quokka::markdown {

namespace {

const QString kSpoilerOpen = QStringLiteral("QKSPOILEROPEN");
const QString kSpoilerClose = QStringLiteral("QKSPOILERCLOSE");
const QString kSupOpen = QStringLiteral("QKSUPOPEN");
const QString kSupClose = QStringLiteral("QKSUPCLOSE");

QString preprocess(QString md, const QString &instanceHost)
{
    md.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));

    // ::: spoiler Title \n body \n :::  ->  **Title** <hidden body on one paragraph>
    static const QRegularExpression spoiler(
        QStringLiteral("^:::\\s*spoiler\\s*(.*?)\\s*\\n([\\s\\S]*?)\\n:::\\s*$"),
        QRegularExpression::MultilineOption);
    QRegularExpressionMatch m;
    int from = 0;
    while ((m = spoiler.match(md, from)).hasMatch()) {
        QString title = m.captured(1).trimmed();
        if (title.isEmpty())
            title = QStringLiteral("Spoiler");
        QString body = m.captured(2).simplified();
        const QString replacement = QStringLiteral("**%1:** %2%3%4")
                                        .arg(title, kSpoilerOpen, body, kSpoilerClose);
        md.replace(m.capturedStart(), m.capturedLength(), replacement);
        from = int(m.capturedStart() + replacement.size());
    }

    // Images become links: ![alt](url "title") -> [Image: alt](url)
    static const QRegularExpression image(QStringLiteral("!\\[([^\\]]*)\\]\\(\\s*<?([^)\\s>]+)>?[^)]*\\)"));
    md.replace(image, QStringLiteral("[Image: \\1](\\2)"));
    md.replace(QStringLiteral("[Image: ]("), QStringLiteral("[Image]("));

    // ^superscript^
    static const QRegularExpression sup(QStringLiteral("\\^([^\\s^][^^\\n]*?)\\^"));
    md.replace(sup, kSupOpen + QStringLiteral("\\1") + kSupClose);

    // Mentions: !community@host and @user@host (not inside words, URLs or emails).
    static const QRegularExpression community(
        QStringLiteral("(^|[\\s(])!([A-Za-z0-9_]+)@([A-Za-z0-9.-]+\\.[A-Za-z]{2,})"));
    md.replace(community, QStringLiteral("\\1[!\\2@\\3](https://\\3/c/\\2)"));
    static const QRegularExpression user(
        QStringLiteral("(^|[\\s(])@([A-Za-z0-9_]+)@([A-Za-z0-9.-]+\\.[A-Za-z]{2,})"));
    md.replace(user, QStringLiteral("\\1[@\\2@\\3](https://\\3/u/\\2)"));
    Q_UNUSED(instanceHost)
    return md;
}

QString postprocess(QString h)
{
    // Keep only the <body> contents.
    const int bodyStart = h.indexOf(QLatin1String("<body"));
    if (bodyStart >= 0) {
        const int contentStart = h.indexOf(QLatin1Char('>'), bodyStart) + 1;
        const int contentEnd = h.lastIndexOf(QLatin1String("</body>"));
        h = h.mid(contentStart, contentEnd > contentStart ? contentEnd - contentStart : -1);
    }
    // Let the theme decide link colours; Qt's importer maps _x_ to underline, Lemmy means italic.
    static const QRegularExpression linkColor(QStringLiteral("\\s*color:#0000ff;"));
    h.remove(linkColor);
    h.replace(QLatin1String("text-decoration: underline;"), QLatin1String("font-style:italic;"));
    static const QRegularExpression qtStyles(QStringLiteral("\\s*-qt-[a-z-]+:[^;\"]*;|\\s*text-indent:0px;"));
    h.remove(qtStyles);
    h.replace(QLatin1String("<span style=\"\">"), QLatin1String("<span>"));

    h.replace(kSpoilerOpen, QStringLiteral("<span style=\"%1\">").arg(QLatin1String(html::kSpoilerToken)));
    h.replace(kSpoilerClose, QStringLiteral("</span>"));
    h.replace(kSupOpen, QStringLiteral("<sup>"));
    h.replace(kSupClose, QStringLiteral("</sup>"));
    h.replace(QLatin1String("<table border=\"1\" cellspacing=\"2\">"),
              QLatin1String("<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\">"));
    return h.trimmed();
}

} // namespace

QString toHtml(const QString &markdown, const QString &instanceHost)
{
    if (markdown.trimmed().isEmpty())
        return {};
    QTextDocument doc;
    doc.setMarkdown(preprocess(markdown, instanceHost), QTextDocument::MarkdownDialectGitHub);
    return postprocess(doc.toHtml());
}

QString toPlainSummary(const QString &markdown, int maxLength)
{
    QString s = markdown;
    s.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    // First paragraph with actual text.
    const QStringList paragraphs = s.split(QRegularExpression(QStringLiteral("\\n\\s*\\n")), Qt::SkipEmptyParts);
    QString first;
    for (const QString &p : paragraphs) {
        const QString t = p.trimmed();
        if (t.isEmpty() || t.startsWith(QLatin1String("---")) || t.startsWith(QLatin1String("!["))
            || (t.startsWith(QLatin1Char('#')) && !t.contains(QLatin1Char('\n'))))
            continue; // rules, images and lone headings make poor summaries
        first = t;
        break;
    }
    static const QRegularExpression link(QStringLiteral("\\[([^\\]]*)\\]\\([^)]*\\)"));
    first.replace(link, QStringLiteral("\\1"));
    static const QRegularExpression marks(QStringLiteral("[*_`#>~|]+"));
    first.replace(marks, QString());
    first = first.simplified();
    if (first.size() > maxLength)
        first = first.left(maxLength - 1).trimmed() + QChar(0x2026);
    return first;
}

} // namespace quokka::markdown
