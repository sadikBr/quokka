# Networking, transports and JSON parsing. No QML / UI dependencies so it is easy to test.
add_library(quokka_core STATIC
    post.h
    comment.h
    community.h
    html.h html.cpp
    parsers.h parsers.cpp
    transport.h transport.cpp
    redditclient.h redditclient.cpp
)
target_include_directories(quokka_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(quokka_core PUBLIC Qt6::Core Qt6::Network)
