# Personal Arc-style preview

This is experimental source for the personal fork, not an installable or
end-to-end-verified release. It is based on Helium
`f29a25968e483f8bb85e47829ccd487bbe229199` / Chromium `156.0.8078.47`.
No upstream contribution is intended. Existing licenses and notices apply.

## Profile switching

Use a **separate test data directory**, enable
`helium://flags/#helium-profile-window-switching`, and restart (or launch with
`--enable-features=HeliumProfileWindowSwitching`). The feature is off by default
until native validation is complete. Do not use an existing daily-use profile.

Selecting another profile in the normal profile menu shows that profile's
browser at the current window's restored bounds and hides the previous window.
The windows, tabs, cookies, storage, and extensions retain their original
Chromium Profile; this is deliberately **not** cross-profile tab transplantation
or literal rebinding of one OS window. Closing the selected window restores a
previously hidden window. Explicit application Quit must not reopen windows.

Use Helium's existing customizable **Profile menu** keyboard action, then select
a profile with the keyboard. One-keystroke next-profile cycling is not added.
Native fullscreen uses Chromium's ordinary separate-window switch, not an
emulated fullscreen/Space transition. Hidden tabs remain live, including media
or background work. Workspace visibility/ordering is session-only and is not
persisted across restart. If showing a new target is deferred for initial paint,
the source stays visible rather than leaving a blank desktop. Late asynchronous
profile loads are rejected before opening their window; window creation and
native activation still require rapid-switch/reentrancy testing on macOS.

## Little Arc

The independent Little Arc patch is a native macOS external-link preview, not an
extension. Enable `helium://flags/#helium-little-arc` and restart. The
feature is off by default. A preview is 720×640; **Command+Shift+Return** or
**Show as tab** promotes the existing live page to a normal window with the
exact same profile, without reloading. External URL events preserve Chromium's
regular-profile preference; an incognito window being most recent does not make
external previews incognito. The direct helper's OTR isolation test is not a
claim that macOS external events route privately. There is no custom promotion-button
artwork or non-macOS external-link hook. Native tests are:
`LittleArcBrowserTest.*`, `LittleArcDisabledBrowserTest.*`,
`AppControllerLittleArcBrowserTest.*`, and
`AppControllerLittleArcStartupBrowserTest.*` (nine authored cases). No default-browser setting, OS URL-handler registration, credential,
OAuth grant, signing identity, or user browser data is changed by these sources.

## GitHub Live Folder scope

The first implementation targets an explicitly activated **public-repository PR
tab group**, not Arc/Zen's complete nested-folder UI or authenticated private
repository experience. Private repositories, account-wide personalized PR
queries, persistent configuration, and OAuth are not implemented. Never treat
an unauthenticated empty/error response as proof that a private repository has
no PRs. Detailed limits and native tests accompany that feature's patch.

## Verification status

Performed on the Linux editing host:

- Existing `devutils` test suite: 2 passed.
- Existing `utils` test suite: 6 passed.
- Actual shipped Live Folder C++ model: 7 groups / 51,693 assertions passed
  with strict C++17 warnings-as-errors and again with AddressSanitizer plus
  UndefinedBehaviorSanitizer. LeakSanitizer is unavailable under this host's
  ptrace environment; the sanitizer run used `ASAN_OPTIONS=detect_leaks=0`.
- Repository configuration/patch registration validation: passed.
- Profile patch application against exact Chromium source after all preceding
  patches touching its files: passed with `patch --fuzz=0`.

The model run executes real production parser/URL/refresh-guard code, but does
not execute Chromium networking, tab groups, or native UI. Tooling/context
checks and model tests are **not browser E2E**. New Chromium controller and
browser integration tests have been authored but not compiled or run.
There is no DMG, completed Chromium build, notarization, or successful native
Mac UI run at this stage. Do not call this ready to install.

## Native validation gates before a release

Use a dedicated temporary user-data directory, a local HTTP fixture server,
public/test GitHub fixtures and disposable profiles. Never import real user
cookies, credentials, history, or extensions for testing.

1. Build the complete browser, ChromeDriver, relevant unit tests, and
   `browser_tests` at the exact pinned source SHA. Patch application and the
   upstream 30-second sanity compile are not a substitute for build completion.
2. Run `ProfileMenuViewWorkspaceTest.*` against the compiled browser. Verify
   repeated switching, cookie/localStorage isolation, live tab/JS state, window
   bounds, absent target creation, and restoration after target closure.
3. Extend/execute profile scenarios for extension isolation, closed/deleted or
   locked target, close during async loading, rapid A/B/C switches, independent
   windows, fullscreen/Spaces, minimize/maximize, application Quit, relaunch,
   crash restoration, and unsaved form/beforeunload handling. No profile data or
   tab may silently migrate to another profile.
4. Exercise Little Arc via real macOS external URL events: browser cold/warm
   start, HTTP/HTTPS vs unsupported URLs, multi-URL events, explicit disabled
   flag, existing/new destination window, same-profile promotion, cookies,
   localStorage, back/forward history, unsaved edits, repeated promotion/close,
   popup/opener behavior, and cancellation. Check browser chrome visually, not
   only web page rendering.
5. Exercise Live Folder using deterministic server/network fixtures: valid and
   invalid repository URLs, fixed allowed API host, cross-origin redirects,
   public/no-auth vs private/404, empty list, malformed/oversized JSON, duplicate
   IDs, pagination, bounded tab count, 403/429/rate limit, 5xx/offline, stale and
   out-of-order responses, refresh throttling, user-closed/moved tabs, and stop
   or group/window/profile destruction during an outstanding request. Preserve
   unsaved tabs when a PR disappears. Verify there is no background polling
   before explicit activation or after the group is stopped/destroyed.
6. Run native Mac interactive QA for all three features together, with repeated
   and interrupted flows and screenshots. Headless ChromeDriver alone cannot
   validate native tab/group/profile/window UI.
7. Package a DMG, verify its integrity, architecture, app launch and signature;
   record exact source/build SHAs and SHA-256. Ad-hoc signing is not notarization
   and may require the user's explicit macOS opening approval.

## Build environment

The supported macOS packaging repository requires macOS 12+, Xcode 26 and its
Metal toolchain. The editing host is Linux (approximately 30 GB free disk,
10 GB RAM), so it cannot perform the supported native macOS build or Mac UI QA.
An approved standard `macos-26` preflight reported 96.22 GiB free, 7 GiB RAM,
3 ARM64 CPUs, Xcode 26.5 and Metal. That establishes available infrastructure,
not successful compilation or sufficient peak memory for every build step.
[Preflight run](https://github.com/iamrajjoshi/helium-macos/actions/runs/40010646037).
No paid runner,
Developer ID, or billing change is authorized or configured by this work.
