#include "appsettings.h"

namespace {
const QString kTheme = QStringLiteral("ui/themeMode");
const QString kNsfw = QStringLiteral("content/showNsfw");
const QString kLastFeed = QStringLiteral("browse/lastFeed");
const QString kDefaultSort = QStringLiteral("browse/defaultSort");
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
    return m_settings.value(kLastFeed, QStringLiteral("popular")).toString();
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
