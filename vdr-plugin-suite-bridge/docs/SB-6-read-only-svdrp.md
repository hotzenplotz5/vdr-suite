# SB.6 Read-Only Native SVDRP Contract

SB.6 exposes the deterministic local-contract payload through VDR's existing
plugin-specific SVDRP command path.

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

Command matching is case-insensitive. JSON field order and schema values remain
defined by the SB.5 local-contract payload.

## Read-only boundary

`SNAP` only captures the current atomic monitor counters and serializes the
result. It does not:

- switch a channel;
- create, edit or delete a timer;
- start or stop a recording;
- control replay;
- alter VDR setup;
- write a file;
- create a worker thread;
- open a plugin-owned socket.

The plugin uses VDR's existing SVDRP server and does not implement its own
listener.

## Capability

After the endpoint has passed build and live acceptance, `local-contract` is
reported as `available`. `mutations` remains `disabled`.

Capability schema, snapshot schema and local-contract schema remain at version
`1`.

## Live acceptance

The live test must prove:

1. plugin version `0.7.0` loads;
2. `HELP` advertises `SNAP`;
3. `PLUG suitebridge SNAP` returns reply code `900`;
4. the returned JSON is byte-consistent with its fields and schemas;
5. an option is rejected with reply code `504`;
6. the command causes no channel change;
7. plugin removal and VDR restart leave no Suite bridge binary loaded.
