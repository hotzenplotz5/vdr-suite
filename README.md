# VDR-Suite

VDR-Suite modernizes the Video Disk Recorder ecosystem with a backend service layer, REST API foundation and future user interfaces.

VDR remains the primary system and source of truth.

## Current Status

Branch: `phase-2-actions`

Current focus: live VDR integration through `vdr-plugin-restfulapi`.

The project is no longer architecture-only. A real VDR system has been reached successfully through the VDR-Suite adapter and service stack.

Validated live stack:

```text
VDR
vdr-plugin-restfulapi
BasicHttpClient
RestfulApiVdrAdapter
VdrService
VdrOverviewService
VdrOverviewJsonSerializer
apps/vdr-probe
```

Validated with:

```text
vdr-plugin-restfulapi 0.2.6.6
127.0.0.1:8002
```

Observed live result:

```text
Recordings: 973
Channels: 342
Events: 37228+
Timers: 0
HTTP status: 200
```

## Current Capabilities

Implemented and tested:

- SQLite database foundation
- recording, metadata, job and dashboard services
- REST controller and router foundation
- daemon runtime foundation
- HTTP abstraction layer
- basic HTTP client
- VDR adapter abstraction
- mock VDR backend
- RESTfulAPI VDR adapter
- RESTfulAPI mappers for status, channels, events, timers and recordings
- VDR overview aggregation
- VDR overview JSON serialization

Validated with a live VDR:

- read VDR status
- read recordings
- read timers
- read channels
- read EPG events
- build aggregated VDR overview
- serialize VDR overview as JSON

## VDR Probe

A diagnostic CLI validates the live RESTfulAPI integration path.

Location:

```text
apps/vdr-probe
```

Build:

```bash
cd apps/vdr-probe
make clean
make vdr-probe
```

Run:

```bash
/tmp/vdr-probe
```

Run with explicit host and port:

```bash
/tmp/vdr-probe 127.0.0.1 8002
```

## Architecture Principles

- VDR remains the primary backend domain.
- VDR-Suite complements VDR; it does not replace it.
- RESTfulAPI is currently the most important live integration path.
- RESTfulAPI details stay behind `RestfulApiVdrAdapter`.
- Higher layers use `IVdrAdapter`, `VdrService` and domain objects.
- Source, capability and permission concepts remain separate from VDR configuration.
- The current implementation may use one local RESTfulAPI backend, but no permanent single-VDR assumption should be introduced.

Current VDR integration boundary:

```text
Consumer
VdrService
IVdrAdapter
RestfulApiVdrAdapter
IHttpClient
RESTfulAPI
VDR
```

## Repository Structure

```text
core/sqlite        SQLite wrapper and database tests
core/recordings   recordings, metadata, jobs, actions and dashboard services
core/http         HTTP abstractions, mock client, basic client and test server
core/vdr          VDR domain objects, adapters, mappers and overview services
api/rest          REST controllers and router
apps/dashboard    dashboard CLI
apps/daemon       daemon entry point
apps/vdr-probe    live RESTfulAPI diagnostic CLI
```

## External Projects

### vdr-plugin-restfulapi

Current primary live VDR integration path.

Implemented read mappings:

```text
/info.json        -> VdrStatus
/recordings.json  -> VdrRecording
/timers.json      -> VdrTimer
/channels.json    -> VdrChannel
/events.json      -> VdrEvent
```

### VDR-Rectools

Planned as a stable CLI and worker integration for recording maintenance workflows.

### TVScraper and scraper integrations

Planned as metadata and artwork integration points.

## Near-Term Direction

- continue real RESTfulAPI validation
- expose live VDR overview through VDR-Suite REST endpoints
- keep adapter and service boundaries clean
- improve diagnostics and error handling
- document live integration behavior as it is validated

## Documentation

Important documentation areas:

```text
docs/development/current-status.md
docs/development/live-restfulapi-validation.md
docs/architecture/restfulapi-integration.md
docs/adr/
docs/planning/
```

## License

See `LICENSE`.
