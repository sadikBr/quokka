#include "medialinks.h"

#include <QRegularExpression>
#include <QSet>
#include <QUrl>

namespace quokka::media {

namespace {

bool endsWithAny(const QString &path, std::initializer_list<const char *> exts)
{
    for (const char *e : exts)
        if (path.endsWith(QLatin1String(e)))
            return true;
    return false;
}

} // namespace

std::optional<MediaItem> classify(const QString &raw)
{
    const QUrl url(raw.trimmed());
    if (!url.isValid() || (url.scheme() != QLatin1String("https") && url.scheme() != QLatin1String("http")))
        return std::nullopt;
    const QString host = url.host().toLower();
    const QString path = url.path().toLower();
    const QStringList parts = url.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);

    MediaItem m;
    m.url = url.toString();

    // imgur: .gifv is an MP4; plain image pages map to the direct file.
    if (host == QLatin1String("i.imgur.com") && path.endsWith(QLatin1String(".gifv"))) {
        m.url.replace(QRegularExpression(QStringLiteral("\\.gifv(\\?.*)?$"), QRegularExpression::CaseInsensitiveOption),
                      QStringLiteral(".mp4"));
        m.video = m.animated = true;
        return m;
    }
    if ((host == QLatin1String("imgur.com") || host == QLatin1String("www.imgur.com") || host == QLatin1String("m.imgur.com"))
        && parts.size() == 1 && !parts.first().contains(QLatin1Char('.'))) {
        m.url = QStringLiteral("https://i.imgur.com/%1.jpg").arg(parts.first());
        return m;
    }

    // Pages that need a lookup to find the video file.
    if (host.endsWith(QLatin1String("redgifs.com")) && parts.size() >= 2
        && (parts.first() == QLatin1String("watch") || parts.first() == QLatin1String("ifr"))) {
        m.video = m.animated = true;
        m.resolve = QStringLiteral("redgifs");
        return m;
    }
    if ((host == QLatin1String("streamable.com") || host == QLatin1String("www.streamable.com")) && parts.size() == 1) {
        m.video = true;
        m.resolve = QStringLiteral("streamable");
        return m;
    }

    // Direct files.
    if (endsWithAny(path, {".mp4", ".webm", ".mov", ".m4v", ".m3u8"})) {
        m.video = true;
        m.animated = path.contains(QLatin1String("gif"));
        return m;
    }
    if (endsWithAny(path, {".jpg", ".jpeg", ".png", ".webp", ".avif", ".gif"})) {
        m.animated = path.endsWith(QLatin1String(".gif"));
        return m;
    }
    return std::nullopt;
}

QList<MediaItem> extractFromText(const QString &text, int max)
{
    QList<MediaItem> out;
    if (text.isEmpty())
        return out;
    static const QRegularExpression urlRe(QStringLiteral("https?://[^\\s<>()\\[\\]\"'`]+"));
    QSet<QString> seen;
    auto it = urlRe.globalMatch(text);
    while (it.hasNext() && out.size() < max) {
        QString u = it.next().captured(0);
        u.replace(QLatin1String("&amp;"), QLatin1String("&"));
        while (!u.isEmpty() && QStringLiteral(".,;:!?*_~").contains(u.back()))
            u.chop(1);
        const auto m = classify(u);
        if (m && !seen.contains(m->url)) {
            seen.insert(m->url);
            out.append(*m);
        }
    }
    return out;
}

} // namespace quokka::media
