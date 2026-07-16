# SB.6 Read-Only Native SVDRP Contract

SB.6 exposes the deterministic local-contract payload through VDR's existing
plugin-specific SVDRP command path. Later slices retain the same command while
versioning the payload contract explicitly.

## Command

The Backend Agent can request the current payload with:

```text
PLUG suitebridge SNAP
```

The command accepts no option and returns one compact JSON line.

## Reply contract

| Case | Reply code | Result |
| --- | ---: | --- |
| valid `SNAP` request | `900` | current deterministic payload |
| `SNAP` with an option | `504` | request rejected |
| unknown command | VDR default | plugin returns unhandled |
| payload preparation failure | `451` | local processing failure |

Command matching is case-insensitive. JSON field order and schema values are
defined by the current local-contract payload.

## Current schema compatibility

Capability schema remains version `1`.

Snapshot schema and local-contract schema are version `2` after SB.8. The payload
adds:

- `counter_epoch`;
- `counter_overflow`.

A consumer that supports only schema `1` must reject or safely degrade. It must
not calculate counter deltas while ignoring the continuity fields.

## Read-only boundary

`SNAP` only captures the current atomic monitor counters and serializes the
result. It does not:

- switch a channel;
- create, edit or delete a timer;
- start or stop a recording;
- control replay;
- reset counters;
- alter VDR setup;
- write a file;
- create a worker thread;
- open a plugin-owned socket.

The plugin uses VDR's existing SVDRP server and does not implement its own
listener.

## Resynchronization

`SNAP` is the full read-only resynchronization point for diagnostic counters.

The Backend Agent accepts a new baseline when:

- it has no previous snapshot;
- `counter_epoch` changes;
- reconnect or transport uncertainty prevents continuity proof.

When `counter_overflow` is true, the Agent must not derive further event deltas
for that epoch.

The counters are not a durable sequence, domain-event history or audit record.

## Capability

`local-contract` is reported as `available`. `mutations` remains `disabled`.

## Current live acceptance target

The SB.8 live test must prove:

1. plugin version `0.9.0` loads;
2. `HELP` advertises `SNAP`;
3. `SNAP` returns reply code `900` and schemas `2`, `1`, `2`;
4. the epoch is valid and stable within one instance;
5. a controlled native observation changes counters without changing the epoch;
6. VDR restart creates a different epoch;
7. normal operation reports no overflow;
8. plugin removal and VDR restart leave no Suite Bridge binary loaded.
