# SB.4 Immutable Status Snapshots

SB.4 introduced the first immutable read model inside the Suite bridge. SB.8
extends that model with explicit diagnostic counter continuity.

## Current snapshot schema

Status snapshot schema version: `2`

Each snapshot contains only bounded scalar telemetry:

- whether the native status monitor was active at capture time;
- channel-switch event count;
- recording event count;
- replaying event count;
- timer-change event count;
- saturating total derived from the four counters;
- immutable process-local counter epoch;
- counter-overflow state.

The original SB.4 schema `1` contained only the active flag and counters. Schema
`2` is required because epoch and overflow semantics are now part of the value
contract.

## Immutability

A `SuiteBridgeStatusSnapshot` exposes no mutating operation. Its fields are set
once during construction, copy assignment is disabled and later VDR callbacks
cannot alter an already captured value.

## Concurrency semantics

The source counters remain independent atomics. Capturing a snapshot is
race-safe and allocation-free, but it is not a transaction across all event
families. A callback that arrives while values are read may appear in the current
or the next snapshot.

That best-effort boundary is explicit because the snapshot is diagnostic
telemetry, not a transactional VDR state export or ordered event stream.

## Saturation

Individual counters never wrap. They saturate at the maximum unsigned 64-bit
value. An event that can no longer be represented marks the current epoch as
overflowed.

The derived total also saturates. If its exact sum is not representable, the
total is the maximum unsigned 64-bit value and the snapshot reports overflow.

## Lifecycle integration

The monitor writes structured snapshot logs when it becomes active and when it
is deactivated. The final inactive snapshot is captured after observation has
been disabled, so no later callback can be accepted by that monitor instance.

Activation and deactivation retain the same counter epoch. A new plugin instance
creates a new epoch and therefore a new comparison baseline.

## Capability boundary

Capability schema version remains `1`.

`snapshots` remains `available` for the bounded status-telemetry scope defined
above. This does not expose channels, timers, recordings or EPG domain data to
the Backend Agent.

`mutations` remains disabled.
