# Suite Bridge Backend Agent Handshake

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Architecture Index](index.md)
- [Target Platform Architecture](target-platform-architecture.md)
- [Backend Agent and Control Plane Boundary](../adr/ADR-0039-backend-agent-control-plane-boundary.md)
- [Suite Bridge Local SVDRP Transport](suite-bridge-svdrp-transport.md)
- [Suite Bridge Handoff](../../vdr-plugin-suite-bridge/docs/VDR-SUITE-HANDOFF.md)

---

## Status

Implementation slice: `SB.10a`

State: completed and repository-wide accepted at
`ba6deddbfba6d50b1152d584654a92f75340dcc3`.

The concrete SB.10b local SVDRP transport is implemented in isolation. Its
automated and live VDR acceptance remain pending and are documented separately
in [Suite Bridge Local SVDRP Transport](suite-bridge-svdrp-transport.md).

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

SB.10a files:

```text
core/agent/include/ISuiteBridgeLocalTransport.h
core/agent/include/SuiteBridgeHandshake.h
core/agent/include/SuiteBridgeLocalContractParser.h
core/agent/include/SuiteBridgeHandshakeService.h
core/agent/src/SuiteBridgeHandshake.cpp
core/agent/src/SuiteBridgeLocalContractParser.cpp
core/agent/src/SuiteBridgeHandshakeService.cpp
```

The SB.10a implementation has no:

- process execution;
- shell command construction;
- socket or HTTP client;
- thread or mutex;
- filesystem access;
- database access;
- VDR adapter dependency;
- daemon runtime wiring;
- plugin implementation change.

The later concrete transport remains a separate implementation behind
`ISuiteBridgeLocalTransport` and does not weaken this transport-neutral source
boundary.

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

SB.10b uses local SVDRP internally while preserving this typed boundary. It does
not expose arbitrary command strings.

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

These remain SB.10a non-goals even though SB.10b now provides a separate direct
socket implementation behind the interface.

---

## Tests

The SB.10a automated contract covers:

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

SB.10b has separate transport boundary, loopback fixture and manual live VDR
tests. See [Suite Bridge Local SVDRP Transport](suite-bridge-svdrp-transport.md).

---

## Current Follow-Up Slice

`SB.10b` implements one bounded local transport for the two typed commands.

The implementation:

- accepts no arbitrary command string;
- uses a direct Agent-owned socket rather than `svdrpsend`;
- distinguishes unavailable, timeout, connection, framing and reply results;
- applies strict response-size and line-count limits;
- remains local to the Backend Agent boundary;
- exposes no VDR-internal port to the Control Plane or public network;
- owns no daemon integration or polling lifecycle.

SB.10b becomes completed only after its automated tests and controlled live VDR
smoke test pass and the shared handoff is updated.

Daemon integration remains a later slice after the transport itself is accepted.

---

## Back

- [Back to Suite Bridge Local SVDRP Transport](suite-bridge-svdrp-transport.md)
- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
