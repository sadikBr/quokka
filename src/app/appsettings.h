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
    // Only NSFW posts and communities (when NSFW is shown at all).
    Q_PROPERTY(bool nsfwOnly READ nsfwOnly WRITE setNsfwOnly NOTIFY nsfwOnlyChanged)
    // Check media links in the background and hide posts whose media no longer loads.
    Q_PROPERTY(bool hideBrokenMedia READ hideBrokenMedia WRITE setHideBrokenMedia NOTIFY hideBrokenMediaChanged)
    // Cover NSFW pictures and videos until tapped (when NSFW posts are shown at all).
    Q_PROPERTY(bool coverNsfw READ coverNsfw WRITE setCoverNsfw NOTIFY coverNsfwChanged)
    // The private section of Settings (NSFW and adult sources) is shown. Unlocked by tapping
    // the version on the About page seven times; on for installs that already show NSFW.
    Q_PROPERTY(bool privateUnlocked READ privateUnlocked WRITE setPrivateUnlocked NOTIFY privateUnlockedChanged)
    // Servers mixed in the "All servers" feed (comma-separated hosts).
    Q_PROPERTY(QString servers READ servers WRITE setServers NOTIFY serversChanged)
    // Extra servers mixed in when NSFW is shown (private section).
    Q_PROPERTY(QString nsfwServers READ nsfwServers WRITE setNsfwServers NOTIFY nsfwServersChanged)
    // The combined feed's source name for PostListModel ("servers:a,b,c").
    Q_PROPERTY(QString serversSource READ serversSource NOTIFY serversSourceChanged)
    // Multiplier for mouse-wheel / touchpad scrolling distance (1 = normal).
    // Play videos and GIFs muted in the feed while they're in the middle of the screen.
    Q_PROPERTY(bool autoplayFeed READ autoplayFeed WRITE setAutoplayFeed NOTIFY autoplayFeedChanged)
    Q_PROPERTY(double scrollSpeed READ scrollSpeed WRITE setScrollSpeed NOTIFY scrollSpeedChanged)

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
    bool nsfwOnly() const;
    void setNsfwOnly(bool on);
    bool hideBrokenMedia() const;
    void setHideBrokenMedia(bool on);
    bool coverNsfw() const;
    void setCoverNsfw(bool cover);
    bool autoplayFeed() const;
    void setAutoplayFeed(bool on);
    bool privateUnlocked() const;
    void setPrivateUnlocked(bool on);
    QString servers() const;
    void setServers(const QString &list);
    QString nsfwServers() const;
    void setNsfwServers(const QString &list);
    QString serversSource() const;
    double scrollSpeed() const;
    void setScrollSpeed(double speed);

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
    void hideBrokenMediaChanged();
    void nsfwOnlyChanged();
    void scrollSpeedChanged();
    void privateUnlockedChanged();
    void serversChanged();
    void nsfwServersChanged();
    void serversSourceChanged();
    void autoplayFeedChanged();

private:
    QSettings m_settings;
};
