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

Status: Completed through Phase 13.7e

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

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
