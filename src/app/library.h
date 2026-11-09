#pragma once

#include "post.h"

#include <QAbstractListModel>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QSet>
#include <QSqlDatabase>
#include <QStringList>

// Everything Quokka remembers on the device: subscriptions, saved posts and reading history.
// Stored in SQLite (quokka.db in the app data folder). Exposed to QML as `Library`.
class Library : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QStringList subscriptions READ subscriptions NOTIFY subscriptionsChanged)
    Q_PROPERTY(QString homeFeed READ homeFeed NOTIFY subscriptionsChanged)
    // Bumped on every change, so QML bindings like `Library.savedRevision, Library.isSaved(id)`
    // re-evaluate.
    Q_PROPERTY(int savedRevision READ savedRevision NOTIFY savedChanged)
    Q_PROPERTY(int historyRevision READ historyRevision NOTIFY historyChanged)
    Q_PROPERTY(int savedCount READ savedCount NOTIFY savedChanged)
    Q_PROPERTY(int historyCount READ historyCount NOTIFY historyChanged)

public:
    explicit Library(QObject *parent = nullptr);
    // For tests: use a specific database file (":memory:" works too).
    explicit Library(const QString &databasePath, QObject *parent = nullptr);
    ~Library() override;

    // One Library per process: main() creates it, QML gets the same instance.
    static Library *instance();
    static void setInstance(Library *library);
    static Library *create(QQmlEngine *, QJSEngine *);

    static constexpr int kMaxHistory = 500;
    static constexpr int kMaxHomeSubreddits = 100; // Reddit limits multireddit URLs

    QStringList subscriptions() const { return m_subscriptions; }
    QString homeFeed() const;
    int savedRevision() const { return m_savedRevision; }
    int historyRevision() const { return m_historyRevision; }
    int savedCount() const { return int(m_saved.size()); }
    int historyCount() const { return int(m_visited.size()); }

    Q_INVOKABLE bool isSubscribed(const QString &name) const;
    Q_INVOKABLE void subscribe(const QString &name);
    Q_INVOKABLE void unsubscribe(const QString &name);
    Q_INVOKABLE void toggleSubscription(const QString &name);
    // Accepts names separated by spaces, commas, "+" or new lines, with or without "r/".
    // Returns how many new communities were added.
    Q_INVOKABLE int importSubscriptions(const QString &text);
    Q_INVOKABLE QString exportSubscriptions() const;

    Q_INVOKABLE bool isSaved(const QString &postId) const;
    Q_INVOKABLE void toggleSaved(const QVariantMap &post);
    Q_INVOKABLE void save(const QVariantMap &post);
    Q_INVOKABLE void unsave(const QString &postId);

    Q_INVOKABLE bool isVisited(const QString &postId) const;
    Q_INVOKABLE void markVisited(const QVariantMap &post);
    Q_INVOKABLE void clearHistory();

    Q_INVOKABLE void clearImageCache();

    // Newest first.
    QList<quokka::Post> savedPosts() const;
    QList<quokka::Post> historyPosts() const;

    static QString normalizeName(const QString &name);

signals:
    void subscriptionsChanged();
    void savedChanged();
    void historyChanged();

private:
    void open(const QString &path);
    void loadCaches();
    QList<quokka::Post> loadPosts(const QString &table) const;

    QString m_connection;
    QStringList m_subscriptions;
    QSet<QString> m_subscribedLower;
    QSet<QString> m_saved;
    QSet<QString> m_visited;
    int m_savedRevision = 0;
    int m_historyRevision = 0;
};

// Saved posts or reading history, for the Saved / History pages.
class LocalPostModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged) // "saved" | "history"
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit LocalPostModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString source() const { return m_source; }
    void setSource(const QString &source);
    int count() const { return int(m_posts.size()); }

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE void reload();

    void setLibrary(Library *library);

signals:
    void sourceChanged();
    void countChanged();

private:
    QString m_source = QStringLiteral("saved");
    QList<quokka::Post> m_posts;
    QPointer<Library> m_library;
};
