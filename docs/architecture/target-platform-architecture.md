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

Implemented on merged `main @ a9620179a442155f0860ef3182ca39186ac46a57`:

- backend registry, backend-scoped snapshots/caches/change feed and server-enforced read-only mode;
- Recordings 2 and guarded Recording actions;
- SearchTimer and native Timer foundations;
- completed Phase 61 persistent Recording/EPG metadata, people, artwork references and Genre read models;
- query-only provider-free Genre and Global Search GET paths;
- backend-neutral RemoteAction and LiveOverlay contracts;
- modular browser Client API boundaries;
- completed Phase-62 production actor identity, scoped RBAC, browser-session security and append-only accountability;
- completed Phase-63 Slice-1 Agent enrollment, protected outbound transport, technical identity, protocol/generation/instance fencing, heartbeat/lease, read-only capabilities, reconnect and credential lifecycle foundation.

Active contract work in Draft PR #138:

- Phase 63 Slice 2 — Read-only Observation and Snapshot Ingestion Foundation;
- independent backend generation, Agent instance, observation domain, snapshot generation, producer sequence and resource revision;
- complete baseline, exact-next sequence, idempotent replay and explicit `resync-required`;
- Suite-owned transactional receipt/fact and ingestion-cursor persistence;
- initial bounded `backend-health` domain;
- no command/result or VDR-native mutation runtime.

Not yet implemented as complete target runtime:

- Phase-63 observation/snapshot runtime implementation beyond the contract;
- durable command/results, native execution and provider selection;
- universal revision/idempotency/job reconciliation;
- TimerIntent orchestration;
- Streaming Gateway;
- legacy OSD bridge;
- stable public `/api/v1`;
- recommendation/knowledge graph.

Phase 63 is not complete.

## Platform and trust boundary

```text
Web / Desktop / Mobile / TV / Automation clients
                         |
                         | future authenticated public API
                         v
+---------------------------------------------------------------+
| VDR-Suite Control Plane                                       |
|                                                               |
| actor identity, sessions, RBAC and policy       [implemented] |
| Suite-owned domain services and repositories    [foundation]  |
| Agent observation ingestion                      [Slice 2]     |
| operations, jobs and reconciliation             [partial]     |
| metadata, people, Genres and search              [implemented] |
| Timer scheduler and assignments                  [Phase 64]    |
| media/OSD session policy                         [65 / 66]     |
| accountability event store                      [implemented] |
+--------------------------+----------------------+-------------+
                           | protected Agent protocol [Phase 63]
                           v
+---------------------------------------------------------------+
| Backend Agent                                                 |
| enrolled identity, generation, heartbeat, lease, capabilities |
| lifecycle [Slice 1]; read-only observations [Slice 2 target]  |
| commands/results and native execution [later Phase 63]        |
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
- Agent observations are evidence with explicit provenance, not hidden authority over direct-adapter facts.
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
  != snapshotGeneration
  != producerSequence
```

## Read and observation flow

```text
VDR Core or local provider
  -> bounded native fact
  -> plugin/local adapter
  -> immutable observation
  -> Backend Agent [remote sites]
  -> authenticated generation/instance-fenced envelope
  -> complete snapshot or exact-next change batch
  -> Control Plane ingestion
  -> atomic receipt/fact plus ingestion cursor
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
- sequence gaps trigger `resync-required` rather than guessed continuity;
- changes require an accepted complete baseline;
- equivalent replay is idempotent and conflicting replay is rejected;
- stale Agent instances and stale backend generations cannot advance current ingestion;
- partial multi-backend reads declare their partial nature;
- normal documented Genre/Search GETs remain query-only and provider-free.

## Observation ingestion target

```text
authenticated Agent observation
  -> request/correlation/technical actor context
  -> backend + Agent + instance + generation fencing
  -> capability-declared observation domain
  -> payload and item-count bounds
  -> complete baseline OR exact-next change sequence
  -> idempotent replay / conflict / gap classification
  -> immutable receipt/fact + cursor transaction
  -> accepted | replayed | rejected | resync-required
  -> Suite-owned read-model projection
```

Observation invariants:

- `backendGeneration` fences replacement or resynchronized backend state;
- `agentInstanceId` fences stale processes in one backend generation;
- `snapshotGeneration` identifies one complete domain lineage;
- `producerSequence` is monotone within that lineage;
- `resourceRevision` remains resource/domain evidence, not transport continuity;
- database failure cannot advance the cursor;
- repository code owns SQLite; HTTP and Agent client code do not issue direct SQLite statements;
- no manual SQLite inspection is required for acceptance.

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

Current Recording actions, selected Timer/SearchTimer paths and Phase-62 policy/accountability provide strong bounded foundations. Phase-63 Slice 1 adds lifecycle fencing. Phase-63 Slice 2 precedes command delivery by establishing trustworthy read-only observation continuity. The universal mutation target still requires later Phase-63 command/result runtime.

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
  -> Phase 63 Slice 1 Agent lifecycle
  -> Phase 63 Slice 2 Observation and Snapshot Ingestion
  -> later Phase 63 command/result and provider ownership slices
  -> Phase 64 Timer Intent and Orchestration
  -> Phase 65 Streaming Gateway
  -> Phase 66 Legacy OSD Bridge
  -> Phase 67 Public API and Client Hardening
  -> Phase 68 Recommendation and Knowledge Graph
```

## Related documents

- [Phase 63 Slice-1 Closeout](../development/phase-63-slice-1-closeout.md)
- [Phase 63 Observation and Snapshot Ingestion](../development/phase-63-observation-ingestion.md)
- [Domain Dependency Map](../planning/domain-dependency-map.md)
- [Implementation Dependency Map](../planning/implementation-dependency-map.md)
- [Strict Roadmap](../planning/roadmap.md)
- [ADR Index](../adr/index.md)
