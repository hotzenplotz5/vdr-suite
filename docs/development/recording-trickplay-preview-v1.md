# Recording Trickplay Preview V1 — slice 1: trusted JPEG command boundary

**Status:** Backend Device-only read-only HTTPS JPEG route, planner, bounded
renderer and regressions. **No VIDAA UI integration and no real-TV test yet.**
No installed service or permission is changed.

Endpoint: `GET /api/v1/recording-playback-sessions/{ms_id}/trickplay?seconds=N`.
Requires the existing paired **Device credential** and an **active
device-owned Recording MediaSession**; not public, not a Browser endpoint.
Session ownership, Device/credential revocation and persistent
`media.recording.play@backend` grants are rechecked before accessing the
native recording and once more before returning bytes. No original pathname
or raw recording ID is accepted or returned.

A completed modern TS index is required. A missing/active/partial recording,
invalid/duplicate/encoded query, invalid session, or out-of-bound time gets
an error (no hidden fallback to unindexed random FFmpeg seeks). Response is
`image/jpeg`, `Cache-Control: no-store`, `nosniff`, sample bucket header.
The caller must **not** issue rapid batches; the server rejects simultaneous
thumbnail renders instead of running multiple FFmpeg processes.

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

The plan runs a **single-frame** FFmpeg JPEG conversion using the existing
private `MediaSessionWorkspace` `input.ffconcat` convention. The internal
`RecordingTrickplayPreviewRenderer` adds a 3-second subprocess deadline,
256 KiB output ceiling, 768 MiB address-space and 2-second CPU limits,
first/last JPEG marker checks, no-follow private-directory/concat checks,
and nonblocking in-process worker contention. It refuses modified FFmpeg
arguments and returns bytes only to its **authorized future caller**. These
are source- and mocked-runner-tested constraints, not hardware evidence or
a public endpoint. The JPEG check is framing validation, not pixel decode.
The renderer returns at most one private JPEG per call; the Device read
route now publishes it only after authorization and a final grant recheck.

## Proposed next slices (authorization and operational acceptance)

1. Implement a private thumbnail cache tied to
   recording fingerprint + exact index generation + time bucket. Reject
   symlinks/foreign owners, cap disk bytes, memory, concurrency and failures.
2. Expose bounded time buckets/manifest and a matching VIDAA one-image
   overlay. The currently playing MSE decoder remains untouched while
   remote scrub navigation changes only the displayed thumbnail. Abort stale
   requests; no second video decoder; no HTML canvas extraction dependency.
3. Real Hisense acceptance: actual thumbnails at requested times, arbitrary
   seek forward/back, 5+ minute picture/audio continuity, denial after grant
   revocation, disabled/unfinished index and space/CPU budget.

**No performance or TV acceptance has been measured for this planner.**

Regression: `make test-phase65-recording-trickplay-preview-plan`,
`make test-phase65-recording-trickplay-preview-renderer` plus
the established `test-fast` suite.
