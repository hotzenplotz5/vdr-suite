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
SB.3 - Native VDR status observation
```

The plugin now owns a read-only VDR status monitor. The monitor is registered
through VDR's native `cStatus` interface, remains inactive until the plugin has
successfully started and is deactivated before the plugin lifecycle stops.

Observed event families:

| Event family | Recorded fields |
| --- | --- |
| channel switch | sequence, channel number, live-view flag, device presence |
| recording | sequence, on/off state, device presence |
| replaying | sequence, on/off state, control presence |
| timer change | sequence, numeric VDR change type, timer presence |

The monitor intentionally does not retain recording names, replay names or file
paths. It stores only atomic per-family counters and emits structured VDR log
lines while active.

Current capability catalogue:

| Capability | State |
| --- | --- |
| `lifecycle` | `available` |
| `status-events` | `available` |
| `snapshots` | `planned` |
| `local-contract` | `planned` |
| `mutations` | `disabled` |

Capability schema version remains `1`.

## Lifecycle boundary

- construction registers the `cStatus` monitor but leaves it inactive;
- successful `Start()` activates observation;
- callbacks received while inactive are ignored;
- `Stop()` disables observation before the lifecycle reaches `stopped`;
- deactivation logs final per-event counters;
- no event queue or background worker is created.

## Deliberate boundaries

The plugin still has:

- no menu entry;
- no network listener;
- no outbound connection;
- no worker thread;
- no database access;
- no filesystem mutation;
- no VDR mutation;
- no public Backend Agent transport contract.

## Build and tests

```bash
make clean
make check
```

`make check` validates the foundation contract, capability contract,
status-event contract, version extraction, lifecycle state machine, capability
catalogue, atomic status-event counters and final shared-object build.

## Staged installation

```bash
rm -rf /tmp/vdr-suitebridge-stage
make DESTDIR=/tmp/vdr-suitebridge-stage install
find /tmp/vdr-suitebridge-stage -type f -print
```

Every new plugin version must pass a controlled VDR load, event and rollback
test before it is left installed in the live VDR plugin directory.
