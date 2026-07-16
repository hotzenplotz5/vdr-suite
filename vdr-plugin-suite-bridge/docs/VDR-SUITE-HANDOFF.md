# VDR-Suite / Suite Bridge Gold-Standard Handoff

## Purpose

This document is the shared coordination contract between:

- the VDR-Suite Control Plane and Backend Agent architecture work on `main` and short-lived Suite branches;
- the native VDR plugin work under `vdr-plugin-suite-bridge/` on `feature/vdr-plugin-suite-bridge-foundation`.

Its goal is not merely to avoid merge conflicts. It defines the quality and architecture standard for every boundary that touches VDR internals.

The target is:

> VDR-Suite shall be the gold standard for safe, modern, multi-backend VDR integration while preserving VDR-native correctness.

This handoff prevents the Suite and plugin workstreams from inventing incompatible responsibilities, command fields, capability names, lifecycle behavior, mutation semantics or test assumptions.

This is an operational coordination document. Accepted VDR-Suite ADRs remain authoritative for long-term platform decisions. Stable plugin documents remain authoritative for implemented plugin slices.

---

## Repository and Branches

Repository:

```text
hotzenplotz5/vdr-suite
```

VDR-Suite integration source of truth:

```text
main
```

Suite Bridge development branch:

```text
feature/vdr-plugin-suite-bridge-foundation
```

Plugin directory:

```text
vdr-plugin-suite-bridge/
```

The bridge branch is synchronized with current `main` before new plugin slices.

Suite work must not edit bridge implementation files unless the change is explicitly coordinated through this handoff.

Plugin work must not modify unrelated Suite production files merely to complete a local bridge slice.

When a shared contract changes, both workstreams must record:

- which side owns the change;
- whether it is implemented or only accepted;
- the schema or capability version impact;
- compatibility and migration requirements;
- the exact tests that prove the boundary.

---

## Status Vocabulary

Every handoff item must use exactly one of these states:

| State | Meaning |
| --- | --- |
| `implemented` | Present in source, covered by tests and available at the stated boundary. |
| `accepted-contract` | Decided by an accepted ADR or explicit shared contract, but not fully implemented. |
| `planned` | Intended direction that is not yet an accepted shared contract. |
| `blocked` | Cannot proceed until a named Suite or plugin prerequisite exists. |
| `disabled` | Deliberately unavailable at runtime. |
| `superseded` | Retained for history but no longer authoritative. |

An accepted ADR does not make a runtime feature `implemented`.

A plugin capability must never report `available` merely because source code exists. It becomes available only after the complete build, contract and required live-acceptance boundary passes.

---

## Current Plugin Snapshot

Current completed slice:

```text
SB.6 - Read-only native SVDRP contract
```

Plugin identity:

```text
plugin name: suitebridge
version: 0.7.0
shared object: libvdr-suitebridge.so.<VDR-APIVERSION>
```

Implemented local endpoint:

```text
PLUG suitebridge SNAP
```

Current behavior:

- captures one immutable status snapshot;
- returns one deterministic compact JSON line;
- valid `SNAP` returns reply code `900`;
- `SNAP` with options returns `504`;
- payload preparation failure returns `451`;
- unknown commands remain unhandled for the VDR default response;
- command matching is case-insensitive;
- no plugin-owned listener, outbound connection, database, worker thread or filesystem mutation exists.

Current capability catalogue:

| Capability | State |
| --- | --- |
| `lifecycle` | `available` |
| `status-events` | `available` |
| `snapshots` | `available` |
| `local-contract` | `available` |
| `mutations` | `disabled` |

Current schema versions:

```text
capability schema: 1
snapshot schema: 1
local-contract schema: 1
```

Current local-contract payload fields, in fixed order:

```text
contract_schema
capability_schema
snapshot_schema
active
total
channel_switch
recording
replaying
timer_change
```

---

## Architectural Position

```text
Client
  -> VDR-Suite public API
  -> VDR-Suite Control Plane
  -> VDR-Suite Backend Agent
  -> vdr-plugin-suite-bridge
  -> VDR Core
```

The plugin is not the Backend Agent.

The plugin is a small VDR-process-local bridge used by a separate Backend Agent. It must not become:

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
- enforcement that commands target the Agent's own backend identity and active generation.

### `vdr-plugin-suite-bridge` owns

- VDR-process-local lifecycle integration;
- safe access to VDR-native state;
- VDR lock and thread-boundary correctness;
- minimal native snapshots and events;
- truthful local capability reporting;
- native identity and current-state readback where VDR exposes it;
- bounded translation from Agent commands to VDR-native operations;
- local generation or fencing checks that can be proven at the plugin boundary;
- deterministic local results and error categories;
- no leaking of raw VDR pointers or lock ownership across the plugin boundary.

### Shared contracts

The Suite, Agent and plugin must agree on:

- backend identity;
- backend generation;
- command and operation identity;
- capability names and schema versions;
- resource identity and native binding vocabulary;
- revision and stale-state semantics;
- mutation request and result fields;
- verification policy;
- timeout and unknown-outcome semantics;
- protocol versioning and compatibility behavior;
- which side is authoritative for every field.

---

## Gold-Standard VDR Native Rules

These rules apply to every new plugin slice.

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
- Lock ordering must be documented when more than one native structure is required.
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

Callbacks may update bounded atomic counters, enqueue a bounded local event representation or mark a snapshot dirty. Any heavier work belongs outside the VDR callback path.

### 4. Lifecycle is deterministic

The plugin must define and test:

```text
constructed
-> initialized
-> started
-> stopping
-> stopped
```

Required behavior:

- construction has no externally visible side effect beyond safe local registration required by VDR;
- callbacks received before activation are ignored or handled deterministically;
- `Start()` activates only fully initialized components;
- `Stop()` disables callbacks before dependent state is destroyed;
- shutdown does not wait indefinitely for external systems;
- reload or rollback leaves no stale listener, worker, file, lock or registration behind;
- repeated invalid lifecycle transitions are rejected predictably.

### 5. Read-only is the default

- New plugin surfaces start read-only.
- Mutation capabilities remain `disabled` until the entire shared mutation contract, authorization path, generation fencing, revision check, idempotency behavior, readback and live acceptance are available.
- A visible frontend action is never proof that mutation is allowed.
- A reachable SVDRP or plugin service command is never by itself an authorization boundary.

### 6. Capabilities are truthful and versioned

Each capability must define:

- stable capability ID;
- state such as available, degraded, disabled or unsupported;
- schema or contract version;
- VDR and plugin prerequisites;
- whether the capability is read-only or mutating;
- the tests required before state changes to `available`;
- degradation behavior when the native prerequisite disappears.

The plugin must never advertise a write capability that cannot satisfy the full safety and verification contract.

### 7. Snapshots are immutable value contracts

- A snapshot represents one bounded observation, not a live VDR object.
- Snapshot schema and field order are versioned where byte determinism is part of the contract.
- Snapshot generation and resource revision are separate concepts.
- Counters or events must have explicit overflow, reset and resynchronization behavior before they become synchronization primitives.
- A client or Agent must be able to request a full resync after sequence loss.

### 8. Native operations require authoritative readback

For future mutations, transport acknowledgement and VDR method return alone are not necessarily final success.

Examples:

- Recording move or rename: read back the same stable Recording binding with the expected new name or path.
- Recording trash or delete: verify absence or the canonical VDR deletion state.
- Timer create: verify exactly one expected native timer binding.
- Timer update: verify the normalized native fields.
- Timer delete: verify absence of the bound native timer.

Destructive operations default to readback or reconciliation.

### 9. Unknown outcome is a valid result

A timeout after dispatch does not prove failure.

The plugin and Agent protocol must be able to distinguish:

```text
not_dispatched
rejected_before_mutation
executed_unverified
succeeded_verified
failed_verified
outcome_unknown
```

The plugin must not silently retry a possibly executed native mutation. Verification or reconciliation comes first.

### 10. No hidden alternate control plane

The plugin must not independently own:

- user authorization;
- durable idempotency history;
- cross-site policy;
- global job retries;
- client sessions;
- public API versioning;
- metadata provider authority;
- audit retention.

It may enforce local safety and reject commands, but the Suite remains the durable platform authority.

---

## Accepted Safe-Mutation Contract

VDR-Suite ADR-0042 is `accepted-contract`.

It does not mean that plugin mutations are implemented. The current plugin capability remains:

```text
mutations = disabled
```

Every future real mutation must carry or resolve the following shared envelope:

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

### Division of idempotency responsibility

VDR-Suite owns:

- durable idempotency storage;
- normalized request fingerprint;
- returning an existing operation for an identical retry;
- detecting key reuse with a different request;
- long-term operation history and reconciliation.

The Backend Agent and plugin must:

- preserve `operationId` and `idempotencyKey` end to end;
- reject obsolete backend generations;
- avoid blind redispatch after an unknown result;
- expose enough local result and readback data for the Suite to reconcile;
- optionally maintain bounded local replay protection, but never treat in-memory replay protection as the sole production idempotency store.

### Preview-to-execute binding

A future mutation preview must provide:

- backend ID;
- backend generation;
- stable resource identity;
- current resource revision;
- normalized action and payload summary;
- local capability and safety decision;
- expected native effects and warnings.

Execution must fail when the preview is stale, expired, targets another backend or resource, or no longer matches the normalized action.

---

## Protocol Evolution Rules

- Existing field meaning is never changed silently.
- Additive optional fields require documented default behavior.
- Removing or reinterpreting a field requires a new schema or protocol version.
- Capability schema, snapshot schema, local-contract schema and future command schema are versioned independently where their compatibility can evolve independently.
- Unsupported versions are rejected explicitly, not guessed.
- The Agent must be able to degrade safely when the plugin is older than the Control Plane.
- A newer plugin must preserve the documented behavior of supported older contracts.
- Capability negotiation happens before using optional or mutating commands.
- Mutation commands are never enabled through optimistic version guessing.

---

## Error and Result Rules

Plugin-local errors must be deterministic and map cleanly into Suite categories.

Minimum semantic categories:

```text
invalid_request
unsupported_command
capability_unavailable
read_only
backend_inactive
generation_conflict
resource_not_found
revision_conflict
resource_in_use
native_rejected
local_processing_failure
executed_unverified
verification_failed
outcome_unknown
```

Rules:

- native or transport-specific wording may be included as diagnostics but is not the stable API contract;
- errors must not expose credentials, raw pointers, private memory data or unrestricted filesystem paths;
- rejected-before-dispatch and failed-after-possible-dispatch are never collapsed into one generic failure;
- the plugin returns facts about its local boundary; the Suite decides the final durable operation state.

---

## Observability Rules

Every important plugin transition or command should log structured, bounded facts:

```text
component
event
result
plugin version
schema version
backend generation when available
operation ID when available
resource type
command or action
reply or local result category
```

Logging must not include:

- credentials or tokens;
- full user-provided secrets;
- private key material;
- raw object addresses;
- unrestricted payload dumps;
- unbounded metadata or descriptions;
- values that require holding VDR locks during formatting.

The plugin must remain diagnosable when the Agent or Control Plane is unavailable.

---

## Test and Acceptance Standard

Every plugin slice must use layered evidence.

### Required automated layers

1. Source/contract guard for ownership, forbidden side effects and stable fields.
2. Pure C++ unit tests for value contracts and state machines.
3. Build of the final VDR shared object.
4. Version extraction and capability catalogue checks.
5. Deterministic serialization tests where byte stability is promised.
6. Negative tests for malformed, unsupported and stale requests.
7. Lifecycle tests for invalid and repeated transitions.
8. Regression proof that existing read-only behavior remains unchanged.

### Required live VDR layers

For read-only slices:

1. controlled plugin build and staged installation;
2. controlled VDR plugin load;
3. command/help discovery;
4. expected read-only response;
5. proof that channel, timers, recordings, replay and setup remain unchanged;
6. plugin removal or rollback;
7. VDR restart proving no stale Suite Bridge binary remains loaded.

For future mutating slices:

1. accepted Suite mutation contract and capability gate;
2. disposable, explicitly identified test resource;
3. preview and dry-run before real execution;
4. generation and revision conflict tests;
5. duplicate-delivery and lost-response tests;
6. authoritative native readback;
7. restart and reconciliation test;
8. rollback or compensation behavior where possible;
9. destructive actions tested last;
10. no automated mutation of a user's real Recording, Timer or configuration.

A source-only test never replaces live VDR acceptance for a native boundary.

---

## Current Ownership Matrix

| Concern | Suite | Agent | Plugin | Current state |
| --- | ---: | ---: | ---: | --- |
| Public client API | owner | no | no | Suite foundation exists |
| User authentication and RBAC | owner | machine identity only | no | partial/planned |
| Backend ID | authority | carries/enforces | observes local binding | implemented foundation |
| Backend generation | authority | owner at runtime | local fence input | accepted-contract |
| Durable operation record | owner | no | no | accepted-contract |
| Durable idempotency | owner | preserves keys | preserves keys | accepted-contract |
| VDR lifecycle | observes | observes | owner | implemented |
| VDR status counters/events | consumes | transports | owner | implemented |
| Immutable local snapshot | consumes | transports | owner | implemented |
| `SNAP` SVDRP command | no | invokes | owner | implemented |
| Native Recording mutation | coordinates | transports | executes safely | disabled |
| Native Timer mutation | coordinates | transports | executes safely | disabled |
| Native readback | consumes | requests/transports | owner | read-only foundation only |
| Public audit history | owner | contributes facts | contributes facts | planned |
| Plugin-owned network listener | no | no | prohibited by current boundary | disabled |
| Plugin database | no | no | prohibited by current boundary | disabled |

---

## Coordination Workflow

Before each new plugin slice, the plugin workstream must:

1. synchronize the long-lived bridge branch with current `main` without changing bridge files during the sync;
2. read this handoff and the relevant accepted ADRs;
3. inspect the current plugin source and tests rather than relying on chat memory;
4. state the exact slice, affected files, new capability or schema impact and explicit non-goals;
5. keep mutations disabled unless the complete shared prerequisite is present;
6. run all existing plugin checks plus the new slice tests;
7. perform required live acceptance before declaring a native capability available;
8. update this handoff after the slice.

Before each Suite change that affects the plugin boundary, the Suite workstream must:

1. inspect the current bridge branch and this handoff;
2. distinguish accepted contract from runtime implementation;
3. record whether the new requirement belongs to Suite, Agent, plugin or a shared protocol;
4. avoid implementing VDR-native access in the Control Plane;
5. avoid assuming a plugin capability is available until the bridge branch and acceptance evidence say so;
6. update the shared fields, dependency and compatibility notes in this handoff.

---

## Required Handoff Update After Every Plugin Slice

Append or update these facts:

```text
Slice:
Branch:
Head:
Plugin version:
Implemented:
Changed files:
New or changed capabilities:
Schema changes:
New commands or service calls:
New events or snapshots:
Suite-side work unblocked:
Suite-side work still required:
Compatibility risks:
Automated tests:
Live VDR acceptance:
Rollback result:
Mutations state:
Next safe slice:
```

Do not report a slice as complete while any required field is unknown.

---

## Required Handoff Update After Every Suite Contract Change

Append or update these facts:

```text
ADR or contract:
Status:
Suite owner component:
Agent impact:
Plugin impact:
Required shared fields:
Required capability or schema version:
Compatibility behavior:
Plugin work unblocked:
Plugin work still blocked:
Runtime implementation state:
Tests required before use:
```

---

## Current Shared Decisions

### ADR-0042 Safe Mutation, Revision and Idempotency

Status:

```text
accepted-contract
```

Current plugin effect:

```text
mutations remain disabled
```

The plugin workstream may prepare bounded value contracts, capability vocabulary and read-only identity/revision discovery, but must not enable native write commands merely because ADR-0042 is accepted.

### Next Suite contract

```text
ADR-0043 - Job Claim, Retry and Saga Execution Model
```

Expected plugin relevance:

- the plugin does not become the durable job scheduler;
- Agent/plugin execution ownership and cancellation boundaries must remain compatible with the Suite job model;
- long-running native operations must expose bounded progress and result facts without blocking VDR threads;
- unknown outcomes must remain reconcilable.

---

## Immediate Coordination Notes

- The bridge branch currently contains SB.1 through SB.6 foundations.
- The branch is synchronized with `main` through ADR-0042.
- The plugin is deliberately read-only.
- `SNAP` is the only implemented plugin-specific SVDRP command.
- The next plugin slice must be chosen against current Suite contracts, not by independently designing a second Agent protocol.
- A future separate plugin repository may be considered, but the current authoritative location is the `vdr-plugin-suite-bridge/` directory and its long-lived branch.

---

## Gold-Standard Definition of Done

A VDR-related feature is not complete merely because it works once.

It is complete only when:

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
- no existing VDR behavior is weakened to simplify the integration.

That standard applies equally to Recording, Timer, EPG, channel, replay, status, streaming and future plugin-service integration.