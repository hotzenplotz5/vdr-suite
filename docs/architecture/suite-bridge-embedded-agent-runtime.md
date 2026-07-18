# Suite Bridge Embedded Agent Runtime

## Navigation

- [Architecture Index](index.md)
- [SB.10a Agent Handshake](suite-bridge-agent-handshake.md)
- [SB.10b Local SVDRP Transport](suite-bridge-svdrp-transport.md)
- [SB.10c Observation Lifecycle](suite-bridge-observation-lifecycle.md)
- [Plugin Roadmap](../../vdr-plugin-suite-bridge/docs/ROADMAP.md)
- [Shared Handoff](../../vdr-plugin-suite-bridge/docs/VDR-SUITE-HANDOFF.md)

---

## Status

SB.10d implementation contract.

Status: `active`

Primary owners: Backend Agent and Suite runtime.

The runtime implementation and repository-owned controlled-live-acceptance tooling
have passed automated validation. SB.10d remains active until the controlled live
VDR restart, reconnect, shutdown and rollback run has passed on the VDR host.

---

## Purpose

SB.10d integrates the accepted SB.10c read-only observation lifecycle into the
running VDR-Suite daemon without replacing the existing RESTfulAPI domain adapter.

The daemon remains composed as two parallel local read paths:

```text
DaemonRuntime
  -> BackendRuntimeContext
      -> RESTfulAPI adapter
          -> channels, EPG, Timers, Recordings and existing domain reads

DaemonRuntime
  -> BackendRuntimeContext
      -> SuiteBridgeEmbeddedAgentRuntime
          -> SuiteBridgeSvdrpTransport
          -> SuiteBridgeObservationWorker
          -> CAPS 1, then SNAP polling
```

RESTfulAPI remains authoritative for broad structured VDR domain reads. Suite
Bridge supplies only bounded native compatibility, observation continuity and
health facts.

---

## Runtime Configuration

Suite Bridge integration is disabled by default and must be enabled explicitly.

The daemon reads these environment variables:

| Variable | Default | Meaning |
| --- | --- | --- |
| `VDR_SUITE_SUITE_BRIDGE_ENABLED` | `false` | Explicit enable switch |
| `VDR_SUITE_SUITE_BRIDGE_BACKEND_ID` | `default` | Backend that owns the local bridge runtime |
| `VDR_SUITE_SUITE_BRIDGE_HOST` | `127.0.0.1` | Local or numeric SVDRP host |
| `VDR_SUITE_SUITE_BRIDGE_PORT` | `6419` | SVDRP port |
| `VDR_SUITE_SUITE_BRIDGE_CONNECT_TIMEOUT_MS` | `1000` | Bounded connect phase |
| `VDR_SUITE_SUITE_BRIDGE_IO_TIMEOUT_MS` | `1000` | Bounded greeting, send and reply phase |
| `VDR_SUITE_SUITE_BRIDGE_OPERATION_TIMEOUT_MS` | `3000` | Total typed invocation deadline |
| `VDR_SUITE_SUITE_BRIDGE_POLL_INTERVAL_MS` | `5000` | Trusted snapshot polling interval |
| `VDR_SUITE_SUITE_BRIDGE_STALE_AFTER_MS` | `15000` | Snapshot stale threshold |
| `VDR_SUITE_SUITE_BRIDGE_OFFLINE_AFTER_MS` | `60000` | Offline threshold |
| `VDR_SUITE_SUITE_BRIDGE_RECONNECT_INITIAL_MS` | `1000` | First reconnect delay |
| `VDR_SUITE_SUITE_BRIDGE_RECONNECT_MAXIMUM_MS` | `30000` | Maximum reconnect delay |

Invalid values fail closed to documented defaults. Empty values do not enable the
runtime. Timing relationships remain valid: stale is not shorter than polling,
offline is not shorter than stale, and maximum reconnect is not shorter than the
initial reconnect delay.

No VDR password, unrestricted command text or public endpoint is introduced.

---

## Runtime Ownership

`BackendRuntimeContext` owns one optional `SuiteBridgeEmbeddedAgentRuntime` for
its exact backend ID.

The runtime owns:

- the accepted direct local `SuiteBridgeSvdrpTransport`;
- the accepted `SuiteBridgeObservationWorker`;
- start and stop of that worker;
- one bounded backend-scoped health value.

The runtime does not own:

- users, roles or authorization;
- public API routing;
- durable jobs or retries;
- backend-generation authority;
- RESTfulAPI domain state;
- plugin lifecycle or VDR locks;
- mutations.

---

## Construction and Start Ordering

Construction happens while each backend runtime context is assembled.

For the configured backend only:

1. validate and copy the bounded configuration;
2. construct the typed SVDRP transport;
3. construct the SB.10c observation worker;
4. retain both behind the embedded Agent runtime;
5. complete all remaining daemon construction;
6. start the embedded Agent worker only after construction has succeeded.

A disabled configuration constructs no transport and no worker. Starting a
disabled runtime is a no-op.

RESTfulAPI adapter and event-stream construction remain independent. Failure or
absence of Suite Bridge does not remove broad RESTfulAPI reads.

---

## Shutdown Ordering

Shutdown is deterministic and idempotent:

1. request daemon shutdown;
2. stop daemon-owned warmup workers;
3. stop the RESTfulAPI event-stream client;
4. stop and join the Suite Bridge observation worker;
5. release HTTP and API runtime objects;
6. destroy backend runtime contexts and their transports.

The embedded Agent runtime destructor calls `stop()` as a final safety net. No
worker remains joinable after shutdown.

---

## Health Boundary

The embedded Agent runtime exposes one bounded in-process health value containing:

- backend ID;
- whether the runtime is configured;
- whether the worker is running;
- the accepted SB.10c observation snapshot.

The value does not expose:

- raw CAPS or SNAP JSON;
- SVDRP endpoint credentials;
- arbitrary diagnostic payloads;
- public authorization state;
- mutable plugin state.

No public API route is added by SB.10d. A later explicitly reviewed Control Plane
health projection may consume this value without exposing plugin-local transport
or payload details.

---

## RESTfulAPI Coexistence

RESTfulAPI remains the broad VDR domain adapter for:

- channels;
- EPG;
- Timers;
- Recordings;
- existing search and metadata reads;
- existing event-stream hints.

Suite Bridge does not replace those reads and does not become a second public
backend API. It supplies only the native read-only compatibility and observation
lifecycle already accepted in SB.10a through SB.10c.

---

## Plugin and Schema Impact

SB.10d changes no plugin source.

The plugin contract remains:

```text
plugin version       0.10.0
commands             CAPS and SNAP only
discovery schema     1
capability schema    1
snapshot schema      2
local contract       2
mutations            disabled
```

No capability ID or capability state changes.

---

## Non-Goals

SB.10d does not introduce:

- a new plugin command;
- a plugin version or schema change;
- a public health or plugin-payload endpoint;
- a generic SVDRP tunnel;
- a mutation path;
- durable retry storage;
- a replacement for RESTfulAPI;
- media, OSD, Timer, Recording or EPG mutation ownership;
- plugin-owned network, database or worker infrastructure.

---

## Automated Acceptance

Automated acceptance proves:

- default-disabled and explicit-enabled configuration;
- bounded parsing and fallback for every Suite Bridge environment value;
- backend selection by exact backend ID;
- production ownership of the accepted SVDRP transport;
- deterministic injected-transport testing;
- successful worker start, bounded health publication and clean stop;
- no worker for disabled configuration;
- exact DaemonRuntime construction, start and reverse stop wiring;
- RESTfulAPI context construction remains present and independent;
- retained SB.10a, SB.10b and SB.10c tests;
- strict Make source and test inventory;
- complete documentation and architecture checks;
- no public route or mutation fallback;
- controlled-live runner syntax, rollback ordering and forbidden-operation guards;
- successful build of the real embedded-runtime live probe.

The real VDR execution remains a separate completion gate.

---

## Controlled Live Acceptance

Before SB.10d is completed, controlled live acceptance must prove:

1. daemon starts with Suite Bridge disabled and performs no CAPS or SNAP command;
2. daemon starts with Suite Bridge enabled and reaches `snapshot_current`;
3. initial command order is CAPS then SNAP;
4. later polling uses SNAP while continuity is trusted;
5. VDR restart produces bounded degradation, reconnect and a fresh baseline;
6. daemon shutdown joins the worker and leaves no socket or thread alive;
7. RESTfulAPI domain reads continue in parallel;
8. channel, Timer, Recording and `setup.conf` state remain unchanged;
9. plugin configuration and staged binary can be removed completely;
10. VDR and daemon return to their original state;
11. repository worktree is clean and synchronized.

---

## Controlled Acceptance Runner

The repository-owned runner executes the complete opt-in live sequence and always
attempts rollback:

```text
python3 tools/run_sb10d_live_acceptance.py
```

It refuses a pre-existing Suite Bridge installation, records only hashes for
channel, Timer, Recording and setup state, stops and later restores an active
`vdr-suite-daemon.service`, proves that the disabled daemon opens no Suite Bridge
transport connection, stages the plugin, starts the repository daemon with
SB.10d enabled, runs safe REST probes, restarts VDR, requires both a degraded
observation state and a changed plugin epoch, verifies clean worker and daemon
shutdown, removes every staged plugin binary and configuration, restarts VDR,
restores the original daemon-service state and requires a clean worktree.

The runner uses no destructive VDR-Suite API operation.

---

## Back

- [Back to Architecture Index](index.md)
- [Back to SB.10c Observation Lifecycle](suite-bridge-observation-lifecycle.md)
- [Back to Plugin Roadmap](../../vdr-plugin-suite-bridge/docs/ROADMAP.md)
- [Back to Shared Handoff](../../vdr-plugin-suite-bridge/docs/VDR-SUITE-HANDOFF.md)
