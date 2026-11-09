#pragma once

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

} // namespace quokka::parsers
