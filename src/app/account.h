#pragma once

#include "community.h"

#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QStringList>

class ApiInfo;
namespace quokka {
class ApiClient;
class ApiReply;
} // namespace quokka

// Optional sign-in to the current Lemmy server (QML: `Account`). Signed in, the server applies
// the account's own settings (e.g. NSFW in search), Home shows the account's subscriptions and
// Join follows communities on the server. One session token is kept per server; the password is
// never stored.
class Account : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool supported READ supported NOTIFY changed)
    Q_PROPERTY(bool signedIn READ signedIn NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(bool needsTotp READ needsTotp NOTIFY changed)
    Q_PROPERTY(QString name READ name NOTIFY changed)
    Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY changed)
    Q_PROPERTY(bool showNsfw READ showNsfw NOTIFY changed)
    Q_PROPERTY(QStringList follows READ follows NOTIFY followsChanged)
    Q_PROPERTY(QString signupUrl READ signupUrl NOTIFY changed)

public:
    explicit Account(ApiInfo *api, QObject *parent = nullptr);
    ~Account() override;

    static Account *create(QQmlEngine *, QJSEngine *);

    bool supported() const;
    bool signedIn() const { return !m_token.isEmpty(); }
    bool busy() const { return m_busy > 0; }
    QString error() const { return m_error; }
    bool needsTotp() const { return m_needsTotp; }
    QString name() const { return m_info.name; }
    QString avatarUrl() const { return m_info.avatarUrl; }
    bool showNsfw() const { return m_info.showNsfw; }
    QStringList follows() const;
    QString signupUrl() const;

    Q_INVOKABLE void signIn(const QString &user, const QString &password, const QString &totp = {});
    Q_INVOKABLE void signOut();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void clearError();
    // Turns the account's own NSFW preference on or off (affects search on the server).
    Q_INVOKABLE void setShowNsfw(bool show);

    Q_INVOKABLE bool isFollowing(const QString &community) const;
    // communityId may be 0; it is then looked up first.
    Q_INVOKABLE void toggleFollow(const QString &community, qint64 communityId = 0);

signals:
    void changed();
    void followsChanged();
    void signedInChanged(); // also emitted through changed(); handy for one-off reactions

private:
    void backendChanged();
    void applyToken();
    void setToken(const QString &token);
    void setError(const QString &error);
    void beginBusy() { ++m_busy; emit changed(); }
    void endBusy() { m_busy = qMax(0, m_busy - 1); emit changed(); }
    void follow(const QString &community, qint64 id, bool follow);
    QString settingsKey() const;

    QPointer<ApiInfo> m_api;
    QPointer<quokka::ApiClient> m_client;
    QMetaObject::Connection m_expiredConnection;
    QString m_instance;
    QString m_token;
    quokka::AccountInfo m_info;
    QString m_error;
    bool m_needsTotp = false;
    int m_busy = 0;
};
