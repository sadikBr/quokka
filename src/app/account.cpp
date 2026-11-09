#include "account.h"

#include "apiclient.h"
#include "apiinfo.h"
#include "backend.h"
#include "services.h"

#include <QSettings>

#include <algorithm>

namespace {
Account *g_account = nullptr;
}

Account::Account(ApiInfo *api, QObject *parent)
    : QObject(parent)
    , m_api(api)
{
    g_account = this;
    connect(api, &ApiInfo::changed, this, &Account::backendChanged);
    backendChanged();
}

Account::~Account()
{
    if (g_account == this)
        g_account = nullptr;
}

Account *Account::create(QQmlEngine *, QJSEngine *)
{
    QJSEngine::setObjectOwnership(g_account, QJSEngine::CppOwnership);
    return g_account;
}

bool Account::supported() const
{
    auto *backend = services::backend();
    return backend && backend->supportsAccounts();
}

QString Account::settingsKey() const
{
    return QStringLiteral("lemmy/accounts/%1/token").arg(m_instance);
}

QString Account::signupUrl() const
{
    return m_api ? m_api->webBase() + QStringLiteral("/signup") : QString();
}

QStringList Account::follows() const
{
    QStringList out;
    for (const auto &f : m_info.follows)
        out.append(f.name);
    return out;
}

void Account::backendChanged()
{
    auto *backend = services::backend();
    m_client = backend ? backend->client() : nullptr;
    m_instance = m_api ? m_api->instance() : QString();
    m_info = {};
    m_error.clear();
    m_needsTotp = false;
    m_token = supported() ? QSettings().value(settingsKey()).toString() : QString();
    disconnect(m_expiredConnection);
    if (m_client) {
        m_expiredConnection = connect(m_client, &quokka::ApiClient::authenticationExpired, this, [this] {
            setToken({});
            setError(tr("Your session on %1 ended. Please sign in again.").arg(m_instance));
        });
    }
    applyToken();
    emit changed();
    emit followsChanged();
    if (signedIn())
        refresh();
}

void Account::applyToken()
{
    if (m_client)
        m_client->setAuthToken(m_token);
}

void Account::setToken(const QString &token)
{
    const bool was = signedIn();
    m_token = token;
    if (token.isEmpty()) {
        QSettings().remove(settingsKey());
        m_info = {};
        emit followsChanged();
    } else {
        QSettings().setValue(settingsKey(), token);
    }
    applyToken();
    emit changed();
    if (was != signedIn())
        emit signedInChanged();
}

void Account::setError(const QString &error)
{
    m_error = error;
    emit changed();
}

void Account::clearError()
{
    m_needsTotp = false;
    setError({});
}

void Account::signIn(const QString &user, const QString &password, const QString &totp)
{
    auto *backend = services::backend();
    if (!backend || !backend->supportsAccounts())
        return;
    if (user.trimmed().isEmpty() || password.isEmpty()) {
        setError(tr("Enter your username (or email) and password."));
        return;
    }
    // Sign in fresh: an old token would be sent along otherwise.
    if (m_client)
        m_client->setAuthToken({});
    m_error.clear();
    beginBusy();
    quokka::ApiReply *reply = backend->login(user, password, totp);
    connect(reply, &quokka::ApiReply::finished, this, [this](const QByteArray &body) {
        endBusy();
        auto *backend = services::backend();
        QString err;
        const QString token = backend ? backend->parseLogin(body, &err) : QString();
        if (token.isEmpty()) {
            applyToken();
            setError(err);
            return;
        }
        m_needsTotp = false;
        setToken(token);
        refresh();
    });
    connect(reply, &quokka::ApiReply::failed, this,
            [this](const QString &message, int, const QString &code) {
        endBusy();
        applyToken();
        if (code == QLatin1String("missing_totp_token") || code == QLatin1String("incorrect_totp_token"))
            m_needsTotp = true;
        setError(message);
    });
}

void Account::signOut()
{
    auto *backend = services::backend();
    if (backend && signedIn()) {
        // Ends the session on the server too; we forget the token either way.
        if (quokka::ApiReply *reply = backend->logout())
            connect(reply, &quokka::ApiReply::failed, this, [] {});
    }
    m_error.clear();
    setToken({});
}

void Account::refresh()
{
    auto *backend = services::backend();
    if (!backend || !signedIn())
        return;
    beginBusy();
    quokka::ApiReply *reply = backend->account();
    connect(reply, &quokka::ApiReply::finished, this, [this](const QByteArray &body) {
        endBusy();
        auto *backend = services::backend();
        if (!backend)
            return;
        const quokka::AccountInfo info = backend->parseAccount(body);
        if (!info.ok) {
            // The server no longer knows this session.
            setToken({});
            setError(tr("You were signed out. Please sign in again."));
            return;
        }
        m_info = info;
        emit changed();
        emit followsChanged();
    });
    connect(reply, &quokka::ApiReply::failed, this, [this](const QString &message) {
        endBusy();
        setError(message);
    });
}

void Account::setShowNsfw(bool show)
{
    auto *backend = services::backend();
    if (!backend || !signedIn() || show == m_info.showNsfw)
        return;
    beginBusy();
    quokka::ApiReply *reply = backend->setAccountNsfw(show);
    connect(reply, &quokka::ApiReply::finished, this, [this, show](const QByteArray &) {
        endBusy();
        m_info.showNsfw = show;
        emit changed();
    });
    connect(reply, &quokka::ApiReply::failed, this, [this](const QString &message) {
        endBusy();
        setError(message);
    });
}

bool Account::isFollowing(const QString &community) const
{
    for (const auto &f : m_info.follows)
        if (f.name.compare(community, Qt::CaseInsensitive) == 0)
            return true;
    return false;
}

void Account::toggleFollow(const QString &community, qint64 communityId)
{
    auto *backend = services::backend();
    if (!backend || !signedIn() || community.isEmpty())
        return;
    const bool follow = !isFollowing(community);
    if (communityId > 0) {
        this->follow(community, communityId, follow);
        return;
    }
    for (const auto &f : m_info.follows)
        if (f.name.compare(community, Qt::CaseInsensitive) == 0)
            communityId = f.id;
    if (communityId > 0) {
        this->follow(community, communityId, follow);
        return;
    }
    // Look the community up to get its id on this server.
    beginBusy();
    quokka::ApiReply *reply = backend->community(community);
    connect(reply, &quokka::ApiReply::finished, this, [this, community, follow](const QByteArray &body) {
        endBusy();
        auto *backend = services::backend();
        QString err;
        const quokka::Subreddit s = backend ? backend->parseCommunity(body, &err) : quokka::Subreddit{};
        if (s.id <= 0) {
            setError(err.isEmpty() ? tr("Couldn't find %1.").arg(community) : err);
            return;
        }
        this->follow(s.name, s.id, follow);
    });
    connect(reply, &quokka::ApiReply::failed, this, [this](const QString &message) {
        endBusy();
        setError(message);
    });
}

void Account::follow(const QString &community, qint64 id, bool follow)
{
    auto *backend = services::backend();
    if (!backend)
        return;
    // Update right away; undo if the server says no.
    auto apply = [this, community, id](bool on) {
        if (on && !isFollowing(community))
            m_info.follows.append({community, id});
        if (!on)
            m_info.follows.removeIf([&](const auto &f) { return f.name.compare(community, Qt::CaseInsensitive) == 0; });
        std::sort(m_info.follows.begin(), m_info.follows.end(), [](const auto &a, const auto &b) {
            return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
        });
        emit followsChanged();
    };
    apply(follow);
    quokka::ApiReply *reply = backend->follow(id, follow);
    connect(reply, &quokka::ApiReply::failed, this, [this, apply, follow](const QString &message) {
        apply(!follow);
        setError(message);
    });
}
