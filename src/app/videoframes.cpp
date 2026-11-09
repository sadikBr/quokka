#include "videoframes.h"

#include "networkfactory.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QMediaPlayer>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <QVideoFrame>
#include <QVideoSink>

namespace {
constexpr int kMaxRunning = 2;
constexpr int kMaxQueued = 40;
constexpr int kTimeoutMs = 20000;
constexpr int kMaxFramesToSkip = 45;   // dark intro frames to skip before taking one anyway
constexpr int kWidth = 640;

// Average brightness 0..255 from a coarse grid of samples.
int brightness(const QImage &img)
{
    if (img.isNull())
        return 0;
    qint64 sum = 0;
    int n = 0;
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x) {
            sum += qGray(img.pixel(img.width() * (2 * x + 1) / 16, img.height() * (2 * y + 1) / 16));
            ++n;
        }
    return int(sum / n);
}
} // namespace

VideoFrames::VideoFrames(QObject *parent)
    : QObject(parent)
{
}

QString VideoFrames::framePath(const QString &videoUrl)
{
    const QByteArray hash = QCryptographicHash::hash(videoUrl.toUtf8(), QCryptographicHash::Sha1).toHex();
    return NetworkFactory::frameDirectory() + QLatin1Char('/') + QString::fromLatin1(hash) + QStringLiteral(".jpg");
}

QString VideoFrames::frameFor(const QString &videoUrl) const
{
    if (videoUrl.isEmpty())
        return {};
    const QString path = framePath(videoUrl);
    return QFile::exists(path) ? QUrl::fromLocalFile(path).toString() : QString();
}

void VideoFrames::request(const QString &videoUrl)
{
    if (videoUrl.isEmpty() || m_failed.contains(videoUrl) || m_running.contains(videoUrl))
        return;
    if (const QString ready = frameFor(videoUrl); !ready.isEmpty()) {
        QMetaObject::invokeMethod(this, [this, videoUrl, ready] { emit frameReady(videoUrl, ready); },
                                  Qt::QueuedConnection);
        return;
    }
    m_queue.removeAll(videoUrl);
    m_queue.append(videoUrl);
    while (m_queue.size() > kMaxQueued)
        m_queue.removeFirst(); // the oldest requests have long scrolled away
    QMetaObject::invokeMethod(this, &VideoFrames::pump, Qt::QueuedConnection);
}

void VideoFrames::pump()
{
    while (m_running.size() < kMaxRunning && !m_queue.isEmpty())
        start(m_queue.takeLast());
}

void VideoFrames::start(const QString &videoUrl)
{
    m_running.insert(videoUrl);
    auto *player = new QMediaPlayer(this);
    auto *sink = new QVideoSink(player);
    player->setVideoSink(sink);   // no audio output: plays silently
    auto skipped = std::make_shared<int>(0);
    QPointer<QMediaPlayer> guard(player);

    connect(sink, &QVideoSink::videoFrameChanged, player, [this, player, videoUrl, skipped](const QVideoFrame &frame) {
        if (!m_running.contains(videoUrl) || !frame.isValid())
            return;
        QImage img = frame.toImage();
        if (img.isNull())
            return;
        if (brightness(img) < 18 && ++*skipped < kMaxFramesToSkip)
            return; // a dark intro: wait for something to see
        if (img.width() > kWidth)
            img = img.scaledToWidth(kWidth, Qt::SmoothTransformation);
        QDir().mkpath(NetworkFactory::frameDirectory());
        const bool ok = img.convertToFormat(QImage::Format_RGB32).save(framePath(videoUrl), "JPG", 82);
        finish(player, videoUrl, ok);
    });
    connect(player, &QMediaPlayer::errorOccurred, player, [this, player, videoUrl] {
        finish(player, videoUrl, false);
    });
    QTimer::singleShot(kTimeoutMs, player, [this, player, videoUrl] { finish(player, videoUrl, false); });

    player->setSource(QUrl(videoUrl));
    player->play();
}

void VideoFrames::finish(QMediaPlayer *player, const QString &videoUrl, bool ok)
{
    if (!m_running.remove(videoUrl))
        return; // already finished (e.g. an error after the frame was saved)
    player->disconnect(this);
    player->stop();
    player->deleteLater();
    if (ok) {
        emit frameReady(videoUrl, QUrl::fromLocalFile(framePath(videoUrl)).toString());
    } else {
        m_failed.insert(videoUrl);
        emit frameFailed(videoUrl);
    }
    QMetaObject::invokeMethod(this, &VideoFrames::pump, Qt::QueuedConnection);
}
