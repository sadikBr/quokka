#pragma once

#include "comment.h"
#include "community.h"
#include "post.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QList>

// Lemmy v3 JSON -> Quokka types. Names of remote communities and users always carry their
// home server ("technology@lemmy.world"), so they mean the same thing on any instance.
namespace quokka::lemmy {

qint64 parseTime(const QString &iso);       // "2024-05-01T12:00:00.123456Z" -> seconds
QString hostOf(const QString &url);
QString qualifiedName(const QJsonObject &actor); // name@host from name + actor_id

Post postFromView(const QJsonObject &postView, const QString &webBase);
Subreddit communityFromView(const QJsonObject &communityView);
// How big and alive a community is (monthly active users, members, posts, comments).
double communityScore(const Subreddit &community);
// Biggest and most active first, without duplicates or communities under `minPosts` posts.
void rankCommunities(QList<Subreddit> &communities, qint64 minPosts = 25);
UserAccount userFromView(const QJsonObject &personView);
Comment commentFromView(const QJsonObject &commentView, const QString &webBase);

// Orders comment views into display order (parents before children, siblings in the order
// given), sets depth from each comment's path, and appends a "load more" row under any comment
// whose replies were not all returned.
QList<Comment> buildCommentTree(const QJsonArray &commentViews, const QString &webBase,
                                const QString &postAuthor);

} // namespace quokka::lemmy
