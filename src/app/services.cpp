#include "services.h"

namespace {
quokka::Backend *g_backend = nullptr;
}

quokka::Backend *services::backend()
{
    return g_backend;
}

void services::setBackend(quokka::Backend *backend)
{
    g_backend = backend;
}
