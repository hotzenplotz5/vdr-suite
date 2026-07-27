# Architecture Audit Gap Matrix

## Purpose

This living register compares accepted target contracts with implemented `main` behaviour. It was refreshed on 2026-07-27 against `44ae3102ab202ee0dfc974ee0bc9624b9219ad2d` after Phase 61, PRs #102-#108, PR #110 and PR #111.

An ADR changes the target contract. A gap closes only through implementation, tests and required acceptance.

## Status legend

| Status | Meaning |
| --- | --- |
| Implemented | The bounded capability exists in current main and has implementation/test evidence. |
| Strong foundation | Core mechanics exist; a broader cross-domain contract remains incomplete. |
| Partial | Useful behaviour exists, but important target semantics remain missing. |
| Missing | No complete runtime boundary exists. |
| Decision accepted | Target is decided but runtime is absent/incomplete. |
| Deferred | Intentionally postponed with named prerequisites. |

## Executive summary

Current strengths:

- stable backend IDs, registry and server-enforced read-only mode;
- backend-scoped snapshots, caches and change feed;
- Recordings 2, guarded Recording actions and SearchTimer foundations;
- persistent Recording/EPG metadata, people, artwork and Genre read models;
- provider-neutral query-only Genre and global-search GET paths;
- backend-neutral RemoteAction and LiveOverlay foundations;
- global search over persisted Recording/EPG titles, subtitles and people;
- packaging, architecture and regression guardrails.

Largest remaining gaps:

- production actor identity, scoped RBAC and append-only accountability;
- secure Backend Agent lifecycle, generation, lease and command fencing;
- universal revision/idempotency and durable operation semantics;
- production job claim/retry/reconciliation;
- TimerIntent/assignment/scheduler/reconciler;
- canonical cross-provider ProgramEvent identity where required;
- authenticated streaming sessions;
- isolated legacy OSD bridge;
- stable `/api/v1` and universal compatibility/error contracts.

## Detailed gap register

| ID | Architecture capability | Current status | Current evidence / remaining limitation | Target decision | Roadmap destination |
| --- | --- | --- | --- | --- | --- |
| G-01 | Control Plane / Backend Agent boundary | Decision accepted | Direct/local adapters and SuiteBridge foundations exist; no production remote Agent protocol/runtime. | ADR-0039 | Phase 63 |
| G-02 | Backend generation, heartbeat, lease and fencing | Partial | Stable `backendId` exists; generation/lease/reconnect fencing is not production runtime. | ADR-0040 | Phase 63 |
| G-03 | Capability contract and degradation model | Strong foundation | Capability sets and backend reports exist; revision/origin/temporary/channel-scoped degradation is incomplete. | ADR-0012, ADR-0048 | Phase 63/67 |
| G-04 | Backend-scoped RBAC and read-only policy | Partial | Server-enforced read-only mode is implemented; per-actor roles, grants and scopes are missing. | ADR-0013, ADR-0041, ADR-0049 | Phase 62 |
| G-05 | Backend-scoped immutable snapshots | Implemented foundation | Snapshot/cache/change-feed objects exist; Agent generation/revision metadata remains later hardening. | ADR-0016, ADR-0018 | Phase 63 hardening |
| G-06 | Revision, event sequence and resync vocabulary | Partial | Snapshot/change sequence exists; backend generation, resource revision and public cursor vocabulary are not universally separated. | ADR-0016, ADR-0018, ADR-0048 | Phase 63/67 |
| G-07 | Common mutation preview/validation/execution/verification | Strong foundation | Recording actions provide the full bounded pattern; Timer/SearchTimer paths provide partial equivalents; not universal. | ADR-0042 | Phase 62/63 gates and domain slices |
| G-08 | Idempotency and optimistic concurrency | Partial | Operation IDs exist in bounded paths, but durable idempotency, expected revision and generation fencing are not universal. | ADR-0042 | Before new remote writes; Phase 62/63 |
| G-09 | Native lock and pointer isolation | Strong foundation | Adapter/worker boundaries and tests exist; remains a continuous review invariant for VDR/plugin work. | ADR-0007 and native-boundary invariant | Continuous |
| G-10 | Stable Suite Recording identity and native binding | Partial | Backend-scoped cache/native identity, fingerprints and metadata target bindings exist; universal revisioned durable Recording identity across lifecycle/storage is incomplete. | ADR-0014, ADR-0042 | Mutation/storage hardening after Phase 62/63 prerequisites |
| G-11 | Trash, restore and purge lifecycle | Partial | Guarded rename/move/VDR-trash exists; one canonical idempotent restore/purge lifecycle is incomplete. | ADR-0024, ADR-0042 | Domain hardening before remote writes |
| G-12 | Durable job claim, retry and saga model | Partial | Local job/action records exist; claim lease, attempts, retry schedule, cancellation, compensation and reconciliation remain incomplete. | ADR-0043 | Phase 62/63 foundation |
| G-13 | TimerIntent, TimerAssignment and NativeTimer separation | Missing | Native Timer and SearchTimer paths exist; durable user intent and backend assignment are not independent objects. | ADR-0044 | Phase 64 |
| G-14 | Capability-aware Timer scheduler and reconciler | Missing | No central assignment/re-evaluation/reconciliation runtime exists. | ADR-0044 | Phase 64 |
| G-15 | BackendEventRef and canonical ProgramEvent | Partial | Backend-scoped EPG identity/cache and authoritative bounded reconciliation are implemented; cross-provider canonical ProgramEvent identity is not. | ADR-0045 | Phase 64 prerequisite/domain slice |
| G-16 | EPG provenance and merge policy | Strong foundation | Phase 61 persists DVB, TVScraper and derived Genre/media-type evidence with state; universal field-level cross-provider provenance/merge remains incomplete. | ADR-0045, ADR-0038 | Later ProgramEvent/provider hardening |
| G-17 | Suite-owned metadata entities and artwork assets | Implemented for accepted Phase 61 scope | Target bindings, people, Genre evidence/assignments, provider-neutral artwork references and authenticated delivery exist. Derivative processing and every future media entity/provider are outside the closed scope. | ADR-0038 | Completed Phase 60.15/61; extensions explicit backlog |
| G-18 | Unified automation-provider boundary | Partial | SearchTimer/epgsearch foundations exist; providers do not yet produce central TimerIntents. | ADR-0029, ADR-0044 | Phase 64 |
| G-19 | Streaming Gateway and authenticated media sessions | Missing | Live transport concepts/providers exist; no Suite session gateway protects private media endpoints. | ADR-0046 | Phase 65 |
| G-20 | Legacy OSD bridge and controller lease | Missing | RemoteAction/LiveOverlay exists but is not a frame/delta/viewer/controller OSD bridge. | ADR-0047 | Phase 66 |
| G-21 | Central database is not a client/Agent protocol | Implemented invariant for current paths | Genre/global-search reads and frontend wrappers preserve repository/service boundaries; future Agents must continue to obey it. | ADR-0038, ADR-0039, ADR-0050 | Continuous; Phase 63 Agent enforcement |
| G-22 | Agent authentication, protected transport and credential rotation | Missing | Backend access mode exists; no production Agent enrollment/credential/transport lifecycle. | ADR-0041 | Phase 62 identity model, Phase 63 runtime |
| G-23 | Explicit multi-site trust boundary | Partial | Multi-backend IDs/read-only policy exist; authenticated remote site sessions and deterministic revocation do not. | ADR-0039-0041 | Phase 62/63 |
| G-24 | Audit and security event model | Missing | Diagnostics/logs exist; no universal actor/request/decision/outcome accountability repository. | ADR-0049 | Phase 62 |
| G-25 | Public API version/error/compatibility contract | Partial | Suite controllers and `VdrSuiteClientApi` exist; stable `/api/v1`, common errors, ETags and deprecation policy do not. | ADR-0048 | Phase 67 |
| G-26 | Plugin adapter capability degradation | Partial | Adapter capability foundations exist; common per-version degradation and unsafe-operation disablement contract is incomplete. | ADR-0007, ADR-0012, ADR-0048 | Phase 63/67 |
| G-27 | epgd/epg2vdr migration or provider strategy | Deferred | Phase 61 identity/provenance prerequisite is now complete, but no direct shared-database integration is approved. Any adapter must use Suite-owned provider boundaries. | ADR-0038, ADR-0045 | Post-Phase-61 provider backlog after explicit decision |
| G-28 | Shared/remote storage semantics | Partial | Backend ownership and Recording paths/actions exist; shared-filesystem identity and cross-site move ownership remain undefined. | ADR-0014, ADR-0042, future storage ADR | After Phase 63; before cross-site storage mutation |
| G-29 | Offline Agent synchronization and reconciliation | Partial foundation | Snapshot resync/change-feed concepts exist; durable offline Agent queues and generation-aware command outcomes do not. | ADR-0040, ADR-0043 | Phase 63 |
| G-30 | Timer failover and deliberate redundancy | Missing | Backend-native Timer operations exist; failover/reassignment/replica policy requires TimerIntent. | ADR-0044 | Phase 64 |
| G-31 | Backend-scoped global search | Implemented first slice | PR #111 searches one selected backend over persisted Recording/EPG titles, subtitles and people with query-only/provider-free reads. Authorized multi-backend aggregation is not implemented. | ADR-0021, ADR-0038, ADR-0050 | Completed current slice; aggregator deferred |
| G-32 | Backend-neutral remote actions and live overlay | Strong foundation | PRs #99/#110 implement allowlisted actions, capability/read-only checks, overlay snapshots and isolated UI dispatch state. Streaming and legacy OSD remain separate gaps. | ADR-0023, ADR-0030, ADR-0047 | Current scope implemented; OSD Phase 66 |
| G-33 | Recording-person payload completeness | Implemented bounded contract | Current end-to-end bound is 128 people / 65,535 bytes; 52 modelled Pulp Fiction people are retained. Universal completeness beyond 128 is not promised. | Existing RMETA contract | Future versioned change only if justified |

## Priority view

### P0 — before production remote writes

G-01, G-02, G-04, G-07, G-08, G-12, G-22, G-23, G-24 and G-29.

### P1 — next product platform

G-10, G-13-G-18, G-19, G-25 and G-26.

### P2 — compatibility and later expansion

G-20, G-27, G-28 and G-30, plus authorized multi-backend search aggregation beyond G-31.

## Relationship to strict roadmap

```text
Completed: Phase 60.15
Completed: Phase 61
Completed: Post-Phase 61 Performance Hardening (B1-B4)
Completed: Remote/Live Overlay hardening (#110)
Completed: Backend-scoped Global Search (#111)
Next:      Phase 62 Identity/RBAC/accountability
Then:      Phase 63 Agent runtime
           Phase 64 Timer orchestration
           Phase 65 streaming
           Phase 66 legacy OSD
           Phase 67 public API
           Phase 68 recommendations
```

## Maintenance rules

- Update a row only when repository evidence or accepted target ownership changes.
- Do not mark a row implemented from an ADR alone.
- Do not keep a completed phase as a future destination.
- Preserve closed rows for traceability rather than deleting them.
- Every new gap needs an ID, evidence, target owner and roadmap destination.

## Related documents

- [Current State](../CURRENT.md)
- [Strict Roadmap](roadmap.md)
- [Phase Map](phase-map.md)
- [VDR Ecosystem Parity](parity-audit-and-frontend-gap-roadmap.md)
- [Completed Architecture Source Audit](../development/architecture-source-audit-2026-07-15.md)