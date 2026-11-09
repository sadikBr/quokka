#pragma once

namespace quokka {
class Backend;
}

// The content source in use, owned by ApiInfo (or a test). Models read it on every request,
// so switching Lemmy servers takes effect on the next load.
namespace services {
quokka::Backend *backend();
void setBackend(quokka::Backend *backend);
} // namespace services
