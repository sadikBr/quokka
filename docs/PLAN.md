# Reddit Client: Project Plan

A read-only Reddit client for **desktop (Linux, Windows, macOS)** and **Android**, built with
**Qt 6 / QML** and Reddit's public JSON endpoints (add `.json` to a Reddit URL).

---

## 1. Goals and non-goals

### Goals (v1)
- Browse the front page (`/r/popular`, `/r/all`) and any subreddit, sorted by hot / new / top / rising / controversial.
- Open a post and read its comment tree, including collapsing and "load more comments".
- Show images, galleries, videos, link previews and self-text (Markdown).
- Search posts and subreddits.
- View user profiles (about, posts, comments).
- **Local** subscriptions, saved posts and history, stored on the device (no account needed).
- Infinite scrolling, pull-to-refresh, dark and light themes, responsive layout (phone, tablet, desktop).

### Non-goals (v1)
- Logging in, voting, commenting, posting, messaging. The unauthenticated `.json` API is read-only.
  The network layer will be written so OAuth can be added later (see §9).
- iOS. Qt supports it, so it is possible later, but it is out of scope for now.

---

## 2. Technology choices

| Concern | Choice | Why |
|---|---|---|
| Framework | **Qt 6.8 LTS** (or newer 6.x) | Long-term support, mature Android support, current QML tooling |
| UI | **Qt Quick (QML) + Qt Quick Controls 2** | Touch-friendly, GPU-accelerated, one UI for phone and desktop. Qt Widgets work poorly on Android. |
| Style | Controls **Material** style, with custom theme tokens | Looks right on Android and acceptable on desktop. One code path. |
| Core logic | **C++17/20** | Networking, JSON parsing, models and caching stay testable and fast |
| Build | **CMake** with `qt_add_executable` / `qt_add_qml_module` | Standard for Qt 6. Required for Android builds via `androiddeployqt`. |
| Networking | `QNetworkAccessManager` + `QNetworkDiskCache` | Built in, async, HTTP cache for free |
| JSON | `QJsonDocument` | Built in. Responses are small enough. |
| Local storage | **SQLite via Qt SQL** (subscriptions, saved, history), `QSettings` (preferences) | Works the same on all platforms |
| Images | QML `Image` with a custom `QQuickAsyncImageProvider` or the disk-cached NAM factory | Async loading, caching, downscaling for memory on phones |
| Video | **Qt Multimedia** `MediaPlayer` + `VideoOutput` | Plays HLS on Android natively and on desktop through the FFmpeg backend |
| Markdown | QML `Text { textFormat: Text.MarkdownText }`, with a small preprocessor for Reddit-specific syntax | No extra dependency |
| Tests | **Qt Test** (C++) + recorded JSON fixtures; optional `qmltestrunner` for UI | Runs offline, deterministic |
| CI | **GitHub Actions** + `jurplel/install-qt-action` | Builds desktop on 3 OSes plus an Android APK |

Qt modules needed: `Core`, `Gui`, `Network`, `Qml`, `Quick`, `QuickControls2`, `Sql`, `Multimedia`, `Svg`.

---

## 3. The Reddit JSON API: what we use and its constraints

### Endpoints (all `GET`, base `https://www.reddit.com`)

| Feature | URL |
|---|---|
| Subreddit listing | `/r/{sub}/{hot\|new\|top\|rising\|controversial}.json?limit=25&after={fullname}&t={hour\|day\|week\|month\|year\|all}` |
| Front page / popular / all | `/.json`, `/r/popular.json`, `/r/all.json` |
| Multiple subreddits | `/r/{sub1}+{sub2}+{sub3}/hot.json` (this is how local subscriptions become a "home" feed) |
| Post + comments | `/comments/{postId}.json?sort={confidence\|top\|new\|controversial\|old\|qa}&limit=200&depth=10` |
| Single comment thread ("continue this thread") | `/comments/{postId}/_/{commentId}.json` |
| More comments | `/api/morechildren.json?api_type=json&link_id=t3_{postId}&children={id1,id2,...}&sort=...` |
| Subreddit info | `/r/{sub}/about.json`, rules: `/r/{sub}/about/rules.json` |
| User | `/user/{name}/about.json`, `/user/{name}/submitted.json`, `/user/{name}/comments.json` |
| Search posts | `/search.json?q=...&sort=relevance&t=all` or `/r/{sub}/search.json?q=...&restrict_sr=1` |
| Search subreddits | `/subreddits/search.json?q=...`, autocomplete: `/api/subreddit_autocomplete_v2.json?query=...` |
| Popular subreddits | `/subreddits/popular.json` |

### Rules we must follow
1. **Always send a unique, descriptive `User-Agent`**, for example
   `desktop:com.example.redditclient:v0.1.0 (by /u/<your_username>)`. Generic agents such as the Qt
   default or `curl` get throttled or blocked.
2. **Always add `raw_json=1`.** Without it, Reddit HTML-escapes strings (`&amp;` in image URLs, titles and so on).
3. **Rate limits.** Unauthenticated clients get a low limit (roughly 10 requests per minute per IP at last
   documentation; Reddit can change it). Read `X-Ratelimit-Remaining` / `X-Ratelimit-Reset` when present,
   queue requests, back off on HTTP 429, and cache aggressively. Avoid request bursts, for example by not
   prefetching every visible post's comments.
4. **Pagination** uses `after` and `before` with "fullnames" (`t3_abc123`), not page numbers.
5. **Reddit may block unauthenticated `.json` access** from some networks or regions (HTTP 403). This is
   the main project risk. Mitigation: keep the transport behind an interface so that "application-only
   OAuth" (`https://oauth.reddit.com`, no user login, higher limits) can be dropped in. See §9.

### Data model ("Thing" kinds)
- `Listing`: `{ kind: "Listing", data: { after, before, children: [Thing...] } }`
- `t1` comment, `t2` account, `t3` link/post, `t5` subreddit, `more` placeholder for unloaded comments.
- A post's `.json` returns an **array of two Listings**: `[0]` = the post, `[1]` = the comment tree.
  Replies are nested in `data.replies` (an empty string `""` when there are none: handle that).

### Media detection for a `t3` post (priority order)
1. `is_gallery` + `gallery_data.items` + `media_metadata[id].s.u` → gallery
2. `is_video` + `media.reddit_video.hls_url` (with audio) / `fallback_url` (MP4 **without audio**) → video
3. `post_hint == "image"` or URL ends in `.jpg/.png/.gif` → image (`preview.images[0].resolutions[]` for sized thumbnails)
4. `preview.reddit_video_preview` → GIF-like video
5. `is_self` → self-text (`selftext`, Markdown)
6. Otherwise → external link card (`thumbnail`, `domain`, `url`)

Also: `over_18` (blur NSFW), `spoiler`, `stickied`, `locked`, `link_flair_text`, `crosspost_parent_list`.

---

## 4. Architecture

```
┌──────────────────────────── QML UI (Qt Quick Controls) ────────────────────────────┐
│ Main.qml  ─ AppShell (Drawer on phone / side panel on desktop) + StackView         │
│ Pages: FeedPage, PostPage, SubredditPage, UserPage, SearchPage, SettingsPage,      │
│        MediaViewer                                                                 │
│ Components: PostCard, CommentDelegate, MarkdownText, GalleryView, VideoPlayer,     │
│             SortSelector, FlairChip, LoadingFooter, ErrorBanner                    │
└───────────────▲───────────────────────────────────────────────▲────────────────────┘
                │ properties / signals / Q_INVOKABLE             │ QML_ELEMENT / singletons
┌───────────────┴──────────────── C++ core (src/) ──────────────┴────────────────────┐
│ models/   PostListModel (QAbstractListModel, paging)                               │
│           CommentTreeModel (flattened tree: depth, collapsed, "more" rows)         │
│           SubredditListModel, SearchModel                                          │
│ api/      RedditClient  ─ builds URLs, sets headers, request queue + rate limiter  │
│           Parsers       ─ QJsonObject → Post / Comment / Subreddit / User structs  │
│           ITransport    ─ PublicJsonTransport (now), OAuthTransport (later)        │
│ storage/  Database (SQLite): subscriptions, saved_posts, history, hidden           │
│           Settings (QSettings wrapper exposed to QML)                              │
│ util/     MarkdownPreprocessor, MediaResolver, TimeFormatter, ImageCache           │
└────────────────────────────────────────────────────────────────────────────────────┘
```

Key design points:
- **Models live in C++** and the UI only binds to them. This keeps QML simple and lets us unit-test
  parsing, paging and tree logic without a UI.
- **Comment tree is flattened** into a list (`depth` role plus a visible/collapsed state) so a
  `ListView` can virtualize it. Nested `Repeater`s would be slow with large threads on phones.
- **One request queue** in `RedditClient` enforces the rate limit, removes duplicate requests and cancels
  requests for pages the user has left.
- **Async everywhere**: results come back via signals or `QFuture`. Nothing blocks the UI thread.
- **Responsive layout**: one set of pages. Below roughly 700 px wide, use a single column + Drawer. On
  wide windows, use a side panel with subscriptions + feed, and optionally a master-detail post view.

---

## 5. Repository layout

```
Reddit-Client/
├── CMakeLists.txt
├── CMakePresets.json            # desktop-debug, desktop-release, android-arm64 presets
├── src/
│   ├── main.cpp
│   ├── api/        (RedditClient, transports, parsers, rate limiter)
│   ├── models/     (PostListModel, CommentTreeModel, ...)
│   ├── storage/    (Database, Settings)
│   └── util/
├── qml/
│   ├── Main.qml
│   ├── pages/
│   ├── components/
│   └── theme/      (Theme.qml singleton: colors, spacing, typography)
├── resources/      (icons, app icon, fonts)
├── android/        (AndroidManifest.xml, res/ icons, build.gradle overrides)
├── tests/
│   ├── fixtures/   (recorded .json responses)
│   └── tst_parsers.cpp, tst_commenttree.cpp, tst_ratelimiter.cpp
├── docs/
│   └── PLAN.md
└── .github/workflows/  (build-desktop.yml, build-android.yml)
```

---

## 6. Android specifics (common pitfalls)

- **OpenSSL**: Qt 6 on Android needs OpenSSL libraries bundled for HTTPS. Use KDAB's
  `android_openssl` (`include(<path>/android_openssl/android_openssl.cmake)` +
  `add_android_openssl_libraries(app)`). Without this, every request fails.
- **Toolchain**: Android SDK, NDK (the version Qt's docs list for our Qt release), JDK 17, and Qt for
  Android `arm64-v8a` (add `x86_64` for emulators). Set `QT_ANDROID_ABIS`.
- **Manifest**: `INTERNET` permission; `minSdk` per Qt release (roughly 28 for Qt 6.8); target the current
  Play Store requirement.
- **Back button**: handle `Keys.onBackPressed` / `Qt.Key_Back` → `StackView.pop()`, and close the app only on the root page.
- **Safe areas / insets**: respect status bar and gesture bar (`SafeArea` in Qt 6.9+, or JNI insets on older versions).
- **Open links externally**: `Qt.openUrlExternally()` for non-Reddit links.
- **Memory**: always set `sourceSize` on images, and use `resolutions[]` thumbnails instead of full-size `source` in the feed.
- **Signing**: debug keystore for development. Release keystore stored as a CI secret.

## 7. Desktop specifics
- Keyboard navigation: `j/k` next/previous post, `Enter` open, `Esc`/`Backspace` back, `/` search, `r` refresh.
- Mouse-wheel scrolling speed tuning for `ListView` (default flick physics feel touch-oriented).
- Packaging: `windeployqt` (Windows, then an installer via Inno Setup or Qt IFW), `macdeployqt` (.dmg),
  AppImage via `linuxdeploy` + Qt plugin (Linux). Flatpak can come later.

---

## 8. Milestones

Each milestone ends with something runnable on **both** desktop and Android.

### M0: Project skeleton (≈1–2 days)
- CMake project, `qt_add_qml_module`, `Main.qml` with StackView and an empty Feed page.
- CMakePresets for desktop and Android. Android build installs and launches on an emulator or device.
- android_openssl integrated. A "hello request" fetches `/r/popular.json` and shows the raw title count.
- GitHub Actions: Linux desktop build + Android APK artifact.

### M1: Feed browsing (≈1 week)
- `RedditClient` with User-Agent, `raw_json=1`, request queue, rate limiter, disk cache.
- Parsers for `Listing` and `t3`. Unit tests against fixtures.
- `PostListModel` with infinite scroll (`after` paging), pull-to-refresh, sort + time selector.
- `PostCard`: title, subreddit, author, score, comment count, age, flair, thumbnail, NSFW/spoiler blur.
- Subreddit navigation by typing a name. Error and empty states.

### M2: Post and comments (≈1 week)
- `PostPage`: full post header, self-text with Markdown, link card.
- `CommentTreeModel`: flattening, collapse/expand, depth indent colors, OP/mod/admin badges.
- "Load more comments" (`morechildren`) and "Continue this thread".
- Comment sort selector.

### M3: Media (≈1 week)
- `MediaResolver` (§3 priority list).
- Full-screen `MediaViewer`: pinch-zoom and pan on images, swipe through galleries.
- Video playback via HLS, with an MP4 `fallback_url` fallback (no audio). GIF/`reddit_video_preview`.
- Image cache tuning and `sourceSize` everywhere.

### M4: Discovery and profiles (≈1 week)
- Search page: posts + subreddits tabs, subreddit autocomplete.
- Subreddit page: banner, icon, description, subscriber count, rules.
- User page: karma, cake day, posts and comments tabs.

### M5: Local library and settings (≈4–5 days)
- SQLite: subscriptions (home feed = `/r/a+b+c`), saved posts, read history (dim visited posts), hidden posts.
- Import subscriptions from a pasted list of subreddit names.
- Settings: theme (system/light/dark), default sort, NSFW toggle (off by default), autoplay video, card vs compact layout, font scale.

### M6: Polish and release (≈1 week)
- Responsive two-pane desktop layout, keyboard shortcuts.
- Accessibility (`Accessible.name`, focus order, font scaling).
- App icon, splash, About/licenses screen (Qt LGPL notice).
- Release builds: signed APK/AAB, Windows installer, macOS dmg, Linux AppImage, all built in CI on tag push.

### Later / stretch
- Application-only OAuth transport (fallback if `.json` gets blocked, higher rate limits).
- Full user OAuth: login, vote, comment, real subscriptions.
- Offline reading (cache selected posts and comments), multireddits, filters (keywords/domains), widgets, iOS.

---

## 9. Risks and mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| Reddit restricts or blocks unauthenticated `.json` (403/429) | App stops working | `ITransport` abstraction; ready to switch to app-only OAuth with a registered "installed app" client ID; descriptive User-Agent; caching |
| Low rate limit | Slow browsing, errors | Request queue + backoff, HTTP cache, no aggressive prefetch, larger `limit` per page |
| Reddit Markdown differs from CommonMark (spoilers `>!x!<`, superscript `^`, `/r/` and `/u/` autolinks, tables) | Wrong rendering | `MarkdownPreprocessor` converts these before `Text.MarkdownText`; fall back to the sanitized `selftext_html` if needed |
| v.redd.it video audio is in a separate stream | Silent video | Prefer HLS (`hls_url`), which muxes audio; MP4 only as fallback |
| Qt Multimedia backend differences | Video bugs on some platforms | Test on all targets early (M3); the FFmpeg backend is the default on desktop in Qt 6.5+ |
| Android TLS without OpenSSL | No network at all | android_openssl in M0, checked by CI smoke test |
| Trademark ("Reddit" in the app name / icon) | Store rejection | Pick a neutral app name (we use "Quokka") and follow Reddit's brand guidelines if publishing |
| Licensing (Qt LGPLv3) | Compliance | Link Qt dynamically, ship a license notice, publish our source or allow relinking |

---

## 10. Testing strategy
- **Unit tests (Qt Test)**: parsers (every post type from fixtures: self, image, gallery, video, link,
  crosspost, NSFW, deleted/removed), comment tree flatten/collapse/insert-more, URL builder, rate limiter,
  Markdown preprocessor.
- **Fixtures**: record real responses once into `tests/fixtures/` so tests never hit the network.
- **Manual test matrix per milestone**: Linux, Windows, Android phone (and an emulator with a tablet
  profile), light and dark theme.
- **CI**: build + `ctest` on every push; APK artifact on every PR.

---

## 11. Development environment setup (for M0)
1. Install Qt 6.8+ through the Qt Online Installer with these components: Desktop (gcc/MSVC/clang),
   Android (arm64-v8a, x86_64), Qt Multimedia, Qt Creator.
2. Install the Android SDK + NDK + JDK 17. Qt Creator → Preferences → Devices → Android can download
   and configure everything automatically.
3. Clone KDAB `android_openssl` (as a git submodule under `3rdparty/`).
4. Open `CMakeLists.txt` in Qt Creator, or build from the CLI:
   ```bash
   cmake --preset desktop-debug && cmake --build --preset desktop-debug
   # Android
   cmake --preset android-arm64 && cmake --build --preset android-arm64 --target apk
   ```

---

## 12. Decisions

| Question | Decision |
|---|---|
| Qt version | **Qt 6.8 LTS** (CI pins 6.8.3) |
| App name | **Quokka**, package id `io.github.sadikbr.quokka` |
| First desktop platform | **Linux** (AppImage). Windows/macOS later. |
| NSFW | **Hidden by default**, opt-in switch in settings |
| Look and feel | **One custom, playful theme everywhere**: Material controls as a base, own colour tokens (`qml/theme/Theme.qml`), sticker-style cards, bouncy motion, light + dark |
| Distribution | **GitHub Releases only**: pushing a `v*` tag builds and attaches the AppImage and the APK |

## 13. Progress

- [x] **M0**: skeleton, CMake presets, Android packaging (manifest, icons, OpenSSL), CI for Linux + Android, release workflow
- [x] **M1 (core)**: network client (queue, rate limits, retries), listing parser, paged feed model, feed UI (cards, sort bar, pull-to-refresh, infinite scroll, skeletons, error states), community picker, settings drawer
- [ ] M1 polish: verify against live Reddit, card vs compact layout
- [ ] M2: post page + comments
- [ ] M3: media viewer + video
- [ ] M4: search, subreddit and user pages
- [ ] M5: local library (subscriptions, saved, history)
- [ ] M6: desktop two-pane layout, keyboard navigation, accessibility, signed release key
