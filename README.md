# VDR-Suite

VDR-Suite modernizes the Video Disk Recorder ecosystem with a backend service layer, REST API foundation and future user interfaces.

VDR remains the primary system and source of truth.

## Current Status

Branch: `phase-2-actions`

Current verified HEAD before this documentation update: `06667cf`

Recent completed work:

- Phase 8.69: PollingService interface
- Phase 8.70: PollingService implementation
- Phase 8.72: extract VDR source list into make include
- Phase 8.74: extract VDR test targets into make include
- Phase 8.75: extract HTTP source list into make include
- Phase 8.76: extract daemon source list into make include
- Phase 8.77: extract recording source lists into make include
- Phase 8.78: extract action and job source lists into make include
- Phase 8.79: initial root Makefile include conversion
- Fix 06667cf: Fix modular Makefile include conversion

Current architectural direction:

```text
RESTfulAPI
VdrService
VdrSnapshotBuilder
PollingService
Snapshot Cache
API Responses
```

API requests should move toward snapshot-backed data instead of repeatedly querying RESTfulAPI directly.

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
- VDR snapshot domain object
- VDR snapshot builder
- PollingService foundation
- modular Makefile include structure

## Snapshot Architecture

The project is moving from direct live access per request to a daemon-owned snapshot architecture.

Current snapshot components:

- VdrSnapshot
- VdrSnapshotBuilder
- PollingService

Purpose:

- keep RESTfulAPI behind the VDR adapter boundary
- keep API controllers independent from live RESTfulAPI calls
- prepare future multi-source and multi-VDR designs
- allow snapshot refresh cycles inside the daemon
- allow API responses from cached daemon state

## Build

Common build targets:

```bash
make daemon
make test
make test-vdr-snapshot-builder
make test-polling-service
```

Known technical debt before Phase 8.80:

```text
Duplicate VDR test targets exist in both the root Makefile and mk/vdr-tests.mk.
The build still works, but make prints duplicate target warnings.
```

Planned cleanup:

```text
Phase 8.80: remove duplicate VDR test targets from the root Makefile.
```

## Architecture Principles

- VDR remains the primary backend domain.
- VDR-Suite complements VDR; it does not replace it.
- RESTfulAPI is currently the most important live integration path.
- RESTfulAPI details stay behind `RestfulApiVdrAdapter`.
- Higher layers use `IVdrAdapter`, `VdrService`, snapshot builders and domain objects.
- Source, capability and permission concepts remain separate from VDR configuration.
- No permanent single-VDR assumption should be introduced.
- API controllers should move toward snapshot-backed daemon data.

## Repository Structure

```text
core/sqlite        SQLite wrapper and database tests
core/recordings   recordings, metadata, jobs, actions and dashboard services
core/http         HTTP abstractions, mock client, basic client and test server
core/vdr          VDR domain objects, adapters, mappers, overview services and snapshot services
api/rest          REST controllers and router
apps/dashboard    dashboard CLI
apps/daemon       daemon entry point
apps/vdr-probe    live RESTfulAPI diagnostic CLI
mk                modular Makefile include files
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

1. Update status documentation after phases 8.69-8.79.
2. Remove duplicate VDR test targets from the root Makefile.
3. Integrate PollingService into DaemonRuntime.
4. Add a snapshot refresh cycle.
5. Add daemon-owned snapshot cache access for API responses.

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
