# Post-Phase-69 HbbTV Media Visibility Hardening

## Status

**Non-numbered correctness hardening between completed Phase 69 and not-yet-started Phase 70.**

Phase 69 remains completed. Phase 70 remains not started and is not authorized by this work.

This hardening covers HbbTV media/application composition, input execution and startup layout in the first-party Live-TV browser surface. Real ZDF/yaVDR acceptance on September 28 proved two VDR-Suite regressions: PR #355 moved INPUT into an unsafe worker-thread execution context, and commit `709914c2` solved a stale presentation-frame problem by putting the complete broadband video above the application canvas, which necessarily covered later native controls. The candidate keeps the fast local transport and canonical player while restoring application-over-video composition with an explicit stale-frame fence.

## User-visible failure

Three symptoms share two concrete regressions. Starting the application before Live video metadata is available can compress its presentation into a shallow horizontal strip. PR #355 can make all HbbTV keys ineffective because INPUT executes `browserClient->ProcessKey()` directly on the new External Plugin worker. Commit `709914c2` then raises broadband video above the application canvas to escape a stale opaque frame; that makes the video visible but prevents later native player controls in the application frame from being visible over it.

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

At the audited base, `applyHbbtvMediaVideoComposition()` raises the external video to z-index 13 while the application canvas is at 12. Git history shows that this ordering was introduced only by `709914c2` on September 28; the earlier working frontend kept the application plane above the canonical video. The actual problem that commit encountered was a stale pre-media presentation frame: lowering the video again without clearing/fencing that stale opaque frame naturally hides the video. The fix must therefore restore the earlier layer ownership and remove the stale frame, not keep the whole video permanently above application UI. The ordinary Suite Remote navigation module does not participate in this HbbTV control path.

The live-fetched provider branch `hotzenplotz5/vdr-plugin-web:work/vdr-suite-hbbtv-runtime-v1` remained at `1b3d2e785a1caf2d9f991fafa9752a433c5a86ae`. Its `VdrSuiteHbbtvPresentationStore::ApplyBgraPatch()` preserves whatever alpha the browser sends, and its unit regression proves only that an explicitly transparent test pixel survives QOI encoding. That does not establish that a real ZDF player frame is transparent over its media area.

The provider in turn uses the unchanged `Zabrimus/cefbrowser` build that had already produced working ZDF video, native controls and key handling before the SuiteBridge transport migration. Its implementation details therefore cannot by themselves explain the new regression. The decisive repository delta is PR #355: before it, `HBBRUN INPUT` reached SuiteBridge through VDR's SVDRP handler thread; afterwards the same typed provider call was executed directly on the External Plugin Interactive worker. Re-audit of the pinned `vdr-plugin-web` runtime shows that LAUNCH is explicitly scheduled into VDR context, while INPUT invokes `browserClient->ProcessKey()` synchronously on its caller. The original #355 statement that runtime INPUT retained provider-owned main-context scheduling was therefore false. The candidate keeps the fast local AF_UNIX transport but marshals only INPUT through a bounded SuiteBridge `MainThreadHook()` handoff.

### Startup and layout ownership

`live-tv-view.createPlayback()` mounts the real `session-frontend-sync.createLivePanel()` before the media has decoded. That owner's video has width 100%, but no stable application aspect ratio. `alignHbbtvCanvas()` accepts every positive `video.getBoundingClientRect().height`, including Chromium's 150px fallback before intrinsic metadata exists. The canonical owner's `switchToExternalStream()` -> `connectMediaPath()` -> `releaseVideo()` -> `load()` can reset those intrinsic dimensions again.

Thus application-frame arrival races Live metadata and external-media attachment. At the audited base the app copies the transient video box. Alignment is driven by presentation/media polling or fullscreen changes, with no direct observer for the final container layout. A later layout can remain stale until another poll. There is no iframe or independently sized backend browser viewport controlled by this measurement: provider frame dimensions are supplied separately with QOI.

## Bounded fix

The candidate keeps the existing canonical Live-TV `<video>` and MediaSession lifecycle.

When external HbbTV media has successfully attached:

- fullscreen media uses the canonical video plane below the HbbTV application canvas (video 11, canvas 12);
- windowed media retains the provider-geometry transform on that lower media plane;
- the current presentation revision is captured as a baseline and only the provider-declared media rectangle is cleared from the stale canvas, immediately exposing the broadband video without erasing unrelated application UI;
- an in-flight presentation frame at or below that baseline is ignored, so it cannot repaint the stale opaque media rectangle after attach;
- the next newer provider frame is accepted in full, allowing its transparent video hole and native controls to coexist above the video;
- pointer input stays owned by the HbbTV application/normalized remote path;
- application-canvas alignment always measures the untransformed canonical video viewport, preventing recursive geometry drift;
- stopping/failing HbbTV media restores the exact previous inline video style and presentation fence before broadcast playback is restored.

While an application frame is available, its dimensions supply the canonical video's application aspect ratio, with automatic height and contained media. These temporary surface styles are independent of the media transform and restored on application close/stop or view deactivation. A `ResizeObserver` tracks the mounted video and slot, aligns on final layout without a network request, and disconnects on replacement/deactivation/teardown. Its callbacks verify current ownership. No startup sleep or second media element is introduced. A late media-attach completion is fenced against the current view/session/playback owner.

Invalid windowed geometry fails closed instead of allowing the video plane to cover unrelated HbbTV application UI.

## Retained architecture

This hardening does **not**:

- create a second `HTMLMediaElement` or playback owner;
- bypass Phase-65 MediaSession ownership;
- expose provider socket paths or raw HbbTV browser/plugin commands;
- alter daemon, provider or public-v1 contracts;
- reopen Phase 67 or Phase 69;
- start Phase 70.

## Regression boundary

Focused coverage must prove:

- fullscreen HbbTV media targets the complete presentation viewport;
- windowed HbbTV media honors provider geometry;
- invalid geometry fails closed;
- the video plane is composed only after canonical external-stream attachment succeeds and remains below the application plane;
- media attach clears exactly the stale provider media rectangle and fences older in-flight presentation frames;
- a newer provider frame can restore opaque controls above the still-visible video;
- application-canvas alignment measures the untransformed canonical video viewport;
- the existing HbbTV overlay/public-surface architecture guard requires the composition boundary.

`test_phase67_hbbtv_surface_lifecycle.js` executes the production Live view, canonical `session-frontend-sync` playback owner and real QOI decoder. It now starts with an opaque pre-media application frame, proves that broadband attach clears the stale media rectangle, keeps video below the application plane, then accepts a newer frame containing a transparent video hole plus an opaque control pixel above the video. It also covers rapid launch before metadata, OK/Enter/color/arrow input, final resize without polling, owner-preserving refresh, close/broadcast restoration, immediate relaunch, stale queued input and observer cleanup. Existing Home composition/navigation-retention and Live playback/stability tests remain in place. Home, cover and Recording-cut product sources are unchanged.

A local headless Chromium experiment loaded the same production view/owner with controlled Suite responses. It remains valid evidence for the startup geometry fix: before metadata, the base produced a 614 x 150 canvas while the candidate produced 614 x 345.375, and a later container resize reached 874 x 491.625 without another HTTP tick. Its synthetic alpha/control composition is not accepted as evidence for real ZDF layering; real yaVDR acceptance remains authoritative for video/control composition.

Real ZDF/yaVDR acceptance was performed against head `384f5315be2b5368bebd987f5024ee02e0fc91bd` and failed: broadband audio ran, the external video became hidden after placing media below an uncleared stale canvas, and OK still did not expose native player controls. Head `f5dc0b7ae428ff821c88235f4a78ea47abd62860` restored real video visibility and startup geometry, but all HbbTV keys still had no application effect. The SuiteBridge main-thread INPUT handoff now restores the pre-PR-355 execution context without reverting to SVDRP. The final composition candidate additionally removes `709914c2`'s permanent video-above-app workaround by clearing/fencing the stale media rectangle before restoring application-over-video layering. Real ZDF acceptance must now prove video visibility, key effect and native controls together before this work is accepted.
