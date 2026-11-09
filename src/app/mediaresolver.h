#pragma once

#include <QHash>
#include <QObject>
#include <QQmlEngine>

class QNetworkAccessManager;
class QNetworkReply;

// Finds the playable file behind a video page (QML: `MediaResolver`).
//   MediaResolver.resolve("https://www.redgifs.com/watch/xyz", "redgifs")
// answers with resolved(pageUrl, mediaUrl) or failed(pageUrl, message). Results are cached.
// Supported: "redgifs" (public API with a temporary token), "streamable".
class MediaResolver : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit MediaResolver(QObject *parent = nullptr);

    Q_INVOKABLE void resolve(const QString &pageUrl, const QString &kind);
    // Copies of a picture to try in turn (see quokka::media::imageSources).
    Q_INVOKABLE QStringList imageSources(const QString &primary, const QString &fallback = {}) const;
    Q_INVOKABLE QString cached(const QString &pageUrl) const { return m_cache.value(pageUrl); }

    // Pure helpers, exposed for tests.
    static QString redgifsId(const QString &pageUrl);
    static QString streamableCode(const QString &pageUrl);
    static QString parseRedgifs(const QByteArray &json);
    static QString parseStreamable(const QByteArray &json);

signals:
    void resolved(const QString &pageUrl, const QString &mediaUrl);
    void failed(const QString &pageUrl, const QString &message);

private:
    QNetworkReply *get(const QUrl &url, const QByteArray &bearer = {});
    void resolveRedgifs(const QString &pageUrl, bool retried);
    void finish(const QString &pageUrl, const QString &mediaUrl, const QString &error);

    QNetworkAccessManager *m_nam;
    QHash<QString, QString> m_cache;
    QByteArray m_redgifsToken;
};
