# Post-Phase-69 HbbTV Media Visibility Hardening

## Status

**Non-numbered correctness hardening between completed Phase 69 and not-yet-started Phase 70.**

Phase 69 remains completed. Phase 70 remains not started and is not authorized by this work.

This hardening covers HbbTV media/application composition and startup layout in the first-party Live-TV browser surface. The September 28 follow-up corrects the earlier video-above-canvas composition described below.

## User-visible failure

A broadcaster HbbTV application and broadband video run, but OK does not reveal the application's player controls. Starting the application before Live video metadata is available can also compress its presentation into a shallow horizontal strip.

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

At the audited base, `applyHbbtvMediaVideoComposition()` raises the video to z-index 13 while the application canvas is at 12. Every opaque video pixel therefore covers the corresponding application control even after correct input and a new application frame. The ordinary Suite Remote navigation module does not participate in this HbbTV control path.

The live-fetched provider branch `hotzenplotz5/vdr-plugin-web:work/vdr-suite-hbbtv-runtime-v1` remained at `1b3d2e785a1caf2d9f991fafa9752a433c5a86ae`. Its `VdrSuiteHbbtvPresentationStore::ApplyBgraPatch()` preserves alpha and its presentation regression verifies a transparent pixel through QOI encoding. The Suite decoder also preserves alpha. Video must be below this application plane; removing the canvas or clearing the whole media rectangle would erase application controls too. An opaque provider pixel remains opaque by design; this fix does not guess color keys or manufacture transparency.

### Startup and layout ownership

`live-tv-view.createPlayback()` mounts the real `session-frontend-sync.createLivePanel()` before the media has decoded. That owner's video has width 100%, but no stable application aspect ratio. `alignHbbtvCanvas()` accepts every positive `video.getBoundingClientRect().height`, including Chromium's 150px fallback before intrinsic metadata exists. The canonical owner's `switchToExternalStream()` -> `connectMediaPath()` -> `releaseVideo()` -> `load()` can reset those intrinsic dimensions again.

Thus application-frame arrival races Live metadata and external-media attachment. At the audited base the app copies the transient video box. Alignment is driven by presentation/media polling or fullscreen changes, with no direct observer for the final container layout. A later layout can remain stale until another poll. There is no iframe or independently sized backend browser viewport controlled by this measurement: provider frame dimensions are supplied separately with QOI.

## Bounded fix

The candidate keeps the existing canonical Live-TV `<video>` and MediaSession lifecycle.

When external HbbTV media has successfully attached:

- fullscreen media uses the exact canonical video plane below the application canvas (video 11, canvas 12);
- windowed media retains the existing provider-geometry transform below the application canvas;
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
- the video plane is composed only after canonical external-stream attachment succeeds and remains below application controls;
- application-canvas alignment measures the untransformed canonical video viewport;
- the existing HbbTV overlay/public-surface architecture guard requires the composition boundary.

`test_phase67_hbbtv_surface_lifecycle.js` executes the production Live view, canonical `session-frontend-sync` playback owner and real QOI decoder. It covers rapid launch before metadata, external attachment resetting metadata, OK/Enter/color/arrow input, transparent video plus opaque controls, final resize without polling, owner-preserving refresh, close/broadcast restoration, immediate relaunch, stale queued input and observer cleanup. Existing Home composition/navigation-retention and Live playback/stability tests remain in place. Home, cover and Recording-cut product sources are unchanged.

A local headless Chromium experiment loaded the same production view/owner with controlled Suite responses and compared the audited base with the fix. Before metadata, the base produced a 614 x 150 canvas; the candidate produced 614 x 345.375. After OK, screenshot sampling found the blue video fixture covering the white control on the base; the candidate retained blue through the transparent hole and displayed the white control. Widening the container without any further HTTP tick left the base at 614 x 150; the candidate observed 874 x 491.625. No browser script errors occurred. These are controlled browser results, not a claim of live ZDF/yaVDR acceptance.

Real broadcaster acceptance must recreate the Live owner, launch HbbTV immediately after Live start, start broadband video, press HbbTV OK/Enter, resize, close and relaunch. Actual provider frames and physical yaVDR acceptance have not been collected in this workstream. SuiteBridge/provider source is unchanged and no plugin build or service restart is required for this frontend candidate.
