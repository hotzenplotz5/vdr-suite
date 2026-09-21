# SB.8 Diagnostic Counter Continuity and Resynchronization Contract

SB.8 makes the existing read-only diagnostic counters safe to compare across
snapshots without turning them into durable event sequences.

## Ownership

The plugin owns process-local counter continuity and saturation. The Backend
Agent owns baseline comparison and resynchronization. The Control Plane must not
interpret these counters as canonical domain events, audit history or user
actions.

No mutation, listener, worker, database, public API, media path or OSD control is
added.

## Counter epoch

Every plugin instance creates one immutable `counter_epoch` value.

The value is:

- exactly 32 lowercase hexadecimal characters;
- stable for the lifetime of one plugin instance;
- different for separately constructed instances;
- regenerated when VDR restarts or the plugin is loaded as a new instance;
- an opaque diagnostic continuity value, not a credential or public identity.

Generation is bounded and allocation-free. It combines process-local sequence,
process identity, monotonic time and wall-clock time. It performs no file,
database, network or random-device access.

## Saturating counters

Each event-family counter uses unsigned 64-bit saturation.

Normal increments remain exact. When a counter reaches the maximum unsigned
64-bit value, it remains at that value. The next event that cannot be represented
sets `counter_overflow` to `true`. The flag remains true for the rest of the
counter epoch.

Counters never wrap to zero.

The derived `total` is also calculated with saturation. If the exact sum cannot
be represented, `total` becomes the maximum unsigned 64-bit value and
`counter_overflow` is true in that snapshot.

## Snapshot and local-contract schema

Snapshot schema version is `2`.

Local-contract schema version is `2`.

Capability schema version remains `1`.

The compact JSON field order is:

1. `contract_schema`;
2. `capability_schema`;
3. `snapshot_schema`;
4. `active`;
5. `total`;
6. `channel_switch`;
7. `recording`;
8. `replaying`;
9. `timer_change`;
10. `counter_epoch`;
11. `counter_overflow`.

`counter_epoch` is serialized as a JSON string. `counter_overflow` is serialized
as a JSON boolean.

## Restart and reset semantics

There is no counter-reset command.

Deactivation does not reset counters or change the epoch. The final inactive
snapshot retains the same epoch and values as the active plugin instance.

A new plugin instance creates a new epoch and fresh counters. Native VDR
callbacks may arrive immediately after activation, so the first active snapshot
is not required to contain only zero values.

## Agent resynchronization

`PLUG suitebridge SNAP` remains the complete read-only resynchronization point.

The Agent applies these rules:

- same epoch and `counter_overflow = false`: values may be compared as cumulative
  diagnostic observations;
- changed epoch: discard the previous baseline and adopt the full `SNAP` payload
  as the new baseline;
- `counter_overflow = true`: do not calculate further deltas for that epoch;
- reconnect or uncertain continuity: request a complete `SNAP` before comparing
  values.

These counters are explicitly not:

- an ordered event stream;
- an audit or security-event history;
- a guaranteed count of user-visible actions;
- a durable sequence across VDR restarts;
- a replacement for a future authenticated Agent event protocol.

One visible channel change may generate more than one native VDR callback.

## Compatibility

Plugin version changes to `0.9.0`.

The following remain unchanged:

- capability IDs and states;
- capability schema version `1`;
- `PLUG suitebridge SNAP` syntax;
- reply codes `900`, `504` and `451`;
- lifecycle and callback-side-effect boundaries;
- `mutations = disabled`.

Consumers that support only local-contract schema `1` must reject or safely
degrade when schema `2` is reported. They must not ignore the new continuity
fields while performing counter deltas.

## Automated acceptance

`make check` must prove:

1. plugin version `0.9.0`;
2. epoch format and stability within one instance;
3. distinct epochs for separately constructed instances;
4. allocation-free and file-free epoch generation;
5. normal counter increments;
6. saturation at the unsigned 64-bit maximum;
7. persistent overflow marking after an unrepresentable event;
8. no wrap to zero;
9. saturating total calculation;
10. immutable snapshot schema `2`;
11. deterministic local-contract schema `2` and field order;
12. unchanged callback-side-effect boundary;
13. all previous lifecycle, capability and SVDRP behavior;
14. final VDR shared-object and ELF validation.

## Required live VDR acceptance

The live test must prove:

1. plugin version `0.9.0` loads;
2. the first `SNAP` reports schema `2`, a valid epoch and no overflow;
3. a controlled channel switch and restore increase counters under the same epoch;
4. no callback-side `status-event` log is emitted;
5. VDR restart with the plugin installed produces a different epoch;
6. the post-restart baseline is accepted without requiring zero counters;
7. `counter_overflow` remains false in normal operation;
8. the original channel, Timer list, Recording list and setup remain restored;
9. two-phase stop and plugin rollback remain correct;
10. the worktree remains clean.
