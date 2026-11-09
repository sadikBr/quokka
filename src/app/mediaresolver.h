#pragma once

#include <QHash>
#include <QSet>
#include <QObject>
#include <QQmlEngine>

class QNetworkAccessManager;
class QNetworkReply;

// Finds the playable file behind a video page (QML: `MediaResolver`).
//   MediaResolver.resolve("https://www.redgifs.com/watch/xyz", "redgifs")
// answers with resolved(pageUrl, mediaUrl) or failed(pageUrl, message). Results are cached.
// Supported: "redgifs" (public API with a temporary token), "streamable", "eporner".
class MediaResolver : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    // Last resort for pictures that fail everywhere or can't be decoded (AVIF/HEIC under a
    // .png name, slightly broken files): a JPEG made by the wsrv.nl conversion service. Saved.
    Q_PROPERTY(bool converterEnabled READ converterEnabled WRITE setConverterEnabled NOTIFY converterEnabledChanged)

public:
    explicit MediaResolver(QObject *parent = nullptr);

    bool converterEnabled() const { return m_converter; }
    void setConverterEnabled(bool on);
    // The converted (JPEG) copy of an image URL.
    static QString convertedUrl(const QString &url);

    Q_INVOKABLE void resolve(const QString &pageUrl, const QString &kind);
    // Copies of a picture to try in turn (see quokka::media::imageSources), minus the ones that
    // already failed this session and those on servers that look down.
    Q_INVOKABLE QStringList imageSources(const QString &primary, const QString &fallback = {}) const;
    // Images report back whether a URL loaded, so dead copies and servers are skipped next time.
    Q_INVOKABLE void reportImage(const QString &url, bool ok);
    // A short, human explanation for an HTTP failure ("deleted", "server down", ...).
    Q_INVOKABLE static QString describeFailure(int httpStatus, const QString &url);
    // Same for an FFmpeg / Qt Multimedia error message.
    Q_INVOKABLE static QString describePlaybackError(const QString &message, const QString &url);
    Q_INVOKABLE QString cached(const QString &pageUrl) const { return m_cache.value(pageUrl); }

    // Animated GIFs. Qt's GIF player (QMovie) calls itself again for every frame it can't read
    // and, on a damaged or cut-off GIF, overflows the stack and crashes the app. So GIFs are
    // played only from a local copy whose every frame decoded: prepareAnimation() downloads and
    // checks one off the UI thread, then answers animationReady(url, fileUrl) or animationFailed(url).
    Q_INVOKABLE void prepareAnimation(const QString &url);
    Q_INVOKABLE QString animationFile(const QString &url) const;   // "" until checked and ready
    Q_INVOKABLE bool animationBroken(const QString &url) const;
    // True when every frame of `data` decodes (and there is at least one).
    static bool isSafeAnimation(const QByteArray &data, int *frames = nullptr);

    // Pure helpers, exposed for tests.
    static QString redgifsId(const QString &pageUrl);
    static QString streamableCode(const QString &pageUrl);
    static QString parseRedgifs(const QByteArray &json);
    static QString parseStreamable(const QByteArray &json);
    // eporner: the page's player hash (32 hex digits) as the video endpoint wants it (four
    // base-36 numbers), and the best MP4 (at most 1080p) from that endpoint's answer.
    static QString epornerHash(const QString &hex);
    static QString parseEporner(const QByteArray &json, QString *error = nullptr);

signals:
    void converterEnabledChanged();
    void animationReady(const QString &url, const QString &fileUrl);
    void animationFailed(const QString &url);
    void resolved(const QString &pageUrl, const QString &mediaUrl);
    void failed(const QString &pageUrl, const QString &message);

private:
    QNetworkReply *get(const QUrl &url, const QByteArray &bearer = {});
    void resolveRedgifs(const QString &pageUrl, bool retried);
    void resolveEporner(const QString &pageUrl);
    void finish(const QString &pageUrl, const QString &mediaUrl, const QString &error);

    QNetworkAccessManager *m_nam;
    QHash<QString, QString> m_cache;
    QByteArray m_redgifsToken;
    QSet<QString> m_deadUrls;
    QHash<QString, int> m_hostFailures;
    QSet<QString> m_hostsOk;
    bool m_converter = true;
    QSet<QString> m_badAnimations;
    QSet<QString> m_pendingAnimations;
};
