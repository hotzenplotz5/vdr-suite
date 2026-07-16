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
SB.5 - Deterministic local contract payload
```

The plugin now converts an immutable status snapshot into compact, versioned
JSON with a fixed field order and fixed 320-byte storage. The payload contains:

- contract schema;
- capability schema;
- snapshot schema;
- monitor state;
- total event count;
- channel-switch count;
- recording count;
- replaying count;
- timer-change count.

The serializer uses no dynamic container or heap-owned string. It exposes only
a const byte view, byte count and completeness flag. Copy assignment is
disabled.

Active and inactive snapshots log their prepared payload for acceptance
evidence. This diagnostic log is not the Backend Agent transport.

Current capability catalogue:

| Capability | State |
| --- | --- |
| `lifecycle` | `available` |
| `status-events` | `available` |
| `snapshots` | `available` |
| `local-contract` | `planned` |
| `mutations` | `disabled` |

`local-contract` deliberately remains `planned`: the stable payload exists, but
no native VDR request endpoint exposes it yet.

## Lifecycle boundary

- construction registers the `cStatus` monitor but leaves it inactive;
- successful `Start()` activates observation and captures an active snapshot;
- callbacks received while inactive are ignored;
- `Stop()` disables observation before the lifecycle reaches `stopped`;
- deactivation captures the final inactive snapshot;
- each logged snapshot also prepares one deterministic contract payload;
- no event queue or background worker is created.

## Deliberate boundaries

The plugin still has:

- no menu entry;
- no plugin-owned network listener;
- no outbound connection;
- no worker thread;
- no database access;
- no filesystem mutation;
- no VDR mutation;
- no public Backend Agent request endpoint.

## Build and tests

```bash
make clean
make check
```

`make check` validates the foundation, capability, status-event,
status-snapshot and local-contract-payload source contracts, version extraction,
lifecycle state machine, capability catalogue, atomic counters, immutable
snapshots, deterministic payload bytes and final shared-object build.

## Staged installation

```bash
rm -rf /tmp/vdr-suitebridge-stage
make DESTDIR=/tmp/vdr-suitebridge-stage install
find /tmp/vdr-suitebridge-stage -type f -print
```

Every new plugin version must pass a controlled VDR load, event, payload and
rollback test before it is left installed in the live VDR plugin directory.
