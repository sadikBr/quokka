pragma Singleton
import QtQuick

// App-wide navigation requests. Pages call these; Main.qml listens and pushes pages.
QtObject {
    id: nav

    // The application window, set by Main.qml (used for full-screen toggling).
    property var window: null

    signal postRequested(var post)                       // a post map (see postmap.cpp)
    signal threadRequested(string postId, string commentId)
    signal subredditRequested(string name)
    signal userRequested(string name)
    signal mediaRequested(var post, int index)
    signal imageRequested(string url)
    signal searchRequested(string query, string subreddit)
    signal savedRequested()
    signal historyRequested()
    signal rootFeedRequested(string feed)   // switch the main feed (drawer)

    function openPost(post) { postRequested(post) }
    function openThread(postId, commentId) { threadRequested(postId, commentId || "") }
    function openSubreddit(name) { subredditRequested(name) }
    function openUser(name) { if (name && name !== "[deleted]") userRequested(name) }
    function openMedia(post, index) { mediaRequested(post, index || 0) }
    function openSaved() { savedRequested() }
    function openHistory() { historyRequested() }
    function goToFeed(feed) { rootFeedRequested(feed) }
    function openSearch(query, subreddit) { searchRequested(query || "", subreddit || "") }

    // Routes any URL: Reddit links open inside the app, images in the viewer,
    // everything else in the system browser.
    function openLink(link) {
        const url = String(link).trim()
        const m = url.match(/^https?:\/\/([^\/?#]+)(\/[^?#]*)?/i)
        if (!m) {
            if (url.startsWith("/"))
                return openLink("https://www.reddit.com" + url)
            Qt.openUrlExternally(url)
            return
        }
        const host = m[1].toLowerCase()
        const path = (m[2] || "/").replace(/\/+$/, "")
        const parts = path.split("/").filter(p => p.length > 0)

        if (host === "redd.it" && parts.length === 1)
            return openThread(parts[0], "")
        const isImageFile = /\.(jpe?g|png|webp|gif)$/i.test(path)
        if (host === "i.redd.it" || host === "preview.redd.it" || (host === "i.imgur.com" && isImageFile)) {
            imageRequested(url)
            return
        }
        const isReddit = host === "reddit.com" || host.endsWith(".reddit.com")
        if (isReddit) {
            // /r/<sub>/comments/<id>/<slug>/<comment>
            const ci = parts.indexOf("comments")
            if (ci >= 0 && parts.length > ci + 1)
                return openThread(parts[ci + 1], parts.length > ci + 3 ? parts[ci + 3] : "")
            if ((parts[0] === "r") && parts.length >= 2 && parts[2] !== "s" && parts[2] !== "wiki")
                return openSubreddit(parts[1])
            if ((parts[0] === "u" || parts[0] === "user") && parts.length >= 2)
                return openUser(parts[1])
        }
        Qt.openUrlExternally(url)
    }
}
