# Post-Phase-69 HbbTV Media Visibility Hardening

## Status

**Non-numbered correctness hardening between completed Phase 69 and not-yet-started Phase 70.**

Phase 69 remains completed. Phase 70 remains not started and is not authorized by this work.

This hardening keeps only the VDR-Suite browser-surface changes that remained necessary after real ZDF/yaVDR acceptance. The earlier hypotheses that HBBRUN INPUT needed a special SVDRP route or a SuiteBridge MainThreadHook were disproved by real PAUSE tests and are not part of the accepted architecture.

The complementary provider/browser correction is merged in `hotzenplotz5/vdr-plugin-web/main` at `35f50dd8a3a611d56039d190c093a489e4a0193e`.

## User-visible failures

The accepted failure set had three independent visible symptoms:

1. starting HbbTV before Live video metadata stabilized could compress the application into a shallow strip;
2. putting broadband video above the HbbTV presentation canvas made video visible but necessarily covered later native application/player controls;
3. lowering broadband video again without dealing with the previous opaque presentation frame could hide the video behind stale application pixels.

The separate provider/browser regression caused the HbbTV page/control lifetime and input failures. That provider-side cause is not repaired by this VDR-Suite slice.

## VDR-Suite root cause

The canonical Live-TV `HTMLMediaElement` is already the correct playback owner and is reused for HbbTV broadband media. No second player is required.

At the pre-fix VDR-Suite base:

- the application canvas is at z-index 12;
- the external HbbTV video had been raised to z-index 13 to escape an opaque stale presentation frame;
- therefore any later native HbbTV controls painted by the provider canvas were necessarily hidden below the video;
- lowering the video without clearing/fencing the stale frame made the broadband image disappear again;
- early layout accepted Chromium's metadata-less fallback video height, so the application could inherit a compressed viewport before final media geometry existed.

These are first-party browser composition/lifecycle defects. They are independent of the final provider/browser input fix.

## Bounded fix

The accepted VDR-Suite slice keeps the existing canonical Live-TV video and MediaSession lifecycle.

When external HbbTV media attaches:

- broadband video remains below the HbbTV application plane (video 11, canvas 12);
- only the provider-declared stale media rectangle is cleared from the current application frame so the video becomes immediately visible;
- same/older in-flight presentation revisions are fenced after media attachment;
- the next newer provider frame is accepted normally, allowing transparent media pixels and opaque native controls to coexist above the video;
- windowed media continues to honor provider geometry;
- pointer input remains owned by the HbbTV application/normalized remote path;
- late media-attach completion is rejected when the view/session/playback owner changed.

For startup/layout:

- application-frame dimensions temporarily define the canonical video's aspect ratio while intrinsic media metadata is absent or reset;
- a `ResizeObserver` tracks the actual mounted video/slot and realigns only while ownership is current;
- close, failure, replacement and deactivation restore the exact previous inline video/surface state and disconnect observers.

No startup sleep and no second media element are introduced.

## Retained architecture

This hardening does **not**:

- special-case HBBRUN INPUT onto typed SVDRP;
- add a SuiteBridge MainThreadHook;
- change the normal prioritized SuiteBridge HbbTV transport policy;
- create another playback owner;
- expose provider socket paths or raw browser/plugin commands;
- alter public-v1 contracts;
- reopen Phase 67 or Phase 69;
- start Phase 70.

The final accepted input/control behavior depends on the separately merged provider/browser fix; VDR-Suite keeps its existing normalized HbbTV input path.

## Regression boundary

Focused coverage proves:

- rapid HbbTV launch is single-flight;
- pre-metadata application layout uses the application aspect ratio instead of the transient 150px browser fallback;
- final container resize realigns without a network request;
- the canonical video owner remains unique across HbbTV media switching and unrelated Live refreshes;
- broadband media remains below the application plane;
- media attach clears only the stale media rectangle and fences same/older frames;
- a newer presentation frame can place opaque controls above a transparent video hole;
- close restores broadcast and the previous video styles;
- immediate relaunch cannot reuse stale geometry;
- queued input cannot leak into a later session;
- teardown disconnects layout observers.

The one-line Home composition test fixture correction is retained because current `main` CI is independently red: the test references `discoverySource` without loading `home-recording-discovery.js`. That change modifies test setup only, not Home product code.

## Real acceptance

Real yaVDR acceptance established the sequence that determines the retained scope:

- lowering broadband video below an uncleared canvas hid the video;
- restoring video visibility alone left controls/input broken;
- SuiteBridge MainThreadHook and typed-SVDRP INPUT experiments did not make PAUSE affect playback;
- after the provider/browser lifetime/static correction was installed together with this VDR-Suite browser-surface candidate, broadband video remained visible, native controls appeared, and PAUSE, OK and STOP all worked.

Therefore the VDR-Suite merge retains the proven composition/layout/lifecycle corrections and removes the disproved INPUT transport changes.
