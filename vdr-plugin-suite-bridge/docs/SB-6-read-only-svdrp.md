# SB.6 Read-Only Native SVDRP Contract

SB.6 introduced the deterministic local-contract payload through VDR's existing
plugin-specific SVDRP command path. Later slices retain `SNAP` and may add only
bounded read-only commands with independent versioned contracts.

## Commands

### Status snapshot

```text
PLUG suitebridge SNAP
```

`SNAP` accepts no option and returns the current compact local-contract payload.

### Capability discovery

SB.9 adds:

```text
PLUG suitebridge CAPS [discovery-schema]
```

`CAPS` without an option selects the current discovery schema. `CAPS 1`
explicitly requests discovery schema `1`.

## Reply contract

| Case | Reply code | Result |
| --- | ---: | --- |
| valid `SNAP` request | `900` | current deterministic status payload |
| `SNAP` with an option | `504` | request rejected |
| valid `CAPS` or `CAPS 1` | `900` | deterministic capability payload |
| `CAPS` with unsupported numeric schema | `504` | schema not supported |
| `CAPS` with malformed schema | `501` | invalid argument syntax |
| payload preparation failure | `451` | local processing failure |
| unknown command | VDR default | plugin returns unhandled |

Command matching is case-insensitive. JSON field order and schema values are
defined by each command's own payload contract.

## Schema compatibility

The independent plugin-local schema axes are:

| Schema | Version |
| --- | ---: |
| Capability discovery | `1` |
| Capability catalogue | `1` |
| Status snapshot | `2` |
| Local contract | `2` |

The public Suite API, future authenticated Agent protocol and plugin software
version are separate compatibility axes.

A consumer that supports only local-contract schema `1` must reject or safely
degrade when schema `2` is reported. It must not calculate counter deltas while
ignoring continuity fields.

A consumer that does not support discovery schema `1` must not infer capability
support from the plugin version. An older plugin that leaves `CAPS` unhandled is
treated as legacy or unknown; optional and mutating functions remain disabled.

## Read-only boundary

`SNAP` captures the current atomic monitor counters and serializes one immutable
snapshot. `CAPS` serializes only compile-time and immutable plugin contract
values and deliberately does not capture a status snapshot.

Neither command:

- switches a channel;
- creates, edits or deletes a Timer;
- starts or stops a Recording;
- controls replay;
- resets counters;
- alters VDR setup;
- writes a file;
- creates a worker thread;
- opens a plugin-owned socket;
- enables mutations.

The plugin uses VDR's existing SVDRP server and does not implement its own
listener.

## Resynchronization

`SNAP` remains the complete read-only resynchronization point for diagnostic
counters.

The Backend Agent accepts a new baseline when:

- it has no previous snapshot;
- `counter_epoch` changes;
- reconnect or transport uncertainty prevents continuity proof.

When `counter_overflow` is true, the Agent must not derive further event deltas
for that epoch.

The counters are not a durable sequence, domain-event history or audit record.
`CAPS` does not change the epoch or any counter.

## Capability

`local-contract` is reported as `available`. `mutations` remains `disabled`.
Capability discovery reports that state but does not constitute authorization.

## Current live acceptance target

The SB.9 live test must prove:

1. plugin version `0.10.0` loads;
2. `HELP` advertises `CAPS` and `SNAP`;
3. `CAPS` and `CAPS 1` return byte-identical reply-`900` payloads;
4. unsupported and malformed discovery schemas return `504` and `501`;
5. discovery reports schemas `1`, `1`, `2`, `2`;
6. all five capability IDs and states are exact;
7. `mutations` is disabled;
8. `SNAP` remains schema `2` and unchanged across discovery calls;
9. callback logs and VDR state remain unchanged;
10. plugin removal and VDR restart leave no Suite Bridge binary loaded.
