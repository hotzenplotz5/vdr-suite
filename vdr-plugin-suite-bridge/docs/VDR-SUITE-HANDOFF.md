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

Repository changes are made directly through GitHub where tooling permits. Manual full-file copy operations are not part of the normal coordinated workflow. If direct repository editing is technically unavailable, only one bounded reviewable patch is supplied.

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

A plugin capability must never report `available` merely because source code exists. It becomes available only after required build, contract tests and live VDR acceptance have passed.

---

## Current Plugin Snapshot

| Item | Current value |
| --- | --- |
| Last completed slice | `SB.7 - Gold-standard lifecycle and callback-boundary hardening` |
| Plugin name | `suitebridge` |
| Plugin version | `0.8.0` |
| Shared object | `libvdr-suitebridge.so.<VDR-APIVERSION>` |
| SB.7 implementation completion commit | `5f8c4d7434157766da2853db4e58b3623b40f6c1` |
| SB.7 live-acceptance repository head | `2b4df6bc239489cf1aeed239927d347db28234b9` |
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
- lifecycle is `constructed → initialized → started → stopping → stopped`;
- VDR status callbacks perform only an active-state check and one atomic counter increment;
- callback bodies perform no logging, serialization, allocation, locking, waiting or external work;
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

## SB.7 Live VDR Acceptance

Status: `implemented`

The controlled live acceptance passed on VDR `2.7.9` with API version `11` at repository head `2b4df6bc239489cf1aeed239927d347db28234b9`.

Proven live behavior:

- VDR loaded `libvdr-suitebridge.so.11` as plugin version `0.8.0`;
- lifecycle reached `initialized` and `started`;
- `local-contract` was reported as `available`;
- `mutations` remained `disabled`;
- `PLUG suitebridge HELP` advertised `SNAP` and plugin version `0.8.0`;
- the baseline `SNAP` returned reply code `900`, schema versions `1`, fixed field order and `151` bytes;
- original channel `7` was captured;
- controlled test channel `1` was selected and channel `7` was restored;
- the channel-switch counter changed from `4` to `8` to `12`;
- Recording, replaying and Timer-change counters remained `0`;
- the final active `SNAP` remained schema-compatible and was `153` bytes because the observed counters had grown;
- no callback-side `status-event` log was emitted;
- three successful `SNAP` requests were recorded through bounded SVDRP logs;
- Timer and Recording listings remained unchanged;
- the VDR setup-file hash remained `e80c952eddeba470bd23a67060f079e3693e53b601781ba5cb9134d0064636dc`;
- no mutation command or VDR write operation was invoked.

Observed baseline active payload:

`{"contract_schema":1,"capability_schema":1,"snapshot_schema":1,"active":true,"total":4,"channel_switch":4,"recording":0,"replaying":0,"timer_change":0}`

Observed final active payload:

`{"contract_schema":1,"capability_schema":1,"snapshot_schema":1,"active":true,"total":12,"channel_switch":12,"recording":0,"replaying":0,"timer_change":0}`

Two-phase stop evidence:

1. lifecycle logged `stop-begin` with state `stopping`;
2. the status monitor became `inactive`;
3. the final inactive snapshot and local-contract payload were produced outside callbacks;
4. lifecycle logged `stop-complete` with state `stopped`.

Observed final inactive payload:

`{"contract_schema":1,"capability_schema":1,"snapshot_schema":1,"active":false,"total":12,"channel_switch":12,"recording":0,"replaying":0,"timer_change":0}`

Rollback evidence:

- configuration symlink removed;
- plugin configuration removed;
- API-versioned shared object removed;
- VDR restarted;
- no Suite Bridge shared object remained mapped into the VDR process;
- original channel, Timer list, Recording list and setup hash remained restored;
- build artifacts were removed;
- repository worktree remained clean.

SB.7 therefore satisfies the required source, build, contract, VDR-native callback, deterministic lifecycle, live load and rollback boundary.

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
- a Streaming Gateway;
- a public Legacy OSD session service;
- a durable audit store;
- a substitute for the Suite's durable operation, job, audit or reconciliation layers.

---

## Responsibility Boundary

### VDR-Suite Control Plane owns

- users, services and actor identities;
- roles, permissions and backend-scoped authorization;
- server-enforced read-only policy;
- public `/api/v1` contract and client compatibility;
- stable Suite resource identities;
- durable operation and idempotency records;
- job scheduling, retry and saga coordination;
- multi-site policy and backend administration;
- TimerIntent, TimerAssignment and reconciliation policy;
- canonical ProgramEvent identity, observation history and provenance policy;
- MediaSession policy and public Streaming Gateway authorization;
- LegacyOsdSession policy and controller-lease authority;
- append-only AccountabilityEvent persistence, redaction, retention and protected queries;
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
- transport of backend-scoped Timer, EPG, media, OSD and accountability evidence;
- preservation of operation, job, attempt and idempotency identities.

### `vdr-plugin-suite-bridge` owns

- VDR-process-local lifecycle integration;
- safe access to VDR-native state;
- VDR lock and thread-boundary correctness;
- minimal native snapshots and events;
- truthful local capability reporting;
- bounded native Timer, EPG, media-provider or OSD access where later implemented;
- native identity and current-state readback where VDR exposes it;
- bounded translation from Agent commands to VDR-native operations;
- local generation or fencing checks that can be proven at the plugin boundary;
- deterministic local results and error categories;
- bounded local diagnostic facts without owning durable public audit history;
- no leaking of raw VDR pointers or lock ownership across the plugin boundary.

### Shared contracts

The Suite, Agent and plugin agree on:

- backend identity and backend generation;
- command, operation, job and attempt identity;
- capability names and schema versions;
- resource identity and native-binding vocabulary;
- revision and stale-state semantics;
- mutation request and result fields;
- verification policy;
- timeout and unknown-outcome semantics;
- protocol versioning and compatibility behavior;
- bounded producer evidence for accountability;
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
- New code includes tests or review evidence that no lock-owning object escapes its scope.

### 3. VDR callbacks remain non-blocking

Status callbacks and VDR-thread callbacks must not:

- open network connections;
- perform synchronous Agent requests;
- access databases;
- perform filesystem mutation;
- wait for worker completion;
- parse or serialize large payloads;
- log per-event content;
- retain or dereference native pointers after the callback.

The current callback path performs only an atomic active-state check, one relaxed atomic counter increment and immediate return.

### 4. Lifecycle is deterministic

Implemented lifecycle:

`constructed → initialized → started → stopping → stopped`

Required behavior:

- construction has no externally visible side effect beyond safe local registration required by VDR;
- callbacks received before activation are ignored deterministically;
- `Start()` activates only fully initialized components;
- `Stop()` enters `stopping`, disables callbacks, emits the final inactive snapshot and then enters `stopped`;
- shutdown does not wait for external systems;
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
- runtime state;
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
- Current counters are diagnostic observations, not synchronization sequences.
- Counter overflow, restart, reset, epoch and resynchronization behavior must be explicit before counters become synchronization primitives.

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

The shared protocol distinguishes:

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

## Accepted Shared Contracts

### ADR-0042 — Safe Mutation, Revision and Idempotency

Status: `accepted-contract`

Current plugin effect: `mutations = disabled`

Future mutation envelope fields include:

`operationId`, `idempotencyKey`, `actorId`, `backendId`, `backendGeneration`, `resourceType`, `resourceId`, `nativeResourceId`, `expectedRevision`, `action`, `payload`, `verificationPolicy`, `deadline`, `previewToken`.

The Control Plane owns durable idempotency, authorization and operation state. The Agent and plugin preserve identities, reject obsolete generations, avoid blind redispatch and provide bounded readback evidence.

### ADR-0043 — Job Claim, Retry and Saga Execution

Status: `accepted-contract`

- Control Plane owns durable job, attempt, retry, cancellation, saga and compensation state.
- Agent executes fenced assignments and preserves operation, job and attempt identities.
- Plugin executes only bounded native steps and owns no durable retry or saga scheduler.
- VDR callbacks never wait for Suite jobs.

Runtime state: no plugin job queue, retry scheduler, saga engine or durable execution store exists.

### ADR-0044 — Timer Intent, Assignment and Native Timer

Status: `accepted-contract`

- Control Plane owns TimerIntent, TimerAssignment, scheduling and reconciliation.
- Agent transports generation-bound observations and commands.
- Plugin may later expose bounded native Timer access and authoritative readback.
- Native Timer IDs remain backend-scoped.
- Native Timer mutation remains disabled.

### ADR-0045 — Canonical EPG Event Identity and Provenance

Status: `accepted-contract`

- Control Plane owns canonical ProgramEvent identity, revisions, provenance and merge policy.
- Agent transports backend-scoped observations and source evidence.
- Plugin may later expose bounded native EPG observations.
- Raw VDR event pointers and lock-owning objects never cross the plugin boundary.

### ADR-0046 — Streaming Gateway and Media Session Boundary

Status: `accepted-contract`

- Control Plane owns MediaSession authorization, stable resource identity, route policy and revocation.
- Streaming Gateway owns the public media plane.
- Agent owns local provider selection, credentials, generation fencing and local cleanup.
- Plugin may later act as one internal provider adapter, but it does not expose permanent internal URLs or become the public Streaming Gateway.

Runtime state: no plugin media listener, public stream session or media authorization exists.

### ADR-0047 — Legacy OSD Compatibility Bridge

Status: `accepted-contract`

- Control Plane owns LegacyOsdSession authorization, viewer policy and controller-lease authority.
- Agent owns backend-generation fencing, local adapter selection, sequencing and resynchronization transport.
- Plugin may later copy native OSD state into bounded immutable frames and translate allowlisted input actions.
- Plugin does not own user authentication, public sessions, global controller arbitration or arbitrary command execution.

Runtime state: no OSD capture, frame protocol or remote-input command is implemented by the plugin.

### ADR-0048 — Public API Versioning, Error and Compatibility

Status: `accepted-contract`

- Public client API is owned by the Control Plane below `/api/v1`.
- Public API version, Agent protocol version, media protocol, OSD protocol and plugin-local schemas are independent.
- Plugin SVDRP reply codes and local JSON are not the public client contract.
- Public errors, request IDs, correlations, ETags, pagination and deprecation policy remain Control Plane concerns.

Runtime state: SB.7 changes no public API or plugin schema.

### ADR-0049 — Audit and Security Event Model

Status: `accepted-contract`

- Control Plane owns immutable AccountabilityEvent identity, append-only persistence, actor and authorization context, redaction, retention and protected queries.
- Agent may produce reconnect-safe backend-scoped source evidence with producer identity and sequence.
- Plugin may produce bounded native facts and local diagnostic logs.
- Plugin does not own global audit policy, user identity, authorization decisions, canonical event storage or retention.
- Callback paths remain free of audit I/O and blocking event submission.

Runtime state: no plugin audit database, durable event queue or security-event transport exists.

---

## Protocol Evolution Rules

- Existing field meaning is never changed silently.
- Additive optional fields require documented defaults.
- Removing or reinterpreting a field requires a new schema or protocol version.
- Capability, snapshot, local-contract, Agent, media, OSD and public API schemas evolve independently.
- Unsupported versions are rejected explicitly, not guessed.
- The Agent degrades safely when the plugin is older than the Control Plane.
- Capability negotiation happens before optional or mutating commands are used.
- Mutation commands are never enabled through optimistic version guessing.

---

## Error and Result Rules

Minimum semantic categories include:

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

Native or transport-specific wording may be diagnostic but is not the stable public API contract. Rejected-before-dispatch and failed-after-possible-dispatch are never collapsed into one generic failure. The plugin returns local facts; the Suite decides durable operation state.

---

## Observability Rules

Important plugin transitions and commands log only bounded facts such as component, event, result, plugin version, schema version, command, reply and local result category.

Logging must not include credentials, tokens, key material, raw object addresses, unrestricted payload dumps, unbounded metadata or values that require holding VDR locks during formatting.

Per-event VDR callback logging is prohibited. Runtime logs are diagnostic and are not the authoritative audit history.

---

## Test and Acceptance Standard

### Required automated layers

1. Source and contract guards for ownership, forbidden side effects and stable fields.
2. Pure C++ unit tests for value contracts and state machines.
3. Build of the final VDR shared object.
4. Version extraction and capability-catalogue checks.
5. Deterministic serialization tests where byte stability is promised.
6. Negative tests for malformed and unsupported requests.
7. Lifecycle tests for invalid and repeated transitions.
8. Regression proof that existing read-only behavior remains unchanged.
9. API-versioned staged installation.
10. Repository documentation checks.

### Required live VDR layers for read-only slices

1. Controlled build and staged installation.
2. Controlled VDR plugin load.
3. Command and help discovery where applicable.
4. Expected read-only response.
5. Proof of the intended native observation.
6. Proof that unrelated Timers, Recordings, replay and setup remain unchanged.
7. Plugin removal and VDR restart.
8. Proof that no stale Suite Bridge binary remains loaded.
9. Restored original VDR state and clean worktree.

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
| Public client API | owner | no | no | accepted `/api/v1` contract |
| Authentication and RBAC | owner | machine identity only | no | accepted-contract / partial runtime |
| Backend ID | authority | carries/enforces | observes local binding | implemented foundation |
| Backend generation | authority | runtime owner | local fence input | accepted-contract |
| Durable operation and idempotency | owner | preserves identities | preserves identities | accepted-contract |
| Job claims, retries and sagas | owner | executes assignments | bounded native step only | accepted-contract |
| VDR lifecycle | observes | observes | owner | implemented and live accepted |
| VDR status counters | consumes | transports | owner | implemented and live accepted |
| Immutable local snapshot | consumes | transports | owner | implemented and live accepted |
| `SNAP` SVDRP command | no | invokes | owner | implemented and live accepted |
| TimerIntent and assignment | owner | transports | no | accepted-contract |
| Native Timer mutation | coordinates | transports | executes safely | disabled |
| ProgramEvent identity/provenance | owner | transports observations | bounded native observation | accepted-contract |
| MediaSession and public streaming | owner | local media broker | optional internal adapter | accepted-contract |
| Legacy OSD session and control lease | owner | local OSD broker | optional native adapter | accepted-contract |
| Native Recording mutation | coordinates | transports | executes safely | disabled |
| Native readback | consumes | requests/transports | owner | read-only foundation only |
| AccountabilityEvent history | owner | bounded producer | bounded local facts | accepted-contract |
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

## Latest Completed Plugin Slice Record

### Slice

`SB.7 - Gold-standard lifecycle and callback-boundary hardening`

### Branch

`feature/vdr-plugin-suite-bridge-foundation`

### Heads

- implementation completion commit: `5f8c4d7434157766da2853db4e58b3623b40f6c1`;
- live-acceptance repository head: `2b4df6bc239489cf1aeed239927d347db28234b9`.

### Plugin version

`0.8.0`

### Implemented

- explicit `Stopping` lifecycle state;
- deterministic `BeginStop()` and `CompleteStop()` transition;
- monitor deactivation between `stopping` and `stopped`;
- final inactive snapshot outside callback execution;
- VDR callbacks reduced to argument discard, atomic active-state check and one atomic counter increment;
- removal of per-event callback logs and other callback-side work;
- unchanged read-only `SNAP` contract.

### Changed files

- `vdr-plugin-suite-bridge/Makefile`;
- `vdr-plugin-suite-bridge/README.md`;
- `vdr-plugin-suite-bridge/docs/SB-3-status-events.md`;
- `vdr-plugin-suite-bridge/docs/SB-7-lifecycle-callback-hardening.md`;
- `vdr-plugin-suite-bridge/suitebridge.cpp`;
- `vdr-plugin-suite-bridge/suitebridge_lifecycle.cpp`;
- `vdr-plugin-suite-bridge/suitebridge_lifecycle.h`;
- `vdr-plugin-suite-bridge/suitebridge_status_monitor.cpp`;
- `vdr-plugin-suite-bridge/suitebridge_status_monitor.h`;
- `vdr-plugin-suite-bridge/tests/check_foundation_contract.py`;
- `vdr-plugin-suite-bridge/tests/check_status_events_contract.py`;
- `vdr-plugin-suite-bridge/tests/test_suitebridge_lifecycle.cpp`;
- this handoff after successful acceptance.

### Capabilities

No capability ID or state changed:

- `lifecycle`: `available`;
- `status-events`: `available`;
- `snapshots`: `available`;
- `local-contract`: `available`;
- `mutations`: `disabled`.

### Schema changes

None. Capability, snapshot and local-contract schemas remain at version `1`.

### New commands or service calls

None. `PLUG suitebridge SNAP` remains the only plugin-specific SVDRP command. No plugin service call or listener was added.

### New events or snapshots

No new event family or payload field was added. Existing status counters remain observable through immutable snapshots. The final inactive snapshot is now ordered explicitly inside the two-phase stop sequence.

### Suite-side work unblocked

- Backend Agent may rely on the explicit `stopping` and `stopped` lifecycle order for local shutdown supervision.
- Agent may consume the read-only snapshot without per-event callback logging overhead.
- Future bounded native observation work can build on a callback path proven free of external side effects.

### Suite-side work still required

- authenticated Agent transport;
- backend identity and generation framing;
- capability freshness and protocol compatibility handling;
- durable jobs, idempotency and reconciliation;
- TimerIntent and ProgramEvent runtime models;
- Streaming Gateway and media sessions;
- Legacy OSD session and controller-lease runtime;
- public `/api/v1` runtime migration;
- durable AccountabilityEvent runtime.

All mutation execution remains blocked.

### Compatibility risks

- Per-event `status-event` log lines were deliberately removed.
- Status counters remain diagnostic observations, not synchronization sequences.
- One user-visible channel change may produce more than one native `ChannelSwitch` callback; SB.7 observed increments `4 → 8 → 12` for switch and restore.
- Counter overflow, restart, reset, epoch and resynchronization semantics remain undefined.
- SVDRP availability and host restrictions remain deployment concerns.
- Reply code `900` remains plugin-specific and not part of the public Client API.

### Automated tests

Passed:

- foundation source contract;
- capability source contract;
- callback-side-effect and status-event source contract;
- status-snapshot source contract;
- local-contract payload source contract;
- read-only SVDRP source contract;
- lifecycle C++ unit test including stopping transitions and invalid repeats;
- capability C++ unit test;
- status-event C++ unit test;
- status-snapshot C++ unit test;
- local-contract C++ unit test;
- SVDRP command C++ unit test;
- version extraction check for `0.8.0`;
- final VDR shared-object build and ELF validation;
- API-11 staged installation;
- repository documentation and ADR checks.

### Live VDR acceptance

Passed on VDR `2.7.9`, API version `11`:

- plugin version `0.8.0` loaded;
- lifecycle initialization and start passed;
- capability publication passed;
- baseline, intermediate and final `SNAP` responses passed;
- original channel `7` restored after controlled channel `1` test;
- channel-switch counter increased `4 → 8 → 12`;
- Recording, replaying and Timer-change counters remained unchanged;
- no callback-side `status-event` logs were emitted;
- Timer list, Recording list and setup hash remained unchanged;
- two-phase `stopping → inactive snapshot → stopped` order passed.

### Rollback result

Passed:

- plugin, configuration and symlink removed;
- VDR restarted;
- no Suite Bridge binary remained loaded;
- original VDR state remained restored;
- worktree clean.

### Mutation state

`disabled`

### Next safe slice

`SB.8 - Diagnostic counter continuity and resynchronization contract`

Before code, SB.8 must inspect VDR restart and callback semantics and define a read-only contract for:

- process-local counter epoch or generation;
- restart and reset behavior;
- unsigned counter overflow handling;
- full-snapshot resynchronization;
- explicit statement that counters are not durable event sequences.

SB.8 must add no mutation, listener, worker, database, public API, audit store, media path, OSD control or second Agent protocol.

---

## Immediate Coordination Notes

- The bridge branch contains SB.1 through fully live-accepted SB.7.
- The coordinated branch includes accepted Suite contracts ADR-0042 through ADR-0049.
- ADR acceptance does not mean the corresponding runtime exists.
- The plugin remains deliberately read-only.
- `SNAP` remains the only implemented plugin-specific SVDRP command.
- `mutations` remains `disabled`.
- The authoritative plugin location remains `vdr-plugin-suite-bridge/` on the long-lived bridge branch.
- The next plugin slice starts only after remote, handoff, source, tests and ownership boundaries have been inspected again.

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

This standard applies equally to Recording, Timer, EPG, channel, replay, status, streaming, Legacy OSD and future plugin-service integration.
