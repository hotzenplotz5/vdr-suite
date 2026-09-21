# SB.5 Deterministic Local Contract Payload

SB.5 introduced the first versioned payload that can be returned to the
VDR-Suite Backend Agent through a native VDR read-only endpoint. SB.8 extends the
payload with diagnostic counter continuity.

## Current payload schema

Local-contract schema version `2` serializes one immutable status snapshot as
compact JSON with this fixed field order:

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

The original schema `1` ended after `timer_change`. Schema `2` adds the two
required continuity fields and references snapshot schema `2`.

The payload is generated into a fixed 448-byte buffer. It does not use dynamic
containers, heap allocation, a network listener or a worker thread.

## Determinism

For the same capability schema and snapshot values, the produced bytes are
identical. Field order, names, boolean spelling, epoch string representation and
number formatting are part of the tested contract.

The payload exposes only a const character view, byte count and completeness
flag. Copy assignment is disabled.

## Continuity fields

`counter_epoch` is a 32-character lowercase hexadecimal JSON string. It is
stable for one plugin instance and changes with a new instance.

`counter_overflow` is a JSON boolean. When true, a consumer must not infer
further event deltas for that epoch.

These values do not form a durable event stream or audit history.

## Capability boundary

The `local-contract` capability is `available` because the payload is exposed by
the acceptance-tested read-only `SNAP` command.

Capability schema remains `1`. Snapshot and local-contract schemas are version
`2`. `mutations` remains disabled.

## Logging

Active and inactive status snapshots log the prepared payload for bounded
acceptance testing. The log is diagnostic evidence and is not the Backend Agent
transport or authoritative audit history.
