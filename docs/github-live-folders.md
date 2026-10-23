# GitHub Live Folder: native, public-repository MVP

This personal-fork feature is implemented inside Chromium's native tab strip,
not as an extension. It is a live **tab group**, not a nested sidebar folder.
It is source work under test, **not an install-ready release**: the native
controller, menu integration and Chromium tests still need a full build and
real browser end-to-end validation on the target platform.

## Using it after building

1. Open an HTTPS GitHub repository page in an ordinary, unpinned, ungrouped tab.
2. Right-click that tab and choose **Create GitHub live folder (10 public PRs,
   every 5 min)**. This explicitly opts that repository into recurring public
   GitHub API requests for this browser window.
3. The existing tab becomes the source tab in a native group. Up to ten of the
   most recently updated open pull requests open in background tabs in it.
4. Right-click any member tab to refresh or stop the live folder. Refresh has
   a one-minute cooldown. Stopping keeps every tab and the ordinary group.

There are at most three live groups per window. Each live folder can keep at
most ten automatically created tabs still open, including preserved/ungrouped
tabs and those moved to another window. If that hard limit is reached, the
status says `tab limit reached`; close unwanted preserved tabs and refresh to
free slots. Nothing is closed automatically to make space.

A full ten-item result is
labelled `10 latest open PRs`, rather than implying the complete repository
backlog is shown. The API's remaining pages are intentionally not fetched.
The source tab remains available for visiting GitHub's complete PR list.

## Behavior and limits

- Only public repositories on `github.com` are supported. GitHub Enterprise,
  private repositories, account-wide assigned/review-requested queries and
  OAuth/token setup are not implemented.
- Configuration is window/session-only. Restored groups after restarting are
  ordinary groups and do not restart requests. Moving a group into a different
  browser window also stops refreshing; use a fresh ungrouped source tab to
  create another live group when needed.
- A PR that drops out of the latest-ten snapshot is **ungrouped, never closed**.
  It may still be open but have fallen below the ten-item cutoff. This keeps
  drafts and page state alive. User-added tabs are not automatically removed.
- Closing, moving, or navigating an automatically added tab suppresses its
  recreation while that PR remains in the current snapshot. The controller
  does not undo the user's tab changes. Navigated tabs are left alone.
- An explicitly customized group name, color and collapsed state are retained.
  Automatic status appears in the default group name; a custom name hides that
  status. Menu strings currently use English.
- Network failures and malformed responses keep the last good tabs. HTTP
  403/429 pauses requests for one hour; 401/404/redirect responses use a
  thirty-minute retry delay. Repeated manual refresh cannot bypass these waits.
- API requests omit credentials and cookies, have no referrer, reject redirects,
  and use a fixed `https://api.github.com/repos/.../pulls` endpoint. No secret or
  token is created or stored. The actual PR tabs are ordinary browser pages,
  so opening them uses the browser's normal GitHub website session and page
  networking, just as opening a tab yourself would.
- Parsing is limited to 1 MiB, 32 JSON nesting levels and ten validated, unique
  PR numbers. Every returned PR URL must match the configured repository on
  `https://github.com`. Requests time out after thirty seconds.
- Closing/removing the group, stopping it, or destroying the window cancels
  in-flight work. Weak lifetimes and generation checks reject stale results.
  Incognito, guest, installed-web-app, pinned and split tabs cannot create one.

GitHub's official [list pull requests endpoint documentation](https://docs.github.com/en/rest/pulls/pulls#list-pull-requests)
describes public unauthenticated requests, case-insensitive repository names,
the `updated` sort, and `per_page` pagination used here.

## Tests and verification boundary

Run the actual shipped pure-C++ policy/model from its quilt patch:

```sh
python3 tests/run_github_live_folder_model_tests.py
ASAN_OPTIONS=detect_leaks=0 python3 tests/run_github_live_folder_model_tests.py --sanitize
```

The runner extracts the exact model source from `patches/series`, compiles it
with C++17 and warnings as errors, and runs the executable. It does not test
a Python/JavaScript copy. Coverage includes origin spoofing, all byte values in
name allowlists, boundaries, fixed API origin, canonical PR URLs, stale response
generations, permanently stopped refreshes and independent refresh guards.
Leak detection is disabled only for the sanitizer run in traced environments;
address and undefined-behavior sanitizers remain enabled.

The Chromium `GitHubLiveFolderControllerTest.*` component tests use the real
tab-strip model/controller with fake web contents and networking. They cover
opt-in-only networking, no authentication/referrer, redirect rejection policy,
snapshot updates, preserving tabs and user changes, malformed/oversized lists,
the snapshot/no-pagination cap, lifetime open-tab cap, stopping/removing groups,
rate limits, folder caps, incognito, duplicate suppression, split preservation,
visual notifications and reentrant stop/destruction.
These tests are added to `chrome/browser/ui/tabs:unit_tests` but have **not** been
compiled or executed in the source-only development environment.

Before an install-ready build is claimed, run those Chromium tests and test the
actual right-click flow in the packaged browser, including repeated create,
refresh, stop, manual tab edits, offline/recovery, group moves, window close,
restart/restore, incognito, and the target platform's horizontal/vertical tab UI.
