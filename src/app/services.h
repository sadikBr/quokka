#pragma once

namespace quokka {
class RedditClient;
}

// Process-wide services, created in main() before the QML engine loads.
namespace services {
quokka::RedditClient *reddit();
void setReddit(quokka::RedditClient *client);
} // namespace services
