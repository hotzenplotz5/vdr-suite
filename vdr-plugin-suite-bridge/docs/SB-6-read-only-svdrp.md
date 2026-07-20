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

```text
PLUG suitebridge CAPS [discovery-schema]
```

`CAPS` without an option selects the current discovery schema. `CAPS 1`
explicitly requests discovery schema `1`.

### Preferred EPG artwork

```text
PLUG suitebridge ARTW <channel-id> <event-id>
```

`ARTW` resolves one preferred TVScraper image reference for the selected EPG
event. It returns schema `1` JSON with `found`, provider, path and validated
image dimensions.

### Extended EPG scraper metadata

```text
PLUG suitebridge META <channel-id> <event-id>
```

`META` resolves one bounded TVScraper metadata document for the selected EPG
event. Schema `1` contains only allowlisted fields:

- movie or series identity;
- titles, overview and provider references;
- season and episode values;
- release, runtime, rating, genre, country and network values;
- preferred artwork;
- bounded people and role entries with optional portraits;
- bounded gallery entries with explicit orientation.

The payload is capped below the Backend Agent's complete SVDRP reply limit. An
overflow returns `451`; no truncated JSON is accepted as a successful reply.

## Detached-event boundary

`ARTW` and `META` acquire VDR's schedule read lock only to locate and copy the
requested `cEvent`. The lock is released before TVScraper is called, images are
inspected or JSON is serialized. This prevents plugin-service and filesystem
work from extending the global schedule-lock duration.

Both commands use TVScraper's public `GetScraperVideo` service. They do not read
TVScraper's database or internal cache schema directly.

## Reply contract

| Case | Reply code | Result |
| --- | ---: | --- |
| valid `SNAP` request | `900` | current deterministic status payload |
| `SNAP` with an option | `504` | request rejected |
| valid `CAPS` or `CAPS 1` | `900` | deterministic capability payload |
| `CAPS` with unsupported numeric schema | `504` | schema not supported |
| `CAPS` with malformed schema | `501` | invalid argument syntax |
| valid `ARTW` request | `250` | artwork schema `1`, including valid `found:false` |
| malformed `ARTW` request | `501` | invalid argument syntax |
| valid `META` request | `250` | metadata schema `1`, including valid `found:false` |
| malformed `META` request | `501` | invalid argument syntax |
| artwork or metadata payload overflow | `451` | bounded local processing failure |
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
| Preferred artwork | `1` |
| EPG scraper metadata | `1` |

The public Suite API, authenticated Agent protocol and plugin software version
are separate compatibility axes.

A consumer that does not support a command's JSON schema must reject or safely
degrade that optional result. It must not infer compatibility from the plugin
version alone.

## Read-only boundary

`SNAP` captures the current atomic monitor counters. `CAPS` serializes only
compile-time and immutable contract values. `ARTW` and `META` resolve read-only
information for an existing VDR event.

None of these commands:

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

## Metadata semantics

TVScraper's `HD` and `Language` integer values are provider metadata and are
named `scraperHd` and `scraperLanguage` in schema `1`. They are not a substitute
for actual VDR component data such as video format, audio languages, Dolby
tracks, audio description or subtitles. Those properties require a separate
read-only VDR event-component contract.

Image paths remain private bridge references. The Backend Agent and Control
Plane must validate, persist and expose approved public image URLs. Browser
clients must never consume TVScraper paths directly.

## Resynchronization

`SNAP` remains the complete read-only resynchronization point for diagnostic
counters. `CAPS`, `ARTW` and `META` do not change the counter epoch or values.

The counters are not a durable sequence, domain-event history or audit record.

## Capability

`local-contract` is reported as `available`. `mutations` remains `disabled`.
Capability discovery reports that state but does not constitute authorization.

## Live acceptance target for plugin version 0.11.0

A controlled live test must prove:

1. plugin version `0.11.0` loads;
2. `HELP` advertises `CAPS`, `SNAP`, `ARTW` and `META`;
3. existing capability and snapshot schemas remain unchanged;
4. malformed `ARTW` and `META` arguments return `501`;
5. an existing event returns complete schema-`1` JSON;
6. a missing TVScraper match returns a valid `found:false` document;
7. metadata replies remain below the transport bound;
8. person and gallery entries contain only validated optional image references;
9. channel, Timer, Recording and setup state remain unchanged;
10. plugin removal and VDR restart leave no Suite Bridge binary loaded.
