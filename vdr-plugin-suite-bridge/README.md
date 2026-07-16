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
SB.1 - Deterministic plugin lifecycle
```

The plugin now owns a small VDR-independent lifecycle state machine with the
states `constructed`, `initialized`, `started` and `stopped`.

Lifecycle rules:

- construction remains side-effect free;
- `Start()` is rejected before successful initialization;
- repeated successful initialization and start calls are idempotent;
- stop is idempotent;
- a stopped plugin instance cannot be restarted;
- VDR logs include event, result, state and version fields.

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

`make check` validates the source contract, version extraction, lifecycle state
machine and final shared-object build.

## Staged installation

```bash
rm -rf /tmp/vdr-suitebridge-stage
make DESTDIR=/tmp/vdr-suitebridge-stage install
find /tmp/vdr-suitebridge-stage -type f -print
```

The plugin must not yet be installed into the live VDR plugin directory without
a controlled load and rollback test.
