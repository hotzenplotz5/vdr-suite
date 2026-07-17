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
| Plugin architecture decision | `vdr-plugin-suite-bridge/docs/ADR-0001-plugin-role-and-native-integration-strategy.md` |
| Plugin roadmap | `vdr-plugin-suite-bridge/docs/ROADMAP.md` |

The bridge branch is synchronized with current `main` before a new coordinated slice begins.

Suite work must not edit bridge implementation files unless the change is explicitly coordinated through this handoff. Plugin work must not modify unrelated Suite production files merely to complete a local bridge slice.

Repository changes are made directly through GitHub where tooling permits. No force-push or destructive reset is permitted for coordinated bridge work.

---

## Status Vocabulary

| State | Meaning |
| --- | --- |
| `implemented` | Present in source, covered by tests and available at the stated boundary. |
| `accepted-contract` | Decided by an accepted ADR or explicit shared contract, but not fully implemented. |
| `planned` | Intended direction that is not yet an accepted shared contract. |
| `blocked` | Cannot proceed until a named prerequisite exists. |
| `disabled` | Deliberately unavailable at runtime. |
| `superseded` | Retained for history but no longer authoritative. |

An accepted ADR does not make a runtime feature `implemented`.

A plugin capability becomes `available` only after source checks, C++ tests, final shared-object build, staged installation and required live VDR acceptance have passed.

---

## Current Plugin Snapshot

| Item | Current value |
| --- | --- |
| Last completed runtime slice | `SB.9 - Read-only capability discovery and compatibility negotiation` |
| Plugin name | `suitebridge` |
| Plugin version | `0.10.0` |
| Shared object | `libvdr-suitebridge.so.<VDR-APIVERSION>` |
| SB.9 implementation and automated-test completion commit | `1212c44b10cd81144ebc345f3cb81eca0bda47cf` |
| SB.9 live-acceptance repository head | `fbd0fab648dca85521fe27107174ede36b0b6d95` |
| Plugin ADR and roadmap documentation acceptance head | `efc885a5f9d811f3dd87c6ad204fbf3fe3f0db71` |
| Live VDR version | `2.7.9` |
| Live VDR API version | `11` |
| Live ELF build ID | `a9ff5c5917526a61eb8bf7c2bff098ea0f158820` |
| Live shared-object size | `253848` bytes |
| Mutation state | `disabled` |

The live-acceptance head contains only later unrelated Suite-side Phase 60.15g changes after the SB.9 implementation head. Comparison from `1212c44b` to `fbd0fab6` shows no change below `vdr-plugin-suite-bridge/`.

The accepted plugin ADR and roadmap are documentation-only coordination artifacts. They do not change plugin runtime behavior, plugin version, capability IDs, capability states, schema versions or mutation state.

### Implemented local endpoints

```text
PLUG suitebridge CAPS [discovery-schema]
PLUG suitebridge SNAP
```

`CAPS` is processed before any status snapshot is captured. Capability discovery therefore reads only immutable plugin contract values and does not inspect VDR-native objects or alter diagnostic counters.

`SNAP` remains the complete read-only diagnostic resynchronization point.

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
| Discovery schema | `1` |
| Capability schema | `1` |
| Snapshot schema | `2` |
| Local-contract schema | `2` |

These are independent compatibility axes. Plugin software version, discovery schema, capability schema, snapshot schema, local-contract schema, future event schema, future OSD schema, future Agent protocol version and public `/api/v1` version must never be inferred from one another.

---

## Implemented CAPS Contract

### Accepted requests

| Request | Reply | Meaning |
| --- | ---: | --- |
| `CAPS` | `900` | Return current discovery schema. |
| `CAPS 1` | `900` | Return explicitly requested discovery schema `1`. |
| `caps 01` | `900` | Case-insensitive command; leading zero is accepted. |
| `CAPS 2` | `504` | Syntactically valid but unsupported schema. |
| `CAPS abc` | `501` | Malformed decimal schema argument. |
| payload preparation failure | `451` | Local processing failure. |
| unknown command | VDR default | Plugin returns unhandled. |

Only ASCII decimal digits and defined ASCII whitespace are accepted by the option parser. Caller-supplied option text is not logged.

### Discovery field order

1. `discovery_schema`
2. `plugin_name`
3. `plugin_version`
4. `capability_schema`
5. `snapshot_schema`
6. `local_contract_schema`
7. `capabilities`

The capability array retains the static catalogue order, while consumers evaluate entries by stable ID and state rather than by position.

### Safe Agent interpretation

- unsupported discovery schemas are rejected rather than guessed;
- an unknown or absent capability ID is unavailable;
- unknown additive capability IDs may be ignored;
- `mutations=disabled` is a hard write prohibition;
- an absent `mutations` capability is also treated as disabled;
- schema compatibility is evaluated from explicit fields, not plugin version alone;
- a legacy plugin that does not implement `CAPS` enables no optional or mutating function through optimistic fallback;
- plugin capabilities are not user authorization.

### Observed live payload

```json
{"discovery_schema":1,"plugin_name":"suitebridge","plugin_version":"0.10.0","capability_schema":1,"snapshot_schema":2,"local_contract_schema":2,"capabilities":[{"id":"lifecycle","state":"available"},{"id":"status-events","state":"available"},{"id":"snapshots","state":"available"},{"id":"local-contract","state":"available"},{"id":"mutations","state":"disabled"}]}
```

Observed payload size: `364` bytes.

`CAPS`, `CAPS 1` and `caps 01` produced byte-identical payloads.

---

## Implemented SNAP and Counter-Continuity Contract

Current local-contract field order:

1. `contract_schema`
2. `capability_schema`
3. `snapshot_schema`
4. `active`
5. `total`
6. `channel_switch`
7. `recording`
8. `replaying`
9. `timer_change`
10. `counter_epoch`
11. `counter_overflow`

Each plugin instance owns one immutable 32-character lowercase hexadecimal `counter_epoch`.

Event-family counters and the derived total saturate instead of wrapping. `counter_overflow` becomes sticky when an event or total can no longer be represented exactly.

Agent rules:

- same epoch and `counter_overflow=false`: diagnostic cumulative comparison is permitted;
- changed epoch: discard the prior baseline and adopt a complete `SNAP`;
- uncertain transport continuity: adopt a complete `SNAP`;
- `counter_overflow=true`: do not derive further deltas for that epoch;
- counters are diagnostic observations, not durable sequences, audit history or guaranteed user-action counts.

Observed SB.9 live epoch:

```text
26f5b0fc557edf7767a4f2ea3a02584d
```

Observed active payload before and after all `CAPS` requests:

```json
{"contract_schema":2,"capability_schema":1,"snapshot_schema":2,"active":true,"total":4,"channel_switch":4,"recording":0,"replaying":0,"timer_change":0,"counter_epoch":"26f5b0fc557edf7767a4f2ea3a02584d","counter_overflow":false}
```

The payload remained byte-identical across all successful and rejected `CAPS` requests.

Observed final inactive payload during rollback:

```json
{"contract_schema":2,"capability_schema":1,"snapshot_schema":2,"active":false,"total":4,"channel_switch":4,"recording":0,"replaying":0,"timer_change":0,"counter_epoch":"26f5b0fc557edf7767a4f2ea3a02584d","counter_overflow":false}
```

---

## SB.9 Live VDR Acceptance

Status: `implemented`

The controlled live acceptance passed on VDR `2.7.9`, API version `11`, at repository head `fbd0fab648dca85521fe27107174ede36b0b6d95`.

Proven behavior:

- plugin version `0.10.0` loaded as `libvdr-suitebridge.so.11`;
- VDR mapped the expected API-versioned shared object;
- general plugin `HELP` listed command names `CAPS` and `SNAP`;
- `HELP CAPS` returned `CAPS [discovery-schema]` and the expected description;
- `HELP SNAP` returned the expected `SNAP` description;
- discovery schema `1`, capability schema `1`, snapshot schema `2` and local-contract schema `2` were exact;
- the seven discovery fields appeared in deterministic order;
- all five capability IDs and states were exact;
- `mutations` remained `disabled`;
- `CAPS`, `CAPS 1` and `caps 01` returned byte-identical reply-`900` payloads;
- `CAPS 2` returned reply `504` with `CAPS discovery schema unsupported`;
- `CAPS abc` returned reply `501` with `CAPS requires a decimal discovery schema`;
- `CAPS` requests did not change the `SNAP` epoch or counters;
- no callback-side `status-event` log was emitted;
- command logs remained bounded to command, result, reply, byte-count and schema facts;
- original channel `7` remained unchanged;
- Timer list and Recording list remained unchanged;
- VDR setup hash remained `e80c952eddeba470bd23a67060f079e3693e53b601781ba5cb9134d0064636dc`;
- no mutation command or VDR write operation was invoked.

The earlier two live-test interruptions were test-harness defects only:

1. the general VDR help overview was incorrectly expected to retain command options;
2. reply parsing did not normalize the line ending returned by `svdrpsend`.

Both interrupted runs triggered complete rollback. No plugin implementation defect was found.

### Two-phase stop evidence

1. lifecycle entered `stopping`;
2. status observation became inactive while retaining epoch and counters;
3. the final schema-2 inactive snapshot and local-contract payload were produced outside callbacks;
4. lifecycle entered `stopped`.

### Rollback evidence

- configuration symlink removed;
- plugin configuration removed;
- API-versioned shared object removed;
- VDR restarted;
- no Suite Bridge shared object remained mapped into the VDR process;
- original channel, Timer list, Recording list and setup hash remained restored;
- build artifacts and temporary logs were removed;
- repository worktree remained clean and synchronized.

---

## SB.8 Continuity Acceptance Retained

SB.8 remains the accepted basis for schema-2 continuity behavior.

| Item | Value |
| --- | --- |
| Plugin version | `0.9.0` |
| Implementation and test head | `396eeccc3af775c88247f748fbb200059b4b2d31` |
| Live-acceptance head | `396eeccc3af775c88247f748fbb200059b4b2d31` |
| First observed epoch | `03da379df7b143a8b5d328fcc09bbe0f` |
| Second observed epoch after VDR restart | `646821ee8491b0ed4604209fa2e0fe65` |
| First-instance channel counter | `4 → 8 → 12` |
| Overflow during normal live test | `false` |

SB.8 proved stable epoch within one instance, a changed epoch after VDR restart, saturating counters, sticky overflow semantics through unit tests, full resynchronization rules, two-phase stop for both instances and complete rollback.

---

## Accepted Plugin ADR and Roadmap

### Documentation status

- [ADR-0001: Suite Bridge Plugin Role and Native Integration Strategy](ADR-0001-plugin-role-and-native-integration-strategy.md): `accepted-contract`;
- [VDR Suite Bridge Plugin Roadmap](ROADMAP.md): accepted coordination plan with explicit `active`, `planned`, `candidate`, `blocked` and `disabled` states;
- documentation acceptance repository head: `efc885a5f9d811f3dd87c6ad204fbf3fe3f0db71`.

ADR-0001 decides that:

- Suite Bridge complements RESTfulAPI and does not replace it;
- RESTfulAPI remains preferred for broad structured VDR reads and existing domain-oriented operations;
- Suite Bridge owns immediate VDR-native observations, continuity, bounded native readback, safe Legacy OSD adaptation and narrowly typed native operations where a concrete gap is proven;
- the Backend Agent owns local transport, compatibility negotiation, buffering, freshness and local brokering;
- the Control Plane owns users, authorization, public API, multi-site policy, durable jobs, idempotency, retries, reconciliation and audit retention;
- public clients never connect directly to the plugin, SVDRP, RESTfulAPI, plugin-service interfaces or local remote-control devices;
- read-only remains the default;
- generic SVDRP, shell, raw-keycode and plugin-service tunnels are prohibited.

The roadmap establishes the coordinated sequence:

```text
SB.10  Agent consumption of existing CAPS and SNAP contracts
SB.11  bounded sequenced native event feed
SB.12  OSD notification feed
SB.13  replay and AV state
SB.14  view-only Legacy OSD
SB.15+ controller input and mutations only after all external prerequisites exist
```

### Documentation acceptance evidence

Passed at repository head `efc885a5f9d811f3dd87c6ad204fbf3fe3f0db71`:

- exact ADR, roadmap and README link-contract check;
- all plugin foundation, capability, capability-discovery, continuity, status-event, status-snapshot, local-contract and SVDRP source contracts;
- all existing plugin C++ tests;
- plugin version extraction for `0.10.0`;
- final `libvdr-suitebridge.so` build;
- ELF validation through `readelf`;
- plugin build cleanup;
- `git diff --check`;
- clean worktree synchronized with `origin/feature/vdr-plugin-suite-bridge-foundation`.

No live VDR test was required for this documentation-only change because no plugin source, binary contract, command, capability, schema or runtime behavior changed.

### Runtime impact

- plugin version remains `0.10.0`;
- implemented plugin commands remain `CAPS` and `SNAP` only;
- discovery schema remains `1`;
- capability schema remains `1`;
- snapshot and local-contract schemas remain `2`;
- `mutations` remains `disabled`;
- SB.9 remains the latest completed and live-accepted plugin runtime slice.

---

## Current SB.10 Agent Status

### SB.10a — Transport-neutral local handshake contract

Status at the Agent contract boundary: `implemented`.

Implementation head:

`d70ebee00edcab1cd019ca9e0c2541a06bf7d587`

Implemented Agent-side concepts:

- typed `ISuiteBridgeLocalTransport` boundary;
- fixed logical operations `DiscoverSchema1` and `Snapshot`;
- transport, reply and payload failure separation;
- bounded purpose-built JSON parsing;
- `CAPS 1` before `SNAP`;
- independent discovery, capability, snapshot and local-contract schema validation;
- safe legacy or unknown-plugin result;
- hard enforcement of `mutations=disabled`;
- initial baseline creation;
- epoch replacement;
- overflow handling;
- no arbitrary command string, shell, process, database, filesystem, worker or plugin-runtime coupling.

Targeted automated evidence passed locally:

- `tools/check_suite_bridge_agent_boundary.py`;
- `make test-suite-bridge-agent-boundary`;
- `make test-suite-bridge-handshake` with `test_suite_bridge_handshake passed`;
- corrected root Make/test inventory with zero orphan test sources;
- independent plugin `make check`;
- documentation checks after the required `## Back` correction.

### Coordinated acceptance state

SB.10a is not yet recorded as a fully completed coordinated slice.

The final repository-wide architecture gate failed on the synchronized branch because the then-current global SQLite boundary rejected three parallel Suite repository implementations:

```text
core/vdr/src/EpgEventRepository.cpp
core/vdr/src/EpgSearchNativeFuzzyCapabilityRepository.cpp
core/vdr/src/VdrRecordingCacheRepository.cpp
```

Those files and the SQLite architecture rule are owned by the parallel Suite/database work, not by SB.10a. The Suite Bridge workstream must not modify them merely to make its local slice green.

Before SB.10a receives final coordinated acceptance, the bridge branch must be synchronized with the current `main` architecture fix and the complete relevant repository gates must be rerun on the resulting head.

No plugin runtime change or live VDR acceptance is required for SB.10a itself because it is a transport-neutral Agent value and compatibility contract.

---

## Architectural Position

```text
Client
  → VDR-Suite public API
  → VDR-Suite Control Plane
  → VDR-Suite Backend Agent
  → local RESTfulAPI and/or Suite Bridge adapter
  → vdr-plugin-suite-bridge where native access is required
  → VDR Core
```

The plugin is not the Backend Agent.

It must not become:

- a second public Control Plane;
- a user or role database;
- a public Internet-facing HTTP API;
- an independent multi-site coordinator;
- a metadata platform;
- a long-running workflow engine;
- a Streaming Gateway;
- a public Legacy OSD session service;
- a durable audit store;
- a substitute for Suite-owned jobs, idempotency, reconciliation or authorization;
- a replacement for broad structured RESTfulAPI reads.

---

## Responsibility Boundary

### VDR-Suite Control Plane owns

- users, services and actor identities;
- roles, permissions and backend-scoped authorization;
- public `/api/v1` contract and client compatibility;
- stable Suite resource identities;
- durable operation and idempotency records;
- job scheduling, retry and saga coordination;
- multi-site policy and backend administration;
- TimerIntent, TimerAssignment and reconciliation policy;
- canonical ProgramEvent identity, history and provenance policy;
- MediaSession policy and public Streaming Gateway authorization;
- LegacyOsdSession policy and controller-lease authority;
- append-only AccountabilityEvent persistence, redaction, retention and protected queries;
- final client-visible mutation state and public error contracts.

### VDR-Suite Backend Agent owns

- authenticated machine relationship to the Control Plane;
- backend identity, generation, lease and heartbeat participation;
- local transport selection and plugin invocation;
- protocol framing and result forwarding;
- capability negotiation and freshness publication;
- comparison of counter epochs and baselines;
- full resynchronization after restart, reconnect or uncertainty;
- refusal to calculate deltas after overflow;
- bounded event and OSD buffering where later implemented;
- protection of VDR-internal credentials and transports;
- transport of backend-scoped Timer, EPG, media, OSD and accountability evidence;
- preservation of operation, job, attempt and idempotency identities.

### `vdr-plugin-suite-bridge` owns

- VDR-process-local lifecycle integration;
- safe access to VDR-native state;
- VDR lock and thread-boundary correctness;
- minimal immutable native observations;
- truthful local capability reporting;
- process-local diagnostic epoch, saturation and overflow facts;
- bounded native events and OSD observations where later implemented;
- bounded native Timer, EPG, media-provider or OSD access where later implemented;
- native identity and current-state readback where VDR exposes it;
- deterministic local results and error categories;
- no leaking of raw VDR pointers or lock ownership across the boundary.

---

## Gold-Standard VDR Native Rules

### VDR internals stay behind the plugin boundary

Raw VDR pointers, iterators, lock guards and internal addresses never cross into the Agent or Suite. Native values are copied into bounded immutable value objects before leaving the VDR access scope.

### Lock scopes are minimal and explicit

Acquire only the recommended VDR lock, copy required values, and release the lock before serialization, logging, network I/O, disk I/O, waiting, retry or long computation.

### VDR callbacks remain non-blocking

The current callback path performs only:

1. atomic active-state check;
2. one saturating atomic counter increment;
3. immediate return.

Callbacks perform no logging, serialization, allocation, locking, waiting, network access, database access, filesystem mutation or external invocation.

Future event or OSD callback work may only use fixed-capacity bounded state after a dedicated slice proves the exact VDR callback and concurrency contract.

### Lifecycle is deterministic

```text
constructed → initialized → started → stopping → stopped
```

`Stop()` enters `stopping`, disables callbacks, emits the final inactive snapshot outside callbacks and then enters `stopped`.

### Read-only is the default

Mutation capabilities remain disabled until authorization, generation fencing, revision checks, idempotency behavior, authoritative readback and live acceptance are complete.

### Capabilities are truthful and versioned

A capability is not enabled by source presence, software-version guessing or frontend visibility. Optional behavior is selected only after explicit negotiation.

### Snapshots are immutable value contracts

A snapshot is one bounded observation, not a live VDR object. Diagnostic counters are not event sequences. A changed epoch invalidates the previous comparison baseline.

### Native operations require authoritative readback

A transport acknowledgement or native method return is not necessarily final success. Future mutations require bounded readback or reconciliation.

### Unknown outcome is a valid result

The plugin must not silently retry a possibly executed native mutation. Verification or reconciliation comes first.

### No hidden alternate control plane

The plugin does not independently own authorization, durable idempotency, cross-site policy, global retries, client sessions, public API versioning, metadata authority or audit retention.

---

## Accepted Shared Contracts

### Plugin ADR-0001 — Plugin Role and Native Integration Strategy

Status: `accepted-contract`

Suite Bridge complements RESTfulAPI. Native event, OSD and typed-action surfaces are added only through bounded versioned capabilities after a concrete gap and all safety prerequisites are proven.

### ADR-0042 — Safe Mutation, Revision and Idempotency

Status: `accepted-contract`

Current plugin effect: `mutations=disabled`.

The Control Plane owns durable idempotency, authorization and operation state. Agent and plugin preserve identities, reject obsolete generations, avoid blind redispatch and provide bounded readback evidence.

### ADR-0043 — Job Claim, Retry and Saga Execution

Status: `accepted-contract`

Control Plane owns durable job and saga state. Agent executes fenced assignments. Plugin executes only bounded native steps and owns no durable retry scheduler.

### ADR-0044 — Timer Intent, Assignment and Native Timer

Status: `accepted-contract`

Control Plane owns TimerIntent, TimerAssignment and scheduling. Native Timer mutation remains disabled.

### ADR-0045 — Canonical EPG Event Identity and Provenance

Status: `accepted-contract`

Control Plane owns canonical ProgramEvent identity, revision and provenance. Plugin may later expose bounded native observations only.

### ADR-0046 — Streaming Gateway and Media Session Boundary

Status: `accepted-contract`

Control Plane owns MediaSession policy. Streaming Gateway owns the public media plane. Plugin does not expose permanent internal URLs or become the public Gateway.

### ADR-0047 — Legacy OSD Compatibility Bridge

Status: `accepted-contract`

Control Plane owns session and controller policy. Agent owns local broker and sequencing. Plugin may later expose bounded native OSD frames and allowlisted input translation only.

### ADR-0048 — Public API Versioning, Error and Compatibility

Status: `accepted-contract`

Public `/api/v1`, Agent protocol, media protocol, OSD protocol and plugin-local schemas are independent. SB.9 changes only plugin-local discovery availability and plugin version.

### ADR-0049 — Audit and Security Event Model

Status: `accepted-contract`

Control Plane owns append-only AccountabilityEvent history and policy. Plugin may produce bounded local facts only. Callback paths remain free of audit I/O.

---

## Protocol Evolution and Error Rules

- existing field meaning is never changed silently;
- additive fields require documented defaults;
- breaking changes require a new schema or protocol version;
- unsupported versions are rejected explicitly rather than guessed;
- capability negotiation occurs before optional or mutating commands;
- unknown or missing write capability means disabled;
- plugin SVDRP reply codes and JSON are not the public client API;
- transport acknowledgement and domain success are distinct;
- `not_dispatched`, `rejected_before_mutation`, `executed_unverified`, `succeeded_verified`, `failed_verified` and `outcome_unknown` remain distinct future mutation outcomes.

---

## Observability Rules

Important transitions and commands log only bounded facts such as component, event, result, plugin version, schema, command, reply and byte count.

Logs must not include credentials, tokens, raw pointers, unrestricted payloads, unbounded metadata or caller-supplied command options.

Per-event callback logging is prohibited. Runtime logs are diagnostic, not authoritative audit history.

OSD frame or message content is not written to normal logs or durable audit records.

---

## Test and Acceptance Standard

### Required automated layers

1. source and contract guards;
2. pure C++ unit tests;
3. deterministic serialization tests;
4. malformed and unsupported request tests;
5. lifecycle and callback-boundary tests;
6. epoch, saturation and overflow tests where applicable;
7. regression proof for prior read-only behavior;
8. final VDR shared-object build and ELF validation;
9. API-versioned staged installation where plugin runtime changes;
10. repository Make inventory, documentation and architecture checks.

### Required live VDR layers for plugin read-only runtime slices

1. controlled plugin build and load;
2. command and help discovery;
3. exact read-only response and negative replies;
4. proof that unrelated VDR state remains unchanged;
5. bounded log evidence and callback-side-effect exclusion;
6. deterministic shutdown;
7. plugin removal and VDR restart;
8. proof that no stale binary remains loaded;
9. restored original VDR state;
10. clean synchronized worktree.

Agent-only transport-neutral contracts do not require a live VDR test. A concrete local transport or daemon integration slice requires the live acceptance appropriate to that boundary.

### Additional requirements before any future mutation slice

- accepted shared mutation contract;
- explicit capability gate;
- disposable test resource;
- generation and revision conflict tests;
- duplicate-delivery and lost-response tests;
- authoritative native readback;
- restart and reconciliation evidence;
- rollback or compensation where possible;
- destructive actions tested last;
- no automated mutation of a user's real Recording, Timer or configuration.

---

## Current Ownership Matrix

| Concern | Suite | Agent | Plugin | Current state |
| --- | --- | --- | --- | --- |
| Public client API | owner | no | no | accepted `/api/v1` contract |
| Authentication and RBAC | owner | machine identity | no | accepted-contract / partial runtime |
| Backend identity and generation | authority | runtime owner | local fence input only | accepted-contract |
| Durable operations, jobs and idempotency | owner | preserves/executes | bounded native step | accepted-contract |
| VDR lifecycle | observes | observes | owner | implemented and live accepted |
| Diagnostic status counters | consumes | compares epoch/baseline | owner | schema `2`, live accepted |
| Immutable local snapshot | consumes | transports/resynchronizes | owner | schema `2`, live accepted |
| `SNAP` command | no | invokes | owner | implemented and live accepted |
| Capability catalogue | consumes | negotiates/publishes | owner | schema `1`, live accepted |
| `CAPS` discovery command | no | invokes/interprets | owner | discovery schema `1`, live accepted |
| Transport-neutral Suite Bridge handshake | consumes health | owner | unchanged endpoint | implemented at Agent boundary; final coordinated gate pending |
| Local typed Suite Bridge SVDRP transport | consumes health | planned owner | unchanged endpoint | next active slice |
| Native sequenced event feed | consumes | validates/buffers | future producer | planned |
| OSD notification feed | consumes | validates/buffers | future producer | planned |
| View-only Legacy OSD | session owner | local broker | future native adapter | planned |
| Native Timer mutation | coordinates | transports | executes safely | disabled |
| Native Recording mutation | coordinates | transports | executes safely | disabled |
| ProgramEvent identity/provenance | owner | transports | bounded observation | accepted-contract |
| Public streaming | owner | local broker | optional internal adapter | accepted-contract |
| Accountability history | owner | bounded producer | bounded local facts | accepted-contract |
| Plugin-owned network listener | no | no | prohibited | disabled |
| Plugin database | no | no | prohibited | disabled |

---

## Latest Completed Plugin Slice Record

### Slice

`SB.9 - Read-only capability discovery and compatibility negotiation`

### Branch

`feature/vdr-plugin-suite-bridge-foundation`

### Heads

- implementation and automated-test completion: `1212c44b10cd81144ebc345f3cb81eca0bda47cf`;
- live-acceptance repository head: `fbd0fab648dca85521fe27107174ede36b0b6d95`.

### Plugin version

`0.10.0`

### Implemented

- allocation-free fixed-capacity capability discovery payload;
- discovery schema `1`;
- deterministic plugin and schema metadata;
- deterministic five-entry capability catalogue;
- ASCII-only schema option parser;
- `CAPS`, `CAPS 1` and case-insensitive compatible request forms;
- explicit replies `900`, `501`, `504` and `451`;
- discovery dispatch before any snapshot capture;
- bounded command logging without caller option values;
- unchanged `SNAP` schema and behavior.

### Changed plugin files

- `vdr-plugin-suite-bridge/Makefile`;
- `vdr-plugin-suite-bridge/README.md`;
- `vdr-plugin-suite-bridge/docs/SB-2-capabilities.md`;
- `vdr-plugin-suite-bridge/docs/SB-6-read-only-svdrp.md`;
- `vdr-plugin-suite-bridge/docs/SB-9-capability-discovery.md`;
- `vdr-plugin-suite-bridge/suitebridge.cpp`;
- `vdr-plugin-suite-bridge/suitebridge_capability_discovery.cpp`;
- `vdr-plugin-suite-bridge/suitebridge_capability_discovery.h`;
- `vdr-plugin-suite-bridge/tests/check_capability_discovery_contract.py`;
- `vdr-plugin-suite-bridge/tests/check_foundation_contract.py`;
- `vdr-plugin-suite-bridge/tests/check_svdrp_contract.py`;
- `vdr-plugin-suite-bridge/tests/test_suitebridge_capability_discovery.cpp`;
- this handoff after successful acceptance.

### Capabilities and schemas

No capability ID or state changed.

- discovery schema: new version `1`;
- capability schema: remains `1`;
- snapshot schema: remains `2`;
- local-contract schema: remains `2`;
- mutations: remains `disabled`.

### Automated tests

Passed:

- foundation source contract;
- capability source contract;
- capability-discovery source contract;
- counter-continuity source contract;
- status-event and callback-side-effect source contract;
- status-snapshot source contract;
- local-contract source contract;
- read-only SVDRP source contract;
- all existing lifecycle, capability, continuity, event, snapshot, local-contract and SVDRP C++ tests;
- new deterministic capability-discovery C++ tests;
- supported, unsupported, malformed and payload-exhaustion reply tests;
- version extraction for `0.10.0`;
- final VDR shared-object build and ELF validation;
- API-11 staged installation;
- repository documentation, ADR and phase checks.

### Live acceptance

Passed at `fbd0fab648dca85521fe27107174ede36b0b6d95` with:

- VDR `2.7.9`, API `11`;
- ELF build ID `a9ff5c5917526a61eb8bf7c2bff098ea0f158820`;
- discovery payload size `364` bytes;
- epoch `26f5b0fc557edf7767a4f2ea3a02584d`;
- exact help overview and detail behavior;
- exact capability catalogue and schemas;
- byte-identical successful discovery requests;
- exact `504` and `501` replies;
- unchanged `SNAP` payload;
- unchanged channel, Timers, Recordings and setup hash;
- bounded logs and log-free callback path;
- two-phase stop;
- complete rollback.

### Mutation state

`disabled`

---

## Next Safe Coordinated Slice

`SB.10b - Backend Agent local typed SVDRP transport`

Primary ownership: **Backend Agent**.

### Prerequisite acceptance gate

Before SB.10b implementation changes are accepted:

1. synchronize the bridge branch with the current `main` that contains the Suite-owned SQLite architecture-boundary correction;
2. prove that neither `core/agent/` nor `vdr-plugin-suite-bridge/` was changed unintentionally by that synchronization;
3. rerun the SB.10a boundary and handshake tests;
4. rerun the root Make/test inventory;
5. rerun plugin `make check`;
6. rerun documentation and architecture checks;
7. require a clean synchronized worktree;
8. record the resulting SB.10a automated-acceptance head before declaring SB.10a complete.

### SB.10b scope

Implement one dedicated local Agent transport that can execute only the typed SB.10a operations:

```text
DiscoverSchema1
Snapshot
```

The transport must:

- connect to the configured local VDR SVDRP endpoint;
- validate the greeting;
- issue only fixed allowlisted Suite Bridge requests;
- parse single-line and multiline replies deterministically;
- preserve reply code separately from payload;
- normalize supported line endings;
- enforce connection, read, operation and payload limits;
- distinguish unavailable, timeout, failed and rejected replies;
- close resources deterministically;
- expose no arbitrary command-text method;
- use no shell, `system()`, `popen()`, fork or subprocess;
- log only bounded transport facts.

The permanent architecture is a small direct Agent-owned SVDRP client or an equivalently strict typed local transport. The existing mutation-specific `SvdrpChannelMoveExecutor` must not become the generic Agent transport.

Expected plugin changes for SB.10b: **none**.

A plugin change is justified only if the Agent implementation and tests demonstrate a concrete bounded compatibility gap that cannot be handled safely on the Agent side.

SB.10b non-goals:

- no native mutation;
- no plugin listener or outbound connection;
- no backend ID or generation ownership in the plugin;
- no public API coupling to plugin JSON;
- no Timer, Recording, EPG, media or OSD surface;
- no durable job, idempotency or audit store in the plugin;
- no arbitrary SVDRP command tunnel;
- no reinterpretation of `counter_epoch` as backend generation or security identity.

---

## Planned Plugin Direction After SB.10

The accepted roadmap identifies the following direction, subject to a fresh precheck for every slice:

- `SB.11`: bounded sequenced native event feed for exact VDR transitions;
- `SB.12`: bounded OSD notification feed for short-lived native messages;
- `SB.13`: read-only replay and AV state;
- `SB.14`: view-only Legacy OSD with epoch, frame sequence and full resynchronization;
- `SB.15+`: controller input, typed native actions and mutations only after the complete Control Plane and Agent prerequisites exist.

No future capability ID, schema or command name is considered implemented merely because it appears in the roadmap.

---

## Coordination Workflow

Before every coordinated slice:

1. inspect current remote branch and compare with the last accepted head;
2. read this complete handoff, Plugin ADR-0001 and relevant accepted VDR-Suite ADRs;
3. inspect actual source, documentation, Make ownership and tests;
4. distinguish Suite, Agent, plugin and shared ownership;
5. state exact scope, affected files, schema/capability impact and non-goals;
6. keep mutations disabled unless every shared prerequisite exists;
7. preserve parallel changes;
8. run all existing and new tests;
9. perform required live acceptance before declaring a native surface available;
10. update this handoff only after acceptance.

---

## Immediate Coordination Notes

- The bridge branch contains fully live-accepted SB.1 through SB.9.
- Plugin ADR-0001 and `ROADMAP.md` are accepted at documentation head `efc885a5f9d811f3dd87c6ad204fbf3fe3f0db71`.
- SB.10a is implemented at the transport-neutral Agent contract boundary at `d70ebee00edcab1cd019ca9e0c2541a06bf7d587`.
- SB.10a targeted tests passed; final coordinated acceptance still requires synchronization with the Suite-owned architecture fix and a clean repository-wide gate.
- `CAPS` and `SNAP` remain the only implemented plugin-specific SVDRP commands.
- Capability discovery schema is `1`.
- Capability schema is `1`.
- Snapshot and local-contract schemas are `2`.
- Diagnostic counter continuity is explicit through epoch, saturation, overflow and full-resynchronization rules.
- The plugin remains deliberately read-only.
- `mutations` remains `disabled`.
- The next active implementation work is SB.10b in the Backend Agent after the prerequisite acceptance gate.
- The first expected plugin-expanding slice after complete SB.10 acceptance is SB.11, subject to a new full source and callback audit.
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
- required repository-wide gates pass;
- required live VDR acceptance passes;
- rollback is proven;
- documentation and this handoff match the implementation;
- no existing VDR behavior is weakened to simplify integration.

This standard applies equally to Recording, Timer, EPG, channel, replay, status, streaming, Legacy OSD and future plugin-service integration.
