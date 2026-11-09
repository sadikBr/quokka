#include "appsettings.h"

namespace {
const QString kTheme = QStringLiteral("ui/themeMode");
const QString kNsfw = QStringLiteral("content/showNsfw");
const QString kLastFeed = QStringLiteral("browse/lastFeed");
const QString kDefaultSort = QStringLiteral("browse/defaultSort");
const QString kCompact = QStringLiteral("ui/compactCards");
const QString kDimVisited = QStringLiteral("ui/dimVisited");
const QString kNsfwOnly = QStringLiteral("content/nsfwOnly");
const QString kHideBroken = QStringLiteral("media/hideBroken");
const QString kCoverNsfw = QStringLiteral("content/coverNsfw");
const QString kScrollSpeed = QStringLiteral("ui/scrollSpeed");
const QString kVideoMuted = QStringLiteral("media/videoMuted");
const QString kAutoplayFeed = QStringLiteral("media/autoplayFeed");
const QString kCommentSort = QStringLiteral("browse/commentSort");
} // namespace

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
{
}

AppSettings::ThemeMode AppSettings::themeMode() const
{
    const int v = m_settings.value(kTheme, int(System)).toInt();
    return (v >= System && v <= Dark) ? ThemeMode(v) : System;
}

void AppSettings::setThemeMode(ThemeMode mode)
{
    if (mode == themeMode())
        return;
    m_settings.setValue(kTheme, int(mode));
    emit themeModeChanged();
}

bool AppSettings::showNsfw() const
{
    // NSFW content is hidden unless the user opts in.
    return m_settings.value(kNsfw, false).toBool();
}

void AppSettings::setShowNsfw(bool show)
{
    if (show == showNsfw())
        return;
    m_settings.setValue(kNsfw, show);
    emit showNsfwChanged();
}

QString AppSettings::lastFeed() const
{
    return m_settings.value(kLastFeed, QStringLiteral("all")).toString();
}

void AppSettings::setLastFeed(const QString &feed)
{
    if (feed == lastFeed())
        return;
    m_settings.setValue(kLastFeed, feed);
    emit lastFeedChanged();
}

QString AppSettings::defaultSort() const
{
    return m_settings.value(kDefaultSort, QStringLiteral("hot")).toString();
}

void AppSettings::setDefaultSort(const QString &sort)
{
    if (sort == defaultSort())
        return;
    m_settings.setValue(kDefaultSort, sort);
    emit defaultSortChanged();
}

QString AppSettings::commentSort() const
{
    return m_settings.value(kCommentSort, QStringLiteral("confidence")).toString();
}

void AppSettings::setCommentSort(const QString &sort)
{
    if (sort == commentSort())
        return;
    m_settings.setValue(kCommentSort, sort);
    emit commentSortChanged();
}

bool AppSettings::videoMuted() const
{
    return m_settings.value(kVideoMuted, false).toBool();
}

void AppSettings::setVideoMuted(bool muted)
{
    if (muted == videoMuted())
        return;
    m_settings.setValue(kVideoMuted, muted);
    emit videoMutedChanged();
}

bool AppSettings::compactCards() const
{
    return m_settings.value(kCompact, false).toBool();
}

void AppSettings::setCompactCards(bool compact)
{
    if (compact == compactCards())
        return;
    m_settings.setValue(kCompact, compact);
    emit compactCardsChanged();
}

bool AppSettings::dimVisited() const
{
    return m_settings.value(kDimVisited, true).toBool();
}

void AppSettings::setDimVisited(bool dim)
{
    if (dim == dimVisited())
        return;
    m_settings.setValue(kDimVisited, dim);
    emit dimVisitedChanged();
}

QVariant AppSettings::value(const QString &key, const QVariant &defaultValue) const
{
    return m_settings.value(key, defaultValue);
}

void AppSettings::setValue(const QString &key, const QVariant &value)
{
    m_settings.setValue(key, value);
}

bool AppSettings::coverNsfw() const
{
    return m_settings.value(kCoverNsfw, true).toBool();
}

void AppSettings::setCoverNsfw(bool cover)
{
    if (cover == coverNsfw())
        return;
    m_settings.setValue(kCoverNsfw, cover);
    emit coverNsfwChanged();
}

double AppSettings::scrollSpeed() const
{
    return qBound(0.5, m_settings.value(kScrollSpeed, 1.0).toDouble(), 4.0);
}

void AppSettings::setScrollSpeed(double speed)
{
    speed = qBound(0.5, speed, 4.0);
    if (qFuzzyCompare(speed, scrollSpeed()))
        return;
    m_settings.setValue(kScrollSpeed, speed);
    emit scrollSpeedChanged();
}

bool AppSettings::autoplayFeed() const
{
    return m_settings.value(kAutoplayFeed, true).toBool();
}

void AppSettings::setAutoplayFeed(bool on)
{
    if (on == autoplayFeed())
        return;
    m_settings.setValue(kAutoplayFeed, on);
    emit autoplayFeedChanged();
}

bool AppSettings::nsfwOnly() const
{
    return m_settings.value(kNsfwOnly, false).toBool();
}

void AppSettings::setNsfwOnly(bool on)
{
    if (on == nsfwOnly())
        return;
    m_settings.setValue(kNsfwOnly, on);
    emit nsfwOnlyChanged();
}

bool AppSettings::hideBrokenMedia() const
{
    return m_settings.value(kHideBroken, true).toBool();
}

void AppSettings::setHideBrokenMedia(bool on)
{
    if (on == hideBrokenMedia())
        return;
    m_settings.setValue(kHideBroken, on);
    emit hideBrokenMediaChanged();
}
