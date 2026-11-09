#include "profiles.h"

#include "backend.h"
#include "services.h"

SubredditInfo::SubredditInfo(QObject *parent)
    : QObject(parent)
{
}

SubredditInfo::~SubredditInfo()
{
    if (m_reply)
        m_reply->abort();
}

bool SubredditInfo::isPseudoFeed(const QString &name)
{
    auto *backend = services::backend();
    return !backend || backend->isPseudoFeed(name);
}

void SubredditInfo::setName(const QString &name)
{
    if (name == m_name)
        return;
    m_name = name;
    emit nameChanged();
    fetch();
}

void SubredditInfo::refresh()
{
    fetch();
}

void SubredditInfo::fetch()
{
    if (m_reply)
        m_reply->abort();
    m_sub = {};
    m_loaded = false;
    m_error.clear();
    emit changed();

    auto *backend = services::backend();
    if (!backend || isPseudoFeed(m_name))
        return;
    auto *reply = backend->community(m_name);
    m_reply = reply;
    connect(reply, &quokka::ApiReply::finished, this, [this, reply](const QByteArray &body) {
        if (reply != m_reply)
            return;
        m_reply = nullptr;
        auto *backend = services::backend();
        if (!backend)
            return;
        QString err;
        m_sub = backend->parseCommunity(body, &err);
        m_loaded = err.isEmpty();
        m_error = err;
        emit changed();
    });
    connect(reply, &quokka::ApiReply::failed, this, [this, reply](const QString &message) {
        if (reply != m_reply)
            return;
        m_reply = nullptr;
        m_error = message;
        emit changed();
    });
}

UserInfo::UserInfo(QObject *parent)
    : QObject(parent)
{
}

UserInfo::~UserInfo()
{
    if (m_reply)
        m_reply->abort();
}

void UserInfo::setName(const QString &name)
{
    if (name == m_name)
        return;
    m_name = name;
    emit nameChanged();
    fetch();
}

void UserInfo::fetch()
{

    if (m_reply)
        m_reply->abort();
    m_user = {};
    m_loaded = false;
    m_error.clear();

    auto *backend = services::backend();
    if (!backend || m_name.isEmpty()) {
        emit changed();
        return;
    }
    auto *reply = backend->user(m_name);
    m_reply = reply;
    emit changed();
    connect(reply, &quokka::ApiReply::finished, this, [this, reply](const QByteArray &body) {
        if (reply != m_reply)
            return;
        m_reply = nullptr;
        auto *backend = services::backend();
        if (!backend)
            return;
        QString err;
        m_user = backend->parseUser(body, &err);
        m_loaded = err.isEmpty();
        m_error = err;
        emit changed();
    });
    connect(reply, &quokka::ApiReply::failed, this, [this, reply](const QString &message) {
        if (reply != m_reply)
            return;
        m_reply = nullptr;
        m_error = message;
        emit changed();
    });
}
