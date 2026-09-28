# Post-Phase-69 HbbTV Media Visibility Hardening

## Status

**Non-numbered correctness hardening between completed Phase 69 and not-yet-started Phase 70.**

Phase 69 remains completed. Phase 70 remains not started and is not authorized by this work.

This candidate addresses one demonstrated HbbTV presentation/media composition regression in the first-party Live-TV browser surface.

## User-visible failure

A broadcaster HbbTV application launches normally and its application frame remains visible. Starting a broadband HbbTV video produces audible audio and the Live-TV status reports the HbbTV medium as running, but the moving video image is not visible.

## Root cause

The backend/provider contract already carries the HbbTV media presentation facts required by the browser:

- `fullscreen`;
- `geometry.x/y/width/height`;
- a distinct `mediaRevision`.

The browser also correctly switches the **existing canonical Live-TV `HTMLMediaElement`** from the broadcast MediaSession to the HbbTV broadband MediaSession. No second player is required or allowed.

The missing boundary was visual composition:

1. `live-tv-view.js` appended the HbbTV presentation canvas after the canonical playback element.
2. The canvas is positioned over the exact Live-TV video viewport with `z-index: 12`.
3. HbbTV `fullscreen` and `geometry` were stored in frontend state but never used to compose the canonical video plane.
4. Therefore the external HbbTV video could decode and play audio correctly while its pixels remained behind the still-opaque HbbTV application canvas.

This is a browser presentation-plane bug, not a provider transport, MediaSession ownership or audio/video attach failure.

## Bounded fix

The candidate keeps the existing canonical Live-TV `<video>` and MediaSession lifecycle.

When external HbbTV media has successfully attached:

- fullscreen media raises that exact video plane above the application canvas for the full presentation viewport;
- windowed media maps the provider geometry from the HbbTV presentation-frame coordinates into the browser video viewport and raises only that transformed video rectangle;
- pointer input stays owned by the HbbTV application/normalized remote path;
- application-canvas alignment always measures the untransformed canonical video viewport, preventing recursive geometry drift;
- stopping/failing HbbTV media restores the exact previous inline video style before broadcast playback is restored.

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
- the video plane is raised only after canonical external-stream attachment succeeds;
- application-canvas alignment measures the untransformed canonical video viewport;
- the existing HbbTV overlay/public-surface architecture guard requires the composition boundary.

Real yaVDR/browser acceptance remains the final evidence for the reported visual regression because hosted CI cannot prove physical browser video pixels.
