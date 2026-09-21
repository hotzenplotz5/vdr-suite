# VDR-Suite Target Platform Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Architecture Index](index.md)
- [Current State](../CURRENT.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Domain Dependency Map](../planning/domain-dependency-map.md)
- [Implementation Dependency Map](../planning/implementation-dependency-map.md)
- [ADR Index](../adr/index.md)

---

## Purpose

This document is the canonical target-architecture diagram set for the architecture contract package accepted through ADR-0049.

It shows the intended ownership and communication boundaries. It does not claim that all shown components are already implemented.

For implemented runtime truth, use:

- [Current Architecture State](../development/current-architecture-state.md)
- [Completed Phases](../development/completed-phases.md)
- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)

---

## Status Vocabulary

The diagrams distinguish:

```text
implemented foundation
  existing repository code or tested behavior

accepted target
  architecture decided by an ADR, runtime incomplete

future implementation
  planned work in Phase 60.15 through Phase 68
```

A box in a target diagram is not evidence that the runtime exists.

---

# 1. Platform and Trust Boundary

```text
+-----------------------------------------------------------------------+
| Clients and external actors                                           |
|                                                                       |
| Web UI | Desktop | Mobile | TV | Automation | Administration          |
+-----------------------------------+-----------------------------------+
                                    |
                                    | authenticated public contract
                                    v
+-----------------------------------------------------------------------+
| VDR-Suite Control Plane                                               |
|                                                                       |
| Public API /api/v1                                                    |
| Identity, sessions, RBAC and policy                                   |
| Suite-owned domain services                                           |
| Operations, jobs, sagas and reconciliation                            |
| Metadata, canonical identities and provenance                         |
| Timer scheduler and assignment                                        |
| Media-session and OSD-session policy                                  |
| Accountability event store                                            |
+----------------------+----------------------+-------------------------+
                       |                      |
                       | protected Agent      | public media access
                       | protocol             | grants
                       v                      v
+----------------------------------+   +--------------------------------+
| Backend Agent                    |   | Streaming Gateway              |
|                                  |   |                                |
| enrolled device identity         |   | validates short-lived grants   |
| backend generation and lease     |   | hides internal providers       |
| local capability publication     |   | enforces route epoch/revocation|
| snapshot and event transport     |   | streams media bytes            |
| fenced command execution         |   +---------------+----------------+
| reconnect reconciliation         |                   |
| local provider and plugin access |                   | internal route
+----------------+-----------------+                   v
                 |                         +-----------------------------+
                 | local-only contracts    | Agent media/provider path   |
                 v                         +---------------+-------------+
+-----------------------------------------------------------------------+
| VDR site                                                             |
|                                                                       |
| vdr-plugin-suite-bridge | RESTfulAPI | SVDRP | Streamdev | osd2web    |
| epgsearch | TVScraper | scraper2vdr | native VDR files and databases  |
|                                                                       |
|                         VDR Core                                      |
+-----------------------------------------------------------------------+
```

Rules:

- Clients do not receive permanent Backend Agent, VDR plugin, SVDRP, Streamdev or filesystem credentials.
- Backend Agents normally connect outward from the site.
- The Control Plane owns global identity, authorization, orchestration and policy.
- The Agent owns the bounded site-local transport and execution boundary.
- VDR remains authoritative for VDR-native runtime state.
- A plugin is never the public security boundary or global scheduler.

Relevant decisions:

- ADR-0039 through ADR-0041
- ADR-0046 through ADR-0049

---

# 2. Contract and Data-Plane Separation

```text
Public Client API
  /api/v1
  resources, commands, operations, errors, revisions

Backend Agent Protocol
  enrollment, generation, lease, capabilities, snapshots,
  fenced commands, receipts, results and reconnect evidence

Media Plane
  MediaSession, MediaAccessGrant, Gateway connection,
  ProviderStreamLease and media bytes

Legacy OSD Plane
  LegacyOsdSession, viewer binding, ordered frames/deltas,
  controller lease and allowlisted input commands

Plugin Local Contract
  bounded capabilities, native observations, command/result facts

Internal C++ APIs
  services, repositories, adapters and serializers
```

These contracts have independent versions and independent security rules.

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

---

# 3. Read and Observation Flow

```text
VDR Core or local provider
        |
        | native fact under bounded local rules
        v
Plugin / local adapter
        |
        | immutable bounded observation
        v
Backend Agent
        |
        | backendId + backendGeneration + producer sequence
        v
Control Plane ingestion
        |
        +--> native snapshot and change feed
        |
        +--> BackendEventRef / EventObservation
        |
        +--> Recording native binding observation
        |
        +--> NativeTimerBinding observation
        |
        +--> bounded producer evidence for accountability
        v
Suite-owned canonical read models
        |
        +--> authorized /api/v1 resources
        +--> frontend client API
        +--> scheduler and reconciliation inputs
```

Read-side invariants:

- backend-native identifiers remain backend scoped;
- producer sequence is not a resource revision;
- backend generation is not snapshot generation;
- a transient cache key is not a public stable identity;
- missing sequence data causes resynchronization, not guessed state;
- partial multi-backend reads declare that they are partial.

---

# 4. Safe Mutation and Durable Execution Flow

```text
Client command
    |
    v
Public API request context
requestId | correlationId | actor | backend scope
    |
    v
Authentication and authorization
    |
    +--> denied
    |      |
    |      +--> structured problem response
    |      +--> required accountability event
    |
    v
Validation and preconditions
capability | read-only policy | expectedRevision | backendGeneration
    |
    v
Operation persistence
operationId | idempotency scope | normalized request fingerprint
    |
    +--> required pre-dispatch AccountabilityEvent / outbox
    |
    v
Durable job and attempt
jobId | attemptId | claimEpoch | claimToken
    |
    v
Record dispatch boundary before external call
    |
    v
Fenced Agent command
backendId | backendGeneration | operationId | idempotency key
    |
    v
Bounded native executor
    |
    v
Authoritative readback / verification
    |
    +--> requested state proven ------> succeeded
    |
    +--> no dispatch proven ----------> failed_before_dispatch
    |
    +--> effect disproven ------------> failed_verified
    |
    +--> outcome cannot be proven ----> outcome_unknown
                                            |
                                            v
                                      reconciliation job
```

Mutation invariants:

- no production mutation uses an implicit backend;
- no dispatch starts when required pre-dispatch accountability evidence cannot be persisted;
- the durable dispatch boundary is written before the external call;
- timeout or transport failure after possible dispatch never triggers speculative duplicate mutation;
- retries preserve the same logical operation and idempotency scope;
- verification is domain specific and stronger than executor acknowledgement;
- reconciliation appends evidence and never rewrites history;
- stale Agent generations and stale job claims cannot complete current work.

Relevant decisions:

- ADR-0042
- ADR-0043
- ADR-0048
- ADR-0049

---

# 5. Identity and Provenance Model

```text
MetadataEntity
  editorial work, movie, series or episode
        |
        | may describe many broadcasts
        v
ProgramEvent
  canonical Suite identity for one programme occurrence
        |
        +--> EventObservation from Backend A
        +--> EventObservation from Backend B
        +--> provider evidence
        |
        v
TimerIntent
  desired recording outcome
        |
        +--> TimerAssignment primary
        |        |
        |        v
        |   NativeTimerBinding on Backend A
        |
        +--> optional deliberate replica assignment
                 |
                 v
            NativeTimerBinding on Backend B
```

```text
RecordingId
  stable Suite recording identity
        |
        v
NativeRecordingBinding
  backendId + backendGeneration + native identity/revision
        |
        v
MetadataAssignment
  links the Recording to Suite-owned metadata and provenance
```

Identity invariants:

- `MetadataEntityId`, `ProgramEventId`, `TimerIntentId`, `TimerAssignmentId`, `RecordingId` and native IDs are never interchangeable;
- title, time, path and channel name are matching evidence, not stable identity;
- deliberate replicas are explicit assignments, not unexplained duplicate native timers;
- provider-derived values retain provenance and confidence;
- external native objects remain external unless an explicit adoption decision succeeds.

Relevant decisions:

- ADR-0014
- ADR-0038
- ADR-0044
- ADR-0045

---

# 6. Timer Orchestration Boundary

```text
User request / SearchTimer / epgsearch / automation provider
                         |
                         | proposal or direct intent creation
                         v
                    TimerIntent
                         |
                         v
              central scheduler decision
                         |
          +--------------+--------------+
          |                             |
          v                             v
TimerAssignment primary       deliberate replica assignment
          |                             |
          v                             v
Backend eligibility             independent eligibility
capability | channel | health    capability | channel | health
          |                             |
          v                             v
ADR-0042 operation and ADR-0043 jobs
          |                             |
          v                             v
NativeTimerBinding              NativeTimerBinding
```

The SearchTimer definition remains an automation source. It is not the global scheduler and does not independently own cross-backend native writes.

---

# 7. Media Session Boundary

```text
Client
  |
  | request Live TV or Recording playback
  v
Control Plane authorization and routing policy
  |
  v
MediaSession
  |
  +--> MediaRoute
  +--> short-lived MediaAccessGrant
  |
  v
Streaming Gateway
  |
  v
Backend Agent
  |
  v
ProviderStreamLease
  |
  +--> Streamdev
  +--> recording file/provider
  +--> future remux/transcode provider
```

Media invariants:

- `mediaSessionId` is not a bearer credential;
- a client never receives a permanent internal provider URL;
- playback permission is separate from download/export permission;
- read-only backend policy may allow playback while still forbidding persistent mutation;
- media bytes do not travel through ordinary JSON API responses;
- Agent and provider routes are fenced by backend generation and route epoch.

---

# 8. Legacy OSD Compatibility Boundary

```text
Native VDR OSD
     |
     | copied bounded state
     v
Plugin / local OSD adapter
     |
     v
Backend Agent sequencer
     |
     +--> full OsdFrame
     +--> ordered OsdDelta
     |
     v
LegacyOsdSession in Control Plane
     |
     +--> many OsdViewerBinding objects
     |
     +--> at most one OsdControllerLease
                 |
                 v
          allowlisted OsdInputCommand
                 |
                 v
          fenced local execution
```

OSD invariants:

- domain-first APIs remain the primary user interface;
- OSD control is a privileged compatibility feature;
- read-only backends may expose viewing but not OSD control;
- sequence loss requires full resynchronization;
- expired or disconnected commands are never replayed later;
- arbitrary shell, SVDRP, plugin-service or raw-key tunnels are forbidden.

---

# 9. Accountability and Security Flow

```text
Authentication / authorization / operation / job / Agent / domain transition
                                   |
                                   v
                        AccountabilityEvent policy
                                   |
                    +--------------+--------------+
                    |                             |
                    v                             v
                 audit                        security
                    |                             |
                    +--------------+--------------+
                                   |
                                   v
                         append-only canonical store
                                   |
                 +-----------------+------------------+
                 |                 |                  |
                 v                 v                  v
          protected query    retention process    export adapter
                 |                 |                  |
                 +--> each privileged audit operation is itself audited
```

Accountability invariants:

- logs, metrics, debug state, domain events and AccountabilityEvents remain separate;
- historical actor, decision and outcome are not edited in place;
- corrections and annotations are appended;
- secrets, media bytes, OSD frames and full provider payloads are excluded;
- Agent evidence is accepted only for its enrolled identity, backend and generation;
- a security event may feed alerts, but clearing an alert does not delete evidence.

---

# 10. Ownership Matrix

| Concern | Control Plane | Backend Agent | Plugin / local adapter | VDR Core / provider |
| --- | --- | --- | --- | --- |
| Public user identity | Owns | Does not own | Does not own | Does not own |
| RBAC and backend policy | Owns | Enforces supplied bounded command scope | Does not own global policy | Native behavior only |
| Backend generation and lease | Authoritative policy/state | Maintains local session evidence | Reports local lifecycle facts | Runtime process |
| Canonical Suite identities | Owns | Transports references | Reports native identities | Owns native identities |
| Operations and idempotency | Owns | Preserves command identity | Bounded execution only | Applies native effect |
| Jobs, retries and sagas | Owns | Executes assigned bounded work | No global retry ownership | No Suite workflow ownership |
| Canonical metadata/provenance | Owns | Transports observations | Reports bounded provider facts | Produces native/provider data |
| Timer scheduler | Owns | Executes assigned commands | Native timer access only | Owns native timer state |
| MediaSession and grants | Owns | Enforces local route/lease | Local provider access only | Produces media |
| Legacy OSD session/lease | Owns | Sequences/fences | Copies state and applies allowlisted input | Owns native OSD state |
| Canonical accountability store | Owns | Buffers and submits bounded evidence | Local diagnostics only | Native facts only |
| Public API | Owns | Never exposes directly | Never exposes directly | Never exposes directly |

---

# 11. Deployment Topology for Two Houses

```text
House A / Control Plane site

Clients
   |
   v
VDR-Suite Control Plane ---- Streaming Gateway
   |                              |
   | Agent protocol               | media route
   v                              v
Backend Agent A -------------- local VDR A


House B / remote site

Outbound protected Agent connection
   |
   v
Backend Agent B -------------- local VDR B
   |
   +--> no publicly exposed RESTfulAPI, SVDRP, Streamdev or plugin port
```

The user may receive full rights for VDR A and read/stream-only rights for VDR B. The same frontend and public API remain in use; authorization and backend scope decide the allowed operation.

---

# 12. Current Foundation Versus Future Runtime

Already present as foundations include:

- daemon and REST runtime;
- backend registry and backend-scoped access mode;
- snapshot, cache and change-feed infrastructure;
- RESTfulAPI adapter and HTTP abstractions;
- guarded Recording actions;
- native Timer actions;
- SearchTimer preview and execution foundations;
- backend-scoped EPG cache and searches;
- frontend Client API and module ownership;
- Suite Bridge read-only foundations through SB.7.

Accepted but not yet complete include:

- production actor identities and RBAC;
- universal revisions and durable idempotency;
- production job claims, retries and sagas;
- Backend Agent runtime and secure multi-site transport;
- canonical ProgramEvent persistence and resolver;
- TimerIntent persistence, scheduler and reconciler;
- Streaming Gateway runtime;
- Legacy OSD bridge runtime;
- `/api/v1` migration;
- append-only accountability persistence and outbox.

---

## Architecture Rules

1. VDR remains the native authority for native VDR state.
2. The Control Plane owns global identity, policy, orchestration and public contracts.
3. Backend Agents are enrolled, generation-bound site representatives, not autonomous Control Planes.
4. Plugins and local adapters remain narrow, bounded and VDR-lock-safe.
5. Public clients never depend on backend-specific transports or filesystem paths.
6. Reads preserve backend scope, provenance and partial-result truth.
7. Mutations require authorization, capability, revision, generation, idempotency, durable dispatch evidence and verification.
8. Uncertain outcomes reconcile before retry.
9. Streaming and Legacy OSD use isolated, short-lived session boundaries.
10. Accountability evidence is structured, append-only and separate from runtime logs.
11. Accepted architecture does not become implemented runtime until its phase exit criteria and tests pass.

---

## Related Decisions

- [ADR-0038](../adr/ADR-0038-suite-metadata-database-and-external-provider-strategy.md)
- [ADR-0039](../adr/ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040](../adr/ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041](../adr/ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0042](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0043](../adr/ADR-0043-job-claim-retry-saga-execution-model.md)
- [ADR-0044](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0045](../adr/ADR-0045-canonical-epg-event-identity-provenance.md)
- [ADR-0046](../adr/ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0047](../adr/ADR-0047-legacy-osd-compatibility-bridge.md)
- [ADR-0048](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [ADR-0049](../adr/ADR-0049-audit-security-event-model.md)

---

## Back

- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)
