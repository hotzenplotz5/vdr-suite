# SB.9 Read-Only Capability Discovery and Compatibility Negotiation

SB.9 exposes the existing static capability catalogue through one bounded,
read-only VDR plugin command for the Backend Agent.

## Command

```text
PLUG suitebridge CAPS [discovery-schema]
```

Accepted forms:

- `CAPS` selects the current discovery schema;
- `CAPS 1` explicitly selects discovery schema `1`;
- command matching is case-insensitive;
- surrounding ASCII whitespace around the decimal schema is ignored;
- leading zeroes are accepted when the resulting value is `1`.

## Reply contract

| Case | Reply code | Result |
| --- | ---: | --- |
| current or explicit schema `1` | `900` | deterministic capability payload |
| syntactically valid unsupported schema | `504` | unsupported discovery schema |
| malformed schema option | `501` | invalid decimal schema argument |
| payload preparation failure | `451` | local processing failure |
| unknown command | VDR default | plugin returns unhandled |

The plugin logs only bounded command, result, reply, byte-count and schema facts.
It never logs a caller-supplied option value.

## Independent schema axes

Capability discovery introduces:

```text
discovery_schema = 1
```

It does not change:

- capability schema `1`;
- snapshot schema `2`;
- local-contract schema `2`;
- the public VDR-Suite API version;
- a future authenticated Backend Agent protocol version.

Plugin version `0.10.0` is reported for diagnostics. Compatibility decisions are
made from explicit schema and capability fields, not inferred from the software
version alone.

## Deterministic payload

The compact JSON field order is:

1. `discovery_schema`;
2. `plugin_name`;
3. `plugin_version`;
4. `capability_schema`;
5. `snapshot_schema`;
6. `local_contract_schema`;
7. `capabilities`.

The capability array retains the catalogue order:

1. `lifecycle = available`;
2. `status-events = available`;
3. `snapshots = available`;
4. `local-contract = available`;
5. `mutations = disabled`.

The payload is produced in one fixed-capacity stack-owned array. Construction
uses no dynamic container, heap allocation, file, database, socket, worker or
background thread.

## Safe Agent interpretation

The Backend Agent applies these rules:

- an unsupported `discovery_schema` is rejected rather than guessed;
- an unknown or absent capability ID is treated as unavailable;
- unknown additive capability IDs may be ignored;
- known IDs are evaluated by ID and state, not array position;
- `mutations = disabled` is a hard write prohibition;
- an absent `mutations` capability is also treated as disabled;
- snapshot and local-contract schemas are validated independently;
- an older plugin that does not handle `CAPS` is treated as legacy or unknown;
- optional and mutating functions are not enabled through optimistic fallback.

The Control Plane must not interpret plugin capabilities as user authorization.
The Backend Agent remains responsible for authenticated machine transport and
capability freshness publication.

## Read-only and callback boundaries

`CAPS` reads only compile-time and immutable plugin contract values. It does not
capture a status snapshot, inspect VDR-native objects or alter diagnostic
counters.

The command does not:

- switch channels;
- modify Timers, Recordings or replay;
- reset counters;
- access files or a database;
- open a listener or outbound connection;
- create a worker;
- change the existing `SNAP` payload;
- enable mutations.

## Automated acceptance

`make check` must prove:

1. plugin version `0.10.0`;
2. discovery schema `1`;
3. unchanged capability schema `1`;
4. unchanged snapshot and local-contract schemas `2`;
5. deterministic field and capability order;
6. exact five capability IDs and states;
7. explicit `mutations = disabled`;
8. byte-identical replies for `CAPS` and `CAPS 1`;
9. case-insensitive command handling;
10. reply `504` for unsupported numeric schemas;
11. reply `501` for malformed schemas;
12. reply `451` for payload exhaustion;
13. unknown commands remain unhandled;
14. the previous `SNAP` contract remains unchanged;
15. no dynamic container, listener, worker, database or filesystem access;
16. final VDR shared-object and ELF validation.

## Required live VDR acceptance

The live test must prove:

1. plugin version `0.10.0` loads;
2. `HELP` advertises `CAPS` and `SNAP`;
3. `CAPS` and `CAPS 1` return byte-identical reply-`900` payloads;
4. discovery, capability, snapshot and local-contract schemas are exact;
5. all five capability IDs and states are exact;
6. `mutations` remains disabled;
7. `CAPS 2` returns reply `504`;
8. malformed input returns reply `501`;
9. `SNAP` epoch and counters remain unchanged across discovery requests;
10. no callback-side status-event log is emitted;
11. channel, Timer list, Recording list and setup remain unchanged;
12. two-phase stop and complete rollback remain correct.
