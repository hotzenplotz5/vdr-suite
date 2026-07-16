# vdr-plugin-suite-bridge

Native VDR integration bridge for VDR-Suite.

## Architectural position

```text
VDR-Suite Control Plane
        |
        v
VDR-Suite Backend Agent
        |
        v
vdr-plugin-suite-bridge
        |
        v
VDR Core
```

The plugin is not the Backend Agent. It is the small VDR-process-local bridge
used by the separate Backend Agent.

## Current slice

```text
SB.7 - Gold-standard lifecycle and callback-boundary hardening
```

The plugin keeps the read-only native SVDRP endpoint introduced by SB.6:

```text
PLUG suitebridge SNAP
```

`SNAP` captures the current immutable status snapshot and returns one compact
JSON line with reply code `900`. The command accepts no options. Unknown options
return reply code `504`, and unknown commands remain unhandled so VDR can issue
its standard error response.

The endpoint is read-only. It does not change channels, timers, recordings,
playback, setup data or any other VDR state.

Current capability catalogue:

| Capability | State |
| --- | --- |
| `lifecycle` | `available` |
| `status-events` | `available` |
| `snapshots` | `available` |
| `local-contract` | `available` |
| `mutations` | `disabled` |

Capability schema version remains `1`.

## Lifecycle boundary

The lifecycle state machine is:

```text
constructed
  -> initialized
  -> started
  -> stopping
  -> stopped
```

- construction registers the `cStatus` monitor but leaves it inactive;
- successful `Start()` activates observation and captures an active snapshot;
- callbacks received while inactive are ignored;
- `Stop()` enters `stopping` before observation is deactivated;
- deactivation captures the final inactive snapshot outside callback execution;
- only after deactivation does the lifecycle reach `stopped`;
- invalid or repeated transitions are deterministic;
- no event queue or background worker is created.

## Callback boundary

Each VDR status callback is deliberately bounded to:

1. discard pointer and descriptive arguments;
2. check the atomic active flag;
3. increment one relaxed atomic counter;
4. return.

The callback path performs no logging, serialization, network access, file or
database work, allocation, waiting or external invocation. Structured lifecycle,
snapshot and SVDRP logging remains outside the callbacks.

## Deliberate boundaries

The plugin still has:

- no menu entry;
- no plugin-owned network listener;
- no outbound connection;
- no worker thread;
- no database access;
- no filesystem mutation;
- no VDR mutation;
- no write-capable SVDRP command;
- no Streaming Gateway or media-session ownership.

The Backend Agent reaches `SNAP` through VDR's already configured SVDRP access.
Network exposure, source restrictions and authentication remain deployment
responsibilities outside this plugin slice. ADR-0046 keeps public media sessions,
routing, authorization and byte delivery outside the plugin boundary.

## Build and tests

```bash
make clean
make check
```

`make check` validates the foundation, capability, status-event,
status-snapshot, local-contract-payload and read-only SVDRP source contracts,
version extraction, the two-phase lifecycle state machine, callback-side-effect
exclusion, capability catalogue, atomic counters, immutable snapshots,
deterministic payload bytes, command handling and the final shared-object build.

## Staged installation

```bash
rm -rf /tmp/vdr-suitebridge-stage
make DESTDIR=/tmp/vdr-suitebridge-stage install
find /tmp/vdr-suitebridge-stage -type f -print
```

Every new plugin version must pass a controlled VDR load, read-only SVDRP request
and rollback test before it is left installed in the live VDR plugin directory.
