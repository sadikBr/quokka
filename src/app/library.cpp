#include "library.h"

#include "networkfactory.h"
#include "postlistmodel.h"
#include "postmap.h"

#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

namespace {
Library *g_library = nullptr;

QString defaultPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/quokka.db");
}
} // namespace

Library::Library(QObject *parent)
    : Library(defaultPath(), parent)
{
}

Library::Library(const QString &databasePath, QObject *parent)
    : QObject(parent)
    , m_connection(QStringLiteral("quokka-") + QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    open(databasePath);
    loadCaches();
}

Library::~Library()
{
    if (g_library == this)
        g_library = nullptr;
    {
        QSqlDatabase db = QSqlDatabase::database(m_connection, false);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(m_connection);
}

Library *Library::instance()
{
    return g_library;
}

void Library::setInstance(Library *library)
{
    g_library = library;
}

Library *Library::create(QQmlEngine *, QJSEngine *)
{
    if (!g_library)
        g_library = new Library; // owned by the engine in this fallback case
    else
        QJSEngine::setObjectOwnership(g_library, QJSEngine::CppOwnership);
    return g_library;
}

void Library::open(const QString &path)
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connection);
    db.setDatabaseName(path);
    if (!db.open()) {
        qWarning("Library: cannot open %s: %s", qPrintable(path), qPrintable(db.lastError().text()));
        return;
    }
    QSqlQuery q(db);
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS subscriptions ("
                          " name TEXT PRIMARY KEY COLLATE NOCASE, added_at INTEGER)"));
    q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS saved ("
                          " id TEXT PRIMARY KEY, at INTEGER, data TEXT)"));
    q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS history ("
                          " id TEXT PRIMARY KEY, at INTEGER, data TEXT)"));
}

void Library::loadCaches()
{
    QSqlDatabase db = QSqlDatabase::database(m_connection);
    QSqlQuery q(db);
    m_subscriptions.clear();
    m_subscribedLower.clear();
    q.exec(QStringLiteral("SELECT name FROM subscriptions ORDER BY name COLLATE NOCASE"));
    while (q.next()) {
        m_subscriptions.append(q.value(0).toString());
        m_subscribedLower.insert(q.value(0).toString().toLower());
    }
    m_saved.clear();
    q.exec(QStringLiteral("SELECT id FROM saved"));
    while (q.next())
        m_saved.insert(q.value(0).toString());
    m_visited.clear();
    q.exec(QStringLiteral("SELECT id FROM history"));
    while (q.next())
        m_visited.insert(q.value(0).toString());
}

QString Library::normalizeName(const QString &name)
{
    QString n = name.trimmed();
    // Accepts "r/pics", "c/linux", "!linux@lemmy.ml", "https://lemmy.ml/c/linux", ...
    static const QRegularExpression urlForm(
        QStringLiteral("^https?://([^/]+)/(r|c)/([A-Za-z0-9_]+)/?$"), QRegularExpression::CaseInsensitiveOption);
    const auto m = urlForm.match(n);
    if (m.hasMatch()) {
        const QString host = m.captured(1).toLower();
        n = m.captured(3);
        if (!host.endsWith(QLatin1String("reddit.com")))
            n += QLatin1Char('@') + host;
    }
    static const QRegularExpression prefix(QStringLiteral("^(/?(r|c)/|!)"), QRegularExpression::CaseInsensitiveOption);
    n.remove(prefix);
    while (n.endsWith(QLatin1Char('/')))
        n.chop(1);
    static const QRegularExpression valid(
        QStringLiteral("^[A-Za-z0-9_]{2,50}(@[A-Za-z0-9-]+(\\.[A-Za-z0-9-]+)+)?$"));
    if (!valid.match(n).hasMatch())
        return {};
    const int at = n.indexOf(QLatin1Char('@'));
    return at < 0 ? n : n.left(at) + n.mid(at).toLower();
}

QString Library::homeFeed() const
{
    return m_subscriptions.mid(0, kMaxHomeSubreddits).join(QLatin1Char('+'));
}

bool Library::isSubscribed(const QString &name) const
{
    return m_subscribedLower.contains(name.toLower());
}

void Library::subscribe(const QString &name)
{
    const QString n = normalizeName(name);
    if (n.isEmpty() || isSubscribed(n))
        return;
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.prepare(QStringLiteral("INSERT OR IGNORE INTO subscriptions (name, added_at) VALUES (?, ?)"));
    q.addBindValue(n);
    q.addBindValue(QDateTime::currentSecsSinceEpoch());
    q.exec();
    loadCaches();
    emit subscriptionsChanged();
}

void Library::unsubscribe(const QString &name)
{
    if (!isSubscribed(name))
        return;
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.prepare(QStringLiteral("DELETE FROM subscriptions WHERE name = ? COLLATE NOCASE"));
    q.addBindValue(name);
    q.exec();
    loadCaches();
    emit subscriptionsChanged();
}

void Library::toggleSubscription(const QString &name)
{
    isSubscribed(name) ? unsubscribe(name) : subscribe(name);
}

int Library::importSubscriptions(const QString &text)
{
    static const QRegularExpression sep(QStringLiteral("[\\s,;+]+"));
    QSqlDatabase db = QSqlDatabase::database(m_connection);
    db.transaction();
    int added = 0;
    for (const QString &token : text.split(sep, Qt::SkipEmptyParts)) {
        const QString n = normalizeName(token);
        if (n.isEmpty() || m_subscribedLower.contains(n.toLower()))
            continue;
        QSqlQuery q(db);
        q.prepare(QStringLiteral("INSERT OR IGNORE INTO subscriptions (name, added_at) VALUES (?, ?)"));
        q.addBindValue(n);
        q.addBindValue(QDateTime::currentSecsSinceEpoch());
        if (q.exec()) {
            m_subscribedLower.insert(n.toLower());
            ++added;
        }
    }
    db.commit();
    if (added > 0) {
        loadCaches();
        emit subscriptionsChanged();
    }
    return added;
}

QString Library::exportSubscriptions() const
{
    QStringList out;
    for (const QString &s : m_subscriptions)
        out.append(QStringLiteral("r/") + s);
    return out.join(QLatin1Char('\n'));
}

bool Library::isSaved(const QString &postId) const
{
    return m_saved.contains(postId);
}

void Library::save(const QVariantMap &post)
{
    const QString id = post.value(QStringLiteral("postId")).toString();
    if (id.isEmpty())
        return;
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.prepare(QStringLiteral("INSERT OR REPLACE INTO saved (id, at, data) VALUES (?, ?, ?)"));
    q.addBindValue(id);
    q.addBindValue(QDateTime::currentMSecsSinceEpoch());
    q.addBindValue(QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(post)).toJson(QJsonDocument::Compact)));
    if (q.exec()) {
        m_saved.insert(id);
        ++m_savedRevision;
        emit savedChanged();
    }
}

void Library::unsave(const QString &postId)
{
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.prepare(QStringLiteral("DELETE FROM saved WHERE id = ?"));
    q.addBindValue(postId);
    q.exec();
    m_saved.remove(postId);
    ++m_savedRevision;
    emit savedChanged();
}

void Library::toggleSaved(const QVariantMap &post)
{
    const QString id = post.value(QStringLiteral("postId")).toString();
    isSaved(id) ? unsave(id) : save(post);
}

bool Library::isVisited(const QString &postId) const
{
    return m_visited.contains(postId);
}

void Library::markVisited(const QVariantMap &post)
{
    const QString id = post.value(QStringLiteral("postId")).toString();
    if (id.isEmpty() || post.value(QStringLiteral("title")).toString().isEmpty())
        return;
    QSqlDatabase db = QSqlDatabase::database(m_connection);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("INSERT OR REPLACE INTO history (id, at, data) VALUES (?, ?, ?)"));
    q.addBindValue(id);
    q.addBindValue(QDateTime::currentMSecsSinceEpoch());
    q.addBindValue(QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(post)).toJson(QJsonDocument::Compact)));
    q.exec();
    q.exec(QStringLiteral("DELETE FROM history WHERE id NOT IN "
                          "(SELECT id FROM history ORDER BY at DESC, rowid DESC LIMIT %1)").arg(kMaxHistory));
    m_visited.insert(id);
    if (m_visited.size() > kMaxHistory)
        loadCaches(); // the oldest entry was just trimmed
    ++m_historyRevision;
    emit historyChanged();
}

void Library::clearHistory()
{
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.exec(QStringLiteral("DELETE FROM history"));
    m_visited.clear();
    ++m_historyRevision;
    emit historyChanged();
}

void Library::clearImageCache()
{
    QDir(NetworkFactory::cacheDirectory()).removeRecursively();
}

QList<quokka::Post> Library::loadPosts(const QString &table) const
{
    QList<quokka::Post> out;
    QSqlQuery q(QSqlDatabase::database(m_connection));
    q.exec(QStringLiteral("SELECT data FROM %1 ORDER BY at DESC, rowid DESC").arg(table));
    while (q.next()) {
        const QJsonDocument doc = QJsonDocument::fromJson(q.value(0).toString().toUtf8());
        if (doc.isObject())
            out.append(postFromVariantMap(doc.object().toVariantMap()));
    }
    return out;
}

QList<quokka::Post> Library::savedPosts() const
{
    return loadPosts(QStringLiteral("saved"));
}

QList<quokka::Post> Library::historyPosts() const
{
    return loadPosts(QStringLiteral("history"));
}

// ---- LocalPostModel ----------------------------------------------------------------------

LocalPostModel::LocalPostModel(QObject *parent)
    : QAbstractListModel(parent)
{
    setLibrary(Library::instance());
}

void LocalPostModel::setLibrary(Library *library)
{
    if (m_library)
        disconnect(m_library, nullptr, this, nullptr);
    m_library = library;
    if (m_library) {
        connect(m_library, &Library::savedChanged, this, [this] {
            if (m_source == QLatin1String("saved"))
                reload();
        });
        connect(m_library, &Library::historyChanged, this, [this] {
            if (m_source == QLatin1String("history"))
                reload();
        });
    }
    reload();
}

int LocalPostModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_posts.size());
}

QVariant LocalPostModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_posts.size())
        return {};
    return PostListModel::postRoleData(m_posts.at(index.row()), role);
}

QHash<int, QByteArray> LocalPostModel::roleNames() const
{
    return PostListModel::postRoleNames();
}

void LocalPostModel::setSource(const QString &source)
{
    if (source == m_source)
        return;
    m_source = source;
    emit sourceChanged();
    reload();
}

QVariantMap LocalPostModel::get(int row) const
{
    if (row < 0 || row >= m_posts.size())
        return {};
    return postToVariantMap(m_posts.at(row));
}

void LocalPostModel::reload()
{
    beginResetModel();
    m_posts = !m_library ? QList<quokka::Post>{}
            : m_source == QLatin1String("history") ? m_library->historyPosts()
                                                    : m_library->savedPosts();
    endResetModel();
    emit countChanged();
}
