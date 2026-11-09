#pragma once

#include "comment.h"
#include "post.h"

#include <QJsonObject>
#include <QList>
#include <QString>

class QByteArray;

namespace quokka::parsers {

struct PostListing
{
    QList<Post> posts;
    QString after;  // fullname of the last item, empty when there is no next page
    bool ok = false;
    QString error;
};

// Preview images narrower than this are skipped when a larger one exists.
inline constexpr int kPreferredPreviewWidth = 640;

// Parses the "data" object of a t3 thing.
Post parsePost(const QJsonObject &data);

// Parses a Listing whose children are t3 things (subreddit feeds, search results...).
PostListing parsePostListing(const QByteArray &json);

struct CommentsPage
{
    Post post;
    QList<Comment> comments; // flattened in display order, with depth set
    bool ok = false;
    QString error;
};

// Parses /comments/<id>.json: [post listing, comment listing].
CommentsPage parseCommentsPage(const QByteArray &json);

// Parses /api/morechildren.json. Returned rows are in display order, with depth starting at
// baseDepth for the comments that hang directly below the "more" placeholder's parent.
QList<Comment> parseMoreChildren(const QByteArray &json, const QString &linkAuthor, int baseDepth,
                                 QString *error = nullptr);

// Parses a Listing of t1 things (e.g. /user/<name>/comments).
QList<Comment> parseCommentListing(const QByteArray &json, QString *after, QString *error = nullptr);

} // namespace quokka::parsers
