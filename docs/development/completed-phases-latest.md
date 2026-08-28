# Completed Phases Latest Marker

## Latest completed numbered runtime phase

```text
Phase 65 - Streaming Gateway and Media Sessions
```

Phase 65 is completed. It establishes the authenticated Recording/Live MediaSession and Streaming Gateway boundary, least-transformation media delivery/output policy and the normalized persistent first-party playback semantics required by ADR-0056. The final runtime-sensitive follow-up was the bounded completed-Recording network-interruption recovery accepted through PR #228.

Final Phase-65 runtime marker:

```text
accepted_final_phase65_runtime_candidate=7193797368cd1ff637062d02d0d7c9e5bf435ebe
source_ci_run_number=8303
source_ci_run_id=33166818230
source_ci_result=PASS
YAVDR_EXACT_INSTALL_RUNTIME_IDENTITY=PASS
REAL_ANDROID_EDGE_LONG_OUTAGE_RECOVERY=PASS
NETWORK_RECOVERY_USER_ACTION_REQUIRED=NO
merge_pr=228
merge_commit=131f669c0f4e360f3306cfb34f50380653a9fdfc
```

See [Phase 65 Final Closeout](phase-65-closeout.md).

## Previous completed numbered runtime phases

### Phase 64 - Timer Intent and Multi-Backend Orchestration

Phase 64 established TimerIntent -> TimerAssignment -> NativeTimerBinding, deterministic multi-backend assignment, managed native Timer fulfillment, authoritative readback/reconciliation and controlled reassignment/failover.

### Phase 63 - Backend Agent and Secure Multi-Site Runtime

Phase 63 established secure Agent enrollment/identity, backend generation and lease fencing, observation ingestion, durable command/result handling, fenced native execution, explicit provider ownership/selection and the generic protected-write foundation used by Phases 64 and 65.

### Phase 62 - Identity, RBAC and Accountability Foundation

Phase 62 established persistent actor/device/session/credential identity, scoped server-side authorization, browser-session lifecycle and CSRF policy, protected central mutations and append-only decision/outcome evidence.

Legacy Basic compatibility remains transitional. Its removal requires a separate deployment-migration contract and does not reopen Phase 62.

## Completed non-numbered platform work

Historical completed non-numbered work includes:

```text
Post-Phase 61 Performance Hardening (B1-B4)
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)
Configurable photorealistic VDR Remote (#115)
TVScraper classification and refresh corrections (#118)
Public-base-path-safe EPG artwork delivery (#123)
Guarded external series-artwork fallback and backend settings (#132)
Channel-detail artwork/text layout correction (96b97378 / 2d04a963)
```

These features do not create new numbered phases.

## Next strict runtime phase

```text
Phase 66 - Media Home and Browse Experience
```

Phase 66 is next but **not started**. Accepted ADR-0058 defines its Media Home / Browse architecture, but the Phase-65 closeout does not authorize Media Home runtime implementation. A separate explicit kickoff is required.

## Evidence boundary

Historical acceptance hashes remain tied to their accepted candidates. The documentation-only Phase-65 closeout does not change the final accepted Phase-65 runtime tree. Later daemon, client or broadcast work requires its own CI and runtime evidence and does not rewrite historical Phase-62 through Phase-65 completion records.

Optional read-only media pipeline diagnostics and shared fMP4/MSE helper deduplication are not Phase-65 completion gates. Growing-Recording seek and Live-TV timeshift remain truthful deferred capability work rather than unfinished Phase 65.

## Maintenance rules

- Keep this marker aligned with CURRENT, Handoff, Roadmap, Phase Map and Current Status.
- Keep numbered phases, non-numbered hardening and cross-cutting completed features distinguishable.
- Do not promote planned phases or accepted ADRs to completed runtime without implementation and evidence.
- Keep historical acceptance heads/hashes separate from later runtime fingerprints.
- Update the matching closeout whenever a phase or bounded platform slice closes.
- Do not treat Phase 66 as started until an explicit Phase-66 kickoff is recorded.
