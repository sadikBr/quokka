#include "mediaprobe.h"

#include "apiclient.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QUrl>

namespace quokka {

namespace {
constexpr int kMaxRunning = 4;
constexpr int kTimeoutMs = 12000;
constexpr qint64 kRememberDeadSecs = 7 * 24 * 3600;

QString storeKey(const QString &url)
{
    return QStringLiteral("deadMedia/")
        + QString::fromLatin1(QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Sha1).toHex());
}

// Links we can't usefully check with a plain request.
bool uncheckable(const QString &url)
{
    if (url.isEmpty())
        return true;
    const QUrl u(url);
    if (u.isLocalFile() || (u.scheme() != QLatin1String("http") && u.scheme() != QLatin1String("https")))
        return true;
    const QString host = u.host().toLower();
    // Pages looked up through their own API first (see MediaResolver).
    // (redgifs files too: unsure how its media servers answer a bare ranged request.)
    return host.endsWith(QLatin1String("redgifs.com")) || host.endsWith(QLatin1String("streamable.com"))
        || host.endsWith(QLatin1String("eporner.com"));
}
} // namespace

MediaProbe *MediaProbe::instance()
{
    static MediaProbe *probe = new MediaProbe;
    return probe;
}

MediaProbe::MediaProbe(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

bool MediaProbe::looksLikeMedia(int status, const QByteArray &contentType)
{
    if (status != 200 && status != 206)
        return false;
    const QByteArray type = contentType.toLower();
    if (type.isEmpty())
        return true; // some file hosts send none; give them the benefit of the doubt
    return type.startsWith("image/") || type.startsWith("video/") || type.startsWith("audio/")
        || type.contains("mpegurl") || type.contains("dash+xml") || type.startsWith("application/octet-stream")
        || type.startsWith("binary/");
}

MediaProbe::State MediaProbe::state(const QString &url) const
{
    if (const auto it = m_states.constFind(url); it != m_states.constEnd())
        return it.value();
    const qint64 since = m_store.value(storeKey(url), 0).toLongLong();
    if (since > 0 && QDateTime::currentSecsSinceEpoch() - since < kRememberDeadSecs)
        return State::Dead;
    return State::Unknown;
}

void MediaProbe::markDead(const QString &url)
{
    if (url.isEmpty())
        return;
    m_states.insert(url, State::Dead);
    m_store.setValue(storeKey(url), QDateTime::currentSecsSinceEpoch());
}

void MediaProbe::markAlive(const QString &url)
{
    if (url.isEmpty())
        return;
    m_states.insert(url, State::Alive);
    m_store.remove(storeKey(url));
}

void MediaProbe::check(const QStringList &urls, QObject *context, std::function<void(bool)> done)
{
    // Already decided?
    bool allDead = !urls.isEmpty();
    for (const QString &u : urls) {
        if (uncheckable(u) || state(u) == State::Alive)
            return; // works (or can't be checked): nothing to do
        if (state(u) != State::Dead)
            allDead = false;
    }
    if (urls.isEmpty())
        return;
    if (allDead) {
        // Known dead: answer once the caller has finished what it's doing.
        QPointer<QObject> ctx(context);
        QMetaObject::invokeMethod(this, [ctx, done = std::move(done)] { if (ctx) done(false); },
                                  Qt::QueuedConnection);
        return;
    }
    m_waiters.append({urls, context, std::move(done)});
    for (const QString &u : urls)
        if (state(u) == State::Unknown && !m_running.contains(u) && !m_queue.contains(u))
            m_queue.append(u);
    pump();
}

void MediaProbe::pump()
{
    while (m_running.size() < kMaxRunning && !m_queue.isEmpty())
        probe(m_queue.takeFirst());
}

void MediaProbe::probe(const QString &url)
{
    m_running.insert(url);
    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::UserAgentHeader, ApiClient::defaultUserAgent());
    req.setRawHeader("Range", "bytes=0-1023");
    req.setTransferTimeout(kTimeoutMs);
    QNetworkReply *reply = m_nam->get(req);

    auto decided = std::make_shared<bool>(false);
    // Decide on the headers; the body isn't needed (and servers that ignore Range would send
    // the whole file).
    auto decide = [this, reply, url, decided](bool finished) {
        if (*decided)
            return;
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (!finished && (status == 0 || (status >= 300 && status < 400)))
            return; // no answer yet, or a redirect being followed
        *decided = true;
        if (finished && status == 0) {
            // No HTTP answer at all (offline, DNS, refused, timeout, a proxy in the way): that says
            // nothing about the link, so it isn't condemned; only a server's own answer can.
            settle(url, true);
        } else {
            settle(url, looksLikeMedia(status, reply->header(QNetworkRequest::ContentTypeHeader).toByteArray()));
        }
        // Stop the download, but not from inside the reply's own signal, and never once it has
        // finished (Qt warns "must only be called once" and can misbehave).
        if (!finished)
            QMetaObject::invokeMethod(reply, [reply] { if (!reply->isFinished()) reply->abort(); },
                                      Qt::QueuedConnection);
    };
    connect(reply, &QNetworkReply::metaDataChanged, this, [decide] { decide(false); });
    connect(reply, &QNetworkReply::finished, this, [this, reply, url, decide] {
        decide(true);
        reply->deleteLater();
        m_running.remove(url);
        pump();
    });
}

void MediaProbe::settle(const QString &url, bool alive)
{
    alive ? markAlive(url) : markDead(url);
    // Answer every waiter whose fate is now known, but later: never from inside a network
    // signal (answers may remove posts from lists, which deletes their delegates).
    QList<Waiter> ready;
    for (int i = int(m_waiters.size()) - 1; i >= 0; --i) {
        bool anyAlive = false, allDead = true;
        for (const QString &u : m_waiters.at(i).urls) {
            const State s = state(u);
            anyAlive = anyAlive || s == State::Alive;
            allDead = allDead && s == State::Dead;
        }
        if (anyAlive || allDead)
            ready.append(m_waiters.takeAt(i));
    }
    if (ready.isEmpty())
        return;
    QMetaObject::invokeMethod(this, [this, ready] {
        for (const Waiter &w : ready) {
            if (!w.context)
                continue;
            bool anyAlive = false;
            for (const QString &u : w.urls)
                anyAlive = anyAlive || state(u) == State::Alive;
            w.done(anyAlive);
        }
    }, Qt::QueuedConnection);
}

} // namespace quokka
