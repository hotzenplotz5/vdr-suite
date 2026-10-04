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
| G-20 | Legacy OSD viewer/controller bridge | Closed foundation for accepted scope | Phase 68 is completed through 68.G: semantic observation, authenticated transport, authorized sessions/viewers, exclusive controller lease and fenced allowlisted native input. RemoteAction/LiveOverlay remains separate from the Legacy OSD plane. | ADR-0047 / Phase 68 Closeout |
| G-21 | Central database is not a client/Agent protocol | Continuous invariant | Repository/service boundaries remain mandatory for clients, Agents and providers. | ADR-0038, ADR-0039, ADR-0050 |
| G-22 | Agent authentication and credential lifecycle | Closed foundation | Agent identity, enrolled trust and credential generation/lifecycle are established. | ADR-0041 |
| G-23 | Explicit multi-site trust boundary | Closed foundation | Agent/backend/site identity and generation fencing provide the platform trust boundary; media and later domains must reuse it. | ADR-0039-0041 |
| G-24 | Accountability and security events | Closed foundation | Append-only authorization/mutation accountability exists; broader audit reader/export/redaction/retention is a cross-cutting product milestone. | ADR-0049 |
| G-25 | Stable public API version/error/compatibility contract | Closed foundation | Phase 69.A-F are completed; the deliberately declared public-v1 set has accepted request/error, revision/precondition/idempotency, collection, compatibility and independent reference-client coverage without promoting private/pre-v1 domains. | ADR-0048 / Phase 69 closeout |
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
| G-37 | Account/backend access administration product | Active cross-cutting milestone | MU.0-MU.7 are completed; MU.6B Public Account item/revision and MU.6C public lifecycle mutation are completed, MU.6D Atomic Account CREATE + durable idempotency merged as PR #411, and MU.7 grant administration passed REAL YAVDR acceptance and merged as PR #413. MU.8A safe credential/session metadata merged as PR #415 and retains separately documented real-runtime acceptance debt; MU.8B merged as PR #416 after focused yaVDR acceptance; MU.8C merged as PR #417 after focused yaVDR acceptance with PR and post-merge main CI green. MU.9A read-only browser Account administration merged as PR #418 after exact merged-main yaVDR acceptance; PR and post-merge hosted CI are green; MU.9B Account Lifecycle Mutation UI is now the active bounded implementation candidate. | ADR-0061, ADR-0065, ADR-0066, ADR-0067 / Multiuser workstream |
| G-38 | Teletext domain service | Closed foundation | Canonical service/page/subpage domain, fenced provider/Agent path, authorized HTTP reads and first-party 25 x 40 browser/TV rendering are implemented and accepted on real yaVDR. | ADR-0054 / Phase 67 Teletext closeout |
| G-39 | HbbTV broadcast application domain/runtime | Closed foundation | Canonical HbbTV discovery, authorized application session, normalized input and presentation/media runtime are implemented without public raw plugin/browser commands and accepted on real yaVDR. | ADR-0054 / Phase 67 closeout |
| G-40 | Legacy Basic retirement | Closed deployment milestone | Guarded real-yaVDR migration/rollback evidence was accepted, then the Legacy Basic runtime authenticator/mode/fallback authority was removed. Preserved old defaults lines are inert; the rollback sequence is historical evidence only. | P2 Legacy Basic retirement closeout |
| G-41 | Recommendation/content graph | Deferred vision | Requires stable identities, privacy/preferences, provenance and Phase-69 public resource semantics plus a dedicated ADR. | future ADR / Phase 70 |
| G-42 | Normalized playback presentation/timeline/continuity/failure semantics | Closed foundation | ADR-0056 mandatory semantics are completed: provider-free `MediaPlaybackContract`, canonical owner lifecycle publication, explicit presentation generation/discontinuity and classified failures. | ADR-0056 / Phase 65.D |
| G-43 | Responsive Media Home / browse-first preview composition | Closed foundation for accepted scope | Phase 66 completed responsive Home composition, Live hero browsing, deferred preview, truthful Continue Watching, discovery rails and accepted desktop/mobile Golden Journeys; later bounded hardening does not reopen the phase. | ADR-0058 / Phase 66 closeout |

## Priority view

### Next numbered runtime product domain — Phase 70

Phase 70 Recommendation and Content Knowledge Graph is the next strict numbered runtime phase but is not started. It still requires a dedicated accepted runtime ADR and must preserve the completed identity, privacy/preferences, provenance and Phase-69 public-resource boundaries.

### Completed numbered product domains

Phase 66 Media Home, Phase 67 Teletext/HbbTV, Phase 68 Legacy OSD and Phase 69 public API/client compatibility are completed for their accepted bounded scopes. Later consumers and bounded hardening must reuse those owners rather than reopening the numbered phases.

### Cross-cutting product work

Legacy Basic retirement is completed; see the P2 retirement closeout. Multiuser / Account and Backend Access Administration is the active cross-cutting product workstream: MU.0-MU.7 are complete; MU.6D merged as PR #411 and MU.7 grant administration merged as PR #413 after REAL YAVDR acceptance. MU.8 implementation is complete through MU.8C; MU.8A retains its separately documented real-runtime acceptance debt. MU.9 is in progress: MU.9A merged as PR #418 after exact merged-main yaVDR acceptance, with PR and post-merge hosted CI green; MU.9B Account Lifecycle Mutation UI is now the active bounded implementation candidate. MU.8A merged as PR #415. MU.8B passed focused yaVDR acceptance, merged as PR #416, and both PR and post-merge hosted CI are green; live daemon installation/restart was not required for that bounded slice. Broad Timer UI remains gated on the required Multiuser administration surface. Audit/operations, client-family rollout, release packaging and federation/pairing remain separate cross-cutting work. None of these starts Phase 70.

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
