# VDR-Suite Target Platform Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Current Architecture State](../development/current-architecture-state.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Domain Dependency Map](../planning/domain-dependency-map.md)
- [ADR Index](../adr/index.md)

---

## Status and purpose

This document defines the stable target ownership and contract boundaries of VDR-Suite.

It intentionally does **not** contain current branch heads, active PR numbers, CI checkpoints or a mutable implementation overlay. Exact operational progress belongs only in [Current State](../CURRENT.md). Implemented architecture is summarized without active-head duplication in [Current Architecture State](../development/current-architecture-state.md).

A target box or accepted ADR is not proof that its runtime implementation is complete.

## Platform and trust boundary

```text
Web / Desktop / Mobile / TV / Automation clients
                         |
                         | authenticated Suite client contracts
                         v
+---------------------------------------------------------------+
| VDR-Suite Control Plane                                       |
|                                                               |
| actor identity, sessions, authorization and policy            |
| Suite-owned domain services and repositories                  |
| backend observations and read-model projection                |
| operations, jobs, protected writes and reconciliation         |
| metadata, people, Genres and search                           |
| TimerIntent scheduling and assignments                        |
| MediaSession / OSD session policy                             |
| accountability and security-event linkage                     |
+--------------------------+------------------------------------+
                           | protected Agent protocol
                           v
+---------------------------------------------------------------+
| Backend Agent                                                 |
| enrolled identity, generation, heartbeat, lease, capabilities |
| observations, commands, receipts, results and local fencing   |
| explicit local provider ownership/selection and cleanup       |
+--------------------------+------------------------------------+
                           | local/private adapter contracts
                           v
+---------------------------------------------------------------+
| VDR site                                                      |
| VDR Core | SuiteBridge | RESTfulAPI | SVDRP | Streamdev       |
| epgsearch | TVScraper | native files and databases            |
+---------------------------------------------------------------+
```

Rules:

- VDR remains authoritative for VDR-native runtime state and execution.
- The Control Plane owns external identity, authorization, policy, orchestration, reconciliation and client contracts.
- Backend Agents own bounded site-local observation/execution and local provider access, not global policy.
- Private plugins/providers are never the public security or compatibility boundary.
- Reachability does not grant provider authority.
- Clients receive no permanent VDR/plugin/Agent/provider credentials or private provider URLs.

## Contract separation

```text
Public Client API
  Suite resources, commands, operations, errors and revisions

Backend Agent Protocol
  enrollment, generation, lease, capabilities, observations,
  fenced commands, receipts, results and reconnect evidence

Media Plane
  MediaSession, short-lived access grant, route epoch,
  ProviderStreamLease and media bytes

Legacy OSD Plane
  session, viewer binding, ordered frames/deltas,
  controller lease and allowlisted input

Plugin Local Contract
  bounded native capabilities, observations and execution facts

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
  -> Backend Agent where applicable
  -> authenticated generation/instance-fenced envelope
  -> complete snapshot or exact-next change batch
  -> Control Plane ingestion
  -> atomic receipt/fact plus ingestion cursor
  -> Suite-owned persistent read models
  -> authorized Suite API
  -> client API wrapper
  -> frontend owner
```

Read invariants:

- backend-native IDs remain backend scoped;
- producer sequence, snapshot generation, backend generation and resource revision are distinct;
- titles, times, channel names and filesystem paths are evidence, not universal identity;
- sequence gaps require explicit resynchronization instead of guessed continuity;
- equivalent replay is idempotent and conflicting replay is rejected;
- stale Agent instances and stale backend generations cannot advance current ingestion;
- partial multi-backend reads declare their partial nature;
- ordinary frontend GET paths do not acquire hidden provider authority.

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
- `agentInstanceId` fences stale processes within one backend generation;
- `snapshotGeneration` identifies one complete domain lineage;
- `producerSequence` is monotone within that lineage;
- `resourceRevision` remains domain evidence, not transport continuity;
- database failure cannot advance the ingestion cursor;
- repository code owns SQLite;
- manual SQLite inspection is not a normal acceptance dependency.

## Safe mutation and durable execution target

```text
client command
  -> request/correlation/actor context
  -> authentication and centralized authorization
  -> backend access/capability/revision/generation preconditions
  -> durable Operation + idempotency scope
  -> required pre-dispatch accountability evidence
  -> durable Job/Attempt claim
  -> recorded dispatch boundary
  -> fenced Agent/native command
  -> authoritative readback and verification
  -> succeeded | failed-before-dispatch | failed-verified | outcome-unknown
  -> reconciliation when outcome is uncertain
```

Mutation invariants:

- no production mutation uses an implicit backend;
- no protected dispatch begins when required authorization/accountability evidence cannot be persisted;
- a possible dispatch followed by timeout never causes speculative duplicate mutation;
- retries preserve the same logical operation/idempotency scope;
- resource-scoped concurrency/lease fencing prevents conflicting protected writes;
- backend generation and provider ownership are explicit execution fences;
- verification is domain-specific and stronger than adapter acknowledgement where required;
- stale Agent generations, stale claims and stale provider epochs cannot complete current work;
- an unknown outcome remains unknown until evidence permits reconciliation.

## Identity and provenance model

```text
MetadataEntity
  -> may describe many broadcasts/recordings

ProgramEvent
  -> canonical programme occurrence where available
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
- deliberate replicas are explicit assignments, not unexplained duplicate native timers.

## Timer orchestration target

```text
user / SearchTimer / epgsearch / automation provider
  -> AutomationProposal or TimerIntent
  -> central scheduler decision
  -> TimerAssignment
  -> backend/channel/capability/health eligibility
  -> durable protected operation
  -> NativeTimerBinding
  -> authoritative native readback and reconciliation
```

Timer rules:

- the user request remains backend-neutral;
- SearchTimer and epgsearch are automation sources, not owners of the global multi-backend scheduler;
- provider reachability does not authorize execution;
- primary and deliberate replica assignments are explicit;
- unknown mutation outcomes block unsafe blind replacement/retry;
- a native VDR Timer is an execution binding/evidence object, not the owner of Suite intent.

## Media session target

```text
client playback request
  -> Control Plane authorization/admission
  -> MediaSession + short-lived MediaAccessGrant
  -> Streaming Gateway
  -> MediaRoute + route epoch
  -> Backend Agent
  -> explicitly owned ProviderStreamLease
  -> private StreamProvider / VDR source
  -> media bytes
```

Media rules:

- Streamdev may be an explicitly owned internal StreamProvider; it is not the public playback API.
- permanent Streamdev, VDR, Agent or provider URLs are never public client contracts;
- user login/session identifiers and `mediaSessionId` are not reusable URL bearer credentials;
- playback and unrestricted download permissions remain separate;
- capability negotiation chooses Suite media profiles, not provider identities;
- transformation preference is pass-through, then remux/repackage, then transcode only when materially required;
- slow clients, network backpressure and expensive media processing remain outside VDR callbacks/locks;
- ordinary live playback does not silently imply timeshift;
- LiveOverlay/SSE carries state updates, not media bytes.

## Client playback boundary

First-party clients use a small Suite playback abstraction over mature platform-appropriate playback engines. VDR-Suite does not require one universal cross-platform decoder/renderer core.

The server owns authorized MediaSession/profile semantics. The platform player owns decode/render/audio/lifecycle integration. Kodi may be an architectural reference or client integration target without becoming a shared player-code dependency.

## Legacy OSD compatibility target

```text
local structured/native OSD observation
  -> immutable ordered OsdFrame / delta
  -> LegacyOsdSession
  -> viewer fan-out and full resync
  -> one fenced controller lease
  -> allowlisted/rate-limited input
```

Viewing and controlling are separate permissions. Legacy OSD compatibility is not the same subsystem as LiveOverlay or media streaming.

## Public API target

A resource enters the stable public API only when these are explicit:

- stable Suite identity and backend scope;
- actor authorization and redaction;
- resource revision and conditional mutation rules;
- structured errors and request/correlation IDs;
- pagination and partial-result semantics;
- operation/idempotency behaviour;
- accountability producer/outcome evidence;
- compatibility and deprecation policy.

The public API version, Agent protocol version, media protocol version and plugin-local contract remain independently evolvable.

## Golden product acceptance

Architecture completion is not judged only by component/unit tests. The platform must also support vertical user journeys such as:

- channel/EPG selection -> authorized Live playback -> picture and sound -> clean channel switch;
- Recording detail -> authorized playback -> seek where supported -> stop/resume semantics;
- EPG programme -> TimerIntent -> TimerAssignment -> NativeTimerBinding -> authoritative VDR result;
- multi-backend scheduling without private provider selection by the user;
- classified failure without hidden unsafe recovery.

See [Golden User Journeys](../planning/golden-user-journeys.md).

## Implementation-order rule

The binding numbered phase sequence and completion gates are maintained in the [Strict Roadmap](../planning/roadmap.md). The current completed/active/next phase position is maintained only in [Current State](../CURRENT.md).

This target architecture must not duplicate active PR tips or exact repository heads.

## Related decisions

- [ADR-0039: Backend Agent and Control Plane Boundary](../adr/ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](../adr/ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](../adr/ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0043: Job Claim, Retry and Saga Execution Model](../adr/ADR-0043-job-claim-retry-saga-execution-model.md)
- [ADR-0044: Timer Intent, Assignment and Native Timer Model](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0045: Canonical EPG Event Identity and Provenance](../adr/ADR-0045-canonical-epg-event-identity-provenance.md)
- [ADR-0046: Streaming Gateway and Media Session Boundary](../adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0047: Legacy OSD Compatibility Bridge](../adr/ADR-0047-legacy-osd-compatibility-bridge.md)
- [ADR-0048: Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [ADR-0049: Audit and Security Event Model](../adr/ADR-0049-audit-security-event-model.md)
- [ADR-0050: Domain Repository SQLite Boundary](../adr/ADR-0050-domain-repository-sqlite-boundary.md)

## Back

- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)
