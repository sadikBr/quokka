#pragma once

#include <map>

#include <QObject>
#include <QQmlEngine>
#include <QUrl>

#include <memory>

class NetworkFactory;
namespace quokka {
class ApiClient;
class Backend;
class Transport;
} // namespace quokka

// The content source as QML sees it (`Api`): which server, how names are shown, and switching
// Lemmy servers. Owns the ApiClient and Backend that the models use through services::.
class ApiInfo : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Api)
    QML_SINGLETON

    Q_PROPERTY(QString backendId READ backendId NOTIFY changed)
    Q_PROPERTY(QString serviceName READ serviceName NOTIFY changed)
    Q_PROPERTY(QString instance READ instance NOTIFY changed)
    Q_PROPERTY(QString webBase READ webBase NOTIFY changed)
    Q_PROPERTY(QString communityPrefix READ communityPrefix NOTIFY changed)
    Q_PROPERTY(bool isLemmy READ isLemmy NOTIFY changed)

public:
    // fixturesDir: serve recorded JSON from there instead of the network (offline UI work).
    ApiInfo(NetworkFactory *network, QString fixturesDir, QObject *parent = nullptr);
    ~ApiInfo() override;

    static ApiInfo *instance_();
    static ApiInfo *create(QQmlEngine *, QJSEngine *);

    void useLemmy(const QString &host);
    void useReddit();

    QString backendId() const;
    QString serviceName() const;
    QString instance() const { return m_instance; }
    QString webBase() const;
    QString communityPrefix() const;
    bool isLemmy() const { return backendId() == QLatin1String("lemmy"); }

    // Switches to another Lemmy server ("lemmy.ml", "https://programming.dev/"...).
    // Returns false if the name is not a plausible host.
    Q_INVOKABLE bool setInstance(const QString &host);
    static QString normalizeHost(const QString &host);

    // "technology@lemmy.world" -> "c/technology" on lemmy.world, "c/technology@lemmy.world"
    // elsewhere. Reddit names get "r/".
    Q_INVOKABLE QString community(const QString &name) const;
    Q_INVOKABLE QString user(const QString &name) const;
    // Drops "@<this server>" from a qualified name.
    Q_INVOKABLE QString shortName(const QString &name) const;
    // Turns a relative permalink into a full web URL on the current server.
    Q_INVOKABLE QString webUrl(const QString &permalink) const;
    Q_INVOKABLE bool isPseudoFeed(const QString &name) const;

signals:
    void changed();

private:
    void install(std::unique_ptr<quokka::ApiClient> client, std::unique_ptr<quokka::Backend> backend,
                 const QString &instance);
    std::unique_ptr<quokka::Transport> makeTransport(const QUrl &base, const QString &suffix);
    quokka::Backend *source(const QString &name);   // see services::backendFor()

    NetworkFactory *m_network;
    QString m_fixtures;
    QObject m_namOwner;
    std::unique_ptr<quokka::ApiClient> m_client;
    std::unique_ptr<quokka::Backend> m_backend;
    QString m_instance;
    struct Extra {
        std::unique_ptr<quokka::ApiClient> client;
        std::unique_ptr<quokka::Backend> backend;
    };
    std::map<QString, Extra> m_sources;
};
