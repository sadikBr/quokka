#include "services.h"

namespace {
quokka::Backend *g_backend = nullptr;
services::SourceFactory g_factory;
}

quokka::Backend *services::backend()
{
    return g_backend;
}

void services::setBackend(quokka::Backend *backend)
{
    g_backend = backend;
}

quokka::Backend *services::backendFor(const QString &source)
{
    if (source.isEmpty() || !g_factory)
        return g_backend;
    quokka::Backend *b = g_factory(source);
    return b ? b : g_backend;
}

void services::setSourceFactory(SourceFactory factory)
{
    g_factory = std::move(factory);
}
