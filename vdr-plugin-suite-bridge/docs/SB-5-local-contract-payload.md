# SB.5 Deterministic Local Contract Payload

SB.5 defines the first versioned payload that can later be returned to the
VDR-Suite Backend Agent through a native VDR read-only endpoint.

## Payload schema

Contract schema version `1` serializes one immutable status snapshot as compact
JSON with a fixed field order:

- `contract_schema`;
- `capability_schema`;
- `snapshot_schema`;
- `active`;
- `total`;
- `channel_switch`;
- `recording`;
- `replaying`;
- `timer_change`.

The payload is generated into a fixed 320-byte buffer. It does not use dynamic
containers, heap allocation, a network listener or a worker thread.

## Determinism

For the same capability schema and snapshot values, the produced bytes are
identical. Field order, names, boolean spelling and number formatting are part of
the tested contract.

The payload exposes only a const character view, byte count and completeness
flag. Copy assignment is disabled.

## Capability boundary

The `local-contract` capability remains `planned` in SB.5.

SB.5 proves the payload representation but does not yet expose a VDR request
endpoint. The capability may change to `available` only after the Backend Agent
can retrieve the payload through an implemented and acceptance-tested read-only
VDR interface.

## Logging

Active and inactive status snapshots log the prepared payload for acceptance
testing. The log is diagnostic evidence and is not considered the Backend Agent
transport.
