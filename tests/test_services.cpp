// Stand-in for the services defined in main.cpp: tests run without a Reddit client.
#include "services.h"

namespace {
quokka::RedditClient *g_client = nullptr;
}

quokka::RedditClient *services::reddit() { return g_client; }
void services::setReddit(quokka::RedditClient *client) { g_client = client; }
