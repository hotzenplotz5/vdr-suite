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
SB.8 - Diagnostic counter continuity and resynchronization contract
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

Current schema versions:

| Schema | Version |
| --- | ---: |
| Capability | `1` |
| Snapshot | `2` |
| Local contract | `2` |

## Counter continuity

Each plugin instance owns one immutable `counter_epoch` consisting of 32
lowercase hexadecimal characters. A new plugin instance creates a new epoch.
The epoch is an opaque diagnostic continuity value, not a credential or public
identity.

Event-family counters and the derived total use unsigned 64-bit saturation.
Counters never wrap to zero. `counter_overflow` becomes true when an event or
sum can no longer be represented exactly and remains true for the current
epoch.

The Backend Agent compares values only while the epoch is unchanged and overflow
is false. A changed epoch or uncertain continuity requires a complete `SNAP`
baseline. These values are diagnostic observations, not an ordered event stream,
audit history or guaranteed count of user actions.

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
- deactivation retains the current counter epoch and counter values;
- invalid or repeated transitions are deterministic;
- no event queue or background worker is created.

## Callback boundary

Each VDR status callback is deliberately bounded to:

1. discard pointer and descriptive arguments;
2. check the atomic active flag;
3. increment one saturating atomic counter;
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
- no counter-reset command;
- no write-capable SVDRP command;
- no Streaming Gateway or media-session ownership.

The Backend Agent reaches `SNAP` through VDR's already configured SVDRP access.
Network exposure, source restrictions and authentication remain deployment
responsibilities outside this plugin slice.

## Build and tests

```bash
make clean
make check
```

`make check` validates the foundation, capability, counter-continuity,
status-event, status-snapshot, local-contract-payload and read-only SVDRP source
contracts; version extraction; the two-phase lifecycle state machine;
callback-side-effect exclusion; epoch format and instance separation; saturating
atomic counters and totals; immutable snapshots; deterministic payload bytes;
command handling; and the final shared-object build.

## Staged installation

```bash
rm -rf /tmp/vdr-suitebridge-stage
make DESTDIR=/tmp/vdr-suitebridge-stage install
find /tmp/vdr-suitebridge-stage -type f -print
```

Every new plugin version must pass a controlled VDR load, read-only SVDRP request
and rollback test before it is left installed in the live VDR plugin directory.
