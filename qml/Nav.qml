pragma Singleton
import QtQuick

// App-wide navigation requests. Pages call these; Main.qml listens and pushes pages.
QtObject {
    id: nav

    // The application window, set by Main.qml (used for full-screen toggling).
    property var window: null
    // True when the navigation sidebar is permanently visible (wide desktop windows).
    property bool sidebarDocked: false

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
    signal aboutRequested()
    signal shortcutsRequested()

    function openPost(post) { postRequested(post) }
    function openThread(postId, commentId) { threadRequested(postId, commentId || "") }
    function openSubreddit(name) { subredditRequested(name) }
    function openUser(name) { if (name && name !== "[deleted]") userRequested(name) }
    function openMedia(post, index) { mediaRequested(post, index || 0) }
    function openAbout() { aboutRequested() }
    function openSaved() { savedRequested() }
    function openHistory() { historyRequested() }
    function goToFeed(feed) { rootFeedRequested(feed) }
    function openSearch(query, subreddit) { searchRequested(query || "", subreddit || "") }

    // Hosts we treat as Lemmy servers when routing links (other sites also use /c/ and /u/).
    readonly property var knownLemmyHosts: [
        "lemmy.world", "lemmy.ml", "programming.dev", "sh.itjust.works", "lemm.ee", "beehaw.org",
        "sopuli.xyz", "discuss.tchncs.de", "lemmy.dbzer0.com", "midwest.social", "reddthat.com",
        "lemmy.zip", "startrek.website", "lemmy.ca", "aussie.zone", "infosec.pub", "slrpnk.net",
        "mander.xyz", "lemmy.blahaj.zone", "lemmy.today", "jlai.lu", "feddit.org", "feddit.uk",
        "feddit.nl", "feddit.it", "feddit.dk", "lemmy.sdf.org", "piefed.social"
    ]
    function isLemmyHost(host) {
        return host === Api.instance || knownLemmyHosts.indexOf(host) >= 0
            || host.startsWith("lemmy.") || host.indexOf(".lemmy.") >= 0 || host.startsWith("feddit.")
    }

    // Routes any URL: Lemmy communities, users and posts (and Reddit's, when reading Reddit) open
    // inside the app, images in the viewer, everything else in the system browser.
    function openLink(link) {
        const url = String(link).trim()
        const m = url.match(/^https?:\/\/([^\/?#]+)(\/[^?#]*)?/i)
        if (!m) {
            if (url.startsWith("/"))
                return openLink(Api.webBase + url)
            Qt.openUrlExternally(url)
            return
        }
        const host = m[1].toLowerCase()
        const path = (m[2] || "/").replace(/\/+$/, "")
        const parts = path.split("/").filter(p => p.length > 0)

        if (host === "redd.it" && parts.length === 1 && !Api.isLemmy)
            return openThread(parts[0], "")
        const isImageFile = /\.(jpe?g|png|webp|gif)$/i.test(path)
        if (host === "i.redd.it" || host === "preview.redd.it" || (host === "i.imgur.com" && isImageFile)) {
            imageRequested(url)
            return
        }
        // Lemmy-style links work on every Lemmy server: /c/name, /u/name, /post/id.
        const lemmyHost = Api.isLemmy ? Api.instance : ""
        const lemmyish = Api.isLemmy && isLemmyHost(host)
        if ((parts[0] === "c" || parts[0] === "m") && parts.length >= 2 && lemmyish) {
            const name = parts[1].indexOf("@") >= 0 ? parts[1] : parts[1] + "@" + host
            return openSubreddit(name)
        }
        if (parts[0] === "u" && parts.length >= 2 && lemmyish) {
            const name = parts[1].indexOf("@") >= 0 ? parts[1] : parts[1] + "@" + host
            return openUser(name)
        }
        // Post ids belong to one server, so only open those from the server we read.
        if (parts[0] === "post" && parts.length >= 2 && host === lemmyHost)
            return openThread(parts[1], parts.length >= 3 ? parts[2] : "")

        const isReddit = host === "reddit.com" || host.endsWith(".reddit.com")
        if (isReddit && !Api.isLemmy) {
            // /r/<sub>/comments/<id>/<slug>/<comment>
            const ci = parts.indexOf("comments")
            if (ci >= 0 && parts.length > ci + 1)
                return openThread(parts[ci + 1], parts.length > ci + 3 ? parts[ci + 3] : "")
            if ((parts[0] === "r") && parts.length >= 2 && parts[2] !== "s" && parts[2] !== "wiki")
                return openSubreddit(parts[1])
            if ((parts[0] === "u" || parts[0] === "user") && parts.length >= 2)
                return openUser(parts[1])
        }
        if (/\/pictrs\/image\//.test(path) || isImageFile) {
            imageRequested(url)
            return
        }
        Qt.openUrlExternally(url)
    }
}
