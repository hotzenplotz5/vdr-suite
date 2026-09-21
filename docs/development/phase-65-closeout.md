# Phase 65 Final Closeout — Streaming Gateway and Media Sessions

## Status

**Phase 65 is completed.**

Phase 65 closes the authenticated VDR-Suite Media Plane and the bounded first-party playback semantics required by ADR-0046, ADR-0053, ADR-0055 and ADR-0056. The final runtime-sensitive follow-up was the completed-Recording network-interruption recovery accepted through PR #228.

This closeout does **not** start Phase 66. Phase 66 remains the next numbered runtime phase and requires a separate explicit kickoff before any Teletext/HbbTV runtime implementation begins.

## Final repository lineage

The final runtime-sensitive accepted candidate was:

```text
accepted_final_phase65_runtime_candidate=7193797368cd1ff637062d02d0d7c9e5bf435ebe
source_ci_workflow=VDR-Suite CI
source_ci_run_number=8303
source_ci_run_id=33166818230
source_ci_result=PASS
merge_pr=228
merge_commit=131f669c0f4e360f3306cfb34f50380653a9fdfc
```

PR #228 merged the final accepted Phase-65 runtime tree into `main` after exact yaVDR install/runtime identity verification and real Android/Edge long-network-outage acceptance.

The documentation-only closeout that records Phase-65 completion does not alter that accepted runtime tree and therefore does not invalidate its real-system evidence.

## Accepted architecture boundary

Phase 65 establishes the Suite-owned media flow:

```text
private Recording / Live source
  -> explicitly owned StreamProvider
  -> ProviderStreamLease
  -> least-transformation media adaptation
  -> Streaming Gateway / MediaSession
  -> short-lived MediaAccessGrant
  -> normalized MediaPlaybackContract
  -> persistent client playback owner
  -> replaceable transport adapter
  -> platform playback engine
```

The completed boundary preserves these invariants:

- clients never receive permanent provider-native Streamdev, SuiteBridge, filesystem or site-local media URLs;
- provider reachability never creates provider authority;
- active routes do not silently switch providers;
- backend generation, provider generation, route epoch and playback presentation generation remain explicit and distinct fences/concepts;
- `mediaSessionId` is an identifier rather than a bearer credential;
- the server owns authorization, source/provider routing, adaptation selection and deterministic lifecycle cleanup;
- first-party clients consume normalized playback semantics while platform-native playback engines remain responsible for decode/render execution;
- transformation remains `pass-through -> remux/repackage -> transcode`, except where a demonstrated operation requires stronger adaptation for correctness and the path remains policy-governed/fail-closed;
- unsupported range, seek, growing-Recording or timeshift capability remains explicitly unsupported rather than fabricated;
- playback failures are classified without creating silent provider/profile/session fallback authority.

## Completed Phase-65 product verticals

### 65.A — Existing-Recording playback

Phase 65.A proved the authenticated existing-Recording media path through the Suite MediaSession/Gateway boundary with real browser picture and sound, least-transformation adaptation, interlace/UHD handling, graceful cleanup and server-owned hard-disconnect cleanup.

Durable evidence is recorded in [Phase 65 Recording Playback Closeout](phase-65-recording-playback-closeout-readiness.md).

Key final lifecycle evidence:

```text
accepted_65a_lifecycle_candidate=485c990c9f5692f00aa0e2e087967b236676c154
source_ci_run_number=7881
source_ci_run_id=32176309565
source_ci_result=PASS
ACTIVE_PLAYBACK_NOT_REAPED=PASS
HARD_DISCONNECT_IDLE_EXPIRY=PASS
FFMPEG_CLEANUP=PASS
WORKSPACE_CLEANUP=PASS
```

### 65.B — Live-TV playback

Phase 65.B proved the same public media architecture for Live TV, including explicit provider ownership, one continuous FFmpeg consumer on the accepted browser hot path, repeated channel replacement and deterministic old-route cleanup.

Durable evidence is recorded in [Phase 65.B Live-TV Playback Closeout](phase-65-live-tv-closeout.md).

```text
accepted_65b_runtime_candidate=7da9a3defc87b9442f1f75f90fb67ac514fd10cd
source_ci_run_number=7966
source_ci_run_id=32303041048
source_ci_result=PASS
LIVE_TV_PICTURE_SOUND=PASS
LIVE_TV_REPEATED_ZAPPING=PASS
PRO7_15_MINUTE_STABILITY=PASS
LIVE_HOT_PATH_FFPROBE=absent
```

### 65.C — Recording delivery performance and media output/transcode settings

Phase 65.C closed the demonstrated completed-Recording startup/performance and backend-scoped output-policy gaps.

The first block introduced truthful `progressive-direct`/`progressive-fmp4` completed-Recording delivery with HLS retained as compatibility fallback:

```text
accepted_65c_startup_candidate=51de13337edd0a072308a9df1bad6e245a764ac2
source_ci_run_number=7972
source_ci_run_id=32350815560
source_ci_result=PASS
merge_pr=206
merge_commit=0513edf6166e096aa60cf313b74a43073cacd786
```

The second block introduced backend-scoped `auto` / `software` / `vaapi` output policy under ADR-0055 with calibrated selection and fail-closed forced-VAAPI behavior:

```text
accepted_65c_output_policy_candidate=85478311b9af6c027a25980272a2acde551e5508
source_ci_run_number=7976
source_ci_run_id=32415860281
source_ci_result=PASS
merge_pr=208
merge_commit=8716bbe9f1ab8ebd4cdf597d620419ef0fcf098a
```

### 65.D — Client playback abstraction

Phase 65.D closes the bounded first-party playback abstraction around mature platform playback engines rather than introducing a universal Suite decoder/player core.

Accepted work includes:

- Persistent Browser Playback Shell;
- Recording Play/Pause/Stop, position, seek and stop/resume semantics;
- normalized Recording audio selection;
- normalized browser-deliverable Recording subtitle selection with explicit OFF semantics;
- browser-local Volume/Mute;
- bounded continuous-fMP4 MSE forward-buffer/backpressure;
- compatibility timeline drag ownership;
- exact non-zero HLS Recording resume synchronization;
- normalized provider-free `MediaPlaybackContract`;
- canonical playback-owner lifecycle snapshot/subscription;
- explicit absolute timeline, continuity/discontinuity and playback-presentation generation semantics;
- classified playback failure semantics;
- bounded completed-Recording network-interruption recovery after demonstrated transport loss.

The ADR-0056 mandatory semantic sequence was completed through:

```text
PR #224 — normalized MediaPlaybackContract
accepted_head=16b90081e2f458d84fccefeece49c70874f3267d
CI #8266 PASS

PR #225 — canonical playback-owner lifecycle publication
accepted_head=ec3af8506eb8086a3f717f977aaca85a93bdb2f3
CI #8280 PASS

PR #226 — timeline + continuity/discontinuity semantics
accepted_head=9cc26870515c32970767e0e418e841fcdbdcba5d
CI #8282 PASS

PR #227 — classified playback failure semantics
accepted_head=64c7f2c7dfdb4d3291d51d23daeec1a300e176a7
CI #8286 PASS
```

The consolidation contract explicitly defines read-only media pipeline diagnostics as recommended follow-up work rather than a Phase-65.D completion gate. Shared fMP4/MSE helper deduplication is likewise technical debt, not required runtime scope.

## Final demonstrated network-recovery follow-up

Real Android/Edge testing exposed a final completed-Recording progressive-fMP4 gap after the mandatory classified-failure slice: a long network outage could drain the browser media buffer into post-first-media `waiting` while the streaming reader remained pending and no browser `MediaError` was emitted. A separate recovery-start race could also interrupt a pending browser `play()` with `pause()`.

The final candidate `7193797368cd1ff637062d02d0d7c9e5bf435ebe` closes both demonstrated causes:

- post-first-media sustained no-progress `waiting` remains ordinary buffering initially and becomes transport-failure evidence only after bounded liveness evaluation plus failed read-only same-origin reachability evidence;
- network/browser hints remain advisory rather than authority;
- no replacement MediaSession is created while the Suite origin remains unreachable;
- the fresh owner-authorized recovery session starts with autoplay suppressed;
- canonical absolute Recording position is restored before the single recovery `play()`;
- no provider switch, hidden HLS escalation or unbounded retry loop is introduced.

Exact yaVDR install/runtime identity passed. Real Android/Edge acceptance then demonstrated without user Play/Restart intervention:

```text
playing
  -> waiting
  -> Verbindung unterbrochen · Wiedergabe wird fortgesetzt, sobald das Netzwerk wieder verfügbar ist.
  -> network restored
  -> fresh authorized recovery session
  -> canonical absolute seek
  -> playing
  -> Verbindung wiederhergestellt · Aufnahme läuft weiter.
```

This final acceptance was performed on the exact candidate subsequently merged by PR #228.

## Phase-65 acceptance gate

The Strict Roadmap requires all supported Phase-65 paths to satisfy fourteen exit conditions. They are closed as follows.

1. **Recording picture + sound:** accepted by Phase 65.A real yaVDR/browser playback.
2. **Live-TV picture + sound:** accepted by Phase 65.B real yaVDR/browser playback.
3. **Deterministic stop/disconnect/revocation cleanup:** accepted by Recording graceful/hard-disconnect lifecycle and Live replacement/cleanup evidence.
4. **No public provider URL/credential leakage:** preserved by all accepted MediaSession/Gateway paths and architecture guards.
5. **Explicit route/provider/generation fencing:** implemented by the accepted MediaRoute/ProviderStreamLease/backend/provider generation contracts.
6. **Pass-through when valid:** preserved by ADR-0053 least-transformation selection.
7. **No unnecessary remux/transcode:** preserved, with exact non-zero HLS video resume as a demonstrated operation-specific stronger-adaptation exception.
8. **Truthful range/seek/growing capability:** completed-Recording seek is implemented on supported profiles; unsupported growing/timeshift/broader index behavior remains explicit non-support.
9. **Normalized MediaPlaybackContract:** completed by PR #224.
10. **Explicit decoder-significant continuity/discontinuity:** completed by PR #226 without conflating MediaSession, route epoch and presentation generation.
11. **Classified browser/client failures without hidden recovery:** completed by PR #227; PR #228 adds one separately authorized bounded recovery policy for demonstrated transient Recording transport loss while retaining fail-closed provider/profile boundaries.
12. **Real yaVDR acceptance of supported runtime-sensitive paths:** recorded across 65.A–65.D, including the final exact-head network-loss acceptance.
13. **Golden User Journeys 1, 2 and media Journey 5:** the implemented browser scope proves real Live TV, real Recording playback/seek/cleanup and failure handling without hidden provider recovery.
14. **Complete repository CI, packaging/install and rollback documentation:** each final runtime candidate passed the applicable full hosted CI/install staging; the final Phase-65 runtime candidate passed CI #8303 including packaging/install staging and daemon build. The closeout records the rollback boundary below, and the documentation-only closeout head must pass the complete repository CI before Phase 65 is marked merged-complete.

## Rollback boundary

This closeout does not create a new runtime or database migration. Rolling back the closeout documentation therefore means reverting only the closeout/status documentation commit.

For the final runtime-sensitive Phase-65 candidate, rollback remains an explicit deployment replacement rather than an in-player hidden fallback:

1. stop/restart through the normal packaged/runtime service boundary;
2. reinstall the previously selected known-good VDR-Suite runtime tree/package as one coherent daemon + frontend set;
3. restart `vdr-suite-daemon`;
4. verify service activity and installed frontend/runtime identity before accepting traffic;
5. do not retain or fabricate a half-upgraded playback bundle;
6. do not convert rollback into a client-side provider/profile/session switch.

The real final-candidate acceptance installation used an isolated staging tree and verified the installed `session-frontend-sync.js` hash byte-for-byte against staging before browser acceptance. That deployment identity discipline remains the required rollback/restore boundary.

## Explicitly deferred / not required to close Phase 65

The following remain outside the completed Phase-65 gate and do not reopen it merely because they are not implemented:

- broad polished Timer UI;
- account/backend-access administration product surface;
- read-only media pipeline diagnostics from optional ADR-0056 Slice 5;
- shared fMP4/MSE helper deduplication;
- user-visible broad growing-Recording seek;
- Live-TV timeshift;
- broader VDR-index mapping beyond the accepted completed-Recording paths;
- Teletext/HbbTV runtime;
- Legacy OSD compatibility;
- public third-party API stabilization;
- recommendation/knowledge-graph runtime.

## Next strict phase — not started

The next numbered runtime phase is:

```text
Phase 66 - Broadcast Companion Services: Teletext and HbbTV
```

Phase 66 is **not started by this closeout**. Accepted ADR-0054 defines its architecture, but no Teletext/HbbTV runtime implementation, branch authorization or product acceptance is created here. A separate explicit kickoff is required before Phase-66 runtime work may begin.

## Related documents

- [Current State](../CURRENT.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Golden User Journeys](../planning/golden-user-journeys.md)
- [Phase 65 Recording Playback Closeout](phase-65-recording-playback-closeout-readiness.md)
- [Phase 65.B Live-TV Playback Closeout](phase-65-live-tv-closeout.md)
- [Phase 65.C Recording Startup / Progressive Direct](phase-65-recording-startup-progressive-direct.md)
- [Phase 65.C Media Transcode Performance / Output Policy](phase-65-media-transcode-performance-policy.md)
- [Phase 65.D.1 Persistent Browser Playback Shell Closeout](phase-65d1-persistent-browser-playback-shell-closeout.md)
- [Phase 65.D.2 Recording Playback Controls and Seek Closeout](phase-65d2-recording-playback-controls-seek-closeout.md)
- [Phase 65.D Browser-local Volume/Mute Closeout](phase-65d-browser-volume-mute-closeout.md)
- [Phase 65.D Playback Semantics Consolidation Contract](phase-65d-playback-semantics-consolidation.md)
- [ADR-0046 Streaming Gateway](../adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0053 Client Playback Engine](../adr/ADR-0053-client-playback-engine-media-adaptation-strategy.md)
- [ADR-0055 Media Transcode Backend Selection](../adr/ADR-0055-media-transcode-backend-selection-hardware-acceleration.md)
- [ADR-0056 Playback Semantics](../adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)
- [ADR-0057 Recording Network Interruption Recovery](../adr/ADR-0057-recording-network-interruption-recovery.md)
