# Phase 65.D.2 Recording Playback Controls and Seek Closeout

Status: **Accepted and closed for the bounded Phase-65.D.2 scope.**

## Scope

Phase 65.D.2 extends the accepted Phase 65.D.1 persistent browser playback ownership with truthful Recording playback controls and position semantics. It does not create a second player architecture, does not expose provider-native URLs and does not add Live-TV timeshift.

The accepted Recording flow is:

```text
Suite Recording
  -> authorized MediaSession
  -> selected delivery profile
  -> browser playback owner
  -> play / pause / stop
  -> truthful position + Recording duration
  -> supported seek
  -> stop choice: resume saved position or start from beginning
```

For the normal completed-Recording progressive-fMP4 path, seek repositions the existing authorized Recording MediaSession worker while preserving the Suite-owned session/gateway boundary. For the HLS/transcoding compatibility path, arbitrary time movement is implemented truthfully as restart-seek: the current fallback transport is replaced with a fresh authorized HLS MediaSession started at `startPositionSeconds`.

This closeout does **not** close all of Phase 65.D. ADR-0053 still owns remaining client-playback semantics such as normalized audio/subtitle selection, discontinuity handling and classified playback failure behavior. Phase 66 remains blocked.

## Binding architecture

No new ADR is introduced. The slice remains inside:

- ADR-0046 — Streaming Gateway and Media Session Boundary;
- ADR-0053 — Client Playback Engine and Media Adaptation Strategy;
- ADR-0055 — Media Transcode Backend Selection and Hardware Acceleration.

The server remains authoritative for Recording identity, MediaSession ownership, selected profile, provider privacy and cleanup. Browser controls act only through the Suite playback/session contract. Platform decode/render execution remains browser-owned.

## Accepted runtime candidate

The product behavior was accepted on:

```text
accepted_65d2_runtime_candidate=fd1e64c3c28b3e184fb120d71ce692061b282c82
backend_runtime_foundation_candidate=df126edc5ff3298e737b8bc92f1349029346969c
source_ci_workflow=VDR-Suite CI
source_ci_run_number=8086
source_ci_run_id=32691361919
source_ci_url=https://github.com/hotzenplotz5/vdr-suite/actions/runs/32691361919
frontend_regression=PASS
packaging_regression=PASS
```

The later workflow-governance and Make/test-registration commits do not change the accepted playback product/runtime implementation and therefore do not invalidate this real-system evidence.

## Product result

The accepted implementation provides:

- Recording Play, Pause and Stop controls on the owned browser playback surface;
- truthful current position and authoritative completed-Recording duration derived from Recording/index truth rather than EPG event duration;
- relative `-60`, `-10`, `+10`, `+60` second movement;
- interactive timeline seek;
- direct absolute time seek;
- stop-state choice between resuming at the captured position and starting from the beginning;
- browser reconnect/replacement behavior after a successful progressive-fMP4 seek;
- HLS/transcoding fallback duration and position without pretending that a rolling HLS buffer itself provides arbitrary random access;
- HLS resume by creating a fresh authorized fallback MediaSession at the saved `startPositionSeconds`;
- HLS restart-seek for relative buttons, timeline and direct time entry through the same start-position contract;
- mobile numeric time entry that does not require Android keyboards to expose a colon;
- explicit preservation of provider privacy, authorization and deterministic cleanup.

## Progressive-fMP4 acceptance

The normal completed-Recording path passed real yaVDR/browser acceptance for:

```text
RECORDING_DURATION_TRUTH=PASS
PLAYBACK_CONTROLS_VISIBLE=PASS
PLAY_PAUSE_STOP=PASS
RELATIVE_SEEK=PASS
TIMELINE_SEEK=PASS
DIRECT_ABSOLUTE_SEEK=PASS
SECOND_HALF_SEEK=PASS
STOP_POSITION_CAPTURE=PASS
RESUME_FROM_STOP_POSITION=PASS
START_FROM_BEGINNING_CHOICE=PASS
```

The implementation does not advertise HTTP byte-range semantics on continuous fMP4 merely because time-seek is available through the owned MediaSession operation. Those contracts remain distinct.

## HLS / transcoding fallback acceptance

Some Recordings require the compatibility HLS/transcoding path. Phase 65.D.2 keeps that path explicit rather than silently degrading the controls contract.

The real runtime showed truthful Recording duration and accepted restart-seek behavior:

```text
HLS_CONTROLS_VISIBLE=PASS
HLS_RECORDING_DURATION=PASS
HLS_STOP_POSITION_CAPTURE=PASS
HLS_RESUME_CHOICE_VISIBLE=PASS
HLS_RESUME_AT_SAVED_POSITION=PASS
HLS_RELATIVE_SEEK=PASS
HLS_TIMELINE_SEEK=PASS
HLS_DIRECT_ABSOLUTE_SEEK=PASS
HLS_RESTART_SEEK=PASS
```

Observed acceptance included a completed Recording with duration `01:32:38`. Stop captured the live position, resume restarted playback around that position, and subsequent `+/-` controls, timeline movement and direct `HH:MM:SS` movement all worked through fresh HLS session starts.

The HLS implementation therefore no longer reports arbitrary seek as unsupported merely because the rolling browser HLS buffer itself cannot seek the whole source. It exposes the capability only because the Suite server now has a truthful Recording-index duration and an explicit fresh-session start-position contract.

## Android direct-time entry

Android numeric keyboards may omit `:`. The accepted browser adapter formats numeric entry without requiring a keyboard punctuation mode.

Examples:

```text
01      -> 01:
0135    -> 01:35
0135 + Springen -> 01:35:00
013543  -> 01:35:43
```

Backspace remains usable across automatically inserted separators, and explicit desktop/pasted `HH:MM:SS` input remains valid.

Real Android acceptance confirmed the automatic colon behavior and successful direct seek.

## Truthfulness and deferred boundary

Phase 65.D.2 deliberately distinguishes implemented capability from still-deferred capability:

- completed Recordings with the accepted progressive-fMP4 or HLS restart-seek contracts expose the seek behavior proven above;
- a growing Recording does not become immutable merely to expose the same controls;
- Live-TV playback does not acquire timeshift semantics;
- provider-native paths, filesystem paths and credentials remain private;
- track-selection, discontinuity and classified player-failure semantics remain later Phase-65.D work under ADR-0053;
- no Phase-66 runtime work is started.

## Workflow stabilization

During closeout, a CI-only flake exposed that `test-make-inventory` and the systemd/install contract were still attached to `test-ci-fast` before a later `filter-out` assignment could remove them. GNU Make expands the target prerequisites when that target definition is parsed, so the late filtering did not have the intended effect.

The fix moves the filtering to immediately before `test-ci-fast` expands `CI_FAST_TESTS`. The dedicated `make-test-audit` and packaging jobs continue to own those checks. This removes duplicate concurrent execution and preserves the repository rule that the complete graph is required for merge/closeout while iterative surface validation remains scoped to the changed surface.

## Acceptance conclusion

Phase 65.D.2 **Recording Playback Controls and Seek** is accepted and closed for its bounded scope on runtime candidate `fd1e64c3c28b3e184fb120d71ce692061b282c82`.

The accepted runtime evidence remains valid for later documentation, workflow-governance and test-registration-only commits because those changes do not alter the installed playback product surface. Full Phase 65.D remains open for the remaining ADR-0053 client-playback semantics. Phase 66 remains blocked until Phase 65 is completely closed and separately advanced.
