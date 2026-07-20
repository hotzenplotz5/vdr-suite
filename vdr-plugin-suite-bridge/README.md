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
VDR Core and local VDR plugins
```

The plugin is not the Backend Agent. It is the small VDR-process-local bridge
used by the separate Backend Agent.

## Current read-only commands

The plugin exposes four bounded read-only commands through VDR's existing SVDRP
server:

```text
PLUG suitebridge CAPS [discovery-schema]
PLUG suitebridge SNAP
PLUG suitebridge ARTW <channel-id> <event-id>
PLUG suitebridge EPMD <channel-id> <event-id>
```

`CAPS` returns plugin-local schema versions and the static capability catalogue.
No option and explicit schema `1` produce byte-identical discovery payloads.
Unsupported numeric schemas return reply `504`, malformed schema arguments return
`501`, and payload preparation failure returns `451`.

`SNAP` captures the current immutable status snapshot and returns one compact
JSON line with reply code `900`. It accepts no options. Unknown options return
reply code `504`.

`ARTW` resolves one preferred TVScraper image for a current VDR EPG event. The
preference remains episode or movie, season, show or collection and then
landscape, banner and portrait. The reply contains a bounded local provider
reference and verified image dimensions. Public URLs remain owned by VDR-Suite.

`EPMD` resolves bounded TVScraper content metadata for one current EPG event. It
returns a schema-versioned JSON payload containing available movie, series and
episode fields, normalized cast and crew roles, optional person portraits and a
small multi-orientation image set. The command does not expose a raw TVScraper
object dump and does not mutate TVScraper or VDR state.

Both EPG commands capture a detached event snapshot under the VDR schedule read
lock and invoke TVScraper only after that lock has been released.

Unknown commands remain unhandled so VDR can issue its standard response.
Command matching is case-insensitive.

All endpoints are read-only. They do not change channels, Timers, Recordings,
playback, setup data or any other VDR state.

## EPG metadata boundary

The EPG metadata contract is deliberately bounded:

- titles, descriptions and list values have fixed maximum byte lengths;
- at most twelve normalized people are returned;
- at most two landscape, two banner and two portrait images are returned;
- image files and person portraits are accepted only when their real dimensions
  can be read;
- malformed or oversized payloads fail closed;
- local provider paths remain internal transport data and are never a public Web
  contract;
- HD, audio-track and subtitle truth is not inferred from TVScraper metadata and
  remains a separate future VDR component contract.

Normalized person roles are:

- actor;
- director;
- writer;
- producer;
- moderator;
- guest;
- composer;
- other;
- unknown.

This mapping is compatible with the existing VDR-Suite Person domain and allows
later EPG-person search without creating a second person model.

## Capability discovery

Discovery schema version: `1`

The deterministic payload reports, in fixed top-level order:

1. discovery schema;
2. plugin name;
3. plugin version;
4. capability schema;
5. snapshot schema;
6. local-contract schema;
7. capability entries.

Current capability catalogue:

| Capability | State |
| --- | --- |
| `lifecycle` | `available` |
| `status-events` | `available` |
| `snapshots` | `available` |
| `local-contract` | `available` |
| `mutations` | `disabled` |

An unknown or absent capability is treated as unavailable. An absent or disabled
`mutations` capability is a hard write prohibition. Capability discovery does
not constitute user authorization.

Current schema versions:

| Schema | Version |
| --- | ---: |
| Discovery | `1` |
| Capability | `1` |
| Snapshot | `2` |
| Local contract | `2` |
| Preferred EPG artwork | `1` |
| EPG metadata | `1` |

The plugin software version is informative. Compatibility decisions use the
explicit schema and capability values.

## Counter continuity

Each plugin instance owns one immutable `counter_epoch` consisting of 32
lowercase hexadecimal characters. A new plugin instance creates a new epoch.
The epoch is an opaque diagnostic continuity value, not a credential or public
identity.

Event-family counters and the derived total use unsigned 64-bit saturation.
Counters never wrap to zero. `counter_overflow` becomes true when an event or
sum can no longer be represented exactly and remains true for the current
epoch.

The Backend Agent compares values only while the epoch is unchanged and overflow
is false. A changed epoch or uncertain continuity requires a complete `SNAP`
baseline. These values are diagnostic observations, not an ordered event stream,
audit history or guaranteed count of user actions.

`CAPS` does not capture a status snapshot and cannot change the epoch or counters.

## Lifecycle boundary

The lifecycle state machine is:

```text
constructed
  -> initialized
  -> started
  -> stopping
  -> stopped
```

- construction registers the `cStatus` monitor but leaves it inactive;
- successful `Start()` activates observation and captures an active snapshot;
- callbacks received while inactive are ignored;
- `Stop()` enters `stopping` before observation is deactivated;
- deactivation captures the final inactive snapshot outside callback execution;
- only after deactivation does the lifecycle reach `stopped`;
- deactivation retains the current counter epoch and counter values;
- invalid or repeated transitions are deterministic;
- no event queue or background worker is created.

## Callback boundary

Each VDR status callback is deliberately bounded to:

1. discard pointer and descriptive arguments;
2. check the atomic active flag;
3. increment one saturating atomic counter;
4. return.

The callback path performs no logging, serialization, network access, file or
database work, allocation, waiting or external invocation. Structured lifecycle,
snapshot and SVDRP logging remains outside the callbacks.

## Deliberate boundaries

The plugin still has:

- no menu entry;
- no plugin-owned network listener;
- no outbound connection;
- no worker thread;
- no database access;
- no filesystem mutation;
- no VDR mutation;
- no counter-reset command;
- no write-capable SVDRP command;
- no public artwork or metadata HTTP route;
- no Streaming Gateway or media-session ownership.

The Backend Agent reaches the commands through VDR's already configured SVDRP
access. Network exposure, source restrictions and authentication remain
deployment responsibilities outside this plugin.

## Project documents

- [ADR-0001: Plugin Role and Native Integration Strategy](docs/ADR-0001-plugin-role-and-native-integration-strategy.md)
- [Plugin Roadmap](docs/ROADMAP.md)
- [Shared VDR-Suite Handoff](docs/VDR-SUITE-HANDOFF.md)

The ADR defines the durable role and safety boundary. The roadmap defines the
ordered implementation direction. The shared handoff remains authoritative for
the last fully accepted coordinated slice and its exact test and live-acceptance
state.

## Build and tests

```bash
make clean
make check
```

`make check` validates the foundation, capability catalogue, capability-discovery,
counter-continuity, status-event, status-snapshot, local-contract-payload,
read-only SVDRP, preferred EPG artwork and EPG metadata contracts; version
extraction; lifecycle state machine; deterministic payloads; request validation;
JSON escaping; bounded overflow handling; and the final shared-object build.

The repository-level fast CI also runs the standalone EPG metadata contract and
the foundation guard without requiring a live VDR instance.

## Staged installation

```bash
rm -rf /tmp/vdr-suitebridge-stage
make DESTDIR=/tmp/vdr-suitebridge-stage install
find /tmp/vdr-suitebridge-stage -type f -print
```

Every new plugin version must pass a controlled VDR load, read-only SVDRP request
and rollback test before it is left installed in the live VDR plugin directory.
