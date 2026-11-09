#pragma once

#include <QHash>
#include <QObject>
#include <QQmlEngine>
#include <QSet>
#include <QStringList>

class QMediaPlayer;

// Still pictures for videos that come without one (QML: `VideoFrames`): the video is opened
// without sound, the first frame that isn't (nearly) black is saved as a JPEG in the cache, and
// frameReady(url, fileUrl) is emitted. At most two videos are opened at a time; the newest
// requests go first (they're the ones on screen).
class VideoFrames : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit VideoFrames(QObject *parent = nullptr);

    Q_INVOKABLE void request(const QString &videoUrl);
    Q_INVOKABLE QString frameFor(const QString &videoUrl) const;   // "" until ready
    Q_INVOKABLE bool failedFor(const QString &videoUrl) const { return m_failed.contains(videoUrl); }

    static QString framePath(const QString &videoUrl);

signals:
    void frameReady(const QString &videoUrl, const QString &fileUrl);
    void frameFailed(const QString &videoUrl);

private:
    void pump();
    void start(const QString &videoUrl);
    void finish(QMediaPlayer *player, const QString &videoUrl, bool ok);

    QStringList m_queue;              // newest last
    QSet<QString> m_running;
    QSet<QString> m_failed;
};
