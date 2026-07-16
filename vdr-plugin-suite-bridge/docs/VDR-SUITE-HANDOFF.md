# VDR-Suite / Suite Bridge Gold-Standard Handoff

## Purpose

This document is the shared operational coordination contract between:

- the VDR-Suite Control Plane;
- the VDR-Suite Backend Agent;
- `vdr-plugin-suite-bridge`;
- VDR Core.

The target is:

> VDR-Suite shall be the gold standard for safe, modern, robust and multi-backend VDR integration while preserving VDR-native correctness.

Accepted VDR-Suite ADRs remain authoritative for long-term platform decisions. Stable plugin documents remain authoritative for implemented plugin slices. This handoff records the actual synchronized implementation, test, live-acceptance and dependency state between the workstreams.

---

## Repository and Branches

| Item | Value |
| --- | --- |
| Repository | `hotzenplotz5/vdr-suite` |
| VDR-Suite integration source of truth | `main` |
| Suite Bridge development branch | `feature/vdr-plugin-suite-bridge-foundation` |
| Plugin directory | `vdr-plugin-suite-bridge/` |
| Shared handoff | `vdr-plugin-suite-bridge/docs/VDR-SUITE-HANDOFF.md` |

The bridge branch is synchronized with current `main` before a new plugin slice begins.

Suite work must not edit bridge implementation files unless the change is explicitly coordinated through this handoff. Plugin work must not modify unrelated Suite production files merely to complete a local bridge slice.

When a shared contract changes, both workstreams record:

- which side owns the change;
- whether it is implemented or only accepted;
- schema or capability-version impact;
- compatibility and migration requirements;
- the exact tests that prove the boundary.

No force-push or destructive reset is permitted for coordinated bridge work.

---

## Status Vocabulary

| State | Meaning |
| --- | --- |
| `implemented` | Present in source, covered by tests and available at the stated boundary. |
| `accepted-contract` | Decided by an accepted ADR or explicit shared contract, but not fully implemented. |
| `planned` | Intended direction that is not yet an accepted shared contract. |
| `blocked` | Cannot proceed until a named Suite or plugin prerequisite exists. |
| `disabled` | Deliberately unavailable at runtime. |
| `superseded` | Retained for history but no longer authoritative. |

An accepted ADR does not make a runtime feature `implemented`.

A plugin capability must never report `available` merely because source code exists. It becomes available only after the required build, contract tests and live VDR acceptance have passed.

---

## Current Plugin Snapshot

| Item | Current value |
| --- | --- |
| Last completed slice | `SB.6 - Read-only native SVDRP contract` |
| Plugin name | `suitebridge` |
| Plugin version | `0.7.0` |
| Shared object | `libvdr-suitebridge.so.<VDR-APIVERSION>` |
| Implementation commit | `30516faaa24d12973cb3864bd33f8fb52976a4a0` |
| Live-acceptance repository head | `ae38f546fd437f803926dd41a1e5f6442626f784` |
| Live VDR version | `2.7.9` |
| Live VDR API version | `11` |
| Mutation state | `disabled` |

Implemented local endpoint:

`PLUG suitebridge SNAP`

Current behavior:

- captures one immutable status snapshot;
- returns one deterministic compact JSON line;
- valid `SNAP` returns reply code `900`;
- `SNAP` with options returns reply code `504`;
- payload preparation failure returns reply code `451`;
- unknown commands remain unhandled for the VDR default response;
- command matching is case-insensitive;
- no plugin-owned listener, outbound connection, database, worker thread or filesystem mutation exists.

### Current capabilities

| Capability | State |
| --- | --- |
| `lifecycle` | `available` |
| `status-events` | `available` |
| `snapshots` | `available` |
| `local-contract` | `available` |
| `mutations` | `disabled` |

### Current schema versions

| Schema | Version |
| --- | ---: |
| Capability schema | `1` |
| Snapshot schema | `1` |
| Local-contract schema | `1` |

Current local-contract payload fields, in fixed order:

1. `contract_schema`
2. `capability_schema`
3. `snapshot_schema`
4. `active`
5. `total`
6. `channel_switch`
7. `recording`
8. `replaying`
9. `timer_change`

---

## SB.6 Live VDR Acceptance

Status: `implemented`

The controlled live acceptance passed on VDR `2.7.9` with API version `11`.

Proven live behavior:

- VDR loaded `libvdr-suitebridge.so.11`;
- lifecycle reached `initialized` and `started`;
- `local-contract` was reported as `available`;
- `mutations` remained `disabled`;
- `PLUG suitebridge HELP` advertised `SNAP`;
- `PLUG suitebridge HELP SNAP` returned the detailed read-only description;
- two `PLUG suitebridge SNAP` requests returned reply code `900`;
- both active payloads used schema versions `1` and the fixed field order;
- each active payload was `151` bytes;
- `PLUG suitebridge SNAP NOW` was rejected with reply code `504`;
- channel `7` remained selected;
- Timer and Recording listings remained unchanged;
- the VDR setup-file hash remained unchanged;
- channel-switch, Recording, replaying and Timer-change counters remained unchanged;
- no mutation command or VDR write operation was invoked;
- successful and rejected requests were recorded through bounded structured logs.

Observed active payload:

`{"contract_schema":1,"capability_schema":1,"snapshot_schema":1,"active":true,"total":4,"channel_switch":4,"recording":0,"replaying":0,"timer_change":0}`

Rollback evidence:

- configuration symlink removed;
- plugin configuration removed;
- API-versioned shared object removed;
- VDR restarted;
- no Suite Bridge shared object remained mapped into the VDR process;
- status monitor reached `inactive`;
- lifecycle reached `stopped`;
- final inactive payload was produced deterministically;
- repository worktree remained clean.

Observed final inactive payload:

`{"contract_schema":1,"capability_schema":1,"snapshot_schema":1,"active":false,"total":4,"channel_switch":4,"recording":0,"replaying":0,"timer_change":0}`

SB.6 therefore satisfies the required source, build, contract, native load, command discovery, read-only behavior and rollback boundary.

---

## Architectural Position

Client → VDR-Suite public API → VDR-Suite Control Plane → VDR-Suite Backend Agent → `vdr-plugin-suite-bridge` → VDR Core

The plugin is not the Backend Agent.

It is a small VDR-process-local bridge used by a separate Backend Agent. It must not become:

- a second public Control Plane;
- a user or role database;
- a public Internet-facing HTTP API;
- an independent multi-site coordinator;
- a metadata platform;
- a long-running workflow engine;
- a substitute for the Suite's durable operation, job, audit or reconciliation layers.

---

## Responsibility Boundary

### VDR-Suite Control Plane owns

- users, services and actor identities;
- roles, permissions and backend-scoped authorization;
- server-enforced read-only policy;
- public REST API and client compatibility;
- stable Suite resource identities;
- durable operation records;
- durable idempotency records;
- job scheduling, retry and saga coordination;
- multi-site policy and backend administration;
- TimerIntent, TimerAssignment and reconciliation policy;
- canonical ProgramEvent identity, observation history and provenance policy;
- audit and security events;
- cross-backend reconciliation;
- final client-visible mutation state;
- public error and compatibility contracts.

### VDR-Suite Backend Agent owns

- the authenticated machine relationship to the Control Plane;
- backend generation, lease and heartbeat participation;
- local transport selection and plugin invocation;
- protocol framing between Control Plane and local VDR boundary;
- command delivery and result forwarding;
- local capability and health publication;
- reconnect and offline synchronization behavior;
- protection of VDR-internal credentials and transports;
- enforcement that commands target the Agent's own backend identity and active generation;
- transport of backend-scoped Timer and EPG observations;
- preservation of operation, job, attempt and idempotency identities.

### `vdr-plugin-suite-bridge` owns

- VDR-process-local lifecycle integration;
- safe access to VDR-native state;
- VDR lock and thread-boundary correctness;
- minimal native snapshots and events;
- truthful local capability reporting;
- bounded native Timer and EPG observations where later implemented;
- native identity and current-state readback where VDR exposes it;
- bounded translation from Agent commands to VDR-native operations;
- local generation or fencing checks that can be proven at the plugin boundary;
- deterministic local results and error categories;
- no leaking of raw VDR pointers or lock ownership across the plugin boundary.

### Shared contracts

The Suite, Agent and plugin agree on:

- backend identity;
- backend generation;
- command and operation identity;
- capability names and schema versions;
- resource identity and native-binding vocabulary;
- revision and stale-state semantics;
- mutation request and result fields;
- verification policy;
- timeout and unknown-outcome semantics;
- protocol versioning and compatibility behavior;
- which side is authoritative for every field.

---

## Gold-Standard VDR Native Rules

### 1. VDR internals stay behind the plugin boundary

- Raw VDR pointers, iterators, lock guards and internal object addresses never cross into the Agent or Suite.
- Public or Agent-facing identifiers are stable values, not memory addresses.
- Native identities are copied into bounded value objects before leaving the VDR access scope.
- VDR-internal paths, plugin service pointers and implementation details are adapter data, not public platform identities.

### 2. Lock scopes are minimal and explicit

- Acquire the VDR-recommended lock for the specific native structure.
- Copy only the values required for a snapshot, validation or command decision.
- Release the lock before serialization, logging, network I/O, disk I/O, waiting, retry or long computation.
- Never hold a VDR lock while calling the Control Plane, Backend Agent or an external service.
- Document lock ordering when more than one native structure is required.
- New code must include tests or review evidence that no lock-owning object escapes its scope.

### 3. VDR callbacks remain non-blocking

Status callbacks and VDR-thread callbacks must not:

- open network connections;
- perform synchronous Agent requests;
- access databases;
- perform filesystem mutation;
- wait for worker completion;
- parse large external payloads;
- hold locks while logging large content.

Callbacks may update bounded atomic counters, enqueue a bounded local event representation or mark a snapshot dirty. Heavier work belongs outside the VDR callback path.

### 4. Lifecycle is deterministic

Required target lifecycle:

`constructed → initialized → started → stopping → stopped`

Required behavior:

- construction has no externally visible side effect beyond safe local registration required by VDR;
- callbacks received before activation are ignored or handled deterministically;
- `Start()` activates only fully initialized components;
- `Stop()` disables callbacks before dependent state is destroyed;
- shutdown does not wait indefinitely for external systems;
- reload or rollback leaves no stale listener, worker, file, lock or registration behind;
- repeated invalid transitions are rejected predictably.

### 5. Read-only is the default

- New plugin surfaces begin read-only.
- Mutation capabilities remain `disabled` until authorization, generation fencing, revision checks, idempotency behavior, readback and live acceptance are complete.
- A visible frontend action is not proof that mutation is allowed.
- A reachable SVDRP or plugin-service command is not by itself an authorization boundary.

### 6. Capabilities are truthful and versioned

Every capability defines:

- stable capability ID;
- state such as available, degraded, disabled or unsupported;
- schema or contract version;
- VDR and plugin prerequisites;
- whether it is read-only or mutating;
- tests required before it becomes available;
- degradation behavior when a native prerequisite disappears.

The plugin never advertises a write capability that cannot satisfy the complete safety and verification contract.

### 7. Snapshots are immutable value contracts

- A snapshot represents one bounded observation, not a live VDR object.
- Snapshot schema and field order are versioned where byte determinism is promised.
- Snapshot generation and resource revision are separate concepts.
- Counters or events require explicit overflow, reset and resynchronization behavior before they become synchronization primitives.
- An Agent must be able to request a full resync after sequence loss.

### 8. Native operations require authoritative readback

Transport acknowledgement and a native method return are not necessarily final success.

Examples:

- Recording move or rename: read back the same stable binding with the expected new name or path.
- Recording trash or delete: verify absence or the canonical deletion state.
- Timer create: verify exactly one expected native Timer binding.
- Timer update: verify normalized native fields.
- Timer delete: verify absence of the bound native Timer.

Destructive operations default to readback or reconciliation.

### 9. Unknown outcome is a valid result

A timeout after dispatch does not prove failure.

The shared protocol must distinguish:

- `not_dispatched`;
- `rejected_before_mutation`;
- `executed_unverified`;
- `succeeded_verified`;
- `failed_verified`;
- `outcome_unknown`.

The plugin must not silently retry a possibly executed native mutation. Verification or reconciliation comes first.

### 10. No hidden alternate control plane

The plugin must not independently own:

- user authorization;
- durable idempotency history;
- cross-site policy;
- global job retries;
- client sessions;
- public API versioning;
- metadata-provider authority;
- audit retention.

It may enforce local safety and reject commands, but the Suite remains the durable platform authority.

---

## Accepted Safe-Mutation Contract — ADR-0042

Status: `accepted-contract`

Current plugin effect: `mutations = disabled`

Every future real mutation must carry or resolve this shared envelope:

| Field | Authority and purpose |
| --- | --- |
| `operationId` | Stable Suite identity for one logical mutation. |
| `idempotencyKey` | Durable deduplication key scoped by the Suite contract. |
| `actorId` | Authenticated Suite actor; the plugin does not authenticate end users. |
| `backendId` | Stable target backend identity. |
| `backendGeneration` | Expected active backend generation used for fencing. |
| `resourceType` | Domain type such as Recording or NativeTimer. |
| `resourceId` | Stable Suite resource identity. |
| `nativeResourceId` | Optional bounded local binding resolved behind the Agent/plugin boundary. |
| `expectedRevision` | Opaque resource revision observed by the caller. |
| `action` | Domain-level mutation name. |
| `payload` | Normalized action-specific values. |
| `verificationPolicy` | Required local readback or reconciliation mode. |
| `deadline` | Optional latest point at which dispatch may begin. |
| `previewToken` | Optional binding to a previously accepted preview. |

VDR-Suite owns durable idempotency storage, normalized fingerprints, duplicate-key handling and long-term reconciliation.

The Agent and plugin preserve `operationId` and `idempotencyKey`, reject obsolete backend generations, avoid blind redispatch after an unknown result and expose sufficient bounded evidence for reconciliation.

A future preview must bind backend, generation, resource identity, revision, normalized action, payload summary, safety decision and expected effects. Execution fails when that preview is stale, expired or mismatched.

---

## Accepted Job Execution Contract — ADR-0043

Status: `accepted-contract`

Suite owner: Control Plane job, operation and saga layers.

Agent impact:

- preserve operation, job and attempt identities;
- enforce claim and backend-generation fencing at the transport boundary;
- return bounded dispatch, verification and reconciliation evidence;
- never blindly redispatch a possibly executed native mutation.

Plugin impact:

- does not become a durable job scheduler;
- does not own global retries, sagas or compensation policy;
- bounded native steps return deterministic local facts;
- callbacks and VDR threads never wait for Suite jobs;
- unknown native outcomes remain externally reconcilable.

Runtime state: no job claim, retry, saga or durable Agent execution facility is implemented by the plugin.

---

## Accepted Timer Contract — ADR-0044

Status: `accepted-contract`

Suite owner: Control Plane TimerIntent, TimerAssignment, scheduler and reconciliation layers.

Agent impact:

- transport generation-bound native Timer observations and commands;
- preserve Timer operation, job, assignment and backend identities;
- return native readback evidence without becoming the scheduler.

Plugin impact:

- native Timer IDs remain backend-scoped values;
- plugin does not own TimerIntent or TimerAssignment persistence;
- plugin does not select target backends;
- native Timer mutations remain disabled;
- later Timer write support requires revision checks, fencing and authoritative readback.

Runtime state: no TimerIntent, TimerAssignment or native Timer mutation is implemented by the plugin.

---

## Accepted EPG Identity Contract — ADR-0045

Status: `accepted-contract`

Suite owner: canonical ProgramEvent identity, EventObservation history, field-level provenance, merge policy and canonical revisions.

Agent impact:

- transport backend-scoped event observations and capability facts;
- preserve source, backend and generation evidence;
- never invent canonical cross-backend event identity.

Plugin impact:

- may later expose bounded native EPG observations;
- native event and channel identifiers remain backend-scoped values;
- raw VDR event pointers and lock-owning objects never cross the plugin boundary;
- plugin does not assign canonical `programEventId` values;
- plugin does not run global merge or provenance policy;
- plugin does not access the Control Plane database.

Runtime state: no canonical EPG identity or provenance implementation exists in the plugin.

---

## Protocol Evolution Rules

- Existing field meaning is never changed silently.
- Additive optional fields require documented defaults.
- Removing or reinterpreting a field requires a new schema or protocol version.
- Capability, snapshot, local-contract and future command schemas evolve independently.
- Unsupported versions are rejected explicitly, not guessed.
- The Agent degrades safely when the plugin is older than the Control Plane.
- A newer plugin preserves documented behavior of supported older contracts.
- Capability negotiation happens before optional or mutating commands are used.
- Mutation commands are never enabled through optimistic version guessing.

---

## Error and Result Rules

Minimum semantic categories:

- `invalid_request`;
- `unsupported_command`;
- `capability_unavailable`;
- `read_only`;
- `backend_inactive`;
- `generation_conflict`;
- `resource_not_found`;
- `revision_conflict`;
- `resource_in_use`;
- `native_rejected`;
- `local_processing_failure`;
- `executed_unverified`;
- `verification_failed`;
- `outcome_unknown`.

Rules:

- native or transport-specific wording may be diagnostic but is not the stable API contract;
- errors do not expose credentials, raw pointers, private memory data or unrestricted filesystem paths;
- rejected-before-dispatch and failed-after-possible-dispatch are never collapsed into one generic failure;
- the plugin returns facts about its local boundary; the Suite decides the final durable operation state.

---

## Observability Rules

Important plugin transitions and commands log bounded facts such as:

- component;
- event;
- result;
- plugin version;
- schema version;
- backend generation when available;
- operation ID when available;
- resource type;
- command or action;
- reply or local result category.

Logging must not include credentials, tokens, key material, raw object addresses, unrestricted payload dumps, unbounded metadata or values that require holding VDR locks during formatting.

The plugin remains diagnosable when Agent and Control Plane are unavailable.

---

## Test and Acceptance Standard

### Required automated layers

1. Source and contract guards for ownership, forbidden side effects and stable fields.
2. Pure C++ unit tests for value contracts and state machines.
3. Build of the final VDR shared object.
4. Version extraction and capability-catalogue checks.
5. Deterministic serialization tests where byte stability is promised.
6. Negative tests for malformed, unsupported and stale requests.
7. Lifecycle tests for invalid and repeated transitions.
8. Regression proof that existing read-only behavior remains unchanged.

### Required live VDR layers for read-only slices

1. Controlled build and staged installation.
2. Controlled VDR plugin load.
3. Command and help discovery.
4. Expected read-only response.
5. Proof that channel, Timers, Recordings, replay and setup remain unchanged.
6. Plugin removal or rollback.
7. VDR restart proving no stale Suite Bridge binary remains loaded.

### Additional required layers for future mutation slices

1. Accepted Suite mutation contract and capability gate.
2. Disposable, explicitly identified test resource.
3. Preview and dry-run before real execution.
4. Generation and revision conflict tests.
5. Duplicate-delivery and lost-response tests.
6. Authoritative native readback.
7. Restart and reconciliation test.
8. Rollback or compensation where possible.
9. Destructive actions tested last.
10. No automated mutation of a user's real Recording, Timer or configuration.

A source-only test never replaces live VDR acceptance for a native boundary.

---

## Current Ownership Matrix

| Concern | Suite | Agent | Plugin | Current state |
| --- | --- | --- | --- | --- |
| Public client API | owner | no | no | Suite foundation exists |
| Authentication and RBAC | owner | machine identity only | no | partial/planned |
| Backend ID | authority | carries/enforces | observes local binding | implemented foundation |
| Backend generation | authority | runtime owner | local fence input | accepted-contract |
| Durable operation record | owner | no | no | accepted-contract |
| Durable idempotency | owner | preserves keys | preserves keys | accepted-contract |
| Job claims, retries and sagas | owner | executes assignments | bounded native step only | accepted-contract |
| VDR lifecycle | observes | observes | owner | implemented |
| VDR status counters/events | consumes | transports | owner | implemented |
| Immutable local snapshot | consumes | transports | owner | implemented |
| `SNAP` SVDRP command | no | invokes | owner | implemented and live accepted |
| TimerIntent and assignment | owner | transports | no | accepted-contract |
| Native Timer mutation | coordinates | transports | executes safely | disabled |
| ProgramEvent identity/provenance | owner | transports observations | bounded native observation | accepted-contract |
| Native Recording mutation | coordinates | transports | executes safely | disabled |
| Native readback | consumes | requests/transports | owner | read-only foundation only |
| Public audit history | owner | contributes facts | contributes facts | planned |
| Plugin-owned network listener | no | no | prohibited | disabled |
| Plugin database | no | no | prohibited | disabled |

---

## Coordination Workflow

Before every new plugin slice, the plugin workstream must:

1. inspect the current remote branch;
2. read this complete handoff and relevant accepted ADRs;
3. inspect actual plugin source, documentation and tests;
4. distinguish Suite, Agent, plugin and shared-contract ownership;
5. state the exact slice, affected files, capability/schema impact and non-goals;
6. keep mutations disabled unless every shared prerequisite exists;
7. avoid overwriting parallel changes;
8. use no force-push or destructive reset;
9. run all existing checks plus new slice tests;
10. perform required live acceptance before declaring a native capability available;
11. update this handoff after the slice.

Before every Suite change affecting the plugin boundary, the Suite workstream must:

1. inspect the bridge branch and this handoff;
2. distinguish accepted contract from runtime implementation;
3. record whether the requirement belongs to Suite, Agent, plugin or shared protocol;
4. avoid implementing VDR-native access in the Control Plane;
5. avoid assuming a plugin capability is available until acceptance evidence says so;
6. update shared fields, dependencies and compatibility notes here.

Repository changes are made directly through GitHub where tooling permits. Manual full-file copy operations are not part of the normal coordinated workflow. If direct repository editing is technically unavailable, only a bounded reviewable patch is supplied.

---

## Required Handoff Update After Every Plugin Slice

Every completed slice records:

- Slice;
- Branch;
- HEAD;
- Plugin version;
- Implemented functions;
- Changed files;
- New or changed capabilities;
- Schema changes;
- New commands or service calls;
- New events or snapshots;
- Suite-side work unblocked;
- Suite-side work still required;
- Compatibility risks;
- Automated tests;
- Live VDR acceptance;
- Rollback result;
- Mutation state;
- Next safe slice.

A slice is not reported complete while a required field is unknown.

---

## Required Handoff Update After Every Suite Contract Change

Every shared contract change records:

- ADR or contract;
- Status;
- Suite owner component;
- Agent impact;
- Plugin impact;
- Required shared fields;
- Required capability or schema version;
- Compatibility behavior;
- Plugin work unblocked;
- Plugin work still blocked;
- Runtime implementation state;
- Tests required before use.

---

## Latest Completed Plugin Slice Record

### Slice

`SB.6 - Read-only native SVDRP contract`

### Branch

`feature/vdr-plugin-suite-bridge-foundation`

### Heads

- implementation commit: `30516faaa24d12973cb3864bd33f8fb52976a4a0`;
- live-acceptance repository head: `ae38f546fd437f803926dd41a1e5f6442626f784`.

### Plugin version

`0.7.0`

### Implemented

Read-only plugin-specific SVDRP command that captures the current immutable Suite Bridge status snapshot and returns the deterministic local-contract JSON payload.

### Changed files

- `vdr-plugin-suite-bridge/Makefile`;
- `vdr-plugin-suite-bridge/README.md`;
- `vdr-plugin-suite-bridge/docs/SB-6-read-only-svdrp.md`;
- `vdr-plugin-suite-bridge/suitebridge.cpp`;
- `vdr-plugin-suite-bridge/suitebridge.h`;
- `vdr-plugin-suite-bridge/suitebridge_capabilities.cpp`;
- `vdr-plugin-suite-bridge/suitebridge_svdrp_contract.cpp`;
- `vdr-plugin-suite-bridge/suitebridge_svdrp_contract.h`;
- `vdr-plugin-suite-bridge/tests/check_capabilities_contract.py`;
- `vdr-plugin-suite-bridge/tests/check_foundation_contract.py`;
- `vdr-plugin-suite-bridge/tests/check_svdrp_contract.py`;
- `vdr-plugin-suite-bridge/tests/test_suitebridge_capabilities.cpp`;
- `vdr-plugin-suite-bridge/tests/test_suitebridge_svdrp_contract.cpp`.

### Capabilities

- `local-contract`: `available`;
- `mutations`: `disabled`.

### Schema changes

None. Capability, snapshot and local-contract schemas remain at version `1`.

### New commands or service calls

- `PLUG suitebridge SNAP`;
- no new plugin service call;
- no plugin-owned listener.

### New events or snapshots

No new callback event type. The current immutable status snapshot is retrievable on demand.

### Suite-side work unblocked

The Backend Agent may implement capability-gated read-only invocation of `SNAP`, local payload parsing and forwarding of bounded status facts.

### Suite-side work still required

- authenticated Agent transport;
- backend identity and generation framing;
- capability-freshness publication;
- protocol compatibility handling;
- stable Control Plane read models;
- public error mapping.

All mutation execution, durable jobs, retries, sagas, TimerIntent scheduling, authorization, idempotency and reconciliation remain Suite or Agent work.

### Compatibility risks

- SVDRP availability and host restrictions remain deployment concerns.
- Reply code `900` is plugin-specific.
- VDR lists command names with `PLUG suitebridge HELP` and detailed text with `PLUG suitebridge HELP SNAP`.
- Status counters are diagnostic observations, not synchronization sequences.
- Counter overflow, reset and resynchronization behavior remains undefined.
- The explicit `stopping` lifecycle state is not yet represented in the current lifecycle enum.
- Synchronous logging in VDR callback paths requires a dedicated boundedness and non-blocking review before richer event handling is added.

### Automated tests

- foundation source contract;
- capability source contract;
- status-event source contract;
- status-snapshot source contract;
- local-contract payload source contract;
- read-only SVDRP source contract;
- lifecycle C++ unit test;
- capability C++ unit test;
- status-event C++ unit test;
- status-snapshot C++ unit test;
- local-contract C++ unit test;
- SVDRP command C++ unit test;
- version extraction check;
- final VDR shared-object build;
- ELF validation;
- API-11 staged installation;
- repository documentation checks.

### Live VDR acceptance

Passed on VDR `2.7.9`, API version `11`:

- help discovery passed;
- detailed `HELP SNAP` passed;
- two reply-`900` SNAP requests passed;
- reply-`504` invalid-option request passed;
- schema and deterministic field-order checks passed;
- channel, Timer list, Recording list, callback counters and setup hash remained unchanged;
- structured served and rejected logs passed.

### Rollback result

Passed:

- plugin, configuration and symlink removed;
- VDR restarted;
- no Suite Bridge binary remained loaded;
- inactive snapshot and stopped lifecycle logged;
- worktree clean.

### Mutation state

`disabled`

### Next safe slice

`SB.7 - Gold-standard lifecycle and callback-boundary hardening`

Before code, SB.7 must inspect VDR-native lifecycle and callback requirements and define the exact bounded change. It must add no mutation, listener, worker, database or second Agent protocol.

---

## Immediate Coordination Notes

- The bridge branch contains SB.1 through fully live-accepted SB.6.
- The coordinated branch has accepted Suite contracts through ADR-0045.
- The plugin is deliberately read-only.
- `SNAP` is the only implemented plugin-specific SVDRP command.
- ADR-0042 through ADR-0045 are contracts, not proof of plugin mutation support.
- The next plugin slice is chosen against current Suite contracts and actual plugin evidence.
- The authoritative plugin location remains `vdr-plugin-suite-bridge/` on the long-lived bridge branch.

---

## Gold-Standard Definition of Done

A VDR-related feature is complete only when:

- ownership is unambiguous;
- VDR locks and thread boundaries are correct;
- lifecycle and shutdown are deterministic;
- capabilities are truthful;
- identities and revisions are stable;
- retries cannot cause silent duplicate mutation;
- unknown outcomes are represented and reconcilable;
- native results are read back where required;
- errors are structured and do not leak internals;
- compatibility and schema evolution are defined;
- automated tests cover positive and negative contracts;
- required live VDR acceptance passes;
- rollback is proven;
- documentation and this handoff match the implementation;
- no existing VDR behavior is weakened to simplify integration.

This standard applies equally to Recording, Timer, EPG, channel, replay, status, streaming and future plugin-service integration.
