#include "profiles.h"

#include "parsers.h"
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
    const QString n = name.toLower();
    return n.isEmpty() || n == QLatin1String("popular") || n == QLatin1String("all")
        || n == QLatin1String("home") || n.contains(QLatin1Char('+'));
}

void SubredditInfo::setName(const QString &name)
{
    if (name == m_name)
        return;
    m_name = name;
    emit nameChanged();
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

    auto *client = services::reddit();
    if (!client || isPseudoFeed(m_name))
        return;
    auto *reply = client->get(QStringLiteral("/r/%1/about").arg(m_name));
    m_reply = reply;
    connect(reply, &quokka::ApiReply::finished, this, [this, reply](const QByteArray &body) {
        if (reply != m_reply)
            return;
        m_reply = nullptr;
        QString err;
        m_sub = quokka::parsers::parseSubredditAbout(body, &err);
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

    if (m_reply)
        m_reply->abort();
    m_user = {};
    m_loaded = false;
    m_error.clear();

    auto *client = services::reddit();
    if (!client || m_name.isEmpty()) {
        emit changed();
        return;
    }
    auto *reply = client->get(QStringLiteral("/user/%1/about").arg(m_name));
    m_reply = reply;
    emit changed();
    connect(reply, &quokka::ApiReply::finished, this, [this, reply](const QByteArray &body) {
        if (reply != m_reply)
            return;
        m_reply = nullptr;
        QString err;
        m_user = quokka::parsers::parseUserAbout(body, &err);
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
