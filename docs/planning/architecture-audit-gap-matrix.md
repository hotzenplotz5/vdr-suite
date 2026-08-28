# Architecture Audit Gap Matrix

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Strict Roadmap](roadmap.md)
- [Phase Map](phase-map.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [ADR Index](../adr/index.md)
- [ADR-0056 Playback Semantics](../adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)

---

## Purpose

This living register compares accepted target architecture with durable implemented capability boundaries and named remaining gaps.

It deliberately does **not** contain active PR numbers, branch heads or transient CI checkpoints. Those facts belong to live GitHub state; volatile completed/active/next phase status belongs in [Current State](../CURRENT.md).

A gap is not closed by an ADR alone. Closure requires implementation, tests and the acceptance level appropriate to the capability.

## Status legend

| Status | Meaning |
| --- | --- |
| Closed foundation | The accepted bounded architecture is implemented and reusable. |
| Strong foundation | Major mechanics exist, but broader cross-domain/product completion remains. |
| Planned | Target accepted or named; runtime not yet complete. |
| Proposed | Architecture is being proposed and must be accepted before runtime work. |
| Deferred | Intentionally postponed with prerequisites. |
| Continuous invariant | Must remain true across later work rather than being “finished once”. |

## Platform gap register

| ID | Architecture capability | Status | Durable assessment / remaining boundary | Target owner |
| --- | --- | --- | --- | --- |
| G-01 | Control Plane / Backend Agent boundary | Closed foundation | Enrolled Agent identity, bounded Agent protocol and explicit Control Plane ownership exist. | ADR-0039 |
| G-02 | Backend generation, heartbeat, lease and fencing | Closed foundation | Backend/Agent generation and lifecycle fencing are established reusable semantics. | ADR-0040 |
| G-03 | Capability contract and degradation model | Strong foundation | Capability reporting/selection exists; every future domain must continue truthful version/state/constraint reporting. | ADR-0012, ADR-0048 |
| G-04 | Backend-scoped RBAC and access policy | Closed foundation | Persistent actor identity, exact backend-scoped grants, fixed roles and server-side policy are established. Product administration surfaces remain cross-cutting. | ADR-0013, ADR-0041, ADR-0049 |
| G-05 | Backend-scoped observations and snapshots | Closed foundation | Complete baseline, exact-next sequence, replay/conflict and resync semantics exist as Agent/read-model foundations. | ADR-0016, ADR-0018, ADR-0040 |
| G-06 | Revision, sequence and resync vocabulary | Strong foundation | Backend generation, snapshot generation, producer sequence and resource revision are distinct; every new domain still needs truthful revision semantics. | ADR-0016, ADR-0018, ADR-0048 |
| G-07 | Common protected mutation pipeline | Strong foundation | Authorization, durable dispatch boundary, fencing and authoritative readback patterns exist; domain-specific verification remains required. | ADR-0042 |
| G-08 | Idempotency and optimistic concurrency | Strong foundation | Protected-write safety defines logical idempotency scope, expected revision and unknown-outcome handling; each resource must bind them correctly. | ADR-0042 |
| G-09 | Native lock and pointer isolation | Continuous invariant | VDR pointers/locks remain local; no client/network wait or expensive processing under native locks. | Native boundary invariant |
| G-10 | Stable Suite Recording identity and native binding | Strong foundation | Backend-scoped Recording identity/read models/actions exist; broader shared-storage/federation identity remains explicit future hardening. | ADR-0014, ADR-0042 |
| G-11 | Trash, restore and purge lifecycle | Strong foundation | Guarded Recording actions exist; fully uniform cross-backend lifecycle remains domain hardening. | ADR-0024, ADR-0042 |
| G-12 | Durable job/attempt/reconciliation semantics | Strong foundation | Agent command/result and protected-write safety provide durable execution mechanics; later domains must not invent parallel retry semantics. | ADR-0043 |
| G-13 | TimerIntent / TimerAssignment / NativeTimerBinding separation | Closed foundation | Durable intent/assignment/binding model is implemented and accepted through Phase 64. | ADR-0044 / Phase 64 |
| G-14 | Capability-aware Timer scheduler and reconciler | Closed foundation | Deterministic assignment, native fulfillment, authoritative readback, reconciliation and failover are implemented for the accepted Phase-64 scope. | ADR-0044 / Phase 64 |
| G-15 | Canonical ProgramEvent identity where required | Strong foundation | Backend-scoped EPG identity/cache is mature; broader canonical occurrence identity is introduced only where orchestration/provider semantics require it. | ADR-0045 |
| G-16 | EPG provenance and merge policy | Strong foundation | Provider/native evidence is retained; broader field-level cross-provider reconciliation remains explicit enrichment work. | ADR-0045, ADR-0038 |
| G-17 | Suite-owned metadata entities and artwork | Closed foundation for accepted scope | Persistent metadata/people/Genre/artwork read models plus manual metadata/cast assignment exist behind Suite contracts. | ADR-0038, ADR-0051, ADR-0052 |
| G-18 | Unified automation-provider boundary | Strong foundation | SearchTimer/epgsearch remain sources/proposals; central TimerIntent orchestration is authoritative. Broad automation-product unification remains separate. | ADR-0029, ADR-0044 |
| G-19 | Streaming Gateway and authenticated MediaSession | Closed foundation | Phase 65 is completed with Recording/Live MediaSession/Gateway runtime, provider privacy/leases, least-transformation delivery/output policy, deterministic cleanup and normalized persistent playback semantics. | ADR-0046, ADR-0053, ADR-0055 / Phase 65 |
| G-20 | Legacy OSD viewer/controller bridge | Planned | ADR-0047 is accepted; runtime is sequenced after Broadcast Companion as Phase 68. RemoteAction/LiveOverlay is not the Legacy OSD plane. | ADR-0047 / Phase 68 |
| G-21 | Central database is not a client/Agent protocol | Continuous invariant | Repository/service boundaries remain mandatory for clients, Agents and providers. | ADR-0038, ADR-0039, ADR-0050 |
| G-22 | Agent authentication and credential lifecycle | Closed foundation | Agent identity, enrolled trust and credential generation/lifecycle are established. | ADR-0041 |
| G-23 | Explicit multi-site trust boundary | Closed foundation | Agent/backend/site identity and generation fencing provide the platform trust boundary; media and later domains must reuse it. | ADR-0039-0041 |
| G-24 | Accountability and security events | Closed foundation | Append-only authorization/mutation accountability exists; broader audit reader/export/redaction/retention is a cross-cutting product milestone. | ADR-0049 |
| G-25 | Stable public API version/error/compatibility contract | Planned/partial | Internal Suite client contracts exist; stable independent-client API hardening remains Phase 69. | ADR-0048 / Phase 69 |
| G-26 | Provider capability degradation and disablement | Strong foundation | Explicit provider ownership/capability rules exist; every new provider operation must fail closed when unsafe/unavailable. | ADR-0007, ADR-0012, ADR-0048 |
| G-27 | epgd/epg2vdr/provider expansion | Deferred | New providers must feed Suite-owned identity/evidence boundaries rather than shared DB/public-provider coupling. | ADR-0038, ADR-0045 |
| G-28 | Shared/remote Recording storage semantics | Deferred/partial | Path equality is not shared-storage identity; cross-site storage mutation needs explicit ownership. | ADR-0014, ADR-0042, future storage decision |
| G-29 | Offline Agent command/result reconciliation | Strong foundation | Durable command/result/reconnect semantics exist; each mutation domain must use evidence-aware retry/reconciliation. | ADR-0040, ADR-0043 |
| G-30 | Timer failover and deliberate redundancy | Closed foundation for accepted scope | Explicit primary/replica policy and atomic controlled reassignment/failover are implemented; accidental duplicates are not policy. | ADR-0044 / Phase 64 |
| G-31 | Authorized multi-backend search aggregation | Deferred enhancement | Backend-scoped global search exists; aggregation must authorize each backend independently and stay bounded. | ADR-0021, ADR-0038, ADR-0050 |
| G-32 | Backend-neutral RemoteAction / LiveOverlay | Closed foundation | Useful interaction/state-update capability; explicitly separate from media streaming, Broadcast Companion and Legacy OSD. | ADR-0023, ADR-0030 |
| G-33 | Recording-person payload bounds | Closed bounded contract | Current contract remains bounded/versioned; expansion requires explicit versioned change. | RMETA contract, ADR-0052 |
| G-34 | Client playback engine / media adaptation boundary | Closed foundation | Phase 65 completed browser Recording/Live playback, least-transformation selection, persistent ownership, seek/restart, normalized tracks, Volume/Mute, bounded fMP4 buffering and sync-safe exact HLS resume without another player core. | ADR-0053, ADR-0055 / Phase 65.D |
| G-35 | Golden vertical product acceptance | Strong planning foundation | Component CI is complemented by real end-to-end Timer/media/failure journeys as capabilities land. | Golden User Journeys |
| G-36 | Broad Timer Product UI | Planned cross-cutting milestone | Phase-64 engine is complete, but intent-first polished UI remains gated on required account/backend access administration. | Phase 62 + Phase 64 + Roadmap milestone |
| G-37 | Account/backend access administration product | Planned cross-cutting milestone | Core RBAC exists; generic user/grant/backend administration surfaces were intentionally deferred from Phase 62. | Phase 62 foundation |
| G-38 | Teletext domain service | Planned | No canonical Teletext runtime exists yet. Accepted ADR-0054 models service/page/subpage data independently of OSD rendering. | ADR-0054 / Phase 67 |
| G-39 | HbbTV broadcast application domain/runtime | Planned | No canonical HbbTV runtime exists yet. Accepted ADR-0054 models application discovery/session/runtime without public raw plugin/browser commands. | ADR-0054 / Phase 67 |
| G-40 | Legacy Basic retirement | Deferred deployment migration | Transitional compatibility remains until enforced-mode rollout, recovery and upgrade/rollback are proven. | Phase 62 closeout / deployment milestone |
| G-41 | Recommendation/content graph | Deferred vision | Requires stable identities, privacy/preferences, provenance and Phase-69 public resource semantics plus a dedicated ADR. | future ADR / Phase 70 |
| G-42 | Normalized playback presentation/timeline/continuity/failure semantics | Closed foundation | ADR-0056 mandatory semantics are completed: provider-free `MediaPlaybackContract`, canonical owner lifecycle publication, explicit presentation generation/discontinuity and classified failures. | ADR-0056 / Phase 65.D |
| G-43 | Responsive Media Home / browse-first preview composition | Planned; architecture accepted | ADR-0058 and the Phase-66 contract define responsive Home composition, Live hero browsing, deferred preview, truthful Continue Watching, discovery rails and desktop/mobile Golden Journeys. Runtime has not started. | ADR-0058 / Phase 66 |

## Priority view

### Next numbered runtime product domain — Phase 66

Media Home / Browse architecture is accepted via ADR-0058; runtime remains not started and requires a separate explicit kickoff. Slice 66.1 is Home Shell and Responsive Information Architecture. Later slices add Live hero browsing, deferred canonical preview, truthful Continue Watching, Recording discovery rails, explicit history semantics if needed, accessibility/polish and real desktop/mobile acceptance.

Phase 66 preserves completed Phase-65 MediaSession/playback ownership and existing Channel/ProgramEvent/Recording/Metadata/Genre/artwork truth. Browse focus remains independent of preview state; stale preview must be canceled/relinquished; browser-local state is not fabricated into cross-client authority.

### Following television product domain — Phase 67

Teletext/HbbTV architecture remains accepted via ADR-0054 and follows Phase 66. Runtime is not started.

### Later compatibility/platform work

Legacy OSD (Phase 68), public API hardening (Phase 69), storage federation and Recommendation/Content Graph (Phase 70) remain separate.

### Cross-cutting product work

Account/backend access administration, broad Timer UI, audit/operations and client-family rollout may progress when their own prerequisites are met without advancing the numbered phase.

## Maintenance rules

- Update a row only from accepted repository/runtime evidence or an accepted/proposed architecture decision labeled honestly.
- Do not mark a row closed from an ADR alone.
- Do not copy active heads, PR tips or CI checkpoints into this file.
- Do not let historical closeouts or proposed successors authorize runtime implementation.
- Preserve unresolved gaps instead of hiding them under broad “implemented” labels.
- Completed phase history is never renumbered.
- Future phase mapping is owned by the Strict Roadmap and may change only before those phases start.
- When volatile status is needed, link to `docs/CURRENT.md`.

## Related documents

- [Current State](../CURRENT.md)
- [Strict Roadmap](roadmap.md)
- [Phase Map](phase-map.md)
- [ADR-0056 Playback Semantics](../adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)
- [Phase 65.D Playback Semantics Consolidation](../development/phase-65d-playback-semantics-consolidation.md)
- [Golden User Journeys](golden-user-journeys.md)
- [VDR Ecosystem Parity and Product Gaps](parity-audit-and-frontend-gap-roadmap.md)
- [Completed Phases](../development/completed-phases.md)

## Back

- [Back to Planning Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)
