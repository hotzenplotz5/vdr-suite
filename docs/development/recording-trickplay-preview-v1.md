# Recording Trickplay Preview V1 — slice 1: trusted JPEG command boundary

**Status:** Backend planner and regression tests, **not** a public HTTP image
route and **not** yet available to VIDAA. No Device permission, TV app,
recording files, FFmpeg service settings, or installed package is changed.

## Open-source architecture comparison

- The archived [Plex Media Player](https://github.com/plexinc/plex-media-player/blob/master/src/player/PlayerComponent.cpp)
  seeks within libmpv and uses cache/backbuffer. VIDAA HLS/MSE does not
  expose an equivalent random-access demuxer. Code has **not** been copied.
- [Kodi VideoPlayer](https://github.com/xbmc/xbmc/blob/master/xbmc/cores/VideoPlayer/VideoPlayer.cpp)
  seeks and flushes buffered tracks within its decoder owner. Recreating
  this with a rolling eight-entry HLS playlist is not a safe direct substitution.
- [Jellyfin TrickplayController](https://github.com/jellyfin/jellyfin/blob/master/Jellyfin.Api/Controllers/TrickplayController.cs)
  exposes a separate JPEG tile contract. This is the suitable architectural
  reference for a low-cost VIDAA timeline thumbnail.

## Slice 1 implementation and limits

`RecordingTrickplayPreviewPlanner` lives under `core/media`, with no
server/HTTP dependencies. After the **existing security boundary** has
authorized a specific recording, a future controller may supply a *complete,
trusted VDR index*: known total seconds, segment count and each segment's
index-derived duration. The planner rejects missing, partial, inconsistent
or nonfinite timelines, positions outside the recording, more than 1024
segments, or recordings longer than 12 hours. It quantizes time downward
to 10-second samples; e.g. 125s requests the 120s picture. Request inputs
cannot supply file paths, URL, ffmpeg flags, codecs or filters.

The plan runs a bounded **single-frame** FFmpeg JPEG conversion using the
existing **private MediaSessionWorkspace** `input.ffconcat` convention.
No stream is published from this plan. A consuming controller must still
enforce a 3-second process deadline, 256 KiB output limit, 768 MiB address
space and 2-second CPU ceiling via `MediaProcessRunner`, check JPEG headers,
and cap/consolidate concurrent workers. The renderer is *not included* yet,
so none of those operational properties is claimed as implemented.

## Proposed next slices (separate authorization gates)

1. Implement a bounded renderer and private thumbnail cache tied to
   recording fingerprint + exact index generation + time bucket. Reject
   symlinks/foreign owners, cap disk bytes, memory, concurrency and failures.
2. Route authenticated read-only Device `GET` requests via the existing
   Public-v1 Device principal and **media.recording.play@backend** grant.
   Revalidate recording ownership and grants on every request, and never
   return original paths or a reusable anonymous JPEG URL. Browser clients
   use their own existing authorization context.
3. Expose bounded time buckets/manifest and a matching VIDAA one-image
   overlay. The currently playing MSE decoder remains untouched while
   remote scrub navigation changes only the displayed thumbnail. Abort stale
   requests; no second video decoder; no HTML canvas extraction dependency.
4. Real Hisense acceptance: actual thumbnails at requested times, arbitrary
   seek forward/back, 5+ minute picture/audio continuity, denial after grant
   revocation, disabled/unfinished index and space/CPU budget.

**No performance or TV acceptance has been measured for this planner.**

Regression: `make test-phase65-recording-trickplay-preview-plan` plus
the established `test-fast` suite.
