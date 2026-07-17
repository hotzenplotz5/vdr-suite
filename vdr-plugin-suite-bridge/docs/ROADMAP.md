# VDR Suite Bridge Plugin Roadmap

## Navigation

- [Plugin README](../README.md)
- [Plugin ADR-0001](ADR-0001-plugin-role-and-native-integration-strategy.md)
- [Shared VDR-Suite Handoff](VDR-SUITE-HANDOFF.md)
- [SB.9 Capability Discovery](SB-9-capability-discovery.md)

---

## Purpose

This roadmap defines the safe implementation sequence for
`vdr-plugin-suite-bridge` and its immediate Backend Agent integration.

It applies the durable role and boundary decision from
[ADR-0001](ADR-0001-plugin-role-and-native-integration-strategy.md).

The roadmap is not a promise that every candidate feature will be implemented.
A new plugin surface is created only when actual Agent, Control Plane, frontend
or live-VDR evidence demonstrates a concrete gap that RESTfulAPI or an existing
Suite domain cannot satisfy safely.

The shared handoff remains authoritative for the last fully accepted coordinated
slice and exact implementation, automated-test and live-acceptance heads.

---

## Roadmap Status Vocabulary

| Status | Meaning |
| --- | --- |
| `completed` | Implemented, tested, required live acceptance passed and recorded in the shared handoff. |
| `active` | Current coordinated work with an agreed near-term implementation path. |
| `planned` | Architecturally approved direction, still requiring a slice precheck before implementation. |
| `candidate` | Potentially valuable, but implemented only after a concrete need and prerequisites are proven. |
| `blocked` | Cannot begin until named Control Plane, Agent, capability, identity or safety prerequisites exist. |
| `disabled` | Deliberately unavailable at runtime. |

An accepted architecture decision does not make a runtime feature completed.

---

## Current Baseline

Completed plugin slices:

| Slice | Result |
| --- | --- |
| SB.1 | Plugin foundation and VDR lifecycle integration |
| SB.2 | Static truthful capability catalogue |
| SB.3 | Native read-only status-event observation counters |
| SB.4 | Immutable status snapshots |
| SB.5 | Bounded local-contract payload |
| SB.6 | Read-only SVDRP command contract |
| SB.7 | Lifecycle and callback hardening |
| SB.8 | Counter epoch, saturation, overflow and resynchronization |
| SB.9 | Capability discovery and compatibility negotiation |

Current plugin contract:

```text
plugin version       0.10.0
discovery schema     1
capability schema    1
snapshot schema      2
local contract       2
mutations            disabled
commands             CAPS and SNAP only
```

Current local endpoint set:

```text
PLUG suitebridge CAPS [discovery-schema]
PLUG suitebridge SNAP
```

The current plugin has no menu, listener, outbound connection, worker thread,
database, filesystem mutation or VDR mutation surface.

SB.10a, the transport-neutral Agent handshake contract, is completed.

Implementation head:

```text
d70ebee00edcab1cd019ca9e0c2541a06bf7d587
```

Repository-wide automated-acceptance head:

```text
ba6deddbfba6d50b1152d584654a92f75340dcc3
```

SB.10a is not a new plugin capability and does not change the plugin version,
plugin commands, capability catalogue or local schemas. No live VDR test was
required because SB.10a opens no concrete transport and invokes no VDR command.

---

## Strategic Sequence

```text
SB.10  consume the existing read-only plugin boundary in the Backend Agent
  ->
SB.11  introduce a bounded sequenced native event feed
  ->
SB.12  preserve short-lived OSD notifications
  ->
SB.13  add replay and AV state observations
  ->
SB.14  add view-only Legacy OSD frames and resynchronization
  ->
SB.15+ consider control and mutations only after external prerequisites exist
```

The sequence deliberately prioritizes:

1. transport safety;
2. read-only compatibility;
3. continuity and resynchronization;
4. immediate native observations;
5. view-only OSD;
6. controller and mutation safety;
7. optional fidelity and specialized operations.

---

## Roadmap Overview

| Slice | Status | Primary owner | Plugin change expected |
| --- | --- | --- | --- |
| SB.10a Transport-neutral handshake contract | completed | Backend Agent | no |
| SB.10b Local typed SVDRP transport | active | Backend Agent | no |
| SB.10c Polling, reconnect and freshness | active | Backend Agent | no |
| SB.10d Embedded-Agent integration and live acceptance | active | Backend Agent / Suite runtime | no, unless a proven compatibility gap exists |
| SB.11 Sequenced native event feed | planned | Plugin + Agent | yes |
| SB.12 OSD notification feed | planned | Plugin + Agent | yes |
| SB.13 Replay and AV state | planned | Plugin + Agent | likely |
| SB.14 View-only Legacy OSD | planned | Plugin + Agent + Control Plane | yes |
| SB.15 Controller lease and allowlisted OSD input | candidate / blocked | Control Plane + Agent + Plugin | yes, after prerequisites |
| SB.16 Typed native actions | candidate | Agent + Plugin | only for proven gaps |
| SB.17 Safe Timer mutations | candidate / blocked | Control Plane + Agent + Plugin | yes, after mutation foundation |
| SB.18 Safe Recording actions | candidate / blocked | Control Plane + Agent + Plugin | yes, after mutation foundation |
| SB.19 Optional OSD fidelity | candidate | Plugin + Agent | optional |
| SB.20 Native streaming and tuner supplements | candidate | Agent + Plugin | optional |

---

# SB.10 — Backend Agent Consumption of Existing Plugin Contracts

## Goal

Consume the already live-accepted `CAPS 1` and `SNAP` plugin boundary safely
before adding another plugin command.

Plugin version remains `0.10.0` throughout SB.10 unless implementation proves a
specific bounded plugin compatibility gap.

## SB.10a — Transport-Neutral Handshake Contract

Status: `completed`

Primary owner: Backend Agent.

Implementation head:

```text
d70ebee00edcab1cd019ca9e0c2541a06bf7d587
```

Repository-wide automated-acceptance head:

```text
ba6deddbfba6d50b1152d584654a92f75340dcc3
```

Acceptance classification:

- all targeted SB.10a tests passed;
- strict root Make/test inventory passed;
- independent plugin regression passed;
- complete documentation checks passed;
- global architecture check passed after synchronization with ADR-0050;
- the accepted branch contained current Suite `main` head
  `8ba96dbb46019030f7cb3ebcb95929034b6166d3`;
- no live VDR test was required for this transport-neutral contract;
- plugin runtime, version, commands, capabilities and schemas remain unchanged.

Implemented responsibilities:

- typed local transport interface;
- `CAPS 1` before `SNAP`;
- bounded payload parsing;
- independent validation of discovery, capability, snapshot and local-contract
  schemas;
- safe legacy or unknown-plugin result;
- explicit malformed-payload and reply-rejection categories;
- hard mutation disablement;
- baseline creation from `SNAP`;
- epoch replacement;
- overflow handling;
- no network, process, database, filesystem, thread or plugin coupling.

Non-goals:

- no real SVDRP connection;
- no daemon wiring;
- no public API;
- no plugin modification;
- no mutation.

## SB.10b — Local Typed SVDRP Transport

Status: `active`

Primary owner: Backend Agent.

Goal:

Implement one dedicated local Suite Bridge transport that can execute only the
typed commands admitted by the SB.10a interface.

Allowed logical operations:

```text
DiscoverSchema1
Snapshot
```

Expected transport behavior:

- connect to the configured local VDR SVDRP endpoint;
- read and validate the server greeting;
- issue only fixed allowlisted Suite Bridge requests;
- parse single-line and multiline SVDRP replies deterministically;
- preserve reply code separately from payload;
- normalize supported line endings;
- enforce connection, read and total-operation deadlines;
- enforce reply and payload byte limits;
- handle connection refusal, timeout, short read, malformed reply and disconnect;
- close the connection deterministically;
- expose no arbitrary command-text method;
- expose no shell, `system()`, `popen()`, fork or subprocess surface;
- log only bounded transport facts.

Preferred implementation:

- direct small SVDRP client owned by the Agent transport layer.

A temporary command-line wrapper is not the preferred permanent architecture.
The existing mutation-specific channel-move executor must not be reused as the
generic Suite Bridge transport.

Required tests:

- exact successful `CAPS 1` reply;
- exact successful `SNAP` reply;
- multiline reply termination;
- CRLF and supported line-ending normalization;
- unexpected greeting;
- reply code rejection;
- connection refusal;
- connect timeout;
- read timeout;
- connection closed mid-reply;
- oversized reply;
- malformed reply line;
- no free command text;
- no process or shell invocation;
- deterministic resource cleanup.

Plugin changes expected: none.

## SB.10c — Read-Only Polling, Reconnect and Freshness

Status: `active`

Primary owner: Backend Agent.

Goal:

Turn one successful local handshake into a bounded read-only observation
lifecycle.

Required states include:

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

Required behavior:

- perform capability discovery on initial connection;
- fetch the initial `SNAP` only after compatibility succeeds;
- retain one bounded baseline;
- distinguish plugin epoch from backend generation;
- replace the baseline after epoch change;
- stop deriving diagnostic deltas after overflow;
- publish last-success and freshness timestamps;
- use bounded reconnect backoff;
- avoid optimistic feature fallback;
- avoid mutation fallback;
- expose health without leaking VDR credentials or raw payloads;
- stop cleanly without leaving a connection or worker alive.

Plugin changes expected: none.

## SB.10d — Embedded-Agent Runtime Integration

Status: `active`

Primary owner: Backend Agent and Suite runtime.

Goal:

Integrate the local handshake and observation lifecycle behind the logical Agent
boundary without replacing the existing RESTfulAPI domain adapter.

Expected architecture:

```text
DaemonRuntime
  -> embedded Backend Agent boundary
      -> Suite Bridge local transport
      -> handshake and compatibility service
      -> read-only observation lifecycle

DaemonRuntime
  -> RESTfulAPI adapter
      -> channels, EPG, Timers, Recordings and existing domain reads
```

Required work:

- runtime configuration for local SVDRP host, port and deadlines;
- explicit enabled and disabled state;
- deterministic start and stop ordering;
- backend-scoped Agent health;
- parallel operation with RESTfulAPI;
- no public exposure of plugin-local JSON;
- controlled live VDR handshake;
- proof that plugin and VDR state remain unchanged;
- reconnect and VDR-restart acceptance;
- rollback to the previous daemon and plugin state.

Completion gate:

SB.10 is completed only when automated tests, final Suite build, controlled live
VDR acceptance, shutdown, rollback and shared handoff update pass.

---

# SB.11 — Sequenced Native Event Feed

Status: `planned`

Primary owners: Plugin and Backend Agent.

## Goal

Replace diagnostic-only counter comparisons with a bounded optional stream of
specific native observations while retaining `SNAP` as the complete
resynchronization point.

The existing diagnostic counters remain valid but are not reinterpreted as event
sequences.

## Initial event families

- channel switched;
- Recording started;
- Recording stopped;
- replay started;
- replay stopped;
- Timer changed.

The exact initial set is selected after a source and live-callback audit.

## Required contract

- independent event schema;
- immutable event epoch;
- monotonic sequence within the epoch;
- fixed-capacity ring or equivalent bounded storage;
- maximum event count;
- maximum payload size;
- bounded string lengths;
- explicit first and last retained sequence;
- explicit overflow or sequence-gap result;
- complete baseline or resync command;
- no durable event history;
- no callback network, file, database or logging work;
- no raw pointers or retained VDR objects.

## Agent behavior

- request events after the last accepted sequence;
- validate epoch and schema;
- detect a sequence gap;
- discard stale-epoch events;
- trigger complete resynchronization after gap, overflow, reconnect or unknown
  continuity;
- use events as native facts or targeted refresh hints;
- publish bounded freshness and loss diagnostics.

## Non-goals

- no audit history;
- no guaranteed one-event-per-user-action semantics;
- no mutation;
- no public direct event endpoint from the plugin;
- no unbounded replay.

## Acceptance focus

- callback latency and bounded work;
- deterministic ordering;
- overflow behavior;
- VDR restart and epoch replacement;
- Agent reconnect;
- high-rate event pressure;
- no weakened SB.9 behavior;
- complete rollback.

---

# SB.12 — OSD Notification Feed

Status: `planned`

Primary owners: Plugin and Backend Agent.

## Goal

Preserve short-lived native OSD notifications that may appear and disappear
between RESTfulAPI or frontend polls.

## Candidate observations

- OSD opened;
- OSD closed;
- status message changed;
- status message cleared;
- OSD title changed;
- help-key labels changed;
- selected menu item changed;
- channel display changed;
- present/following display changed.

The first implementation should start with the smallest useful subset, normally
OSD lifecycle and bounded status-message changes.

## Required fields

A versioned notification value may contain:

- OSD epoch;
- event sequence;
- capture time;
- observation kind;
- active or cleared state;
- bounded UTF-8 text;
- truncation flag;
- optional privacy or suppression category.

## Safety requirements

- no normal log contains complete OSD text;
- no durable plugin storage;
- bounded recent retention only;
- no arbitrary semantic classification of unknown plugin text;
- no capture of secret input values;
- explicit unavailable or suppressed state;
- complete resynchronization after loss;
- view-only capability only.

## Expected user-visible value

- immediate frontend toast or status update;
- reliable display of short VDR warnings;
- prompt Recording-start or failure notification;
- visibility of local VDR or plugin maintenance messages;
- faster diagnosis without exposing a native plugin port.

---

# SB.13 — Replay and AV State

Status: `planned`

Primary owners: Plugin and Backend Agent.

## Goal

Expose selected VDR-native state that is currently available only as a broad
snapshot, relative remote key behavior or incomplete status tracking.

## Candidate read-only observations

- replay started and stopped;
- replay source identity within safe bounded limits;
- play, pause and trick-speed state when authoritatively available;
- current position and total duration when safely available;
- volume;
- mute state;
- selected audio track;
- selected audio channel;
- selected subtitle track;
- subtitles enabled or disabled.

## Source audit requirement

Before implementation, inspect the exact supported VDR-version interfaces and
callback semantics. Do not infer state from a sent key.

## Preferred contract

- native observation first;
- explicit unknown or unavailable values;
- event plus current-state snapshot;
- stable bounded track identity;
- no raw VDR pointer or localized display string as the only identity;
- targeted RESTfulAPI refresh where it remains authoritative.

## Non-goals

- no media-byte transport;
- no public player session ownership;
- no assumption that key dispatch proves resulting AV state;
- no mutation until a later typed-action slice.

---

# SB.14 — View-Only Legacy OSD

Status: `planned`

Primary owners: Plugin, Backend Agent and Control Plane.

Related platform phase: Legacy OSD compatibility implementation.

## Goal

Expose one current native OSD surface as a bounded, sequenced, view-only
compatibility surface.

## Required concepts

- `OsdSurfaceRef`;
- `osdEpoch`;
- complete immutable `OsdFrame`;
- `frameSequence`;
- optional ordered OSD event sequence;
- content fingerprint;
- bounded rendering schema;
- full-frame resynchronization;
- inactive, unavailable, degraded and suppressed states.

## Implementation stages

1. source audit of VDR OSD access and available safe adapter techniques;
2. plugin-local immutable frame contract;
3. callback dirty markers and bounded capture outside callbacks;
4. frame epoch and sequence;
5. Agent-local buffering and resynchronization;
6. authenticated Agent transport;
7. Control Plane `osd.view` authorization;
8. multiple bounded viewer bindings;
9. Web or native-client legacy surface;
10. controlled local and multi-site acceptance.

## Initial limits

- control disabled;
- no input command;
- full frames before deltas;
- no permanent frame history;
- no public plugin or RESTfulAPI OSD endpoint;
- no claim of pixel-perfect support for every skin;
- truthful degraded capability reporting.

## Completion gate

View-only OSD becomes available only after sequence loss, OSD recreation, VDR
restart, Agent reconnect, privacy suppression and complete rollback have been
proven.

---

# SB.15 — Controller Lease and Allowlisted OSD Input

Status: `candidate / blocked`

Primary owners: Control Plane, Backend Agent and Plugin.

Blocked by:

- implemented view-only OSD continuity;
- authenticated client and Agent paths;
- `osd.control` permission;
- server-enforced read-only backend policy;
- authoritative controller lease;
- controller-lease epoch;
- backend generation;
- OSD epoch;
- input command identity and deadline;
- rate limits and audit correlation.

## Goal

Allow exactly one authorized VDR-Suite controller to send a small normalized
input vocabulary to one current native OSD surface.

## Rules

- view and control remain separate;
- only one active Suite controller lease per surface;
- stale generation, OSD epoch or lease epoch is rejected;
- read-only backends reject all input;
- unsupported actions are rejected;
- command and repeat rates are bounded;
- expired commands are discarded;
- commands are never queued through Agent downtime;
- local physical remote input remains authoritative native activity;
- key dispatch is not domain-mutation success;
- no arbitrary keyboard text, raw keycode, SVDRP or plugin-service tunnel.

## Initial vocabulary

The first accepted set should be smaller than the complete VDR remote key set.
It may include only basic navigation, confirmation, back and the four color
keys. Playback, volume, numeric or menu shortcuts are added only through explicit
capabilities.

---

# SB.16 — Typed Native Actions

Status: `candidate`

Primary owners: Backend Agent and Plugin.

## Goal

Add narrowly typed operations only where an actual RESTfulAPI or Suite-domain gap
has been demonstrated.

Candidate operations:

- channel switch with authoritative result readback;
- absolute volume set;
- explicit mute set;
- audio-track selection;
- subtitle-track selection;
- one named plugin maintenance operation;
- one named native health or resynchronization operation.

Each operation is a separate capability and schema. There is no generic action
container.

Required proof:

- exact need and source gap;
- typed request and result;
- bounded values;
- generation and revision fences where applicable;
- deterministic native rejection;
- authoritative readback;
- no blind retry after unknown outcome;
- disposable live test;
- rollback.

---

# SB.17 — Safe Timer Mutations

Status: `candidate / blocked`

Primary owners: Control Plane, Backend Agent and Plugin.

Blocked by the complete safe-mutation, operation, job, generation, revision,
idempotency and reconciliation foundation.

Candidate operations:

- create Timer;
- update Timer;
- enable or disable Timer;
- delete Timer.

Required semantics:

- stable Suite operation identity;
- native target identity;
- expected resource revision or comparison facts;
- capability and authorization gates;
- backend generation fence;
- deterministic conflict result;
- bounded native execution;
- authoritative Timer readback;
- duplicate-delivery handling;
- lost-response reconciliation;
- no plugin-owned durable retry scheduler.

Timer mutations should precede Recording mutations because their live-test and
rollback surface is easier to bound with disposable resources.

---

# SB.18 — Safe Recording Actions

Status: `candidate / blocked`

Primary owners: Control Plane, Backend Agent and Plugin.

Candidate operations:

- rename Recording;
- move Recording;
- move to trash;
- restore from trash;
- permanently delete through a separate highly privileged operation.

Required safeguards:

- disposable test Recording;
- no operation on a user's real Recording during automated acceptance;
- backend generation and resource revision;
- authoritative Recording-list and filesystem reconciliation;
- unknown outcome handling;
- long-running work remains a Suite job, not a plugin callback;
- partial-move and restart recovery strategy;
- destructive action tested last;
- explicit rollback or compensation where possible.

Existing RESTfulAPI and Rectools capabilities remain valid candidate executors
behind the Agent. A plugin implementation is justified only if native VDR
correctness requires it.

---

# SB.19 — Optional OSD Fidelity Improvements

Status: `candidate`

Primary owners: Plugin and Backend Agent.

Possible additions:

- frame deltas;
- palette information;
- alpha channel;
- multiple OSD areas;
- logical geometry;
- cursor metadata;
- local-activity hint;
- privacy suppression hint;
- negotiated compression;
- client rendering capability negotiation.

Rules:

- full-frame resynchronization remains mandatory;
- deltas require exact base frame and OSD epoch;
- quality improvements never weaken bounded memory or callback safety;
- clients display degraded fidelity truthfully;
- no feature is added solely to imitate one skin without a portable contract.

---

# SB.20 — Native Streaming and Tuner Supplements

Status: `candidate`

Primary owners: Backend Agent and Plugin.

The plugin does not become the Streaming Gateway.

Potential native supplements include:

- tuner allocation observation;
- receiver or transfer-mode transition;
- CI or decryption-state observation;
- local stream-health fact;
- exact native start or stop confirmation for a locally brokered media session;
- bounded frontend signal observations where RESTfulAPI is unavailable or
  insufficient.

The actual video or Recording byte path remains owned by the media and Streaming
Gateway architecture.

A permanent internal stream URL is never returned directly to a public client.

---

## Cross-Cutting Ownership Rules

### Control Plane owns

- public API;
- users, roles and permissions;
- backend-scoped policy;
- multi-site routing;
- durable operations and jobs;
- idempotency;
- retries and sagas;
- public session lifecycle;
- durable audit and security history;
- final user-visible status.

### Backend Agent owns

- authenticated machine relationship;
- backend identity and generation participation;
- local transport selection;
- compatibility negotiation;
- capability freshness;
- reconnect and health;
- bounded event and frame buffering;
- local OSD broker;
- enforcement of generation, lease and deadline before dispatch;
- protection of local VDR credentials and endpoints.

### Suite Bridge plugin owns

- VDR-process-local lifecycle;
- safe native locks and thread boundaries;
- immutable bounded native observations;
- native event, snapshot and OSD continuity where implemented;
- truthful local capabilities;
- typed bounded native execution where implemented;
- native readback and deterministic local result categories;
- no leaked raw VDR objects.

---

## Version and Capability Policy

A new roadmap slice does not automatically require a plugin version change.

Plugin version changes when the plugin-local implementation or contract changes.
Agent-only work does not change plugin version.

New capability IDs are preferred over widening an existing capability silently.

Examples of possible future capability families:

```text
native-events
osd-notifications
replay-state
av-state
osd-snapshot
osd-input
channel-switch-readback
timer-create
timer-update
timer-delete
recording-rename
recording-move
recording-trash
```

These names are illustrative until their individual slices define and accept the
exact catalogue.

Capability states remain explicit and may include only values defined by the
relevant schema. Source presence or plugin version is never sufficient evidence
of availability.

---

## Slice Precheck Required Before Implementation

Before every slice:

1. inspect the current remote branch and compare it with the last accepted head;
2. read the complete shared handoff;
3. read ADR-0001 and the relevant VDR-Suite ADRs;
4. inspect all actual source, documentation, Make and test files that will be
   affected;
5. identify parallel work and preserve it;
6. state primary owner and all participating boundaries;
7. state exact files expected to change;
8. state plugin version, schema and capability impact;
9. state dependencies and non-goals;
10. define automated tests before code;
11. define live acceptance and rollback before code;
12. keep mutations disabled unless every prerequisite is implemented.

No implementation begins from a roadmap title alone.

---

## Definition of Done for a Plugin Slice

A plugin slice is completed only when:

- ownership is unambiguous;
- exact source and contract scope is documented;
- callback and lock behavior is bounded and correct;
- all payload sizes are bounded;
- lifecycle and shutdown are deterministic;
- capabilities and schemas are truthful;
- positive, negative, malformed and overflow cases are tested;
- all existing plugin tests still pass;
- final VDR shared-object builds and passes ELF validation;
- API-versioned staged installation passes where plugin runtime changes;
- required live VDR behavior is observed where the slice reaches VDR runtime;
- unrelated channel, Timer, Recording and setup state remains unchanged unless a
  disposable mutation is explicitly under test;
- restart, stop and rollback are proven where required;
- no stale plugin binary or configuration remains;
- repository Make inventory, documentation and architecture checks pass;
- the shared handoff is updated only after acceptance.

---

## Immediate Next Work

The next implementation slice is:

```text
SB.10b - Local typed SVDRP transport
```

Before implementation, the Agent-side precheck must read and compare:

- the SB.10a transport-neutral contracts;
- existing HTTP and socket abstractions;
- existing test-server and transport tests;
- the mutation-specific channel-move SVDRP executor;
- VDR SVDRP framing and multiline reply behavior;
- RuntimeConfig and BackendRuntimeContext;
- Make source and test ownership;
- shutdown and live-acceptance paths.

Expected plugin changes: none.

The first plugin-expanding slice after complete SB.10 acceptance is expected to
be SB.11, but it begins only after a new precheck proves the exact native event
contract and VDR callback data that can be copied safely.

---

## Non-Goals

This roadmap does not:

- replace the shared handoff;
- mark SB.10 completed before acceptance;
- enable any mutation;
- change plugin version or schema by documentation alone;
- guarantee implementation of candidate slices;
- require the plugin to duplicate RESTfulAPI;
- make Legacy OSD the primary product UI;
- expose raw SVDRP, shell, plugin-service or keycode tunnels;
- make the plugin a public listener, database, workflow engine, audit store or
  Streaming Gateway.

---

## Related Documents

- [ADR-0001: Plugin Role and Native Integration Strategy](ADR-0001-plugin-role-and-native-integration-strategy.md)
- [Shared VDR-Suite Handoff](VDR-SUITE-HANDOFF.md)
- [SB.3 Native VDR Status Events](SB-3-status-events.md)
- [SB.6 Read-Only SVDRP](SB-6-read-only-svdrp.md)
- [SB.8 Counter Continuity](SB-8-counter-continuity.md)
- [SB.9 Capability Discovery](SB-9-capability-discovery.md)
- [VDR-Suite ADR-0039: Backend Agent and Control Plane Boundary](../../docs/adr/ADR-0039-backend-agent-control-plane-boundary.md)
- [VDR-Suite ADR-0042: Safe Mutation, Revision and Idempotency](../../docs/adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [VDR-Suite ADR-0047: Legacy OSD Compatibility Bridge](../../docs/adr/ADR-0047-legacy-osd-compatibility-bridge.md)
- [VDR-Suite ADR-0050: Domain Repository SQLite Boundary](../../docs/adr/ADR-0050-domain-repository-sqlite-boundary.md)

---

## Back

- [Back to Plugin README](../README.md)
- [Back to Plugin ADR-0001](ADR-0001-plugin-role-and-native-integration-strategy.md)
- [Back to Shared Handoff](VDR-SUITE-HANDOFF.md)
