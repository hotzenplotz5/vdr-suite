# VDR-Suite Completed Phases

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)

---

## Purpose

This file tracks completed implementation phases.

Current status belongs to:

- [Current Project Status](current-status.md)

Future planning belongs to:

- [Roadmap](../planning/roadmap.md)
- [Milestones](../planning/milestones.md)

---

## Phase 0 - Project and Documentation Foundation

Status: Completed

## Phase 1 - Persistence and Core Backend Foundation

Status: Completed

## Phase 2 - Actions, Queue and Worker Foundation

Status: Completed

## Phase 3 - Job Dashboard Service

Status: Completed

## Phase 4 - Recording Dashboard Service

Status: Completed

## Phase 5 - Dashboard Facade

Status: Completed

## Phase 6 - Dashboard JSON and CLI

Status: Completed

## Phase 7 - REST API Foundation

Status: Completed

## Phase 8 - VDR Backend Architecture Foundation

Status: Completed

## Phase 9 - Snapshot Runtime Validation

Status: Completed

## Phase 10 - Runtime Diagnostics and Runtime Wiring

Status: Completed

## Phase 11 - Snapshot Read APIs

Status: Completed

## Phase 12 - Snapshot Change Feed Foundation

Status: Completed

## Phase 13 - Snapshot Change Feed Runtime Integration

Status: Completed through Phase 33.4

---

## Phase 14 - Backend Registry Foundation

Status: Completed through Phase 14.9

Result:

- BackendNode domain model
- BackendRegistry foundation
- BackendRegistry runtime integration
- BackendRegistry service layer
- multi-backend architecture preparation
- backend identity propagation foundation

---

## Phase 15 - Backend-Aware Snapshot Foundation

Status: Completed through Phase 15.9

Result:

- BackendRegistry JSON serialization
- BackendRegistry controller
- BackendRegistry REST API routing
- backend-aware snapshot access
- backend-aware snapshot read service
- backend-aware VDR controller methods
- backend snapshot API routes
- multi-snapshot cache foundation
- multi-backend snapshot cache access
- backend-aware snapshot builder

Verified with:

- make test-vdr-snapshot-builder
- make test-fast
- make daemon
- make test-domain-refresh-policy
- make test-snapshot-refresh-planner
- make test-polling-service
- make test-test-http-server
- GitHub Actions full regression
- GitHub Actions validation

---

## Phase 16 - Multi-Backend Polling Foundation

Status: Completed through Phase 16.9

Result:

- backend-aware snapshot cache service updates
- backend-aware polling service
- backend polling coordinator
- backend runtime context foundation
- daemon runtime context migration
- coordinator runtime integration
- runtime context collection
- runtime context factory
- runtime context creation from registry
- backend-aware snapshot change feed service

Verified with:

- make test-backend-runtime-context
- make test-backend-polling-coordinator
- make test-snapshot-change-feed
- make test-fast
- make daemon

---

## Phase 17 - Multi-Backend Snapshot Read and REST Visibility

Status: Completed through Phase 17.3

Result:

- multi-backend snapshot read foundation
- multi-backend snapshot summary serialization
- multi-backend snapshots REST endpoint
- `GET /api/vdr/snapshots` exposes snapshot summaries for all cached backend snapshots
- multi-backend snapshots REST endpoint test coverage

Verified with:

- make test-vdr-snapshot-read-service
- make test-vdr-snapshot-read-json-serializer
- make test-snapshot-access-service
- make test-api-router
- make test-fast
- make test-docs
- make test-architecture
- make test-phase
- make test-snapshot-change-feed-controller
- make test-api-router
- make daemon

---

## Phase 18 - Real VDR and RESTfulAPI Integration Validation

Status: Completed through Phase 18.4

Result:

- opt-in real RESTfulAPI integration validation
- opt-in real snapshot builder validation
- opt-in real change-state validation
- opt-in real polling initial snapshot validation
- opt-in real polling stability validation
- real VDR data verified through BasicHttpClient, RestfulApiVdrAdapter, VdrService, VdrSnapshotBuilder, PollingService, SnapshotCacheService and SnapshotRefreshPlanner
- repeated polling without VDR changes produces no change events and no refresh work

Verified with:

- make test-real-restfulapi-integration
- make test-real-snapshot-builder
- make test-real-change-state
- make test-real-polling-initial-snapshot
- make test-real-polling-stability
- make test-fast
- make test-docs
- make test-architecture
- make test-phase
- make test-snapshot-change-feed-controller
- make test-api-router
- make daemon

---

## Phase 19 - Snapshot Change Feed Validation

Status: Completed through Phase 19.3

Result:

- snapshot change feed append behavior is covered by service-level validation
- appendChanges assigns the next sequence number
- appendChanges preserves snapshot generation values
- appendChanges skips empty change event lists
- appendChanges preserves backend identity
- appendChanges preserves multiple changed domains in order
- polling-to-change-feed runtime flow is validated
- multi-backend change feed aggregation is implemented in DaemonRuntime
- all backend runtime contexts can append backend-aware feed entries
- SnapshotChangeFeedController exposes multi-backend feed entries through JSON
- ApiRouter validates `GET /api/vdr/changes` for multi-backend feed entries

Verified with:

- make test-snapshot-change-feed
- make test-polling-service
- make test-fast
- make test-docs
- make test-phase
- make test-snapshot-change-feed-controller
- make test-api-router
- make daemon

---

## Phase 20 - Live Transport Foundation

Status: Completed through Phase 20.9

Result:

- LiveUpdateEvent introduced
- SnapshotChangeFeedEntry can be converted into LiveUpdateEvent
- LiveUpdateEventJsonSerializer introduced
- ILiveTransport introduced with publish and read contract
- TestLiveTransport introduced for transport-neutral validation
- LiveTransportService introduced
- LiveTransportFactory introduced
- SseLiveTransport introduced
- LiveTransportController exposes the live transport stream
- ApiRouter exposes `GET /api/vdr/live`
- DaemonRuntime owns the SSE live transport and live transport controller
- DaemonRuntime publishes new snapshot change feed entries into the live transport
- backend-aware feed entries are converted into live update events during polling-to-feed processing

Verified with:

- make test-live-update-event
- make test-live-update-event-json-serializer
- make test-live-transport-interface
- make test-test-live-transport
- make test-live-transport-service
- make test-live-transport-factory
- make test-sse-live-transport
- make test-live-transport-controller
- make test-api-router
- make test-fast
- make test-docs
- make test-phase
- make daemon

---

## Phase 21 - Real VDR Runtime Polling Findings

Status: Completed through Phase 22.0

Result:

- real VDR runtime polling findings documented
- `/events.json` identified as a heavy domain on real systems
- naive cyclic full snapshot polling ruled out
- Events / EPG classified as a heavy domain
- future metadata, image and preview data classified as potential heavy domains
- RESTfulAPI event stream strategy prepared as an architecture step
- `VdrEventQuery` introduced as a backend-neutral selective event query contract
- `IVdrAdapter` supports query-based event access while preserving the existing full-event method
- `VdrService` supports selective event queries
- `RestfulApiVdrAdapter` maps selective event queries to existing RESTfulAPI event filters
- test adapters and RESTfulAPI adapter tests validate the new query contract
- real selective RESTfulAPI EPG validation completed
- selective EPG requests measured about 0.10 to 0.30 seconds instead of about 29.65 seconds for full `/events.json`
- `DomainRefreshPolicy` introduced as the heavy-domain refresh policy foundation
- Events / EPG are classified as a heavy domain
- `SnapshotRefreshPlanner` no longer creates automatic full EPG refresh work for `EventsChanged`

Architecture rule:

VDR-Suite should prefer selective backend queries over full-domain transfers whenever possible.

Heavy domains such as EPG, metadata, posters, fanart and preview data must not use full-domain runtime refreshes as the default strategy.

Preferred strategies are channel-scoped queries, time-window queries, object-specific queries and change-hint driven refreshes.

Performance goal:

Backend workload should remain comparable to established VDR frontends such as live whenever equivalent information is requested.

Verified with:

- make test-restful-api-vdr-adapter
- make test-vdr-service
- make test-backend-polling-coordinator
- make test-fast
- make test-docs
- make test-phase
- make daemon

---

## Phase 22 - Selective Event Refresh Runtime Architecture

Status: Completed through Phase 22.9

Result:

- selective event refresh plans introduced
- `VdrSnapshotBuilder` can build selective event domains
- `PollingService` can execute selective event refreshes
- `SnapshotCacheService` can merge selective event updates into existing snapshots
- selective event polling uses merge behavior instead of full replacement
- runtime measurements cover selective event refresh behavior
- `VdrCapabilitySet` and `CapabilityResolver` expose `events.read.selective`
- `DomainRefreshPolicy` can fall back to full event refresh when selective refresh is disabled
- `PollingService` accepts injected domain refresh policy

Verified with:

- make test-snapshot-update-plan
- make test-snapshot-refresh-planner
- make test-vdr-snapshot-builder
- make test-polling-service
- make test-snapshot-cache-service
- make test-capability-resolver
- make test-vdr-capability-set
- make test-fast
- make test-docs
- make test-phase

---

## Phase 23 - Capability-Derived Refresh Policy and Runtime Summaries

Status: Completed through Phase 23.1

Result:

- `DomainRefreshPolicy` can be derived from `ICapabilityResolver`
- selective event refresh decisions can now be capability-based
- `RuntimeDiagnosticsSummaryBuilder` aggregates runtime measurements by component and operation
- runtime measurement summaries can be produced without hard-coded selective/full refresh counters
- RESTfulAPI selective event URL generation, VDR service query forwarding and snapshot builder selective event access are already covered by existing tests

Verified with:

- make test-domain-refresh-policy
- make test-runtime-diagnostics-summary-builder
- make test-runtime-diagnostics-json-serializer
- make test-restful-api-vdr-adapter
- make test-vdr-service
- make test-vdr-snapshot-builder
- make test-fast
- make test-docs
- make test-phase

---

## Phase 24 - EPG Query Architecture

Status: Completed through Phase 24.5

Result:

- `EpgQueryFactory` introduced a fachliche EPG query layer above `VdrEventQuery`
- German and English project overview documents were added for external readers
- `EpgQueryScope` and `EpgQueryRequest` introduced backend-neutral EPG query intent types
- `IEpgQueryService` and `EpgQueryService` introduced a service boundary for future clients
- RESTfulAPI integration tests verify that `EpgQueryService` reaches `RestfulApiVdrAdapter` with the expected selective event URLs
- live RESTfulAPI verification against VDR confirmed channel-scoped EPG queries with `from`, `timespan`, `chevents` and `limit`
- live RESTfulAPI verification showed that `only_count=true` returns `count=0`, `total=<full channel total>` and an empty event list, so it must not be used as a reliable fachlicher EPG count path for now

Architecture rule:

Frontends should use `IEpgQueryService` instead of creating `VdrEventQuery` directly.

`only_count=true` must remain adapter-specific and should not be exposed as a fachlicher EPG query feature until its real backend behavior is clarified.

Verified with:

- make test-epg-query-factory
- make test-epg-query-service
- make test-epg-query-service-restfulapi
- make test-fast
- make test-docs
- make test-phase
- live RESTfulAPI `info.json`
- live RESTfulAPI channel EPG query with `from` and `timespan`
- live RESTfulAPI channel EPG query with `chevents=2`
- live RESTfulAPI channel EPG query with `limit=5`
- live RESTfulAPI channel EPG query with `only_count=true`

---

## Next Work

The next work should expose the EPG query service through a REST API boundary.

Goals:

- add a REST-facing EPG controller boundary
- keep REST controllers independent from RESTfulAPI query parameters
- route public EPG requests through `IEpgQueryService`
- prepare `GET /api/vdr/epg` and channel-scoped EPG routes
- preserve backend-neutral EPG query behavior

---

---

## Phase 30 - Recording Actions
Status: Completed through Phase 30.4

Result:
- Phase 30.0 documented the recording actions architecture boundary
- Phase 30.1 introduced the backend-aware recording action request domain model
- Phase 30.2 introduced the recording action validation result domain model
- Phase 30.3 introduced the recording action plan domain model
- Phase 30.4 introduced the recording action job payload domain model
- destructive execution remains out of scope
- action requests carry backend identity, backend-owned recording identity, action type, dry-run flag and parameters
- validation results carry validity state, dry-run state, job planning intent, required capabilities, required permissions, warnings and errors
- action plans carry backend identity, recording identity, action type, dry-run state, execution allowance, job creation intent and planned job type
- job payloads carry backend identity, recording identity, action type, job type, dry-run state, parameters, required capabilities, required permissions and warnings
- move, delete and metadata refresh action types are now represented in the action type model
- Rectools execution remains behind future adapter or job boundaries

Verified with:
- make test-recording-action
- make test-docs
- make test-phase

Architecture decision:
Recording actions must remain backend-aware request, validation, plan and job-payload models until capability checks, permission checks and execution boundaries are implemented.

Follow-up:
The next implementation step is the Recording Action RestfulAPI Dry Run Enforcement.

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
---

## Phase 25 - EPG REST API Boundary

Status: Completed through Phase 25.5

Result:

- EPG REST controller boundary introduced
- EPG REST routes added for Now/Next, TimeWindow and ChannelWindow reads
- REST query string boundary added
- EPG controller query parameters added
- REST query parameter parser added
- EPG query parameters routed through the REST layer
- live EPG REST validation completed against a real VDR
- `/api/epg/now-next` returns real EPG events
- `/api/epg/time-window` returns real EPG events
- `/api/epg/channel-window` returns real EPG events
- selective backend query paths are verified through the adapter and service chain
- daemon startup still loads the legacy full EPG snapshot through `/events.json`; this remains a known heavy-domain follow-up

Verified with:

- make test-rest-query-parameters
- make test-epg-controller
- make test-api-router
- make test-test-http-server
- make daemon
- live daemon test on `127.0.0.1:18080`
- real RESTfulAPI backend on `127.0.0.1:8002`

Live VDR observations:

```text
/api/epg/now-next       -> real EPG events returned
/api/epg/time-window    -> real EPG events returned
/api/epg/channel-window -> real EPG events returned

/events.json?timespan=7200&chevents=2       -> about 440 KB, about 251 ms
/events.json?from=...&timespan=7200         -> about 778 KB, about 432 ms
/events.json?from=...&timespan=7200&limit=5 -> accepted, about 778 KB, about 460 ms
startup /events.json full EPG snapshot      -> about 33 MB, about 34 s
```

---

## Phase 26 - EPG Snapshot Decoupling

Status: Completed through Phase 26.0

Result:

- initial polling no longer builds a full EPG snapshot
- `VdrSnapshotBuilder::buildSnapshotWithoutEvents()` was introduced
- `PollingService` now uses an event-free initial snapshot
- `VdrSnapshotBuilder::buildSnapshot()` still builds a complete snapshot when explicitly requested
- EPG remains available through selective REST APIs
- the daemon startup path no longer blocks on the legacy full `/events.json` transfer
- polling tests were updated to treat Events / EPG as absent from the initial snapshot
- full event refresh remains available for non-selective fallback paths
- selective event refresh remains available for EventsChanged handling

Verified with:

- make test-polling-service
- make test-vdr-snapshot-builder
- make test-backend-polling-coordinator
- make test-api-router
- make daemon

Architecture note:

The initial runtime snapshot now contains status, recordings, timers and channels.
Events / EPG are intentionally excluded from the initial snapshot and should be accessed through selective EPG APIs or explicit refresh paths.

---

## Phase 26.1 - EPG Snapshot Decoupling Validation

Status: Completed

Result:

- event-free initial snapshot was validated against a real VDR
- daemon startup no longer performs the blocking full `/events.json` load
- startup now loads status, recordings, timers and channels only
- EPG remains available through `/api/epg/now-next`, `/api/epg/time-window` and `/api/epg/channel-window`
- live measurements support treating EPG as a query domain
- full EPG remains too heavy for daemon startup
- 24 hour EPG windows remain possible but are significantly heavier than short query windows
- `limit=5` and `limit=20` are accepted but did not reduce the observed RESTfulAPI response size
- EPG REST output exposed a JSON escaping defect because `jq` rejected control characters in response strings

Live VDR measurements:

- `now-next`: about 265 KB, about 0.333 s
- `time-window` 2h: about 469 KB, about 0.587 s
- `time-window` 24h: about 3.5 MB, about 6.092 s
- `channel-window` limit 5: about 469 KB, about 0.602 s
- `channel-window` limit 20: about 469 KB, about 0.588 s

Architecture decision:

EPG / Events should remain outside the initial runtime snapshot.
EPG should be accessed through selective query APIs by default.
A global EPG cache should not become the default runtime strategy.

Follow-up:

The next implementation step should fix JSON escaping for EPG REST response strings.

---

## Phase 26.2 - EPG JSON Escaping

Status: Completed

Result:

- EPG REST string serialization now escapes JSON strings
- quotes, backslashes, newlines, carriage returns and tabs are escaped
- remaining control characters below 0x20 are replaced with spaces
- `/api/epg/now-next` now produces jq-compatible JSON
- EPG REST APIs remain unchanged at the route level
- the fix is local to the EPG REST controller

Verified with:

- make test-epg-controller
- make test-api-router
- make daemon
- live `/api/epg/now-next` request against a real VDR
- `jq '.events | length' /tmp/epg-now-next-fixed.json`

Live VDR validation:

- `/api/epg/now-next` returned valid JSON
- `jq '.events | length'` returned `617`

Follow-up:

The next implementation step should evaluate whether JSON escaping should be shared by VDR REST serializers instead of being local to the EPG controller.

---

## Phase 26.3 - EPG JSON Helper Cleanup

Status: Completed

Result:

- EPG REST JSON key serialization was cleaned up
- `appendKey()` was introduced locally in `EpgController.cpp`
- repeated manual key output was replaced by a helper
- existing `appendQuoted()` JSON escaping remains the single local string writer
- no shared serializer utility was introduced because repository search showed no broad duplication
- EPG REST API routes and response shape remain unchanged

Verified with:

- make test-epg-controller
- make test-api-router
- make daemon

Architecture decision:

Serializer consolidation is not justified yet because the duplicated escaping problem is currently local to the EPG controller.
The next architectural focus can move back to recording query design instead of introducing premature JSON infrastructure.

---

## Phase 27.0 - Backend Optional Runtime

Status: Completed

Result:

- daemon runtime no longer fails initialization only because no VDR backend is available
- backend polling is skipped when no backend runtime contexts exist
- EPG query service and EPG controller wiring are skipped when no backend runtime context exists
- `ApiRouter` accepts an optional `EpgController`
- `/api/epg/...` routes return HTTP 503 with a JSON error when no backend-backed EPG service is available
- backend registry controller coverage now includes an empty backend registry
- test HTTP server wiring was updated for the optional EPG controller contract

Verified with:

- make test-api-router
- make test-backend-registry-controller
- make test-test-http-server
- make daemon
- GitHub Actions full regression

Architecture decision:

The daemon must be able to start as an empty runtime platform before a VDR backend is configured or available.

Backend-specific domains such as EPG must be represented as unavailable instead of crashing runtime initialization.

Follow-up:

The next implementation step should define capability-aware backend and recording query behavior above the backend-optional runtime foundation.

---

## Phase 29 - Backend Recording Query Support

Status: Completed through Phase 29.2

Result:

- `VdrRecording` now carries backend identity.
- `VdrSnapshotBuilder` propagates backend identity into recording domain objects.
- recording query JSON responses expose `backendId`.
- `GET /api/vdr/recordings/query` supports backend filtering through the `backend` query parameter.
- `backend=default` remains compatible with legacy single-backend recordings that do not yet carry an explicit backend ID.

Verified with:

- make test-vdr-domain-objects
- make test-vdr-snapshot-builder
- make test-vdr-recording-query-result-json-serializer
- make test-vdr-recording-query-matcher
- make test-vdr-recording-query-controller
- make test-api-router
- make test-docs
- make test-phase

Architecture decision:

Recording query behavior is now backend-aware without changing the legacy `/api/vdr/recordings` output. Backend-aware recording filters are implemented at the recording query boundary and preserve single-backend compatibility.

Follow-up:

The next implementation step should document backend-aware recording query behavior before moving toward recording actions.

---

## Phase 28.12 - Recording Query API Documentation

Status: Completed

Result:

- capability-aware runtime foundation documented through Phase 27.11
- capability stack validated through the real VDR-Suite daemon
- `/api/vdr/capabilities` confirmed through `127.0.0.1:18080`
- real RESTfulAPI backend confirmed through `127.0.0.1:8002`
- real runtime output observed 342 channels and 973 recordings
- startup observation identified `recordings.json` as the dominant startup payload at about 4.3 MB and about 1.8 seconds

Verified with:

- make daemon
- curl `/api/vdr/capabilities`
- curl `/api/vdr/overview`
- curl `/api/vdr/status`
- make test-docs
- make test-phase

Architecture decision:

Phase 27.x completed the capability-aware API foundation. The next implementation focus should move to recording query architecture, because real VDR validation shows recordings are the dominant startup payload and need query-oriented boundaries.
---

## Phase 28 - Recording Query API

Status: Completed through Phase 29.2

Result:

- `VdrRecordingQuery` introduced a backend-neutral recording query contract
- `VdrRecordingQueryResult` introduced a result boundary with total count, returned count, limit and offset
- `VdrRecordingQueryService` provides the service boundary above `VdrService`
- `VdrRecordingQueryMatcher` centralizes title, path, start-time and duration filtering
- `VdrRecordingQueryResultJsonSerializer` serializes query results for REST use
- `VdrRecordingQueryController` exposes recording query reads through REST
- `ApiRouter` routes `GET /api/vdr/recordings/query`
- query parameters include `title`, `path`, `from`, `to`, `durationMin`, `durationMax`, `sort`, `order`, `limit` and `offset`
- sort fields include `title`, `startTime`, `duration` and `size`
- backend-specific recording filtering was intentionally deferred because `VdrRecording` does not yet carry backend identity

Verified with:

- make test-vdr-recording-query
- make test-vdr-recording-query-result
- make test-vdr-recording-query-service
- make test-vdr-recording-query-matcher
- make test-vdr-recording-query-result-json-serializer
- make test-vdr-recording-query-controller
- make test-api-router
- make test-docs
- make test-phase

Architecture decision:

Recording query behavior is now a backend-neutral API layer. Backend-specific recording filters must wait until backend identity is represented in the `VdrRecording` domain model.

Follow-up:

The next implementation step should define multi-backend recording identity before adding `backend=` query filters.

