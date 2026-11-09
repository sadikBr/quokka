#pragma once

#include "post.h"

#include <QVariantMap>

// Converts a Post to the map QML works with. Keys match PostListModel's role names, so a
// row from any post model can be handed to the post page as-is.
QVariantMap postToVariantMap(const quokka::Post &post);

// The reverse of postToVariantMap(), used for posts stored in the local library.
quokka::Post postFromVariantMap(const QVariantMap &map);

// The model "kind" value for a post (PostListModel::Kind).
int postKindValue(quokka::Post::Kind kind);
