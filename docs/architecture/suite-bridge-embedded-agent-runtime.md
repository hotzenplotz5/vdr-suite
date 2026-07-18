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

The slice remains active until repository-wide automated acceptance and controlled
live VDR restart, reconnect, shutdown and rollback acceptance have passed.

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

## Embedded Agent Boundary

`SuiteBridgeEmbeddedAgentRuntime` owns exactly one transport and one observation
worker for one backend identity.

Its public surface is limited to:

- deterministic `start()`;
- deterministic `stop()`;
- `running()`;
- one bounded backend-scoped health snapshot.

The health snapshot contains:

- bounded backend identity;
- configured and running facts;
- the accepted SB.10c observation state;
- bounded diagnostics and timestamps already defined by SB.10c;
- mutation state, which remains disabled.

It does not contain raw CAPS or SNAP payloads, SVDRP endpoint details, credentials,
VDR pointers or public API data.

The production constructor owns `SuiteBridgeSvdrpTransport`. A separate injected
transport constructor exists only to make the runtime boundary deterministic in
unit tests.

---

## Backend Ownership

`BackendRuntimeContext` owns the embedded Agent runtime beside its existing
RESTfulAPI objects.

Only the backend matching `VDR_SUITE_SUITE_BRIDGE_BACKEND_ID` receives the local
Suite Bridge runtime. This prevents one local SVDRP endpoint from being
optimistically attached to every configured backend.

The existing RESTfulAPI adapter, polling service, event-stream client, EPG cache,
Recording cache and mutation executors are not replaced or widened.

---

## Start and Stop Ordering

Initialization order for each matching backend is:

1. construct the RESTfulAPI context;
2. construct the Suite Bridge embedded Agent runtime;
3. start the Suite Bridge observation worker;
4. start the existing RESTfulAPI event-stream client;
5. publish the fully owned backend context.

Shutdown reverses active local readers before context destruction:

1. stop the RESTfulAPI event-stream client;
2. stop and join the Suite Bridge observation worker;
3. destroy backend contexts;
4. continue the existing daemon teardown.

A disabled or non-matching backend constructs no transport and starts no worker.
Repeated start and stop calls remain safe.

---

## Health Publication

SB.10d publishes backend-scoped health inside `BackendRuntimeContext` through the
embedded Agent runtime value. It does not add a public REST route.

A later authenticated Agent-to-Control-Plane protocol may consume this value. That
future protocol must preserve backend generation, machine identity and public API
separation. SB.10d does not claim those later surfaces are implemented.

---

## Safety and Non-Goals

SB.10d introduces:

- no plugin source change;
- no plugin version change;
- no command or schema change;
- no capability change;
- no mutation;
- no arbitrary SVDRP tunnel;
- no public plugin JSON exposure;
- no replacement of RESTfulAPI;
- no durable retry or health database;
- no Timer, Recording, EPG, media or OSD operation;
- no backend-generation ownership in the plugin.

`mutations=disabled` remains a hard prohibition.

---

## Automated Acceptance

Automated acceptance must prove:

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
- no public route or mutation fallback.

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

## Back

- [Back to Architecture Index](index.md)
- [Back to SB.10c Observation Lifecycle](suite-bridge-observation-lifecycle.md)
- [Back to Plugin Roadmap](../../vdr-plugin-suite-bridge/docs/ROADMAP.md)
- [Back to Shared Handoff](../../vdr-plugin-suite-bridge/docs/VDR-SUITE-HANDOFF.md)
