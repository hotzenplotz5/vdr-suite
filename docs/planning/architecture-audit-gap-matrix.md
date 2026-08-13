# Architecture Audit Gap Matrix

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Strict Roadmap](roadmap.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [ADR Index](../adr/index.md)

---

## Purpose

This living register compares accepted target architecture with durable implemented capability boundaries and named remaining gaps.

It deliberately does **not** contain active PR numbers, exact branch heads or CI checkpoints. Those volatile facts belong only in [Current State](../CURRENT.md).

A gap is not closed by an ADR alone. Closure requires implementation, tests and the acceptance level appropriate to the capability.

## Status legend

| Status | Meaning |
| --- | --- |
| Closed foundation | The accepted bounded architecture is implemented and reusable. |
| Strong foundation | Major mechanics exist, but broader cross-domain/product completion remains. |
| Active domain | Work belongs to the currently active numbered domain; exact checkpoint is in `CURRENT.md`. |
| Planned | Target accepted or named, runtime not yet complete. |
| Deferred | Intentionally postponed with prerequisites. |
| Continuous invariant | Must remain true across later work rather than being “finished once”. |

## Platform gap register

| ID | Architecture capability | Status | Durable assessment / remaining boundary | Target owner |
| --- | --- | --- | --- | --- |
| G-01 | Control Plane / Backend Agent boundary | Closed foundation | Enrolled Agent identity, bounded Agent protocol and explicit Control Plane ownership exist. | ADR-0039 |
| G-02 | Backend generation, heartbeat, lease and fencing | Closed foundation | Backend/Agent generation and lifecycle fencing are established reusable semantics. | ADR-0040 |
| G-03 | Capability contract and degradation model | Strong foundation | Capability reporting/selection exists; every future domain must continue truthful version/state/constraint reporting. | ADR-0012, ADR-0048 |
| G-04 | Backend-scoped RBAC and access policy | Closed foundation | Persistent actor identity, exact backend-scoped grants, fixed roles and server-side policy are established. | ADR-0013, ADR-0041, ADR-0049 |
| G-05 | Backend-scoped observations and snapshots | Closed foundation | Complete baseline, exact-next sequence, replay/conflict and resync semantics exist as Agent/read-model foundations. | ADR-0016, ADR-0018, ADR-0040 |
| G-06 | Revision, sequence and resync vocabulary | Strong foundation | Backend generation, snapshot generation, producer sequence and resource revision are explicitly distinct; each new domain still needs truthful revision semantics. | ADR-0016, ADR-0018, ADR-0048 |
| G-07 | Common protected mutation pipeline | Strong foundation | Authorization, durable dispatch boundary, fencing and authoritative readback patterns exist; domain-specific verification remains required. | ADR-0042 |
| G-08 | Idempotency and optimistic concurrency | Strong foundation | Protected-write safety defines logical idempotency scope, expected revision and unknown-outcome handling; each resource must bind them correctly. | ADR-0042 |
| G-09 | Native lock and pointer isolation | Continuous invariant | VDR pointers/locks remain local; no client/network wait or expensive processing under native locks. | VDR/native boundary invariant |
| G-10 | Stable Suite Recording identity and native binding | Strong foundation | Backend-scoped Recording identity/read models/actions exist; broader lifecycle/shared-storage identity remains explicit future hardening. | ADR-0014, ADR-0042 |
| G-11 | Trash, restore and purge lifecycle | Strong foundation | Guarded Recording actions exist; a fully uniform cross-backend lifecycle remains domain hardening. | ADR-0024, ADR-0042 |
| G-12 | Durable job/attempt/reconciliation semantics | Strong foundation | Agent command/result and protected-write safety provide durable execution mechanics; later domains must not invent parallel retry semantics. | ADR-0043 |
| G-13 | TimerIntent / TimerAssignment / NativeTimerBinding separation | Active domain | Backend-neutral intent and independent assignment/native-binding ownership are the accepted Timer model. Exact implementation checkpoint is in `CURRENT.md`. | ADR-0044 / Phase 64 |
| G-14 | Capability-aware Timer scheduler and reconciler | Active domain | Deterministic eligibility, assignment ownership, readback and reconciliation are Phase-64 engine concerns. | ADR-0044 / Phase 64 |
| G-15 | Canonical ProgramEvent identity where required | Strong foundation | Backend-scoped EPG identity/cache is mature; broader canonical occurrence identity remains introduced only where orchestration/provider semantics require it. | ADR-0045 |
| G-16 | EPG provenance and merge policy | Strong foundation | Provider/native evidence is retained; broader field-level cross-provider reconciliation remains explicit enrichment work. | ADR-0045, ADR-0038 |
| G-17 | Suite-owned metadata entities and artwork | Closed foundation for accepted scope | Persistent metadata/people/Genre/artwork read models exist behind Suite contracts. | ADR-0038 |
| G-18 | Unified automation-provider boundary | Active domain | SearchTimer/epgsearch are sources/proposals; central TimerIntent orchestration must remain authoritative. | ADR-0029, ADR-0044 / Phase 64 |
| G-19 | Streaming Gateway and authenticated MediaSession | Planned | ADR-0046 defines session/grant/route/provider-lease boundaries; production media runtime is a later numbered phase. | ADR-0046 / Phase 65 |
| G-20 | Legacy OSD viewer/controller bridge | Planned | RemoteAction/LiveOverlay is not the Legacy OSD compatibility plane. | ADR-0047 / Phase 66 |
| G-21 | Central database is not a client/Agent protocol | Continuous invariant | Repository/service boundaries remain mandatory for clients, Agents and providers. | ADR-0038, ADR-0039, ADR-0050 |
| G-22 | Agent authentication and credential lifecycle | Closed foundation | Agent identity, enrolled trust and credential generation/lifecycle are established. | ADR-0041 |
| G-23 | Explicit multi-site trust boundary | Closed foundation | Agent/backend/site identity and generation fencing provide the platform trust boundary; media and future domains must reuse it. | ADR-0039-0041 |
| G-24 | Accountability and security events | Closed foundation | Append-only authorization/mutation accountability exists; broader audit product/read/export is separate product scope. | ADR-0049 |
| G-25 | Stable public API version/error/compatibility contract | Planned/partial | Internal Suite client contracts exist; stable independent-client API hardening remains a later numbered phase. | ADR-0048 / Phase 67 |
| G-26 | Provider capability degradation and disablement | Strong foundation | Explicit provider ownership/capability rules exist; every new provider operation must truthfully fail closed when unsafe/unavailable. | ADR-0007, ADR-0012, ADR-0048 |
| G-27 | epgd/epg2vdr/provider expansion | Deferred | Any new provider must feed Suite-owned identity/evidence boundaries rather than shared DB/public-provider coupling. | ADR-0038, ADR-0045 |
| G-28 | Shared/remote Recording storage semantics | Deferred/partial | Path equality is not shared-storage identity; cross-site storage mutation needs an explicit ownership model. | ADR-0014, ADR-0042, future storage decision |
| G-29 | Offline Agent command/result reconciliation | Strong foundation | Durable command/result/reconnect semantics exist; each mutation domain must use evidence-aware retry/reconciliation. | ADR-0040, ADR-0043 |
| G-30 | Timer failover and deliberate redundancy | Active domain | Replica/failover policy must be explicit assignments and cannot reinterpret accidental duplicates as policy. | ADR-0044 / Phase 64 |
| G-31 | Authorized multi-backend search aggregation | Deferred enhancement | Backend-scoped global search exists; any aggregator must authorize each backend independently and keep bounded result semantics. | ADR-0021, ADR-0038, ADR-0050 |
| G-32 | Backend-neutral RemoteAction / LiveOverlay | Closed foundation | Useful interaction/state-update capability; explicitly separate from media streaming and Legacy OSD. | ADR-0023, ADR-0030 |
| G-33 | Recording-person payload bounds | Closed bounded contract | Current contract remains bounded/versioned; expansion requires an explicit versioned change rather than an accidental payload increase. | RMETA contract |
| G-34 | Client playback engine / media adaptation boundary | Proposed decision | Platform-appropriate players and least-transformation delivery are proposed to complement ADR-0046; acceptance must be explicit before becoming canonical. | proposed playback/media ADR |
| G-35 | Golden vertical product acceptance | Strong planning foundation | Component CI must be complemented by real end-to-end Timer/media/failure journeys when those domains reach runtime. | Golden User Journeys |

## Priority view

### Active engine completion

The active numbered domain must complete the reliable Timer orchestration engine required by ADR-0044 and Timer Golden User Journeys before the next numbered runtime phase begins.

Exact authorized successor work is defined only in [Current State](../CURRENT.md); this gap matrix does not authorize a new slice by naming an architectural gap.

### Next media platform

After the Timer engine gate is satisfied, media work must preserve:

- authenticated MediaSession ownership;
- short-lived access grants;
- explicit backend/Agent/provider route ownership;
- route/generation fencing and deterministic cleanup;
- private provider URLs/credentials;
- pass-through first, remux only when needed, transcode only when materially required;
- real browser/client picture-and-sound acceptance.

### Later compatibility/platform work

Legacy OSD, stable public API hardening, storage federation and recommendation/content-graph expansion remain separate domains with their own prerequisites.

## Maintenance rules

- Update a row only from accepted repository/runtime evidence or an accepted architecture decision.
- Do not mark a row closed from an ADR alone.
- Do not copy exact active heads, PR tips or CI checkpoints into this file.
- Do not let a historical closeout or proposed successor name authorize current implementation.
- Preserve explicit unresolved gaps instead of hiding them under broad “implemented” labels.
- When volatile status is needed, link to `docs/CURRENT.md`.

## Related documents

- [Current State](../CURRENT.md)
- [Strict Roadmap](roadmap.md)
- [Phase Map](phase-map.md)
- [Golden User Journeys](golden-user-journeys.md)
- [VDR Ecosystem Parity and Product Gaps](parity-audit-and-frontend-gap-roadmap.md)
- [Completed Phases](../development/completed-phases.md)

## Back

- [Back to Planning Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)
