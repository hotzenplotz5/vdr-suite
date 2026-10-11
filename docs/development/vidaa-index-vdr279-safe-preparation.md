# VIDAA index duration via Public-v1 — VDR 2.7.9 safety candidate

Status: **branch-only development**, no yaVDR daemon deployment,
no recording/index modification, no permission change.

- Current installed yaVDR VDR: `vdr (2.7.9/11)`,
  Debian package `2.7.9-3yavdr0~resolute`.
- Current installed VIDAA: `b20261011.2`.
- Official upstream VDR **2.7.9 tag**:
  `vdr.c` handles `--updindex` through the same
  `GenerateIndex` function as `--genindex`;
  `recording.c::GenerateIndex` first `unlink()`s `index` and then
  rebuilds using `cIndexFileGenerator`. This behavior is destructive
  to any pre-existing index, whether valid or stale.
- VDR-Suite's current `VdrRecordingIndexUpdater::ensure` invokes
  `/usr/bin/vdr --updindex=<recording directory>` in a child process
  running under the recording-directory UID/GID, with canonical TS-path
  checks and an active `.timer` exclusion. The controller can call
  `ensure` whenever the index timeline is missing or invalid, **not
  just when the file is absent**.

## Existing Webfrontend behavior (source verified)

`web/frontend/api/recording-fallback-controls.js` uses
`mediaSession.playback.durationSeconds` and
`playback.resume.preparing`. It displays
"Aufnahmeindex wird erstellt …" and polls operation
`playback-status` at 750 ms intervals, with at most five consecutive
status errors. The existing server
`RecordingMediaSessionController::playbackStatus` checks session owner
and pending index state, and after successful index generation calls
`recordingQueryService_.findRecordingById`,
`segmentDurationsSecondsFromIndex`, and
`updateCachedRecording`. It provides the authoritative duration.

The installed Device Public-v1 responses omit these playback fields,
and their GET status previously returned only Device-owned session
metadata. Therefore VIDAA had no way to learn completion after
an index was regenerated even though the Browser API could.

## Source-only candidate

Branch `work/vidaa-vdr279-index-safety-20261011`:

1. `VdrRecordingIndexUpdater::isSafeTsRecording` rejects pre-existing
   `index` and `index.vdr` (including symlinks) with
   `recording_index_update_existing_index_unsafe`, without starting
   the generator. The child rechecks just before running VDR under
   the directory owner. Thus the generated-index path is intended
   for genuinely absent index files ONLY.
2. Device CREATE includes
   `mediaSession.playback.durationSeconds` and
   `mediaSession.playback.resume.{supported,preparing}` drawn from the
   existing server playback truth.
3. The already-existing Device GET
   `/api/v1/recording-playback-sessions/<ms-id>` remains guarded by
   `PublicDevicePlaybackRegistry::describeOwned` and current grants.
   For an outstanding index job it invokes the existing
   `RecordingMediaSessionController::playbackStatus` with verified
   actor and emits **only the safe Device session id/backend/profile
   and playback facts**, omitting native Recording ID, Browser media
   path, access credentials, and provider internals.
4. If no index job is pending, the Device GET retains its original
   response shape for backward compatibility.
5. Tests include preserving existing index bytes, legacy index and
   dangling symlinks; Public Device create duration/preparing response.

**Important limitation:** The filesystem existence checks are not an
atomic guarantee against an unrelated process creating an index between
the final check and the VDR executable's `unlink()`. They narrow the
race substantially but are not a strict no-overwrite guarantee.
If an absolute no-overwrite guarantee is demanded, use a different
index-generation mechanism with atomic no-replace semantics or
additional coordination with VDR, not this command. No automatic
rewriting of pre-existing indexes is supported here.

This C++ branch has **not passed a full VDR-Suite build/CI or
hardware acceptance**. Never deploy it without compiling, reviewing
security tests, confirming indexed duration and index creation on
isolated synthetic TS fixtures, and obtaining explicit approval.
Do not call `--updindex` or `--genindex` on the production server
as part of any diagnostic.

VIDAA counterpart branch `work/vidaa-index-status-20261011`:
`b20261011.3` reuses the existing Public-v1 session/client and
polls this authorized Device GET every 750ms only while its
`resume.preparing` is true. Real duration is displayed after the
server confirms completion; unknown remains `--:--`. Poll timers
are cancelled on STOP, BACK and MediaSession replacement. The
existing silent detail-view prewarm, MSE, OK pause/resume and seek
architecture are unchanged. Client CI and server CI are separate gates.

## Deployment and acceptance gates

- Inspect the selected actual recording **read-only** first to
  distinguish absent index, existing invalid index, and Public-v1
  projection/cache mismatch. Do not assume `--:--` proves absence.
- Verify downstream yaVDR packaging has not changed VDR behavior.
- Confirm the two isolated branches are fully compiled/tested; only
  then create a reviewed upstream change for server distribution.
- No daemon/service/server/recording/permissions modification without
  a user-approved rollback plan. Static VIDAA deployment must follow
  the existing content-only + metadata-invariant backup/HTTPS protocol.
