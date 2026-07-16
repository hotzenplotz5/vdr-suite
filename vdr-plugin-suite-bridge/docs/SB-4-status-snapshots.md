# SB.4 Immutable Status Snapshots

SB.4 adds the first immutable read model inside the Suite bridge.

## Snapshot schema

Status snapshot schema version: `1`

Each snapshot contains only scalar telemetry:

- whether the native status monitor was active at capture time;
- channel-switch event count;
- recording event count;
- replaying event count;
- timer-change event count;
- total event count derived from the four counters.

## Immutability

A `SuiteBridgeStatusSnapshot` exposes no mutating operation. Its fields are set
once during construction, copy assignment is disabled and later VDR callbacks
cannot alter an already captured value.

## Concurrency semantics

The source counters remain independent atomics. Capturing a snapshot is
race-safe and allocation-free, but it is not a transaction across all event
families. A callback that arrives while the counters are read may appear in the
current or the next snapshot.

That best-effort boundary is explicit because SB.4 is diagnostic telemetry, not
a transactional VDR state export.

## Lifecycle integration

The monitor writes structured snapshot logs when it becomes active and when it
is deactivated. The final inactive snapshot is captured after observation has
been disabled, so no later callback can be accepted by that monitor instance.

## Capability transition

Capability schema version remains `1`.

`snapshots` changes from `planned` to `available` for the status-telemetry scope
defined above. This does not yet expose channels, timers, recordings or EPG data
to the Backend Agent.

`local-contract` remains planned and `mutations` remains disabled.
