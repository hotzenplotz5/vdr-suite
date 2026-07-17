# VDR-Suite / Suite Bridge Gold-Standard Handoff

## Navigation

- [Plugin README](../README.md)
- [Plugin ADR-0001](ADR-0001-plugin-role-and-native-integration-strategy.md)
- [Plugin Roadmap](ROADMAP.md)
- [SB.10a Agent Handshake](../../docs/architecture/suite-bridge-agent-handshake.md)
- [SB.10b Local SVDRP Transport](../../docs/architecture/suite-bridge-svdrp-transport.md)
- [VDR-Suite Documentation Index](../../docs/index.md)

---

## Purpose

This document is the shared operational coordination contract between:

- the VDR-Suite Control Plane;
- the VDR-Suite Backend Agent;
- `vdr-plugin-suite-bridge`;
- VDR Core.

The target is:

> VDR-Suite shall be the gold standard for safe, modern, robust and multi-backend VDR integration while preserving VDR-native correctness.

Accepted VDR-Suite ADRs remain authoritative for long-term platform decisions.
Stable plugin documents remain authoritative for implemented plugin slices. This
handoff records the actual synchronized implementation, automated-test,
live-acceptance and dependency state between the workstreams.

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

The bridge branch is synchronized with current `main` before a new coordinated
slice begins.

Suite work must not edit bridge implementation files unless the change is
explicitly coordinated through this handoff. Plugin work must not modify
unrelated Suite production files merely to complete a local bridge slice.

Repository changes are made directly through GitHub where tooling permits. No
force-push or destructive reset is permitted for coordinated bridge work.

---

## Status Vocabulary

| State | Meaning |
| --- | --- |
| `completed` | Implemented, all required acceptance gates passed and the accepted head is recorded. |
| `implemented` | Present in source and covered by tests at the stated boundary, but final coordinated acceptance may still be pending. |
| `accepted-contract` | Decided by an accepted ADR or explicit shared contract, but not fully implemented. |
| `planned` | Intended direction that is not yet an accepted shared contract. |
| `blocked` | Cannot proceed until a named prerequisite exists. |
| `disabled` | Deliberately unavailable at runtime. |
| `superseded` | Retained for history but no longer authoritative. |

An accepted ADR does not make a runtime feature `implemented` or `completed`.

A plugin capability becomes `available` only after source checks, C++ tests,
final shared-object build, staged installation and required live VDR acceptance
have passed.

---

## Current Coordinated Snapshot

| Item | Current value |
| --- | --- |
| Last completed plugin runtime slice | `SB.9 - Read-only capability discovery and compatibility negotiation` |
| Last completed Agent contract slice | `SB.10a - Transport-neutral local handshake contract` |
| Last completed Agent transport slice | `SB.10b - Local typed SVDRP transport` |
| Next active coordinated slice | `SB.10c - Read-only polling, reconnect and freshness` |
| Plugin name | `suitebridge` |
| Plugin version | `0.10.0` |
| Shared object | `libvdr-suitebridge.so.<VDR-APIVERSION>` |
| SB.9 implementation and automated-test completion commit | `1212c44b10cd81144ebc345f3cb81eca0bda47cf` |
| SB.9 live-acceptance repository head | `fbd0fab648dca85521fe27107174ede36b0b6d95` |
| Plugin ADR and roadmap documentation acceptance head | `efc885a5f9d811f3dd87c6ad204fbf3fe3f0db71` |
| SB.10a Agent implementation head | `d70ebee00edcab1cd019ca9e0c2541a06bf7d587` |
| SB.10a repository-wide automated-acceptance head | `ba6deddbfba6d50b1152d584654a92f75340dcc3` |
| SB.10b implementation, automated-test and live-acceptance head | `3396840d41260bb3ed81bc652921b329263d7e58` |
| Suite `main` head included in SB.10a and SB.10b implementation baseline | `8ba96dbb46019030f7cb3ebcb95929034b6166d3` |
| Current later `main` head observed after SB.10b acceptance | `410f104a45890dc86f3a83813552ffe3dd141202` |
| SQLite architecture decision and guard correction | `892ede4392a5232a7045af3386903f502e450b5d` |
| Live VDR version | `2.7.9` |
| Live VDR API version | `11` |
| SB.9 live ELF build ID | `a9ff5c5917526a61eb8bf7c2bff098ea0f158820` |
| SB.9 live shared-object size | `253848` bytes |
| SB.10b live shared-object SHA-256 | `a84c4571e951da94de2c0b5f9badf2c74034fe94b0c43483dfa9d9345d513b5d` |
| SB.10b live ELF build ID | not present |
| Mutation state | `disabled` |

The accepted plugin ADR and roadmap are coordination artifacts. They do not
change plugin runtime behavior, plugin version, capability IDs, capability
states, schema versions or mutation state.

SB.10a and SB.10b are Agent-side slices. Neither changes the plugin binary or
its local contract.

The later `main` head `410f104a45890dc86f3a83813552ffe3dd141202`
contains parallel Recording genre-artwork work only. It was not part of the
SB.10b acceptance head and must be compared and synchronized before SB.10c.

---

## Current Plugin Runtime Contract

### Implemented local endpoints

```text
PLUG suitebridge CAPS [discovery-schema]
PLUG suitebridge SNAP
```

`CAPS` is processed before any status snapshot is captured. Capability discovery
therefore reads only immutable plugin contract values and does not inspect
VDR-native objects or alter diagnostic counters.

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

These are independent compatibility axes. Plugin software version, discovery
schema, capability schema, snapshot schema, local-contract schema, future event
schema, future OSD schema, future Agent protocol version and public `/api/v1`
version must never be inferred from one another.

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

Only ASCII decimal digits and defined ASCII whitespace are accepted by the
option parser. Caller-supplied option text is not logged.

### Discovery field order

1. `discovery_schema`
2. `plugin_name`
3. `plugin_version`
4. `capability_schema`
5. `snapshot_schema`
6. `local_contract_schema`
7. `capabilities`

The capability array retains the static catalogue order, while consumers
evaluate entries by stable ID and state rather than by position.

### Safe Agent interpretation

- unsupported discovery schemas are rejected rather than guessed;
- an unknown or absent capability ID is unavailable;
- unknown additive capability IDs may be ignored;
- `mutations=disabled` is a hard write prohibition;
- an absent `mutations` capability is also treated as disabled;
- schema compatibility is evaluated from explicit fields, not plugin version;
- a legacy plugin without `CAPS` enables no optional or mutating function through
  optimistic fallback;
- a missing plugin reply `550` enables no fallback and is classified as
  `LegacyOrUnknown`;
- plugin capabilities are not user authorization.

### Observed live payload

```json
{"discovery_schema":1,"plugin_name":"suitebridge","plugin_version":"0.10.0","capability_schema":1,"snapshot_schema":2,"local_contract_schema":2,"capabilities":[{"id":"lifecycle","state":"available"},{"id":"status-events","state":"available"},{"id":"snapshots","state":"available"},{"id":"local-contract","state":"available"},{"id":"mutations","state":"disabled"}]}
```

Observed payload size: `364` bytes.

`CAPS`, `CAPS 1` and `caps 01` produced byte-identical payloads during SB.9.

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

Each plugin instance owns one immutable 32-character lowercase hexadecimal
`counter_epoch`.

Event-family counters and the derived total saturate instead of wrapping.
`counter_overflow` becomes sticky when an event or total can no longer be
represented exactly.

Agent rules:

- same epoch and `counter_overflow=false`: diagnostic cumulative comparison is
  permitted;
- changed epoch: discard the prior baseline and adopt a complete `SNAP`;
- uncertain transport continuity: adopt a complete `SNAP`;
- `counter_overflow=true`: do not derive further deltas for that epoch;
- counters are diagnostic observations, not durable sequences, audit history or
  guaranteed user-action counts.

Observed SB.9 live epoch:

```text
26f5b0fc557edf7767a4f2ea3a02584d
```

Observed active payload before and after all SB.9 `CAPS` requests:

```json
{"contract_schema":2,"capability_schema":1,"snapshot_schema":2,"active":true,"total":4,"channel_switch":4,"recording":0,"replaying":0,"timer_change":0,"counter_epoch":"26f5b0fc557edf7767a4f2ea3a02584d","counter_overflow":false}
```

Observed final inactive payload during SB.9 rollback:

```json
{"contract_schema":2,"capability_schema":1,"snapshot_schema":2,"active":false,"total":4,"channel_switch":4,"recording":0,"replaying":0,"timer_change":0,"counter_epoch":"26f5b0fc557edf7767a4f2ea3a02584d","counter_overflow":false}
```

---

## SB.9 Live VDR Acceptance

Status: `completed`

The controlled live acceptance passed on VDR `2.7.9`, API version `11`, at
repository head `fbd0fab648dca85521fe27107174ede36b0b6d95`.

Proven behavior:

- plugin version `0.10.0` loaded as `libvdr-suitebridge.so.11`;
- VDR mapped the expected API-versioned shared object;
- general plugin `HELP` listed `CAPS` and `SNAP`;
- `HELP CAPS` and `HELP SNAP` returned the expected contracts;
- discovery schema `1`, capability schema `1`, snapshot schema `2` and
  local-contract schema `2` were exact;
- all discovery fields and all five capability entries were exact;
- `mutations` remained `disabled`;
- valid discovery request forms returned byte-identical payloads;
- unsupported and malformed discovery schemas returned exact replies;
- discovery did not change the `SNAP` epoch or counters;
- no callback-side event log was emitted;
- channel, Timers, Recordings and setup hash remained unchanged;
- no mutation command or VDR write operation was invoked;
- two-phase stop and complete rollback passed.

Rollback evidence:

- configuration symlink removed;
- plugin configuration removed;
- API-versioned shared object removed;
- VDR restarted;
- no Suite Bridge shared object remained mapped;
- original VDR state remained restored;
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

SB.8 proved stable epoch within one instance, a changed epoch after VDR restart,
saturating counters, sticky overflow semantics through unit tests, full
resynchronization rules, two-phase stop and complete rollback.

---

## Accepted Plugin ADR and Roadmap

### Documentation status

- [ADR-0001: Suite Bridge Plugin Role and Native Integration Strategy](ADR-0001-plugin-role-and-native-integration-strategy.md): `accepted-contract`;
- [VDR Suite Bridge Plugin Roadmap](ROADMAP.md): accepted coordination plan;
- original documentation acceptance repository head:
  `efc885a5f9d811f3dd87c6ad204fbf3fe3f0db71`.

ADR-0001 decides that:

- Suite Bridge complements RESTfulAPI and does not replace it;
- RESTfulAPI remains preferred for broad structured VDR reads and existing
  domain-oriented operations;
- Suite Bridge owns immediate VDR-native observations, continuity, bounded
  native readback, safe Legacy OSD adaptation and narrowly typed native
  operations where a concrete gap is proven;
- the Backend Agent owns local transport, compatibility negotiation, buffering,
  freshness and local brokering;
- the Control Plane owns users, authorization, public API, multi-site policy,
  durable jobs, idempotency, retries, reconciliation and audit retention;
- public clients never connect directly to plugin-local transports;
- read-only remains the default;
- generic SVDRP, shell, raw-keycode and plugin-service tunnels are prohibited.

The roadmap establishes the coordinated sequence:

```text
SB.10  Agent consumption of existing CAPS and SNAP contracts
SB.11  bounded sequenced native event feed
SB.12  OSD notification feed
SB.13  replay and AV state
SB.14  view-only Legacy OSD
SB.15+ controller input and mutations after all external prerequisites
```

---

## SB.10a Repository-Wide Acceptance

### Slice

`SB.10a - Transport-neutral local handshake contract`

Status: `completed`

Primary owner: **Backend Agent**.

### Heads

| Item | Value |
| --- | --- |
| Implementation head | `d70ebee00edcab1cd019ca9e0c2541a06bf7d587` |
| Repository-wide automated-acceptance head | `ba6deddbfba6d50b1152d584654a92f75340dcc3` |
| Included Suite `main` head | `8ba96dbb46019030f7cb3ebcb95929034b6166d3` |
| Included SQLite architecture correction | `892ede4392a5232a7045af3386903f502e450b5d` |

### Implemented Agent-side concepts

- typed `ISuiteBridgeLocalTransport` boundary;
- fixed logical operations `DiscoverSchema1` and `Snapshot`;
- transport, reply and payload failure separation;
- bounded purpose-built JSON parsing;
- `CAPS 1` before `SNAP`;
- independent discovery, capability, snapshot and local-contract schema
  validation;
- safe legacy or unknown-plugin result;
- hard enforcement of `mutations=disabled`;
- initial baseline creation;
- epoch replacement;
- overflow handling;
- no arbitrary command string, shell, process, database, filesystem, worker or
  plugin-runtime coupling.

### Repository-wide acceptance evidence

The coordinated SB.10a acceptance passed at repository head
`ba6deddbfba6d50b1152d584654a92f75340dcc3`.

Proven acceptance:

- the bridge branch was synchronized with Suite `main` head
  `8ba96dbb46019030f7cb3ebcb95929034b6166d3`;
- ADR-0050 and the corrected direct-SQLite architecture boundary were included;
- synchronization changed neither `core/agent/` nor
  `vdr-plugin-suite-bridge/` unintentionally;
- Python source compilation passed for inventory, boundary and architecture
  checks;
- the strict root Make/test inventory passed with zero ungrouped targets, zero
  orphan test sources and zero stale references;
- `tools/check_suite_bridge_agent_boundary.py` passed;
- `make test-suite-bridge-agent-boundary` passed;
- `make test-suite-bridge-handshake` passed;
- independent plugin `make check` passed;
- all existing plugin source-contract and C++ tests passed;
- plugin version extraction remained `0.10.0`;
- final `libvdr-suitebridge.so` build and ELF validation passed;
- plugin build artifacts were cleaned;
- complete repository documentation checks passed;
- ADR index validation passed with ADR-0050 active and ADR-0051 next;
- the global architecture check passed;
- the final worktree was clean;
- the accepted merge head was pushed to the long-lived bridge branch.

### Live-acceptance classification

No live VDR test was required for SB.10a.

SB.10a is a transport-neutral Agent value, parser, compatibility and handshake
contract. It opens no real transport, invokes no VDR command and changes no
plugin runtime behavior.

### Plugin and schema impact

- plugin version remains `0.10.0`;
- implemented plugin commands remain `CAPS` and `SNAP`;
- discovery schema remains `1`;
- capability schema remains `1`;
- snapshot and local-contract schemas remain `2`;
- no capability ID or capability state changed;
- `mutations` remains `disabled`.

---

## SB.10b Local Typed SVDRP Transport Acceptance

### Slice

`SB.10b - Backend Agent local typed SVDRP transport`

Status: `completed`

Primary owner: **Backend Agent**.

### Head

| Item | Value |
| --- | --- |
| Implementation head | `3396840d41260bb3ed81bc652921b329263d7e58` |
| Repository-wide automated-acceptance head | `3396840d41260bb3ed81bc652921b329263d7e58` |
| Controlled live-acceptance head | `3396840d41260bb3ed81bc652921b329263d7e58` |
| Included Suite `main` baseline | `8ba96dbb46019030f7cb3ebcb95929034b6166d3` |
| VDR version | `2.7.9` |
| VDR API version | `11` |
| Live endpoint | `127.0.0.1:6419` |
| Installed shared object | `libvdr-suitebridge.so.11` |
| Installed object SHA-256 | `a84c4571e951da94de2c0b5f9badf2c74034fe94b0c43483dfa9d9345d513b5d` |
| ELF build ID | not present |

### Implemented transport contract

- direct Agent-owned synchronous SVDRP client;
- one independent socket per typed invocation;
- public C++ boundary accepts only `DiscoverSchema1` and `Snapshot`;
- exact wire requests are `PLUG suitebridge CAPS 1` and
  `PLUG suitebridge SNAP`;
- non-blocking connect with bounded `poll()` deadlines;
- connect completion checked through `SO_ERROR`;
- close-on-exec descriptors and no-signal sends;
- greeting code `220` required before a request;
- CRLF and LF accepted;
- deterministic single-line and multiline parsing;
- multiline reply codes must remain identical;
- transport status, reply code, payload and diagnostic remain separate;
- fixed greeting, reply-byte and line-count limits;
- deterministic RAII socket cleanup;
- localhost and numeric addresses only;
- no arbitrary command text;
- no DNS-dependent host lookup;
- no shell, `system()`, `popen()`, fork, subprocess or `svdrpsend`;
- no database, filesystem, RESTfulAPI, daemon-runtime or plugin-runtime coupling;
- no reuse of the mutation-specific `SvdrpChannelMoveExecutor`.

### Automated acceptance evidence

The final automated acceptance passed at
`3396840d41260bb3ed81bc652921b329263d7e58`.

Proven acceptance:

- Python contract sources compiled;
- strict Make inventory reported zero ungrouped test targets;
- strict Make inventory reported zero orphan test sources;
- strict Make inventory reported zero stale test references;
- strict Make inventory reported zero duplicate linker flags;
- strict Make inventory reported zero unclassified runtime test sources;
- SB.10a Agent boundary regression passed;
- SB.10a handshake regression passed;
- real-VDR reply-`550` missing-plugin regression passed;
- SB.10b transport boundary guard passed;
- deterministic loopback socket test passed;
- exact `CAPS 1` and `SNAP` requests were proven;
- successful and rejected reply codes were preserved correctly;
- malformed greeting and malformed reply cases were rejected;
- incomplete and inconsistent multiline replies were rejected;
- greeting and command-reply timeout behavior passed;
- oversized reply handling passed;
- unavailable and invalid configuration handling passed;
- connection failure handling passed;
- peer closure was deterministic;
- independent plugin source-contract tests passed;
- all plugin C++ tests passed;
- plugin version extraction remained `0.10.0`;
- final plugin shared-object build and ELF validation passed;
- plugin build artifacts were cleaned;
- complete documentation checks passed;
- documentation indexes and reachability passed;
- ADR index validation passed with ADR-0050 active and ADR-0051 next;
- completed-phase marker validation passed;
- global architecture check passed;
- `git diff --check` passed;
- final worktree was clean and synchronized.

### Negative live VDR evidence

Before controlled plugin installation:

- VDR was active and reachable at `127.0.0.1:6419`;
- the direct SB.10b socket transport reached VDR successfully;
- VDR returned reply `550 Plugin "suitebridge" not found`;
- no Suite Bridge shared object was installed;
- no Suite Bridge configuration was present;
- no Suite Bridge object was mapped into the VDR process;
- the initial handshake safely stopped before `SNAP`;
- no mutation surface existed.

The real reply exposed and resolved one compatibility classification gap:

- reply `500` means the plugin exists but does not implement `CAPS`;
- reply `550` means the named plugin is missing or not loaded;
- both now map to `LegacyOrUnknown`;
- neither permits a subsequent `SNAP` request;
- other non-`900` discovery replies remain `DiscoveryReplyRejected`;
- all outcomes remain fail-closed with mutations disabled.

### Positive controlled live VDR evidence

After controlled installation:

- plugin version `0.10.0` loaded as `libvdr-suitebridge.so.11`;
- VDR mapped the expected API-versioned object;
- general plugin `HELP` listed `CAPS` and `SNAP`;
- `CAPS 1` returned reply `900`;
- discovery schema `1` was accepted;
- capability schema `1` was accepted;
- snapshot schema `2` was accepted;
- local-contract schema `2` was accepted;
- `mutations` remained `disabled`;
- `SNAP` returned reply `900`;
- the snapshot reported `active=true`;
- the snapshot contained a valid `counter_epoch`;
- the direct Agent handshake returned `status=ready`;
- the live contract reported plugin version `0.10.0`;
- the live contract reported `counter_overflow=false`;
- only `CAPS 1` and `SNAP` were invoked;
- channel state remained unchanged;
- Timer state remained unchanged;
- Recording state remained unchanged;
- `/var/lib/vdr/setup.conf` remained unchanged;
- logs contained only bounded command, result, reply, byte-count and schema facts.

### Complete rollback evidence

- the Suite Bridge configuration symlink was removed;
- the Suite Bridge configuration file was removed;
- `libvdr-suitebridge.so.11` was removed;
- VDR restarted successfully;
- no Suite Bridge shared object remained mapped;
- channel state remained unchanged after rollback;
- Timer state remained unchanged after rollback;
- Recording state remained unchanged after rollback;
- `/var/lib/vdr/setup.conf` remained unchanged after rollback;
- plugin build artifacts were removed;
- temporary evidence was removed;
- repository worktree remained clean and synchronized.

### Plugin and schema impact

- no plugin source changed for SB.10b;
- plugin version remains `0.10.0`;
- implemented plugin commands remain `CAPS` and `SNAP`;
- discovery schema remains `1`;
- capability schema remains `1`;
- snapshot schema remains `2`;
- local-contract schema remains `2`;
- no capability ID or capability state changed;
- `mutations` remains `disabled`.

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
- a substitute for Suite-owned jobs, idempotency, reconciliation or
  authorization;
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
- append-only AccountabilityEvent persistence, redaction, retention and
  protected queries;
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
- transport of backend-scoped Timer, EPG, media, OSD and accountability
  evidence;
- preservation of operation, job, attempt and idempotency identities.

### `vdr-plugin-suite-bridge` owns

- VDR-process-local lifecycle integration;
- safe access to VDR-native state;
- VDR lock and thread-boundary correctness;
- minimal immutable native observations;
- truthful local capability reporting;
- process-local diagnostic epoch, saturation and overflow facts;
- bounded native events and OSD observations where later implemented;
- bounded native Timer, EPG, media-provider or OSD access where later
  implemented;
- native identity and current-state readback where VDR exposes it;
- deterministic local results and error categories;
- no leaking of raw VDR pointers or lock ownership across the boundary.

---

## Gold-Standard VDR Native Rules

### VDR internals stay behind the plugin boundary

Raw VDR pointers, iterators, lock guards and internal addresses never cross into
the Agent or Suite. Native values are copied into bounded immutable value
objects before leaving the VDR access scope.

### Lock scopes are minimal and explicit

Acquire only the recommended VDR lock, copy required values, and release the
lock before serialization, logging, network I/O, disk I/O, waiting, retry or
long computation.

### VDR callbacks remain non-blocking

The current callback path performs only:

1. atomic active-state check;
2. one saturating atomic counter increment;
3. immediate return.

Callbacks perform no logging, serialization, allocation, locking, waiting,
network access, database access, filesystem mutation or external invocation.

Future event or OSD callback work may only use fixed-capacity bounded state after
a dedicated slice proves the exact VDR callback and concurrency contract.

### Lifecycle is deterministic

```text
constructed → initialized → started → stopping → stopped
```

`Stop()` enters `stopping`, disables callbacks, emits the final inactive snapshot
outside callbacks and then enters `stopped`.

### Read-only is the default

Mutation capabilities remain disabled until authorization, generation fencing,
revision checks, idempotency behavior, authoritative readback and live acceptance
are complete.

### Capabilities are truthful and versioned

A capability is not enabled by source presence, software-version guessing or
frontend visibility. Optional behavior is selected only after explicit
negotiation.

### Snapshots are immutable value contracts

A snapshot is one bounded observation, not a live VDR object. Diagnostic counters
are not event sequences. A changed epoch invalidates the previous comparison
baseline.

### Native operations require authoritative readback

A transport acknowledgement or native method return is not necessarily final
success. Future mutations require bounded readback or reconciliation.

### Unknown outcome is a valid result

The plugin must not silently retry a possibly executed native mutation.
Verification or reconciliation comes first.

### No hidden alternate control plane

The plugin does not independently own authorization, durable idempotency,
cross-site policy, global retries, client sessions, public API versioning,
metadata authority or audit retention.

---

## Accepted Shared Contracts

### Plugin ADR-0001 — Plugin Role and Native Integration Strategy

Status: `accepted-contract`

Suite Bridge complements RESTfulAPI. Native event, OSD and typed-action surfaces
are added only through bounded versioned capabilities after a concrete gap and
all safety prerequisites are proven.

### ADR-0042 — Safe Mutation, Revision and Idempotency

Status: `accepted-contract`

Current plugin effect: `mutations=disabled`.

### ADR-0043 — Job Claim, Retry and Saga Execution

Status: `accepted-contract`

Control Plane owns durable job and saga state. Agent executes fenced assignments.
Plugin executes only bounded native steps and owns no durable retry scheduler.

### ADR-0044 — Timer Intent, Assignment and Native Timer

Status: `accepted-contract`

Control Plane owns TimerIntent, TimerAssignment and scheduling. Native Timer
mutation remains disabled.

### ADR-0045 — Canonical EPG Event Identity and Provenance

Status: `accepted-contract`

Control Plane owns canonical ProgramEvent identity, revision and provenance.
Plugin may later expose bounded native observations only.

### ADR-0046 — Streaming Gateway and Media Session Boundary

Status: `accepted-contract`

Control Plane owns MediaSession policy. Streaming Gateway owns the public media
plane. Plugin does not expose permanent internal URLs or become the public
Gateway.

### ADR-0047 — Legacy OSD Compatibility Bridge

Status: `accepted-contract`

Control Plane owns session and controller policy. Agent owns local broker and
sequencing. Plugin may later expose bounded native OSD frames and allowlisted
input translation only.

### ADR-0048 — Public API Versioning, Error and Compatibility

Status: `accepted-contract`

Public `/api/v1`, Agent protocol, media protocol, OSD protocol and plugin-local
schemas are independent.

### ADR-0049 — Audit and Security Event Model

Status: `accepted-contract`

Control Plane owns append-only AccountabilityEvent history and policy. Plugin may
produce bounded local facts only. Callback paths remain free of audit I/O.

### ADR-0050 — Domain Repository SQLite Boundary

Status: `accepted-contract`

Direct SQLite access is restricted to generic SQLite infrastructure, approved
Domain `*Repository.cpp` implementations and explicitly registered schema
contract tests. This decision resolved the repository-wide architecture gate
that had blocked final SB.10a coordination acceptance.

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
- future mutation outcomes remain explicitly differentiated.

---

## Observability Rules

Important transitions and commands log only bounded facts such as component,
event, result, plugin version, schema, command, reply and byte count.

Logs must not include credentials, tokens, raw pointers, unrestricted payloads,
unbounded metadata or caller-supplied command options.

Per-event callback logging is prohibited. Runtime logs are diagnostic, not
authoritative audit history.

OSD frame or message content is not written to normal logs or durable audit
records.

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

### Required live VDR layers for plugin or concrete local-transport slices

1. controlled plugin build and load where required;
2. command and help discovery;
3. exact read-only response and negative replies;
4. proof that unrelated VDR state remains unchanged;
5. bounded log evidence and callback-side-effect exclusion;
6. deterministic shutdown;
7. plugin removal and VDR restart where the plugin was staged;
8. proof that no stale binary remains loaded;
9. restored original VDR state;
10. clean synchronized worktree.

Agent-only transport-neutral contracts do not require a live VDR test. A concrete
local transport or daemon integration slice requires live acceptance appropriate
to that boundary.

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
| Transport-neutral Suite Bridge handshake | consumes health | owner | unchanged endpoint | completed at `ba6deddbfba6d50b1152d584654a92f75340dcc3` |
| Local typed Suite Bridge SVDRP transport | consumes health | owner | unchanged endpoint | completed at `3396840d41260bb3ed81bc652921b329263d7e58` |
| Polling, reconnect and freshness lifecycle | consumes health | owner | unchanged endpoint | next active slice |
| Embedded Agent runtime integration | consumes health | owner with Suite runtime | unchanged endpoint | planned after SB.10c |
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

## Latest Completed Plugin Runtime Slice Record

### Slice

`SB.9 - Read-only capability discovery and compatibility negotiation`

### Branch

`feature/vdr-plugin-suite-bridge-foundation`

### Heads

- implementation and automated-test completion:
  `1212c44b10cd81144ebc345f3cb81eca0bda47cf`;
- live-acceptance repository head:
  `fbd0fab648dca85521fe27107174ede36b0b6d95`.

### Plugin version

`0.10.0`

### Capabilities and schemas

- discovery schema: `1`;
- capability schema: `1`;
- snapshot schema: `2`;
- local-contract schema: `2`;
- mutations: `disabled`.

SB.10a and SB.10b consume the accepted plugin runtime contract but do not create
a later plugin runtime slice.

---

## Next Safe Coordinated Slice

`SB.10c - Backend Agent read-only polling, reconnect and freshness`

Primary ownership: **Backend Agent**.

Expected plugin changes: **none**.

### Completed prerequisites

SB.10a is completed at:

```text
ba6deddbfba6d50b1152d584654a92f75340dcc3
```

SB.10b is completed with automated and controlled live acceptance at:

```text
3396840d41260bb3ed81bc652921b329263d7e58
```

### Required SB.10c precheck

Before SB.10c code is written:

1. inspect the current remote bridge branch and compare it with
   `3396840d41260bb3ed81bc652921b329263d7e58`;
2. inspect current `main` and synchronize only after reviewing its complete delta;
3. preserve the parallel Recording genre-artwork work currently represented by
   `410f104a45890dc86f3a83813552ffe3dd141202`;
4. read this complete shared handoff;
5. read Plugin ADR-0001 and the SB.10 roadmap sections;
6. read the complete SB.10a handshake and SB.10b transport architecture;
7. read all Agent worker, polling, timer, backoff, freshness and shutdown sources;
8. read `RuntimeConfig`, `BackendRuntimeContext`, `DaemonRuntime` and their tests;
9. inspect existing VDR restart, reconnect and local integration test fixtures;
10. define exact state transitions, timestamps, limits, ownership and tests before
    code;
11. keep Runtime integration out of SB.10c unless explicitly justified;
12. preserve all parallel Suite and plugin changes.

### Required SB.10c state model

The lifecycle must represent at least:

```text
not_configured
connecting
plugin_missing
legacy_or_unknown
incompatible
compatible
snapshot_current
snapshot_stale
transport_degraded
overflowed
offline
```

The exact implementation may use a smaller orthogonal state model if every
observable condition above is represented without ambiguity.

### Required behavior

- compose the accepted SB.10b transport rather than widen it;
- perform `CAPS 1` before the initial `SNAP`;
- retain one bounded immutable baseline;
- publish last-attempt, last-success and freshness facts with a defined clock;
- distinguish plugin epoch from backend generation and security identity;
- replace the baseline after epoch change;
- stop deriving diagnostic deltas after overflow;
- classify missing plugin separately from unreachable VDR where useful;
- use bounded reconnect backoff with no unbounded retry loop;
- avoid optimistic capability fallback;
- avoid mutation fallback;
- expose no raw plugin JSON or SVDRP endpoint to public clients;
- stop cleanly without a live socket, worker or callback after shutdown;
- remain read-only.

### SB.10c non-goals

- no plugin command or schema change;
- no plugin version change;
- no native mutation;
- no public API route;
- no durable job or retry store;
- no backend-generation ownership in the plugin;
- no Timer, Recording, EPG, media or OSD surface;
- no arbitrary SVDRP command tunnel;
- no embedded `DaemonRuntime` wiring unless moved into an explicitly reviewed
  SB.10d step.

---

## Planned Plugin Direction After SB.10

The accepted roadmap identifies the following direction, subject to a fresh
precheck for every slice:

- `SB.11`: bounded sequenced native event feed for exact VDR transitions;
- `SB.12`: bounded OSD notification feed for short-lived native messages;
- `SB.13`: read-only replay and AV state;
- `SB.14`: view-only Legacy OSD with epoch, frame sequence and full
  resynchronization;
- `SB.15+`: controller input, typed native actions and mutations only after the
  complete Control Plane and Agent prerequisites exist.

No future capability ID, schema or command name is considered implemented merely
because it appears in the roadmap.

---

## Coordination Workflow

Before every coordinated slice:

1. inspect current remote branch and compare with the last accepted head;
2. read this complete handoff, Plugin ADR-0001 and relevant accepted VDR-Suite
   ADRs;
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

- The bridge branch contains fully live-accepted plugin slices SB.1 through SB.9.
- Plugin ADR-0001 and the plugin roadmap remain authoritative coordination
  documents.
- SB.10a is completed at
  `ba6deddbfba6d50b1152d584654a92f75340dcc3`.
- SB.10b is completed with automated and controlled live acceptance at
  `3396840d41260bb3ed81bc652921b329263d7e58`.
- The direct SB.10b transport supports only `DiscoverSchema1` and `Snapshot`.
- Real VDR reply `550` for a missing plugin is now covered by a regression test
  and maps to `LegacyOrUnknown`.
- The positive SB.10b live handshake returned `status=ready`.
- The positive live test changed neither channel, Timer, Recording nor
  `setup.conf` state.
- The complete rollback removed the staged plugin, configuration and process
  mapping.
- The Suite Bridge plugin is intentionally not installed after rollback.
- `CAPS` and `SNAP` remain the only implemented plugin-specific SVDRP commands.
- Capability discovery schema remains `1`.
- Capability schema remains `1`.
- Snapshot and local-contract schemas remain `2`.
- The plugin remains deliberately read-only.
- `mutations` remains `disabled`.
- Current `main` has later parallel artwork work that was not part of SB.10b;
  synchronize it only after complete comparison during the SB.10c precheck.
- The next active implementation work is SB.10c in the Backend Agent.
- SB.10d remains the later embedded-runtime integration step.
- The first expected plugin-expanding slice after complete SB.10 acceptance is
  SB.11, subject to a new full source and callback audit.
- The authoritative plugin location remains `vdr-plugin-suite-bridge/` on the
  long-lived bridge branch.

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

This standard applies equally to Recording, Timer, EPG, channel, replay, status,
streaming, Legacy OSD and future plugin-service integration.

---

## Back

- [Back to Plugin README](../README.md)
- [Back to Plugin Roadmap](ROADMAP.md)
- [Back to SB.10b Local SVDRP Transport](../../docs/architecture/suite-bridge-svdrp-transport.md)
- [Back to VDR-Suite Documentation Index](../../docs/index.md)
