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
VDR Core and read-only plugin services
```

The plugin is not the Backend Agent. It is the small VDR-process-local bridge
used by the separate Backend Agent. Browser clients never call VDR plugins or
TVScraper directly.

## Current contract

Plugin version: `0.11.0`

The plugin exposes four bounded read-only commands through VDR's existing
SVDRP server:

```text
PLUG suitebridge CAPS [discovery-schema]
PLUG suitebridge SNAP
PLUG suitebridge ARTW <channel-id> <event-id>
PLUG suitebridge META <channel-id> <event-id>
```

Command matching is case-insensitive. Unknown commands remain unhandled so VDR
can issue its standard response.

### `CAPS`

Returns plugin-local schema versions and the static capability catalogue. No
option and explicit schema `1` produce byte-identical discovery payloads.
Unsupported numeric schemas return reply `504`, malformed schema arguments
return `501`, and payload preparation failure returns `451`.

### `SNAP`

Captures the current immutable status snapshot and returns one compact JSON
line with reply code `900`. It accepts no options. Unknown options return reply
code `504`.

### `ARTW`

Resolves the preferred TVScraper artwork for one VDR EPG event. The event is
copied under VDR's schedule read lock; the TVScraper service call and image
inspection happen after that global lock has been released.

The response contains only a validated provider, file path and actual image
dimensions. Series prefer a cover-oriented primary image in this order:

1. current-season artwork;
2. series artwork;
3. any-season artwork;
4. episode artwork;
5. portrait orientation;
6. landscape orientation;
7. banner orientation.

Movies preserve the event/movie-first and landscape-first selection used by
the existing contract.

### `META`

Resolves one bounded TVScraper metadata document for one VDR EPG event. The
same detached-event rule as `ARTW` applies.

Schema `1` can contain:

- movie or series type and TVScraper provider ID;
- title, original title, episode name, tagline and overview;
- season, episode and absolute episode numbers;
- release date, first-air date, runtime and duration deviation;
- IMDb ID, collection, status, rating and vote count;
- genres, production countries and networks;
- preferred artwork;
- bounded cast and crew entries with role, character name and portrait;
- bounded landscape, banner and portrait gallery entries.

The payload capacity is `7680` bytes so the complete framed SVDRP reply remains
below the Backend Agent's `8192`-byte transport bound. An oversized document is
rejected with reply `451`; it is never returned as truncated valid JSON.

TVScraper's numeric `HD` and `Language` values are exposed as
`scraperHd` and `scraperLanguage`. They are provider metadata, not proof of the
actual DVB video format, audio tracks or subtitle streams. Those broadcast
properties require a separate typed VDR component contract.

## Data-source boundary

`ARTW` and `META` use TVScraper's public `GetScraperVideo` VDR service. The
bridge does not read TVScraper's SQLite database or cache layout directly. It
does not expose arbitrary filesystem access and does not transmit image bytes
through SVDRP.

The Backend Agent and Control Plane remain responsible for:

- parsing and validating the metadata response;
- persistence, expiry and retry policy;
- serving approved public image URLs;
- joining EPG events, recordings and people;
- user authorization and all mutation policy.

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

An unknown or absent capability is treated as unavailable. An absent or
disabled `mutations` capability is a hard write prohibition. Capability
discovery does not constitute user authorization.

Current schema versions:

| Schema | Version |
| --- | ---: |
| Discovery | `1` |
| Capability | `1` |
| Snapshot | `2` |
| Local contract | `2` |
| Artwork | `1` |
| EPG scraper metadata | `1` |

The plugin software version is informative. Compatibility decisions use the
explicit schema and capability values.

## Counter continuity

Each plugin instance owns one immutable `counter_epoch` consisting of 32
lowercase hexadecimal characters. Event-family counters and the derived total
use unsigned 64-bit saturation and never wrap to zero. A changed epoch,
overflow or uncertain continuity requires a complete `SNAP` baseline.

These values are diagnostic observations, not an ordered event stream, audit
history or guaranteed count of user actions. `CAPS`, `ARTW` and `META` do not
change the epoch or counters.

## Lifecycle and callback boundary

The lifecycle state machine is:

```text
constructed
  -> initialized
  -> started
  -> stopping
  -> stopped
```

Each VDR status callback is deliberately bounded to checking the active flag
and incrementing one saturating atomic counter. Callback execution performs no
logging, serialization, network access, file or database work, allocation,
waiting or external plugin invocation.

## Deliberate boundaries

The plugin has:

- no menu entry;
- no plugin-owned network listener;
- no outbound connection;
- no worker thread;
- no database access;
- no filesystem mutation;
- no VDR mutation;
- no counter-reset command;
- no write-capable SVDRP command;
- no Streaming Gateway or media-session ownership.

The plugin uses VDR's existing SVDRP server. Network exposure, source
restrictions and authentication remain deployment responsibilities.

## Project documents

- [ADR-0001: Plugin Role and Native Integration Strategy](docs/ADR-0001-plugin-role-and-native-integration-strategy.md)
- [Plugin Roadmap](docs/ROADMAP.md)
- [Read-only SVDRP contract](docs/SB-6-read-only-svdrp.md)
- [Shared VDR-Suite Handoff](docs/VDR-SUITE-HANDOFF.md)
- [Native Recording Metadata Handoff](docs/RECORDING-METADATA-HANDOFF.md)

## Build and tests

```bash
make clean
make test-epg-artwork-contract
make test-epg-metadata-contract
make check
```

`make check` validates the source boundaries, independent request and JSON
contracts, version extraction, lifecycle and status contracts, image validation
and the final shared-object build.

## Controlled live verification

After a staged build and installation, use real channel and event IDs from the
running VDR:

```text
PLUG suitebridge ARTW <channel-id> <event-id>
PLUG suitebridge META <channel-id> <event-id>
```

The live acceptance must confirm valid JSON, bounded reply size, read-only VDR
state and complete rollback. A `found:false` response is valid when TVScraper
has no match for the selected event.
