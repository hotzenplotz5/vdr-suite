# Phase 65.B Live-TV Playback Closeout

Status: **Accepted and closed for the bounded Phase-65.B scope.**

## Binding architecture

Phase 65.B implements existing accepted architecture and does **not** introduce a new ADR.

Binding decisions remain:

- ADR-0017 — Live Transport Boundary;
- ADR-0046 — Streaming Gateway and Media Session Boundary;
- ADR-0053 — Client Playback Engine and Media Adaptation Strategy.

ADR-0053 already requires platform playback engines, capability-driven profile selection and the least-transformation preference `pass-through -> remux/repackage -> transcode`. The single-consumer Live-TV design is an implementation/safety consequence of those decisions, not a new public architecture boundary.

## Accepted implementation candidate

```text
accepted_65b_runtime_candidate=7da9a3defc87b9442f1f75f90fb67ac514fd10cd
source_ci_workflow=VDR-Suite CI
source_ci_run_number=7966
source_ci_run_id=32303041048
source_ci_result=PASS
source_ci_attempt=3
merge_pr=205
```

The first two CI attempts were cancelled while the GitHub-hosted Ubuntu runner was stalled in package installation against the Azure/Ubuntu mirror. No product or packaging test had failed. The third attempt completed the required test graph successfully, including packaging/install staging, fast regression, daemon build, architecture, frontend, documentation and Make test-audit jobs.

## Product result

The accepted browser Live-TV path is:

```text
VDR
  -> SuiteBridge native live receiver
  -> bounded TS/replay conditioning
  -> exactly one FFmpeg socket consumer
  -> browser-safe fragmented MP4
  -> Streaming Gateway
  -> first-party browser player
```

A separate `ffprobe` socket consumer is intentionally absent from the Live-TV hot path. The earlier `ffprobe -> disconnect -> FFmpeg reconnect` sequence could consume/restart the replay boundary at an unsafe point and produced timing-dependent zap/start failures.

For the first-party browser profile, the one continuous FFmpeg process normalizes video to H.264 and audio to AAC. `bwdif=...:deint=interlaced` acts only on frames marked interlaced, so the browser path does not require a separate pre-probe merely to classify progressive versus interlaced source material.

This browser compatibility profile is not the universal Media Plane contract. Later Android, Windows, Kodi-style and other native clients remain capability-driven and may use cheaper pass-through/remux profiles when their platform playback engine can consume the native source safely.

## Real yaVDR acceptance

The exact runtime candidate `7da9a3defc87b9442f1f75f90fb67ac514fd10cd` was installed on the real yaVDR acceptance host.

Observed acceptance:

```text
LIVE_TV_PICTURE_SOUND=PASS
LIVE_TV_REPEATED_ZAPPING=PASS
ZAP_SEQUENCE=Pro7->ZDF->RTL->Pro7->NDR->Pro7
PRO7_15_MINUTE_STABILITY=PASS
PRO7_REENTRY=PASS
RTL_PLAYBACK=PASS
ZDF_PLAYBACK=PASS
NDR_PLAYBACK=PASS
LIVE_HOT_PATH_FFPROBE=absent
VDR_RESTART_DURING_FINAL_ACCEPTANCE=none_observed
```

The previously observed VDR restart under Live-TV stress was not reproduced after the single-consumer fix. This is consistent with the removal of extra consumer open/close/reconnect churn, but the earlier crash had no captured stack trace or coredump, so the causal claim remains deliberately limited to **not reproducible on the accepted candidate under the previously problematic playback/zap stress**.

A later no-crash journal sample retained the same VDR process PID while SuiteBridge continued servicing normal SVDRP metadata/snapshot traffic without overflow, abort, segfault or fatal-error evidence.

## Safety and lifecycle evidence

The accepted vertical preserves the Phase-65 MediaSession/Gateway boundaries:

- stable Suite Channel identity resolves to an explicitly owned private live provider;
- provider-native socket paths remain private;
- MediaSession / MediaRoute / ProviderStreamLease / MediaAccessGrant remain the authorization and lifecycle boundary;
- channel replacement explicitly closes the previous Live session/resource ownership;
- disconnect/revoke/expiry and daemon shutdown keep deterministic cleanup ownership;
- VDR callbacks do not wait on browser/network reads;
- Live receive buffering remains bounded;
- no HLS readiness barrier or browser prebuffer is required for the direct Live-TV hot path;
- no Phase-66 runtime work is included.

## Acceptance conclusion

Phase 65.B is closed for its bounded Live-TV browser product scope.

The next strict Phase-65 vertical is **65.C — Recording seek and growing-recording semantics**. Recording startup/performance work may proceed only as a coherent implementation of the already accepted Phase-65 architecture, including truthful range/seek semantics and the existing `progressive-direct` least-transformation path. Phase 66 remains blocked.
