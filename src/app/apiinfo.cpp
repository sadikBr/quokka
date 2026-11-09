#include "apiinfo.h"

#include "mediasites.h"

#include "apiclient.h"
#include "backend.h"
#include "networkfactory.h"
#include "services.h"

#include <QRegularExpression>
#include <QSettings>
#include <QUrl>

namespace {
ApiInfo *g_api = nullptr;
}

ApiInfo::ApiInfo(NetworkFactory *network, QString fixturesDir, QObject *parent)
    : QObject(parent)
    , m_network(network)
    , m_fixtures(std::move(fixturesDir))
{
    g_api = this;
    services::setSourceFactory([this](const QString &name) { return source(name); });
}

ApiInfo::~ApiInfo()
{
    if (services::backend() == m_backend.get())
        services::setBackend(nullptr);
    services::setSourceFactory({});
    if (g_api == this)
        g_api = nullptr;
}

ApiInfo *ApiInfo::instance_()
{
    return g_api;
}

ApiInfo *ApiInfo::create(QQmlEngine *, QJSEngine *)
{
    QJSEngine::setObjectOwnership(g_api, QJSEngine::CppOwnership);
    return g_api;
}

std::unique_ptr<quokka::Transport> ApiInfo::makeTransport(const QUrl &base, const QString &suffix)
{
    if (!m_fixtures.isEmpty())
        return std::make_unique<quokka::FixtureTransport>(m_fixtures);
    return std::make_unique<quokka::HttpTransport>(m_network->create(&m_namOwner),
                                                   quokka::ApiClient::defaultUserAgent(), base, suffix);
}

void ApiInfo::install(std::unique_ptr<quokka::ApiClient> client, std::unique_ptr<quokka::Backend> backend,
                      const QString &instance)
{
    // Requests still running on the old client are cancelled when it is destroyed; models drop
    // their reply pointers (QPointer) and reload on changed().
    services::setBackend(backend.get());
    m_sources.clear(); // combined feeds may point at the old server
    m_backend = std::move(backend);
    m_client = std::move(client);
    m_instance = instance;
    emit changed();
}

quokka::Backend *ApiInfo::source(const QString &name)
{
    const QString key = name.trimmed().toLower();
    if (key.isEmpty() || key == m_instance)
        return m_backend.get();
    if (auto it = m_sources.find(key); it != m_sources.end())
        return it->second.backend.get();

    Extra e;
    if (key == QLatin1String("redgifs")) {
        e.client = std::make_unique<quokka::ApiClient>(makeTransport(QUrl(QStringLiteral("https://api.redgifs.com")), {}));
        e.client->setServiceName(QStringLiteral("Redgifs"));
        e.backend = std::make_unique<quokka::RedgifsBackend>(e.client.get());
    } else if (key == QLatin1String("eporner")) {
        e.client = std::make_unique<quokka::ApiClient>(makeTransport(QUrl(QStringLiteral("https://www.eporner.com")), {}));
        e.client->setServiceName(QStringLiteral("Eporner"));
        e.backend = std::make_unique<quokka::EpornerBackend>(e.client.get());
    } else if (key.startsWith(QLatin1String("servers:"))) {
        QStringList hosts;
        QHash<QString, quokka::Backend *> servers;
        for (const QString &part : key.mid(8).split(QLatin1Char(','), Qt::SkipEmptyParts)) {
            const QString h = normalizeHost(part);
            if (h.isEmpty() || hosts.contains(h))
                continue;
            if (quokka::Backend *b = source(h)) {
                hosts.append(h);
                servers.insert(h, b);
            }
        }
        if (hosts.isEmpty())
            return m_backend.get();
        e.backend = std::make_unique<quokka::MultiLemmyBackend>(hosts, servers);
    } else {
        const QString h = normalizeHost(key);
        if (h.isEmpty())
            return m_backend.get();
        if (h == m_instance)
            return m_backend.get();
        e.client = std::make_unique<quokka::ApiClient>(makeTransport(QUrl(QStringLiteral("https://") + h), {}));
        e.client->setServiceName(h);
        e.backend = std::make_unique<quokka::LemmyBackend>(e.client.get(), h);
    }
    quokka::Backend *b = e.backend.get();
    m_sources.emplace(key, std::move(e));
    return b;
}

void ApiInfo::useLemmy(const QString &host)
{
    QString h = normalizeHost(host);
    if (h.isEmpty())
        h = QStringLiteral("lemmy.world");
    auto client = std::make_unique<quokka::ApiClient>(makeTransport(QUrl(QStringLiteral("https://") + h), {}));
    auto backend = std::make_unique<quokka::LemmyBackend>(client.get(), h);
    install(std::move(client), std::move(backend), h);
}

void ApiInfo::useReddit()
{
    auto client = std::make_unique<quokka::ApiClient>(
        makeTransport(QUrl(QStringLiteral("https://www.reddit.com")), QStringLiteral(".json")));
    auto backend = std::make_unique<quokka::RedditBackend>(client.get());
    install(std::move(client), std::move(backend), QStringLiteral("reddit.com"));
}

QString ApiInfo::backendId() const
{
    return m_backend ? m_backend->id() : QString();
}

QString ApiInfo::serviceName() const
{
    return m_backend ? m_backend->displayName() : QString();
}

QString ApiInfo::webBase() const
{
    return m_backend ? m_backend->webBase() : QString();
}

QString ApiInfo::communityPrefix() const
{
    return m_backend ? m_backend->communityPrefix() : QStringLiteral("c/");
}

QString ApiInfo::normalizeHost(const QString &host)
{
    QString h = host.trimmed().toLower();
    h.remove(QRegularExpression(QStringLiteral("^[a-z]+://")));
    h = h.section(QLatin1Char('/'), 0, 0);
    static const QRegularExpression valid(QStringLiteral("^[a-z0-9]([a-z0-9-]*[a-z0-9])?(\\.[a-z0-9]([a-z0-9-]*[a-z0-9])?)+$"));
    return valid.match(h).hasMatch() ? h : QString();
}

bool ApiInfo::setInstance(const QString &host)
{
    const QString h = normalizeHost(host);
    if (h.isEmpty())
        return false;
    if (h != m_instance || !isLemmy())
        useLemmy(h);
    QSettings().setValue(QStringLiteral("lemmy/instance"), h);
    return true;
}

QString ApiInfo::shortName(const QString &name) const
{
    const QString suffix = QLatin1Char('@') + m_instance;
    return name.endsWith(suffix, Qt::CaseInsensitive) ? name.left(name.size() - suffix.size()) : name;
}

QString ApiInfo::community(const QString &name) const
{
    return communityPrefix() + shortName(name);
}

QString ApiInfo::user(const QString &name) const
{
    return QStringLiteral("u/") + shortName(name);
}

QString ApiInfo::webUrl(const QString &permalink) const
{
    if (permalink.startsWith(QLatin1String("http")))
        return permalink;
    return webBase() + permalink;
}

bool ApiInfo::isPseudoFeed(const QString &name) const
{
    return !m_backend || m_backend->isPseudoFeed(name);
}
