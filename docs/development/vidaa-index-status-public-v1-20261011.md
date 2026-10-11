# VIDAA index status parity — 2026-10-11 candidate history

**Current repository status (2026-10-11):** The consolidated R2 source was merged into `main` at `8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0`. The original branch/provenance notes below are **historical** and must not be read as a claim that this code is still unmerged. This source checkpoint alone does **not** prove which daemon binary is installed on yaVDR, nor successful real Hisense audio, SRT/WebVTT or STOP/cleanup acceptance. A deployment/restart still needs separate explicit authorization.

## Exact source evidence

The user runs VDR `2.7.9/11` (package
`vdr 2.7.9-3yavdr0~resolute`) and installed VIDAA
`b20261011.2`; the latter shows `--:--` for complete Recording
duration when the public Recording metadata does not yet know a
validated index timeline.

Inspected upstream `vdr-projects/vdr` **tag `2.7.9`**:
- `vdr.c` maps `--updindex=REC` to the deprecated `--genindex=REC`.
- `recording.c::GenerateIndex` invokes `unlink(index)` before
  starting `cIndexFileGenerator`. This is destructive if an existing
  `index` exists; it is not a read-only `--check-index` operation.
- The index generator reads VDR TS packets, locates frames, and
  writes the binary index; the client must never manufacture it.

Inspected current Suite Webfrontend:
- `web/frontend/api/recording-fallback-controls.js`: consumes
  `playback.durationSeconds` and `playback.resume.preparing`,
  shows `Aufnahmeindex wird erstellt …`, polls the existing
  session playback-status every 750 ms.
- `api/rest/src/RecordingMediaSessionCreate.cpp`: when a completed
  TS Recording has no valid indexed timeline, starts
  `VdrRecordingIndexUpdater::ensure` and records pending session context.
- `api/rest/src/RecordingMediaSessionPlaybackStatus.cpp`: checks the
  updater, validates the index and segment durations, enriches
  Recording cache, then returns authoritative `durationSeconds`.
- `core/vdr/include/VdrRecordingDuration.h`: calculates duration from
  validated 8-byte per-frame TS index entries and `info` FPS, not EPG
  time or cumulative HLS EXTINF values.

## Changes on this branch (not deployed)

- `VdrRecordingIndexUpdater::ensure` now explicitly rejects a
  pre-existing `index`, `index.vdr`, or symlink before spawning VDR
  2.7.9's destructive alias; the worker repeats the pre-exec guard.
  Existing invalid indexes are **not silently deleted or rebuilt**.
  An unrelated external writer racing exactly with exec cannot be
  fully synchronized by these checks alone; coordinate such repair
  separately. No chmod/chown is performed by the patch.
- The existing **Device-authenticated**
  `POST /api/v1/recording-playback-sessions` response now includes
  minimal `playback.durationSeconds` and
  `playback.resume.{supported,preparing}` metadata. The values
  come from the canonical MediaSession creation logic.
- The existing **Device-authenticated/owned**
  `GET /api/v1/recording-playback-sessions/{ms-id}` continues to
  verify current Device proof and grant via
  `PublicDevicePlaybackRegistry::describeOwned`, invokes the existing
  internal index-status reconciliation, and returns **only**
  `playback.durationSeconds` and
  `playback.resume.preparing`. An internal native Recording identity,
  worker path, access cookie or provider URL must never leave
  this public response. Duration is independently derived from
  `durationSecondsFromIndex` on the canonical TS index.
- No new public route, custom index format, credential, or browser
  legacy API is introduced.

## Delivery gates

GitHub code changes are an isolated candidate, **not** an installed
daemon update. Test at least:
- `make test-phase65d2-vdr-recording-index-updater`
- Public Device session response C++ test
- Existing Public-v1 Device authorization and ownership tests
- Full relevant CI with a reviewed draft PR
- A real completed TS Recording with a known-missing index
- A completed Recording with an existing index that must remain
  byte-for-byte unchanged
- Long index preparation, cancellation/STOP, error, and client expiry

Deploying or restarting the live daemon needs **separate explicit
authorization and a backup/rollback plan**. No live Recording directory
or filesystem permissions have been touched in preparing this branch.
