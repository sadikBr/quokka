#include "medialinks.h"

#include <QRegularExpression>
#include <QSet>
#include <QUrl>
#include <QUrlQuery>

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

    // Reddit-hosted video (common in cross-posts): the HLS playlist plays with sound.
    if (host == QLatin1String("v.redd.it") && !parts.isEmpty() && !parts.first().contains(QLatin1Char('.'))) {
        m.url = QStringLiteral("https://v.redd.it/%1/HLSPlaylist.m3u8").arg(parts.first());
        m.video = true;
        return m;
    }

    // giphy pages: giphy.com/gifs/some-title-ID, giphy.com/embed/ID, media.giphy.com/media/ID/
    if (host == QLatin1String("giphy.com") || host == QLatin1String("www.giphy.com")
        || ((host.endsWith(QLatin1String(".giphy.com"))) && !endsWithAny(path, {".gif", ".mp4", ".webp"}))) {
        QString id;
        if (parts.size() >= 2 && (parts[0] == QLatin1String("gifs") || parts[0] == QLatin1String("embed")
                                  || parts[0] == QLatin1String("media") || parts[0] == QLatin1String("clips")))
            id = parts[1].section(QLatin1Char('-'), -1);
        if (!id.isEmpty()) {
            m.url = QStringLiteral("https://i.giphy.com/media/%1/giphy.mp4").arg(id);
            m.video = m.animated = true;
            return m;
        }
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

QStringList imageSources(const QString &primary, const QString &fallback)
{
    QStringList out;
    auto add = [&out](const QString &u) {
        if (!u.isEmpty() && !out.contains(u))
            out.append(u);
    };
    add(primary);
    add(fallback);
    for (const QString &u : {primary, fallback}) {
        if (!u.contains(QLatin1String("/pictrs/image/")))
            continue;
        const QUrl url(u);
        QUrlQuery query(url);
        const QString size = query.queryItemValue(QStringLiteral("thumbnail"));
        QUrl bare = url;
        bare.setQuery(QString());
        if (!size.isEmpty()) {
            QUrl png = bare, own = bare;
            png.setQuery(QStringLiteral("thumbnail=%1&format=png").arg(size));
            own.setQuery(QStringLiteral("thumbnail=%1").arg(size));
            add(png.toString());
            add(own.toString());
        }
        add(bare.toString());
    }
    return out;
}

QString youtubeId(const QString &raw)
{
    const QUrl url(raw.trimmed());
    QString host = url.host().toLower();
    if (host.startsWith(QLatin1String("www.")) || host.startsWith(QLatin1String("m.")))
        host = host.section(QLatin1Char('.'), 1);
    const QStringList parts = url.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    QString id;
    if (host == QLatin1String("youtu.be") && !parts.isEmpty())
        id = parts.first();
    else if (host == QLatin1String("youtube.com") || host == QLatin1String("music.youtube.com")) {
        if (!parts.isEmpty() && parts.first() == QLatin1String("watch"))
            id = QUrlQuery(url).queryItemValue(QStringLiteral("v"));
        else if (parts.size() >= 2 && (parts[0] == QLatin1String("shorts") || parts[0] == QLatin1String("embed")
                                       || parts[0] == QLatin1String("live")))
            id = parts[1];
    }
    static const QRegularExpression valid(QStringLiteral("^[A-Za-z0-9_-]{6,20}$"));
    return valid.match(id).hasMatch() ? id : QString();
}

} // namespace quokka::media
