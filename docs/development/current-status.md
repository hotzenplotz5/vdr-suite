# VDR-Suite Current Project Status

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)

---

## Purpose

This document tracks the current verified technical state of VDR-Suite.

It should stay focused on the present state.

Implementation history belongs in [Completed Phases](completed-phases.md).

Future planning belongs in [Roadmap](../planning/roadmap.md).

---

## Project

VDR-Suite

Goal:

Modern service-oriented backend architecture for VDR recordings, metadata management, job processing, dashboard services, JSON export, REST APIs, Web UI, OSD integration and future integration of VDR-Rectools.

VDR remains the primary backend domain and source of truth.

---

## Current Branch

```text
main
```

---

## Current Verified State

Latest completed implementation phase:

```text
Phase 24.5 - Live RESTfulAPI EPG Verification
```

Current major phase status:

```text
Phase 20 live transport foundation is complete through 20.9.
Phase 21.0 documents real VDR runtime polling and EPG performance findings.
Phase 21.1 documents RESTfulAPI event streams as optional backend-specific change hint sources.
Phase 21.2 introduces selective event query support through the VDR adapter boundary.
Phase 21.3 validates selective RESTfulAPI EPG access against a real VDR.
Phase 22.0 introduces the heavy-domain refresh policy foundation.
```

Verified locally with:

```text
make test-restful-api-vdr-adapter
make test-vdr-service
make test-backend-polling-coordinator
make test-fast
make test-docs
make test-phase
make daemon
```

Verification summary:

- selective event query contract compiles through the adapter, service and test layers
- `RestfulApiVdrAdapter` can map selective event queries to existing RESTfulAPI event filters
- test adapters implement the selective event query method
- `make test-fast` passes with the new adapter contract
- documentation phase consistency remains green
- daemon build remains green
- GitHub Actions remains the standard full regression path for normal non-VDR-specific changes

---

## Current Architecture Highlights

- VDR remains the primary backend domain and source of truth.
- Snapshot-based read architecture is completed for the current domain set.
- Snapshot read APIs are available for status, channels, timers, events and recordings.
- Snapshot cache, snapshot access and partial refresh planning are in place.
- Snapshot cache generation tracking is implemented in `SnapshotCacheService`.
- Snapshot change feed service, serializer and read-only REST controller are implemented.
- Snapshot change feed entries can be appended to an existing runtime-owned feed.
- Daemon runtime owns the snapshot change feed and updates it after VDR polling.
- Runtime diagnostics are integrated through structured runtime measurement boundaries.
- Backend identity is present in snapshot change feed entries, snapshot read metadata and cached snapshots.
- `BackendNode` and `BackendRegistry` provide the backend identity foundation.
- `BackendRegistryService`, `BackendRegistryJsonSerializer` and `BackendRegistryController` expose the registry through service and REST boundaries.
- `ApiRouter` exposes backend registry routes, backend-aware snapshot routes and the multi-backend snapshots route.
- `SnapshotCache` can store snapshots per backend while preserving the legacy single-snapshot interface.
- `SnapshotAccessService` resolves snapshots through the multi-backend cache and can return all cached backend snapshots.
- `VdrSnapshotReadService` supports backend-aware reads and multi-backend snapshot list reads.
- `VdrSnapshotReadJsonSerializer` can serialize multi-backend snapshot summary lists.
- `VdrController` exposes default VDR reads, backend-specific reads and multi-backend snapshot summary reads.
- `ApiRouter` regression coverage verifies the multi-backend snapshots REST route.
- `VdrSnapshotBuilder` can assign a stable backend ID to generated snapshots.
- `PollingService` and `BackendPollingCoordinator` support backend-aware polling coordination.
- `DaemonRuntime` creates backend runtime contexts from the backend registry.
- Future multi-VDR runtime configuration can build on the registry-driven context creation model.
- Backend identity, federation, capability and lifecycle strategy are documented through ADRs.
- `VdrEventQuery` provides the first backend-neutral selective EPG query contract.
- `IVdrAdapter` and `VdrService` support query-based event access while preserving legacy full-event access.
- `RestfulApiVdrAdapter` maps selective event queries to existing RESTfulAPI query parameters.
`DomainRefreshPolicy` classifies Events / EPG as a heavy domain.
`SnapshotRefreshPlanner` no longer creates automatic full EPG refresh work for EventsChanged.

---

## Selective Backend Query Rule

VDR-Suite should prefer selective backend queries over full-domain transfers whenever possible.

Heavy domains must not use full-domain runtime refreshes as the default strategy.

Heavy domains currently include:

- EPG
- metadata
- posters
- fanart
- preview data
- scraper-derived data

Preferred runtime strategies are:

- channel-scoped queries
- time-window queries
- object-specific queries
- change-hint driven refreshes

Performance goal:

Backend workload should remain comparable to established VDR frontends such as live whenever equivalent information is requested.

---

## Current Implemented API Areas

Snapshot-backed default VDR read APIs:

```text
GET /api/vdr/status
GET /api/vdr/health
GET /api/vdr/snapshot
GET /api/vdr/snapshots
GET /api/vdr/capabilities
GET /api/vdr/channels
GET /api/vdr/timers
GET /api/vdr/events
GET /api/vdr/recordings
GET /api/vdr/changes
GET /api/vdr/live
```

Backend registry and backend-aware read APIs:

```text
GET /api/backends
GET /api/backends/default
GET /api/backends/{backendId}/status
GET /api/backends/{backendId}/health
GET /api/backends/{backendId}/snapshot
```

Existing application APIs:

```text
GET /api/dashboard
GET /api/jobs
GET /api/recordings
GET /api/metadata
GET /api/runtime/diagnostics
GET /api/runtime/summary
```

---

## Current Test Runtime Observation

The full local regression target is intentionally no longer the default verification path for normal development work.

Recommended local pre-commit verification for architecture work:

```text
make test-fast
make test-docs
make test-architecture
make test-phase
make daemon
```

Project workflow:

```text
GitHub-first
CI-first
```

Targeted local tests remain useful for narrow changes and for real VDR validation.

Real VDR tests are reserved for:

- RESTfulAPI against an actual VDR
- SSE event streams
- polling against an actual VDR
- snapshot runtime against an actual VDR
- multi-VDR or multi-server scenarios
- selective RESTfulAPI EPG validation

---

## Next Technical Focus

```text
Phase 25.0 - EPG REST API Boundary
```

The next step is to introduce domain-aware selective EPG refresh planning on top of the heavy-domain policy.

Validation should compare selective RESTfulAPI event queries against full `/events.json` in terms of response size, HTTP time and returned event count.

Important boundaries:

- existing RESTfulAPI filters must be measured before new endpoints are proposed
- full EPG refresh must not become the default runtime strategy
- selective query behavior must remain behind backend-neutral adapter boundaries
- real VDR validation should continue to use explicit opt-in environment variables
- GitHub Actions must remain independent from a running VDR

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
