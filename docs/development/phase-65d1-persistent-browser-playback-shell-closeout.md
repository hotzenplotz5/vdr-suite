# Phase 65.D.1 Persistent Browser Playback Shell Closeout

Status: **Accepted and closed for the bounded Phase-65.D.1 scope.**

## Scope

Phase 65.D.1 is the first bounded product slice of Phase 65.D Client playback abstraction. It establishes one persistent browser playback owner across internal first-party navigation without creating a second MediaSession, exposing provider-native URLs or replacing the platform playback engine.

The accepted browser flow is:

```text
Suite MediaSession
  -> exact existing HTMLMediaElement
  -> Live-TV view while Live TV owns presentation
  -> persistent playback shell during internal navigation
  -> native browser / Android Picture-in-Picture when requested
  -> same HTMLMediaElement restored to the current Suite presentation owner
```

This closeout does **not** close the complete Phase 65.D Client playback abstraction vertical and does not close Phase 65. Further semantic playback-controller work remains governed by ADR-0053 and the Strict Roadmap.

## Binding architecture

No new ADR is introduced. The slice remains inside:

- ADR-0046 — Streaming Gateway and Media Session Boundary;
- ADR-0053 — Client Playback Engine and Media Adaptation Strategy;
- ADR-0055 — Media Transcode Backend Selection and Hardware Acceleration.

The server remains authoritative for MediaSession, route/provider ownership, selected media profile and cleanup. The browser remains responsible for the platform media element and native Picture-in-Picture integration. The persistent shell is presentation ownership around the existing playback element, not a new decoder, media route or provider contract.

## Accepted runtime candidate

```text
accepted_65d1_runtime_candidate=eec2f218b19aeb7ac3265fce1aaaa967ed9571b6
source_ci_workflow=VDR-Suite CI
source_ci_run_number=8012
source_ci_run_id=32600384269
source_ci_result=PASS
source_ci_url=https://github.com/hotzenplotz5/vdr-suite/actions/runs/32600384269
```

The hosted CI passed on the exact runtime candidate before real-system installation and acceptance.

## Product result

The accepted implementation provides:

- a dedicated Live-TV first-party view whose channel tile starts playback immediately;
- one persistent browser playback shell for an already-running Live MediaSession during internal Suite navigation;
- transfer of the exact existing `HTMLMediaElement` between the Live-TV view and persistent shell instead of rebuilding playback;
- continued MediaSession and stream ownership while navigating to other Suite views;
- shell controls for Live-TV return, play/pause, fullscreen, native Picture-in-Picture and stop;
- deterministic presentation ownership when playback returns to the Live-TV view;
- no stale duplicate shell resurrection after ownership has transferred elsewhere;
- no change to public provider privacy, MediaSession authorization or selected media-profile policy.

## Native Picture-in-Picture stabilization

The final stabilization fixed a real Android/browser regression in the persistent shell.

`enterpictureinpicture` and `leavepictureinpicture` are dispatched on the video element and do not bubble. The compatibility hook had registered its listeners on `document` without capture, so the native PiP window could open while the large persistent shell remained visible as a duplicate presentation.

The accepted fix keeps the existing ownership design and registers the document-level compatibility listeners in the capture phase. Regression coverage explicitly requires capture for both PiP lifecycle events.

Accepted behavior is:

```text
native PiP enter
  -> same video continues in platform PiP
  -> persistent Suite shell is hidden entirely

native PiP leave
  -> shell is restored only if it still owns that same video
  -> no MediaSession restart
  -> no stale shell resurrection after ownership transfer
```

## Real yaVDR installation identity

The exact candidate was installed from the established checkout `/home/yavdr/vdr-suite` on the real yaVDR host.

Observed installation evidence:

```text
INSTALL_IDENTITY=PASS
SOURCE_HEAD=eec2f218b19aeb7ac3265fce1aaaa967ed9571b6
VDR_SERVICE=active
VDR_SUITE_DAEMON_SERVICE=active
SUITEBRIDGE_PLUGIN=loaded
LIVE_SOCKET_ROOT=/run/vdr/vdr-suite-live
LIVE_SOCKET_ROOT_OWNER=vdr:vdr
LIVE_SOCKET_ROOT_MODE=0700
```

The installed daemon matched the branch build artifact, the installed SuiteBridge shared object matched the branch build artifact, and the installed `live-tv-view.js`, `channel-day-program-compat.js`, `api/session-frontend-sync.js` and `recordings2.js` matched the exact source candidate.

The idle live-socket directory was empty as expected. A per-lease Unix socket is created only for an active Live-TV source session and is removed again when that session is stopped.

No new Phase-65.D.1/SuiteBridge startup failure was observed in the scoped service check. Unrelated existing VDR/RESTfulAPI subtitle lookup, EGL and TVScraper diagnostics were not treated as evidence for or against this playback slice.

## Real Android/browser acceptance

The exact installed candidate was exercised from the real Android browser against the real yaVDR runtime.

Observed acceptance:

```text
LIVE_TV_PICTURE_SOUND=PASS
INTERNAL_NAVIGATION_STREAM_CONTINUITY=PASS
PERSISTENT_SHELL_PRESENT_OFF_LIVE_VIEW=PASS
NATIVE_ANDROID_PIP=PASS
PERSISTENT_SHELL_HIDDEN_DURING_PIP=PASS
PIP_PICTURE_SOUND_CONTINUITY=PASS
INTERNAL_NAVIGATION_DURING_PIP=PASS
SAME_SHELL_RESTORED_AFTER_PIP=PASS
MEDIASESSION_RESTART_ON_PIP_EXIT=none_observed
LIVE_VIEW_REATTACH=PASS
STALE_MINI_PLAYER_RESURRECTION=absent
```

The real device showed native Android Picture-in-Picture while the Suite page contained no duplicate large player shell. Navigation continued while PiP remained active. Leaving PiP restored the persistent shell when that shell still owned the video. Returning to the Live-TV view transferred presentation ownership back to the Live-TV view without resurrecting an obsolete mini-player.

## Post-closeout Live-TV lifecycle stability correction

A later post-Phase-66 real-system test exposed a bounded lifecycle regression in the persistent browser playback shell: Live-TV could stop after running for a while even though the VDR receiver and media worker had not independently failed.

Failure evidence from the real yaVDR system showed the Live MediaSession terminating with:

```text
terminal_reason=client_closed
```

SuiteBridge logged a normal `live-source event=close` at the same boundary. This proved that the browser/frontend path was actively closing the canonical Live MediaSession instead of the provider or worker failing first.

The root cause was the shell-level listener in `web/frontend/channel-day-program-compat.js`: it treated the live `HTMLMediaElement` `ended` event as an explicit terminal boundary and called `stop('playback_ended')`. For an open-ended MSE Live stream, `ended` is not equivalent to a user-requested stop; a transient transport EOF can surface as `ended` while the Suite MediaSession is still the canonical owner. Calling `stop()` destroyed the underlying playback adapter and converted the backend terminal reason to `client_closed`.

PR #289 (`Fix Live TV ended lifecycle`) removes `ended` as a shell stop boundary while preserving the established real stop/fencing boundaries:

- explicit Live-TV stop;
- backend change;
- browser-session loss;
- sender replacement / handoff;
- fatal player `error` handling.

Regression coverage in `web/frontend/tests/test_channel_day_program_compat_runtime.js` now dispatches `ended` on an active Live player and requires that:

- the underlying playback adapter is not destroyed;
- the playback shell remains active;
- the same Live MediaSession id remains owned;
- no synthetic `lastStopReason` is introduced.

Real yaVDR validation on branch head `fb55975787b6259c84b4fee0822855fbb23e60f1` passed:

```text
node --check web/frontend/channel-day-program-compat.js = PASS
node web/frontend/tests/test_channel_day_program_compat_runtime.js = PASS
runtime_result = persistent Live playback shell lifecycle ok
LIVE_TV_STABILITY_AFTER_FIX = PASS (observed continuously for at least 10 minutes)
PREVIOUS_CLIENT_CLOSED_ABORT = not reproduced during the acceptance interval
```

The fix was merged through PR #289 to `main` as merge commit `7d850123aaa1d04362d1a94ce584a4cc3c3a5b3b` on 2026-09-17. This is a post-closeout correctness fix and does not reopen Phase 65.D.1 or alter the MediaSession/provider architecture.

## Safety and lifecycle boundary

The accepted slice preserves the established media architecture:

- the same authorized MediaSession remains active across internal navigation;
- no provider-native URL or credential becomes public;
- PiP is a platform presentation state and does not create a second route or lease;
- shell hide/show does not stop, replace or recreate the stream;
- ownership transfer moves the existing media element instead of creating parallel playback;
- stop remains an explicit MediaSession lifecycle action;
- Phase 66 runtime is not started.

## Acceptance conclusion

Phase 65.D.1 **Persistent Browser Playback Shell** is accepted and closed for its bounded scope on runtime candidate `eec2f218b19aeb7ac3265fce1aaaa967ed9571b6`.

The accepted runtime evidence remains valid for documentation-only follow-up commits because no product, packaging, installation or runtime input changes in such a follow-up. Full Phase 65.D remains open until its remaining client-playback-abstraction requirements are implemented and accepted; Phase 66 remains blocked until Phase 65 closes and is separately authorized.
