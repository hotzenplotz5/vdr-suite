# Suite Bridge Backend Agent Handshake

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Architecture Index](index.md)
- [Target Platform Architecture](target-platform-architecture.md)
- [Backend Agent and Control Plane Boundary](../adr/ADR-0039-backend-agent-control-plane-boundary.md)
- [Suite Bridge Handoff](../../vdr-plugin-suite-bridge/docs/VDR-SUITE-HANDOFF.md)

---

## Status

Implementation slice: `SB.10a`

State: transport-neutral Agent contract implemented; local SVDRP transport and daemon integration are not part of this slice.

The plugin boundary consumed here was live accepted in SB.9:

```text
PLUG suitebridge CAPS 1
PLUG suitebridge SNAP
```

---

## Purpose

SB.10a introduces the first concrete Backend Agent code boundary without prematurely implementing the Phase 63 remote Agent runtime.

It consumes the already accepted local plugin contracts and produces one fail-closed handshake result owned by the Agent layer.

The logical path is:

```text
SuiteBridgeHandshakeService
  -> ISuiteBridgeLocalTransport
  -> CAPS 1
  -> compatibility evaluation
  -> SNAP
  -> immutable local baseline
```

The transport is typed. Callers cannot submit arbitrary SVDRP command text through this interface.

---

## Ownership

### Backend Agent owns

- invocation order;
- local transport error classification;
- discovery and snapshot payload validation;
- compatibility decisions;
- safe degradation for legacy or unknown plugins;
- initial counter baseline adoption;
- epoch and overflow comparison rules;
- hard disabling of mutation execution in this slice.

### Plugin owns

- truthful local discovery values;
- local schema values;
- local capability states;
- immutable snapshot payload production;
- process-local counter epoch and overflow facts.

### Control Plane owns

- backend identity and generation;
- user and service authorization;
- public capability presentation;
- durable health, lease and lifecycle records;
- all mutation policy and durable execution state.

Plugin capability is not user authorization and `counter_epoch` is not `backendGeneration`.

---

## Source Boundary

Implemented files:

```text
core/agent/include/ISuiteBridgeLocalTransport.h
core/agent/include/SuiteBridgeHandshake.h
core/agent/include/SuiteBridgeLocalContractParser.h
core/agent/include/SuiteBridgeHandshakeService.h
core/agent/src/SuiteBridgeHandshake.cpp
core/agent/src/SuiteBridgeLocalContractParser.cpp
core/agent/src/SuiteBridgeHandshakeService.cpp
```

The implementation has no:

- process execution;
- shell command construction;
- socket or HTTP client;
- thread or mutex;
- filesystem access;
- database access;
- VDR adapter dependency;
- daemon runtime wiring;
- plugin implementation change.

---

## Typed Transport

The local transport accepts only:

```text
DiscoverSchema1
Snapshot
```

It returns separate fields for:

- transport status;
- local reply code;
- payload;
- bounded diagnostic text.

The later SB.10b transport may use local SVDRP internally, but it must implement this typed boundary rather than expose arbitrary command strings.

---

## Discovery Compatibility

A compatible discovery requires:

```text
discovery_schema      = 1
plugin_name           = suitebridge
capability_schema     = 1
snapshot_schema       = 2
local_contract_schema = 2
snapshots             = available
local-contract        = available
```

The parser does not require JSON field order. Unknown top-level fields and unknown additive capability IDs are accepted within bounded size and nesting limits.

Known capabilities are interpreted by stable ID. Unknown or missing capability states are unavailable.

`mutations` remains Agent-disabled in every SB.10a result, including when:

- the capability is absent;
- the state is unknown;
- the state is disabled;
- a future plugin reports the state as available.

No `SNAP` request is issued until discovery has passed all compatibility and required-capability checks.

---

## Snapshot Validation

A valid initial baseline requires:

- reply code `900`;
- local-contract schema matching discovery;
- capability schema matching discovery;
- snapshot schema matching discovery;
- `active=true`;
- all four counters and `total` represented as unsigned integers;
- `total` equal to the saturating sum of the counters;
- one 32-character lowercase hexadecimal `counter_epoch`;
- boolean `counter_overflow`.

The parser accepts unknown additive snapshot fields and order-independent JSON members.

Payloads larger than `4096` bytes, capability arrays larger than `64` entries and nesting deeper than the bounded parser limit are rejected.

---

## Baseline Rules

The Agent baseline tracker distinguishes:

```text
AdoptedInitial
UpdatedComparable
ReplacedEpochChanged
ReplacedOverflowed
```

A diagnostic delta is available only when:

- both snapshots are active;
- both snapshots have `counter_overflow=false`;
- both snapshots have the same non-empty epoch.

A changed epoch replaces the old baseline. Overflow disables delta calculation. Neither condition changes Control Plane backend generation.

---

## Result Vocabulary

The handshake has explicit result states for:

- ready;
- legacy or unknown plugin;
- discovery transport failure;
- discovery reply rejection;
- discovery payload size or validation failure;
- unexpected plugin identity;
- each incompatible schema axis;
- required capability unavailable;
- snapshot transport failure;
- snapshot reply rejection;
- snapshot payload size or validation failure;
- inactive snapshot.

Transport failure, reply rejection, payload failure and schema incompatibility are never collapsed into one generic error.

---

## Non-Goals

SB.10a adds no:

- real `svdrpsend` execution;
- direct SVDRP socket client;
- RESTfulAPI replacement;
- daemon integration;
- Backend Agent enrollment or authentication;
- backend generation, heartbeat or lease runtime;
- Control Plane protocol;
- public API endpoint;
- capability publication;
- Timer, Recording, EPG, media or OSD surface;
- mutation command or write policy.

---

## Tests

The automated contract covers:

- exact SB.9 discovery and snapshot payloads;
- order-independent object fields;
- unknown additive fields and capability IDs;
- absent and future mutation capability states;
- legacy plugin and local transport failures;
- separate schema incompatibility states;
- malformed, incomplete and oversized payloads;
- required capability degradation;
- proof that incompatible discovery never invokes `SNAP`;
- snapshot transport, reply, schema, activity, total and epoch failures;
- initial, comparable, changed-epoch and overflow baseline updates;
- source guards against transport, daemon, database and plugin coupling.

---

## Next Slice

`SB.10b` may implement one bounded local transport for the two typed commands.

The preferred direction is a small local SVDRP client or an equivalently strict process adapter that:

- does not accept arbitrary command strings;
- distinguishes process, timeout, connection, reply and payload failures;
- applies strict response-size limits;
- remains local to the Backend Agent boundary;
- exposes no VDR-internal port to the Control Plane or public network.

Daemon integration remains a later slice after the transport itself has isolated tests and controlled live VDR acceptance.

---

## Back

- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
