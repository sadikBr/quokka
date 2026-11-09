#pragma once

#include <QString>
#include <QStringList>
#include <QtGlobal>

namespace quokka {

// A row in a comment thread: a real comment, a "load N more" placeholder, or a
// "continue this thread" link (Reddit stops nesting after ~10 levels).
struct Comment
{
    enum class Kind { Comment, More, Continue };

    Kind kind = Kind::Comment;
    QString id;
    QString fullname;     // "t1_xxx"
    QString parentId;     // fullname of the parent (t1_ or t3_)
    int depth = 0;

    QString author;
    QString bodyHtml;     // cleaned for Qt rich text (see html.h)
    QString flair;
    QString distinguished; // "moderator", "admin" or empty
    int score = 0;
    qint64 createdUtc = 0;
    bool isSubmitter = false;
    bool stickied = false;
    bool scoreHidden = false;
    bool edited = false;

    // Context, filled for comments listed outside their thread (e.g. a user's comments).
    QString subreddit;
    QString linkTitle;
    QString permalink;

    // Kind::More
    QStringList moreChildren;
    int moreCount = 0;
};

} // namespace quokka
