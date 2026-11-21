# Phase 65.D Browser-local Volume/Mute Closeout

Status: **Accepted and closed for the bounded browser-local Volume/Mute scope.**

## Scope

This Phase-65.D client-playback slice adds browser-local Volume/Mute semantics to the existing Suite playback owners without introducing another player, MediaSession or server-side volume domain.

The accepted ownership flow is:

```text
existing Recording or Live playback owner
  -> currently active owned HTMLMediaElement
  -> Suite-owned 0..100 Volume UI / mute toggle
  -> HTMLMediaElement.volume / .muted
  -> volumechange readback
```

ADR-0053 classifies volume as transient client-local player state. The server remains authoritative for MediaSession, route/provider selection, media adaptation and cleanup; the browser media element remains authoritative for the actual local volume/mute state.

This closeout does **not** close all of Phase 65.D. Discontinuity handling, classified playback failures and demonstrated additional client gaps remain open. Phase 66 remains blocked.

## Accepted runtime candidate

The product behavior was accepted on:

```text
accepted_65d_volume_mute_runtime_candidate=932aef5cd6e85b0fac1a5bf290a4bbeb06ff2d4b
source_ci_workflow=VDR-Suite CI
source_ci_run_number=8238
source_ci_run_id=32877244600
source_ci_url=https://github.com/hotzenplotz5/vdr-suite/actions/runs/32877244600
source_ci_result=PASS
```

The complete hosted CI graph passed on that exact runtime candidate.

## Product result

The accepted implementation provides:

- one visible Suite 0..100 Volume control shared by Recording and Live playback factories;
- mute/unmute on the active owned `HTMLMediaElement`;
- `volumechange` synchronization so the media element remains source of truth;
- confirmed client-local state preservation across replaceable Recording media-element ownership changes;
- state preservation across persistent Live presentation reparenting and clean Live-owner replacement;
- no second media element and no second MediaSession;
- no playback restart caused by Volume/Mute changes;
- no VDR system-volume mutation and no server-side volume endpoint;
- no browser-brand/user-agent routing;
- fail-safe readback when a platform rejects or does not reflect a requested volume/mute write;
- provider/PID privacy unchanged.

## Browser-freeze regression found during acceptance

The first installed candidate `40f64007a1c9f170f31ee807496f6398f229be4d` exposed a real browser freeze. The Volume/Mute `MutationObserver` observed the complete outer owner shell, including the control nodes that its own callback updated. Real browser `textContent` mutations could therefore retrigger the observer continuously even though the original synthetic test model did not reproduce that behavior.

The accepted runtime candidate fixes the lifecycle contract by keeping the observer broad enough to detect real transport/panel replacement while making the callback inert when the active media element has not changed. The regression test was upgraded to model observer-triggered DOM mutations so the self-triggering loop cannot silently return.

## Real yaVDR/browser acceptance

Real acceptance on the exact runtime candidate proved the bounded Volume/Mute contract together with the required neighboring playback behaviors:

```text
RECORDING_START=PASS
RECORDING_VOLUME_100_30_70=PASS
RECORDING_MUTE_UNMUTE=PASS
RECORDING_SEEK=PASS
RECORDING_SRT=PASS
HLS_COMPATIBILITY_PATH=PASS
HLS_STOP_RESUME_UI=PASS
LIVE_TV_VOLUME_MUTE=PASS
BROWSER_FREEZE_REGRESSION=PASS
```

The real compatibility/HLS path retained the existing Stop/Resume UI and the shared Volume/Mute owner. Live-TV remained playable with the common client-local control layer.

## Separately demonstrated progressive-MSE gap

During broader acceptance/soak testing, the normal progressive-fMP4 path also demonstrated a separate pre-existing browser transport gap:

```text
Failed to execute 'appendBuffer' on 'SourceBuffer':
The SourceBuffer is full, and cannot free space to append additional buffers.
```

The same continuous-fMP4 pump shape already existed on the previously accepted Phase-65.D.2 runtime candidate: it trims old history but has no browser-side forward-buffer/backpressure bound before continuing to read and append the HTTP stream. The error was reproduced again after Recording index generation completed, while the HLS compatibility path retained working Stop/Resume semantics.

Therefore this observation is **not attributed to the Volume/Mute implementation and is not hidden inside this PR**. It is a demonstrated additional Phase-65.D client-playback gap: browser-side continuous-fMP4 MSE forward-buffer/backpressure must be handled in its own coherent playback block.

The earlier Phase-65.C `HTTP_BACKPRESSURE_LONG_PLAYBACK=PASS` evidence concerns the server/HTTP slow-reader resource boundary; it does not prove that the browser MSE `SourceBuffer` itself has bounded forward buffering.

## Acceptance conclusion

Browser-local Volume/Mute is accepted and closed for its bounded Phase-65.D scope on runtime candidate `932aef5cd6e85b0fac1a5bf290a4bbeb06ff2d4b`.

Documentation-only closeout commits after this runtime candidate do not change the accepted installed playback implementation and therefore do not invalidate the real-system evidence. Full Phase 65.D remains open for discontinuity handling, classified playback failure behavior and the now-demonstrated continuous-fMP4 MSE forward-buffer/backpressure gap. Phase 66 remains blocked.
