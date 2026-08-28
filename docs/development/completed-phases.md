# VDR-Suite Completed Phases

## Purpose

This is the compact authoritative entry point for completed implementation. Detailed historical records remain in [the completed-phase archive](completed-phases/README.md); future numbered work belongs in the strict roadmap.

## Latest completed markers

```text
Latest completed numbered runtime phase:
Phase 65 - Streaming Gateway and Media Sessions

Final Phase-65 runtime-sensitive accepted candidate:
7193797368cd1ff637062d02d0d7c9e5bf435ebe

Phase-65 final runtime merge on main:
PR #228 -> 131f669c0f4e360f3306cfb34f50380653a9fdfc

Next strict runtime phase:
Phase 66 - Broadcast Companion Services: Teletext and HbbTV

Current active numbered runtime phase:
none - Phase 66 has not started
```

See [Phase 65 Final Closeout](phase-65-closeout.md) for the complete gate, CI and real yaVDR/browser acceptance evidence.

## Completed range overview

| Range / block | Status | Result | Archive / closeout |
| --- | --- | --- | --- |
| Phase 1.x-45.x | Completed | Core platform, database, daemon, VDR adapter, multi-backend reads, Recording actions, runtime hardening and EPG search. | Historical phase records |
| Phase 46 | Completed | Metadata and people foundations. | [Phase 46](completed-phases/phase-46.md) |
| Phase 47-50 | Completed | SearchTimer backend, REST/native validation and controlled workflow. | [Archive](completed-phases/README.md) |
| Phase 51-55 | Completed | Live parity discovery, SearchTimer preview, adapter/runtime hardening and acceptance. | [Archive](completed-phases/README.md) |
| Phase 56 | Completed | Library boundaries, packaging and developer documentation. | [Phase 56](completed-phases/phase-56.md) |
| Phase 57 | Completed | Multi-site backend administration and server-enforced read-only foundation. | [Phase 57](completed-phases/phase-57.md) |
| Phase 58 | Completed slices; historical umbrella retained | Frontend and Live-parity foundation slices. | [Phase 58](completed-phases/phase-58.md) |
| Phase 59.00-59.15e | Completed | Frontend Client API and module ownership. | [Phase 59](completed-phases/phase-59.md) |
| Phase 60.1-60.15 | Completed | Frontend platform, lazy Recording cache, Recordings 2, metadata and authenticated artwork preparation. | [Phase 60](completed-phases/phase-60.md) |
| Phase 61 | Completed | Persistent Recording/EPG metadata, people and Genre platform, query-only browse paths and frontend integration. | [Phase 61 archive](completed-phases/phase-61.md) / [closeout](phase-61-metadata-genre-performance-closeout.md) |
| Phase 62 | Completed | Persistent identity, scoped RBAC, browser-session lifecycle/CSRF, protected central mutations and append-only authorization/outcome evidence. | [Phase 62 closeout](phase-62-closeout.md) / [Slice 2X runtime closeout](phase-62-slice-2x-runtime-closeout.md) |
| Phase 63 | Completed | Secure Backend Agent lifecycle, fenced observation/command/native execution, explicit provider ownership/selection and protected-write foundation. | Phase-63 development/acceptance records |
| Phase 64 | Completed | TimerIntent/TimerAssignment/NativeTimerBinding orchestration, managed native fulfillment, authoritative reconciliation and controlled failover. | [Phase 64 closeout](phase-64-closeout.md) |
| Phase 65 | Completed | Authenticated Recording/Live MediaSession/Gateway playback, least-transformation delivery/output policy and normalized persistent first-party playback semantics with classified failure and bounded Recording network recovery. | [Phase 65 closeout](phase-65-closeout.md) |
| B1-B4 | Completed, non-numbered | EPG/metadata query, transaction, no-op and snapshot-cadence hardening. | [Performance closeout](phase-61-metadata-genre-performance-closeout.md#post-phase-61-performance-hardening) |
| PR #110 | Completed cross-cutting feature | Mobile Remote pressed-state and duplicate-dispatch behaviour. | [Platform closeout](post-phase-61-platform-runtime-closeout.md) |
| PR #111 | Completed cross-cutting feature | Backend-scoped global search over persisted Recording/EPG titles, subtitles and people. | [Platform closeout](post-phase-61-platform-runtime-closeout.md) |
| PR #115 | Completed cross-cutting feature | Configurable photorealistic VDR Remote asset and interaction path. | Repository history |
| PR #118 | Completed post-Phase-62 correction | TVScraper genre classification, overview/detail consistency and low-latency continuation. | Repository history |
| PR #123 | Completed post-Phase-62 correction | EPG artwork resolution beneath configured public base paths. | Repository history |
| PR #132 | Completed post-Phase-62 platform feature | Guarded external series-artwork fallback, secure backend settings and deterministic provider identity. | [Post-Phase-62 Security Review](post-phase-62-security-review.md) |

## Phase 65 durable completion marker

```text
accepted_final_phase65_runtime_candidate=7193797368cd1ff637062d02d0d7c9e5bf435ebe
source_ci_workflow=VDR-Suite CI
source_ci_run_number=8303
source_ci_run_id=33166818230
source_ci_result=PASS
YAVDR_EXACT_INSTALL_RUNTIME_IDENTITY=PASS
REAL_ANDROID_EDGE_LONG_OUTAGE_RECOVERY=PASS
NETWORK_RECOVERY_USER_ACTION_REQUIRED=NO
merge_pr=228
merge_commit=131f669c0f4e360f3306cfb34f50380653a9fdfc
```

The exact final runtime candidate was accepted before PR #228 was merged. The documentation-only final Phase-65 closeout does not alter that runtime tree and must pass the complete repository CI before merge.

## Previous Phase 64 durable completion marker

```text
accepted_candidate=bdd70d527d640dc115a7c141e505140ce8cdba9a
source_ci_run_number=7689
source_ci_run_id=32023780598
source_ci_result=PASS
PHASE_64_MANAGED_TIMER_FULFILLMENT_ACCEPTANCE=PASS
PHASE_64_REASSIGNMENT_FAILOVER_ACCEPTANCE=PASS
ADVERTISEMENT=timer-commands-activated
REASSIGNMENT=atomic-fail-closed
OUTCOME_UNKNOWN=reconciliation-only
PUBLIC_SVDRP_TIMER_WRITES=closed
merge_commit=72e298a76f7879ea7fc58f6a502e32eca7399f5a
```

## Completion boundaries

- Phase 61 is not reopened by provider adapters, diagnostics or recommendation work.
- Phase 62 is not reopened by later protected feature routes that continue to use its identity, authorization, CSRF and accountability model.
- Phase 63 is not reopened by Timer or media features that use the accepted Agent/provider foundation.
- Phase 64 is not reopened by a later broad Timer UI or media work that consumes the accepted Timer orchestration contracts.
- Phase 65 is not reopened by optional read-only media diagnostics, shared fMP4/MSE helper deduplication, future client-family adapters, Teletext/HbbTV, Legacy OSD or later public-API hardening that consume its accepted MediaSession/playback contracts.
- Growing-Recording seek and Live-TV timeshift remain explicitly deferred capability work; truthful non-support does not reopen Phase 65.
- Historical runtime fingerprints must remain distinguishable from later daemon evidence.
- ADR acceptance remains separate from runtime completion.
- Phase 66 has not started merely because Phase 65 is complete.

## Next work

```text
Phase 66 - Broadcast Companion Services: Teletext and HbbTV
```

Phase 66 is the next strict numbered runtime phase but remains not started. Accepted ADR-0054 defines its architecture; a separate explicit kickoff is required before any Phase-66 runtime implementation begins.

## Verification

```bash
make test-docs
make test-phase
make test-phase-map-coverage
```

## Related documents

- [Current State](../CURRENT.md)
- [Latest Completed Marker](completed-phases-latest.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase 64 Final Closeout](phase-64-closeout.md)
- [Phase 65 Final Closeout](phase-65-closeout.md)
- [Architecture Gap Matrix](../planning/architecture-audit-gap-matrix.md)
