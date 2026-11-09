# Networking, transports, JSON parsing and Markdown rendering. No QML, so it is easy to test.
add_library(quokka_core STATIC
    post.h
    comment.h
    community.h
    html.h html.cpp
    markdown.h markdown.cpp
    medialinks.h medialinks.cpp
    mediaprobe.h mediaprobe.cpp
    parsers.h parsers.cpp
    transport.h transport.cpp
    apiclient.h apiclient.cpp
    backend.h redditbackend.cpp lemmybackend.cpp
    lemmyparsers.h lemmyparsers.cpp
    mediasites.h mediasites.cpp
)
target_include_directories(quokka_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(quokka_core PUBLIC Qt6::Core Qt6::Gui Qt6::Network)
