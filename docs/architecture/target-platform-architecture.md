# VDR-Suite Target Platform Architecture

## Status and purpose

This is the canonical target architecture accepted through ADR-0049, with ADR-0050 reinforcing the domain-repository SQLite boundary. It distinguishes implemented foundations from later target runtime.

A target box is not proof of implementation. Current runtime truth is maintained in:

- [Current Architecture State](../development/current-architecture-state.md)
- [Current State](../CURRENT.md)
- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Completed Phases](../development/completed-phases.md)

The previous full target-diagram snapshot is retained as [historical architecture evidence](history/target-platform-architecture-before-refresh.md).

## Current implementation overlay

Implemented on the verified 2026-08-05 main baseline plus the active Draft PR #137 overlay:

- backend registry, backend-scoped snapshots/caches/change feed and server-enforced read-only mode;
- Recordings 2 and guarded Recording actions;
- SearchTimer and native Timer foundations;
- completed Phase 61 persistent Recording/EPG metadata, people, artwork references and Genre read models;
- query-only provider-free Genre and Global Search GET paths;
- backend-neutral RemoteAction and LiveOverlay contracts;
- modular browser Client API boundaries;
- completed Phase-62 production actor identity, scoped RBAC, browser-session security and append-only accountability;
- active Phase-63 Slice-1 Agent enrollment, protected outbound transport, technical identity, generation, heartbeat/lease, read-only capabilities, reconnect and credential lifecycle foundation.

Not yet implemented as complete target runtime:

- Phase-63 snapshot/change ingestion, durable command/results, native execution and provider selection;
- universal revision/idempotency/job reconciliation;
- TimerIntent orchestration;
- Streaming Gateway;
- legacy OSD bridge;
- stable public `/api/v1`;
- recommendation/knowledge graph.

## Platform and trust boundary

```text
Web / Desktop / Mobile / TV / Automation clients
                         |
                         | future authenticated public API
                         v
+---------------------------------------------------------------+
| VDR-Suite Control Plane                                       |
|                                                               |
| actor identity, sessions, RBAC and policy       [implemented]    |
| Suite-owned domain services and repositories    [foundation]  |
| operations, jobs and reconciliation             [partial]     |
| metadata, people, Genres and search              [implemented] |
| Timer scheduler and assignments                  [Phase 64]    |
| media/OSD session policy                         [65 / 66]     |
| accountability event store                      [implemented]    |
+--------------------------+----------------------+-------------+
                           | protected Agent protocol [Phase 63]
                           v
+---------------------------------------------------------------+
| Backend Agent                                                 |
| enrolled identity, generation, heartbeat, lease, capabilities |
| reconnect/credential lifecycle [Slice 1]; snapshots/commands future |
+--------------------------+------------------------------------+
                           | local/private adapter contracts
                           v
+---------------------------------------------------------------+
| VDR site                                                      |
| VDR Core | SuiteBridge | RESTfulAPI | SVDRP | Streamdev        |
| epgsearch | TVScraper | native files and databases            |
+---------------------------------------------------------------+
```

Rules:

- VDR remains authoritative for VDR-native runtime state and execution.
- The Control Plane owns external identity, policy, orchestration and client contracts.
- Agents own bounded site-local transport and execution, not global policy.
- Private plugins/providers are never the public security boundary.
- Clients receive no permanent VDR/plugin/Agent/provider credentials or private URLs.

## Contract separation

```text
Public Client API
  resources, commands, operations, errors and revisions

Backend Agent Protocol
  enrollment, generation, lease, capabilities, observations,
  fenced commands, receipts, results and reconnect evidence

Media Plane
  MediaSession, short-lived grant, route epoch, provider lease and bytes

Legacy OSD Plane
  session, viewer binding, ordered frames/deltas,
  controller lease and allowlisted input

Plugin Local Contract
  bounded capabilities, native observations and command/result facts

Internal C++ APIs
  domain services, repositories, adapters and serializers
```

These version axes are independent:

```text
publicApiVersion
  != agentProtocolVersion
  != mediaProtocolVersion
  != osdProtocolVersion
  != pluginContractSchema
  != serverVersion
  != resourceRevision
  != backendGeneration
```

## Read and observation flow

```text
VDR Core or local provider
  -> bounded native fact
  -> plugin/local adapter
  -> immutable observation
  -> Backend Agent [future for remote sites]
  -> Control Plane ingestion
  -> Suite-owned persistent read models
  -> authorized Suite API
  -> VdrSuiteClientApi
  -> frontend owner
```

Current local paths may use direct private adapters while preserving the same domain/service/repository boundary.

Read invariants:

- backend-native IDs remain backend scoped;
- producer sequence, snapshot generation, backend generation and resource revision are distinct;
- title, time, channel name and filesystem path are evidence, not universal identity;
- sequence gaps trigger resynchronization rather than guessed continuity;
- partial multi-backend reads declare their partial nature;
- normal documented Genre/Search GETs remain query-only and provider-free.

## Safe mutation and durable execution target

```text
client command
  -> request/correlation/actor context
  -> authentication and centralized authorization
  -> backend access/capability/revision/generation preconditions
  -> durable Operation + idempotency scope
  -> required pre-dispatch AccountabilityEvent/outbox
  -> durable Job/Attempt claim
  -> recorded dispatch boundary
  -> fenced Agent/native command
  -> authoritative readback and verification
  -> succeeded | failed-before-dispatch | failed-verified | outcome-unknown
  -> reconciliation when outcome is uncertain
```

Current Recording actions, selected Timer/SearchTimer paths and Phase-62 policy/accountability provide strong bounded foundations. Phase-63 Slice 1 adds lifecycle fencing only; the universal mutation target still requires later Phase-63 command/result runtime.

Mutation invariants:

- no production mutation uses an implicit backend;
- no protected dispatch begins when required authorization/accountability evidence cannot be persisted;
- possible dispatch followed by timeout never causes speculative duplicate mutation;
- retries preserve the same logical operation/idempotency scope;
- verification is domain-specific and stronger than adapter acknowledgement;
- stale Agent generations and stale job claims cannot complete current work.

## Identity and provenance model

```text
MetadataEntity
  -> may describe many broadcasts/recordings

ProgramEvent
  -> canonical programme occurrence [partial/future expansion]
  -> BackendEventRef observations from one or more backends
  -> provider/native field evidence

RecordingId
  -> NativeRecordingBinding
  -> MetadataAssignment
  -> ArtworkReference

TimerIntent
  -> TimerAssignment primary
  -> optional deliberate replica assignment
  -> NativeTimerBinding per assignment
```

Identity invariants:

- Suite IDs and backend-native IDs are never interchangeable;
- metadata/editorial identity remains separate from broadcast occurrence identity;
- provider values retain source, state and evidence;
- external native objects are adopted only through an explicit decision;
- deliberate replicas are explicit assignments, not unexplained duplicate timers.

Phase 61 implements backend-scoped metadata target bindings, people and Genre evidence/assignments for its accepted scope. Universal ProgramEvent, Recording lifecycle revision and TimerIntent identity remain broader target work.

## Timer orchestration target

```text
user / SearchTimer / epgsearch / automation provider
  -> AutomationProposal or TimerIntent
  -> central scheduler decision
  -> TimerAssignment
  -> backend/channel/capability/health eligibility
  -> durable operation/job
  -> NativeTimerBinding
  -> readback and reconciliation
```

SearchTimer and epgsearch are automation sources. They do not own the future global multi-backend scheduler or bypass central authorization/accountability.

## Media session target

```text
client playback request
  -> Control Plane authorization/routing
  -> MediaSession + short-lived MediaAccessGrant
  -> Streaming Gateway
  -> route epoch and ProviderStreamLease
  -> private Agent/Streamdev/provider path
  -> media bytes
```

Permanent Streamdev, VDR or Agent URLs are never public. Playback and download permissions remain separate. Current LiveOverlay/SSE is not media streaming.

## Legacy OSD compatibility target

```text
local structured/native OSD observation
  -> immutable ordered OsdFrame / delta
  -> LegacyOsdSession
  -> viewer fan-out and full resync
  -> one fenced controller lease
  -> allowlisted/rate-limited RemoteAction input
```

Viewing precedes control. The current RemoteAction/LiveOverlay runtime is a reusable foundation but does not implement OSD frames, sessions, sequencing or controller lease.

## Public API and accountability target

A resource enters stable `/api/v1` only when these are explicit:

- stable identity and backend scope;
- actor authorization and redaction;
- resource revision and conditional mutation rules;
- structured errors and request/correlation IDs;
- pagination/partial-result semantics;
- operation/idempotency behaviour;
- accountability producer and outcome evidence;
- compatibility/deprecation policy.

The current transition API and `VdrSuiteClientApi` are strong foundations, not a completed stable public API.

## Strict implementation order

```text
Completed Phase 61 and post-phase hardening/features
  -> Phase 62 Identity, RBAC and Accountability
  -> Phase 63 Backend Agent and Secure Multi-Site Runtime
  -> Phase 64 Timer Intent and Orchestration
  -> Phase 65 Streaming Gateway
  -> Phase 66 Legacy OSD Bridge
  -> Phase 67 Public API and Client Hardening
  -> Phase 68 Recommendation and Knowledge Graph
```

## Related documents

- [Domain Dependency Map](../planning/domain-dependency-map.md)
- [Implementation Dependency Map](../planning/implementation-dependency-map.md)
- [Strict Roadmap](../planning/roadmap.md)
- [ADR Index](../adr/index.md)