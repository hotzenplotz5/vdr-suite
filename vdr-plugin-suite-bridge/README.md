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

## Foundation scope

The initial foundation contains:

- standard VDR plugin lifecycle;
- plugin version and description;
- lifecycle logging;
- no menu entry;
- no network listener;
- no outbound connection;
- no worker thread;
- no database access;
- no filesystem mutation;
- no VDR mutation.

## Build

```bash
make clean
make check
```

## Staged installation

```bash
rm -rf /tmp/vdr-suitebridge-stage
make DESTDIR=/tmp/vdr-suitebridge-stage install
find /tmp/vdr-suitebridge-stage -type f -print
```

The foundation must not yet be installed into the live VDR plugin directory.
