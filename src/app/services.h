#pragma once

#include <QString>
#include <functional>

namespace quokka {
class Backend;
}

// The content source in use, owned by ApiInfo (or a test). Models read it on every request,
// so switching Lemmy servers takes effect on the next load.
namespace services {
quokka::Backend *backend();
void setBackend(quokka::Backend *backend);

// Other content sources, by name: "redgifs", "eporner", another Lemmy server ("lemmynsfw.com"),
// or several servers as one feed ("servers:lemmy.world,lemmynsfw.com"). Empty or the current
// server: backend(). Created on first use by the factory ApiInfo installs.
quokka::Backend *backendFor(const QString &source);
using SourceFactory = std::function<quokka::Backend *(const QString &source)>;
void setSourceFactory(SourceFactory factory);
} // namespace services
