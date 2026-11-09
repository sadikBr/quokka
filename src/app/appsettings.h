#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QSettings>

// User preferences, persisted with QSettings and exposed to QML as `AppSettings`.
class AppSettings : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(ThemeMode themeMode READ themeMode WRITE setThemeMode NOTIFY themeModeChanged)
    Q_PROPERTY(bool showNsfw READ showNsfw WRITE setShowNsfw NOTIFY showNsfwChanged)
    Q_PROPERTY(QString lastFeed READ lastFeed WRITE setLastFeed NOTIFY lastFeedChanged)
    Q_PROPERTY(QString defaultSort READ defaultSort WRITE setDefaultSort NOTIFY defaultSortChanged)
    Q_PROPERTY(QString commentSort READ commentSort WRITE setCommentSort NOTIFY commentSortChanged)

public:
    enum ThemeMode { System, Light, Dark };
    Q_ENUM(ThemeMode)

    explicit AppSettings(QObject *parent = nullptr);

    ThemeMode themeMode() const;
    void setThemeMode(ThemeMode mode);
    bool showNsfw() const;
    void setShowNsfw(bool show);
    QString lastFeed() const;
    void setLastFeed(const QString &feed);
    QString defaultSort() const;
    void setDefaultSort(const QString &sort);
    QString commentSort() const;
    void setCommentSort(const QString &sort);

signals:
    void themeModeChanged();
    void showNsfwChanged();
    void lastFeedChanged();
    void defaultSortChanged();
    void commentSortChanged();

private:
    QSettings m_settings;
};
