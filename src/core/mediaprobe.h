#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QSettings>
#include <QStringList>
#include <functional>

class QNetworkAccessManager;

namespace quokka {

// Checks whether media links still work, by fetching only their first bytes: a dead link
// answers with an error status or a web page instead of a picture / video. Dead links are
// remembered for a week (QSettings), so posts that only have dead media can be dropped
// before they're shown. One shared instance.
class MediaProbe : public QObject
{
    Q_OBJECT

public:
    enum class State { Unknown, Alive, Dead };

    static MediaProbe *instance();

    // Calls `done(alive)` once some link in `urls` is known to work (true) or all are dead
    // (false). Links that can't be checked directly (empty, video pages needing a lookup) count
    // as working. Called back on this object's thread; `context` going away cancels the call.
    void check(const QStringList &urls, QObject *context, std::function<void(bool alive)> done);

    State state(const QString &url) const;
    void markDead(const QString &url);
    void markAlive(const QString &url);
    // Forget what this session learned about a link, so the next check() asks again.
    void forget(const QString &url) { m_states.remove(url); }

    // True when the reply headers describe media (and not, say, an HTML error page).
    static bool looksLikeMedia(int httpStatus, const QByteArray &contentType);

private:
    explicit MediaProbe(QObject *parent = nullptr);
    void probe(const QString &url);
    void pump();
    void settle(const QString &url, bool alive);

    struct Waiter {
        QStringList urls;
        QPointer<QObject> context;
        std::function<void(bool)> done;
    };

    QNetworkAccessManager *m_nam;
    QSettings m_store;
    QHash<QString, State> m_states;           // this session
    QSet<QString> m_running;
    QStringList m_queue;
    QList<Waiter> m_waiters;
};

} // namespace quokka
