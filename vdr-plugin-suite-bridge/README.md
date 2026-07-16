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
SB.4 - Immutable status snapshots
```

The plugin now captures an immutable, transport-neutral status snapshot from the
native VDR status monitor. Snapshot schema version `1` contains:

- active or inactive monitor state;
- channel-switch event count;
- recording event count;
- replaying event count;
- timer-change event count;
- total event count.

An already captured snapshot cannot be changed by later callbacks. Copy
assignment is disabled and the value exposes no mutating operation.

The source counters remain independent atomics. Snapshot capture is race-safe
and allocation-free, but intentionally not transactional across all event
families. A callback concurrent with capture may appear in that snapshot or in
the following one.

The monitor writes structured snapshots when observation starts and when it is
deactivated. The final snapshot is captured after the active flag has been
cleared.

Current capability catalogue:

| Capability | State |
| --- | --- |
| `lifecycle` | `available` |
| `status-events` | `available` |
| `snapshots` | `available` |
| `local-contract` | `planned` |
| `mutations` | `disabled` |

Capability schema version remains `1`. The current snapshot scope is status
telemetry only; VDR channel, timer, recording and EPG domain snapshots are not
yet exported.

## Lifecycle boundary

- construction registers the `cStatus` monitor but leaves it inactive;
- successful `Start()` activates observation and captures an active snapshot;
- callbacks received while inactive are ignored;
- `Stop()` disables observation before the lifecycle reaches `stopped`;
- deactivation captures and logs the final inactive snapshot;
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
status-event contract, status-snapshot contract, version extraction, lifecycle
state machine, capability catalogue, atomic status-event counters, immutable
snapshot behavior and final shared-object build.

## Staged installation

```bash
rm -rf /tmp/vdr-suitebridge-stage
make DESTDIR=/tmp/vdr-suitebridge-stage install
find /tmp/vdr-suitebridge-stage -type f -print
```

Every new plugin version must pass a controlled VDR load, event, snapshot and
rollback test before it is left installed in the live VDR plugin directory.
