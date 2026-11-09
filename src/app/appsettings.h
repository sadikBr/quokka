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
    Q_PROPERTY(bool videoMuted READ videoMuted WRITE setVideoMuted NOTIFY videoMutedChanged)
    Q_PROPERTY(bool compactCards READ compactCards WRITE setCompactCards NOTIFY compactCardsChanged)
    Q_PROPERTY(bool dimVisited READ dimVisited WRITE setDimVisited NOTIFY dimVisitedChanged)
    // Cover NSFW pictures and videos until tapped (when NSFW posts are shown at all).
    Q_PROPERTY(bool coverNsfw READ coverNsfw WRITE setCoverNsfw NOTIFY coverNsfwChanged)

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
    bool videoMuted() const;
    void setVideoMuted(bool muted);
    bool compactCards() const;
    void setCompactCards(bool compact);
    bool dimVisited() const;
    void setDimVisited(bool dim);
    bool coverNsfw() const;
    void setCoverNsfw(bool cover);

    // Generic storage for small UI state (window geometry and the like).
    Q_INVOKABLE QVariant value(const QString &key, const QVariant &defaultValue = {}) const;
    Q_INVOKABLE void setValue(const QString &key, const QVariant &value);

signals:
    void themeModeChanged();
    void showNsfwChanged();
    void lastFeedChanged();
    void defaultSortChanged();
    void commentSortChanged();
    void videoMutedChanged();
    void compactCardsChanged();
    void dimVisitedChanged();
    void coverNsfwChanged();

private:
    QSettings m_settings;
};
