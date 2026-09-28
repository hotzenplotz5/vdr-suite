# Post-Phase-69 HbbTV Media Visibility Hardening

## Status

**Non-numbered correctness hardening between completed Phase 69 and not-yet-started Phase 70.**

Phase 69 remains completed. Phase 70 remains not started and is not authorized by this work.

This hardening covers HbbTV media/application composition and startup layout in the first-party Live-TV browser surface. Real ZDF/yaVDR acceptance on September 28 invalidated the synthetic assumption that the captured application plane could safely stay above the external media plane. The candidate therefore retains the startup/layout hardening but restores the last real-video-visible media ordering.

## User-visible failure

Two separate failures are involved. Starting the application before Live video metadata is available can compress its presentation into a shallow horizontal strip. Separately, ZDF broadband media can play audio while its external video or native application controls are not visible. Real acceptance of head `384f5315be2b5368bebd987f5024ee02e0fc91bd` showed that lowering the external media below the captured application plane regresses the formerly visible video without restoring the controls.

## Root cause

The backend/provider contract already carries the HbbTV media presentation facts required by the browser:

- `fullscreen`;
- `geometry.x/y/width/height`;
- a distinct `mediaRevision`.

The browser also correctly switches the **existing canonical Live-TV `HTMLMediaElement`** from the broadcast MediaSession to the HbbTV broadband MediaSession. No second player is required or allowed.

Live audit base: `main` at `c5e852ad8e8cda4a012d2f63a800799bbb58e4e8`, fetched September 28, 2026. No open PR was returned by the audit. The reported [CI run 9514 / 36466665482](https://github.com/hotzenplotz5/vdr-suite/actions/runs/36466665482), head `3cd935bb50d33b0050912374327cdc6d2873137a`, failed only in the frontend job: the Home composition regression referenced `discoverySource` without reading its production file. Loading `home-recording-discovery.js` restores the intended assertion that Home clicks must not re-arm discovery; no Home runtime change is needed.

### Input and application controls

The existing production chain is:

1. `createHbbtvRemote()` OK click, or canvas Enter through `handleHbbtvOverlayKey()`.
2. `sendHbbtvInput('ok')`: active-session capability check, serialized input queue and session/generation recheck.
3. `VdrSuiteClientApi.fetchClientHbbtvSessionInput()` -> `HbbtvApiRuntime` -> `HbbtvApplicationSessionService::input()`.
4. Actor/client ownership, permission, expiry and current application checks -> `SuiteBridgeHbbtvRuntimeResolver` -> private SuiteBridge provider service.
5. The provider maps its normalized OK action to Enter and sends it to the HbbTV browser. `DaemonHbbtvRuntime` supplies the resolver/authorization wiring; it does not own browser layout. `HbbtvMediaSessionRuntime` owns authorized media issuance, not key routing.
6. The response kicks the existing presentation read; QOI is decoded and drawn into the application canvas.

At the audited base, `applyHbbtvMediaVideoComposition()` raises the external video to z-index 13 while the captured application canvas is at 12. That ordering is required for real ZDF media visibility with the current browser/provider behavior. It does mean that controls cannot be recovered merely by moving the complete captured application plane above the video. The ordinary Suite Remote navigation module does not participate in this HbbTV control path.

The live-fetched provider branch `hotzenplotz5/vdr-plugin-web:work/vdr-suite-hbbtv-runtime-v1` remained at `1b3d2e785a1caf2d9f991fafa9752a433c5a86ae`. Its `VdrSuiteHbbtvPresentationStore::ApplyBgraPatch()` preserves whatever alpha the browser sends, and its unit regression proves only that an explicitly transparent test pixel survives QOI encoding. That does not establish that a real ZDF player frame is transparent over its media area.

The provider in turn uses `Zabrimus/cefbrowser`. At upstream commit `d14517be4a7aecfc3ba0b77831af237721ef48e9`, `browserclient.cpp::OnPaint()` makes only the exact magic-magenta pixel `0xfffe2e9a` transparent. More importantly, `static-content/js/video_quirks.js` applies the ZDF video-start quirk by adding `quirk_hide_element` to the complete `#root`; that CSS hides the root and all descendants. Native ZDF player controls inside that root therefore cannot become visible after OK while the video quirk is active, even though VDR-Suite correctly delivers `VK_ENTER`. This is a cefbrowser/provider presentation issue, not a reason to hide the external media behind an opaque captured frame.

### Startup and layout ownership

`live-tv-view.createPlayback()` mounts the real `session-frontend-sync.createLivePanel()` before the media has decoded. That owner's video has width 100%, but no stable application aspect ratio. `alignHbbtvCanvas()` accepts every positive `video.getBoundingClientRect().height`, including Chromium's 150px fallback before intrinsic metadata exists. The canonical owner's `switchToExternalStream()` -> `connectMediaPath()` -> `releaseVideo()` -> `load()` can reset those intrinsic dimensions again.

Thus application-frame arrival races Live metadata and external-media attachment. At the audited base the app copies the transient video box. Alignment is driven by presentation/media polling or fullscreen changes, with no direct observer for the final container layout. A later layout can remain stale until another poll. There is no iframe or independently sized backend browser viewport controlled by this measurement: provider frame dimensions are supplied separately with QOI.

## Bounded fix

The candidate keeps the existing canonical Live-TV `<video>` and MediaSession lifecycle.

When external HbbTV media has successfully attached:

- fullscreen media uses the exact canonical video plane above the captured application canvas (video 13, canvas 12), preserving the last real-video-visible behavior;
- windowed media retains the existing provider-geometry transform above the captured application canvas;
- pointer input stays owned by the HbbTV application/normalized remote path;
- application-canvas alignment always measures the untransformed canonical video viewport, preventing recursive geometry drift;
- stopping/failing HbbTV media restores the exact previous inline video style before broadcast playback is restored.

While an application frame is available, its dimensions supply the canonical video's application aspect ratio, with automatic height and contained media. These temporary surface styles are independent of the media transform and restored on application close/stop or view deactivation. A `ResizeObserver` tracks the mounted video and slot, aligns on final layout without a network request, and disconnects on replacement/deactivation/teardown. Its callbacks verify current ownership. No startup sleep or second media element is introduced. A late media-attach completion is fenced against the current view/session/playback owner.

Invalid windowed geometry fails closed instead of allowing the video plane to cover unrelated HbbTV application UI.

## Retained architecture

This hardening does **not**:

- create a second `HTMLMediaElement` or playback owner;
- bypass Phase-65 MediaSession ownership;
- expose provider socket paths or raw HbbTV browser/plugin commands;
- alter SuiteBridge, daemon, provider or public-v1 contracts;
- reopen Phase 67 or Phase 69;
- start Phase 70.

## Regression boundary

Focused coverage must prove:

- fullscreen HbbTV media targets the complete presentation viewport;
- windowed HbbTV media honors provider geometry;
- invalid geometry fails closed;
- the video plane is composed only after canonical external-stream attachment succeeds and remains above the captured application plane until the cefbrowser/provider boundary can preserve native controls separately;
- application-canvas alignment measures the untransformed canonical video viewport;
- the existing HbbTV overlay/public-surface architecture guard requires the composition boundary.

`test_phase67_hbbtv_surface_lifecycle.js` executes the production Live view, canonical `session-frontend-sync` playback owner and real QOI decoder. It covers rapid launch before metadata, external attachment resetting metadata, OK/Enter/color/arrow input, media-plane visibility ordering, final resize without polling, owner-preserving refresh, close/broadcast restoration, immediate relaunch, stale queued input and observer cleanup. It deliberately no longer claims that a synthetic transparent QOI pixel proves native broadcaster controls can overlay the external video. Existing Home composition/navigation-retention and Live playback/stability tests remain in place. Home, cover and Recording-cut product sources are unchanged.

A local headless Chromium experiment loaded the same production view/owner with controlled Suite responses. It remains valid evidence for the startup geometry fix: before metadata, the base produced a 614 x 150 canvas while the candidate produced 614 x 345.375, and a later container resize reached 874 x 491.625 without another HTTP tick. Its synthetic alpha/control composition is not accepted as evidence for real ZDF layering because the actual cefbrowser ZDF quirk hides the application root during video playback.

Real ZDF/yaVDR acceptance was performed against head `384f5315be2b5368bebd987f5024ee02e0fc91bd` and failed: the broadband audio ran, the external video became hidden after the z-index change, and OK still did not expose native player controls. The VDR-Suite correction restores media visibility while retaining the startup/resize hardening. Native ZDF control visibility remains pending a cefbrowser/provider correction and must not be marked accepted by VDR-Suite CI alone. SuiteBridge/provider source is unchanged by this PR.
