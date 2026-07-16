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
SB.2 - Native capability description
```

The plugin now owns an immutable, VDR-independent capability catalogue with
schema version `1`. It reports only capabilities that are already available and
keeps future work explicitly marked as planned or disabled.

Current catalogue:

| Capability | State |
| --- | --- |
| `lifecycle` | `available` |
| `status-events` | `planned` |
| `snapshots` | `planned` |
| `local-contract` | `planned` |
| `mutations` | `disabled` |

Capability state meanings:

- `available`: implemented and covered by acceptance tests;
- `planned`: reserved for a later implementation slice;
- `disabled`: deliberately unavailable in the current architecture state.

During plugin initialization VDR receives one structured log line per
capability containing schema version, capability ID and state.

The capability catalogue does not expose a transport or public Backend Agent
contract yet. That boundary remains a later slice.

## Deliberate boundaries

The plugin still has:

- no menu entry;
- no network listener;
- no outbound connection;
- no worker thread;
- no database access;
- no filesystem mutation;
- no VDR mutation.

## Build and tests

```bash
make clean
make check
```

`make check` validates the foundation contract, capability source contract,
version extraction, lifecycle state machine, capability catalogue and final
shared-object build.

## Staged installation

```bash
rm -rf /tmp/vdr-suitebridge-stage
make DESTDIR=/tmp/vdr-suitebridge-stage install
find /tmp/vdr-suitebridge-stage -type f -print
```

Every new plugin version must pass a controlled VDR load and rollback test
before it is left installed in the live VDR plugin directory.
