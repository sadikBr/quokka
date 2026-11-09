#pragma once

#include <QList>
#include <QString>
#include <QtGlobal>

namespace quokka {

// A subreddit ("t5").
struct Subreddit
{
    qint64 id = 0;             // server-side id (Lemmy), used to follow
    QString name;              // display_name, e.g. "pics"
    QString title;
    QString publicDescription; // one-liner
    QString descriptionHtml;   // sidebar, cleaned for Qt rich text
    qint64 subscribers = 0;
    qint64 activeUsers = -1;   // -1 when unknown
    QString iconUrl;
    QString bannerUrl;
    QString color;             // "#rrggbb" or empty
    bool nsfw = false;
    qint64 createdUtc = 0;
};

// A user account ("t2").
struct UserAccount
{
    QString name;
    QString iconUrl;
    QString bannerUrl;
    QString description;
    qint64 linkKarma = 0;
    qint64 commentKarma = 0;
    qint64 totalKarma = 0;
    qint64 createdUtc = 0;
    bool suspended = false;
    bool hasKarma = true;   // Reddit has karma; Lemmy has post/comment counts instead
    qint64 postCount = -1;
    qint64 commentCount = -1;
};

// The signed-in account, as far as Quokka cares.
struct AccountInfo
{
    bool ok = false;
    QString error;
    QString name;      // qualified, e.g. "alice@lemmy.world"
    QString avatarUrl;
    bool showNsfw = false;
    struct Follow
    {
        QString name;  // qualified community name
        qint64 id = 0;
    };
    QList<Follow> follows;
};

} // namespace quokka
