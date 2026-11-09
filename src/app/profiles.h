#pragma once

#include "community.h"
#include "redditclient.h"

#include <QObject>
#include <QPointer>
#include <QQmlEngine>

// About a subreddit: `SubredditInfo { name: "pics" }`. Fetches /r/<name>/about on change.
// Stays empty (loaded: false) for pseudo-feeds like popular, all and multireddits.
class SubredditInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QString displayName READ displayName NOTIFY changed)
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(QString description READ description NOTIFY changed)
    Q_PROPERTY(QString sidebarHtml READ sidebarHtml NOTIFY changed)
    Q_PROPERTY(qint64 subscribers READ subscribers NOTIFY changed)
    Q_PROPERTY(qint64 activeUsers READ activeUsers NOTIFY changed)
    Q_PROPERTY(QString iconUrl READ iconUrl NOTIFY changed)
    Q_PROPERTY(QString bannerUrl READ bannerUrl NOTIFY changed)
    Q_PROPERTY(QString color READ color NOTIFY changed)
    Q_PROPERTY(bool nsfw READ nsfw NOTIFY changed)
    Q_PROPERTY(qint64 createdUtc READ createdUtc NOTIFY changed)

public:
    explicit SubredditInfo(QObject *parent = nullptr);
    ~SubredditInfo() override;

    QString name() const { return m_name; }
    void setName(const QString &name);
    bool loaded() const { return m_loaded; }
    QString error() const { return m_error; }
    QString displayName() const { return m_sub.name; }
    QString title() const { return m_sub.title; }
    QString description() const { return m_sub.publicDescription; }
    QString sidebarHtml() const { return m_sub.descriptionHtml; }
    qint64 subscribers() const { return m_sub.subscribers; }
    qint64 activeUsers() const { return m_sub.activeUsers; }
    QString iconUrl() const { return m_sub.iconUrl; }
    QString bannerUrl() const { return m_sub.bannerUrl; }
    QString color() const { return m_sub.color; }
    bool nsfw() const { return m_sub.nsfw; }
    qint64 createdUtc() const { return m_sub.createdUtc; }

    // True for names that are not a single real community.
    static bool isPseudoFeed(const QString &name);

signals:
    void nameChanged();
    void changed();

private:
    void fetch();

    QString m_name;
    quokka::Subreddit m_sub;
    bool m_loaded = false;
    QString m_error;
    QPointer<quokka::ApiReply> m_reply;
};

// About a user: `UserInfo { name: "spez" }`.
class UserInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY changed)
    Q_PROPERTY(bool loading READ loading NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QString iconUrl READ iconUrl NOTIFY changed)
    Q_PROPERTY(QString bannerUrl READ bannerUrl NOTIFY changed)
    Q_PROPERTY(QString description READ description NOTIFY changed)
    Q_PROPERTY(qint64 linkKarma READ linkKarma NOTIFY changed)
    Q_PROPERTY(qint64 commentKarma READ commentKarma NOTIFY changed)
    Q_PROPERTY(qint64 totalKarma READ totalKarma NOTIFY changed)
    Q_PROPERTY(qint64 createdUtc READ createdUtc NOTIFY changed)
    Q_PROPERTY(bool suspended READ suspended NOTIFY changed)

public:
    explicit UserInfo(QObject *parent = nullptr);
    ~UserInfo() override;

    QString name() const { return m_name; }
    void setName(const QString &name);
    bool loaded() const { return m_loaded; }
    bool loading() const { return m_reply != nullptr; }
    QString error() const { return m_error; }
    QString iconUrl() const { return m_user.iconUrl; }
    QString bannerUrl() const { return m_user.bannerUrl; }
    QString description() const { return m_user.description; }
    qint64 linkKarma() const { return m_user.linkKarma; }
    qint64 commentKarma() const { return m_user.commentKarma; }
    qint64 totalKarma() const { return m_user.totalKarma; }
    qint64 createdUtc() const { return m_user.createdUtc; }
    bool suspended() const { return m_user.suspended; }

signals:
    void nameChanged();
    void changed();

private:
    QString m_name;
    quokka::UserAccount m_user;
    bool m_loaded = false;
    QString m_error;
    QPointer<quokka::ApiReply> m_reply;
};
