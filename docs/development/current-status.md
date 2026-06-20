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
Phase 46.34 - Real VDR Person Metadata Validation
```

Current major phase status:

```text
Phase 45 completed the EPG search API block through architecture, request, matcher, result, JSON contract, service, controller, REST validation and API documentation.

Phase 46.0 adds ADR-0028 as the content classification architecture decision.

Phase 46.7 documents the current genre architecture.

Next implementation focus:
Phase 46.8 - Content Rating Domain Foundation
```

Verified locally in the preceding implementation phases with targeted EPG search tests, documentation checks, phase consistency checks and daemon build validation. Direct GitHub documentation synchronization should still be followed by local `make test-docs` and `make test-phase` after pulling.

Verification summary:

- VDR remains the primary backend domain and source of truth.
- Selective EPG REST APIs are implemented for Now/Next, TimeWindow and ChannelWindow reads.
- `/api/epg/search` is implemented as an additive search endpoint over selective EPG windows.
- EPG search has backend-neutral request, matcher, service, result and JSON contract foundations.
- EPG search REST validation rejects invalid `timespan`, `limit`, `offset`, `sort` and `order` values.
- The implemented EPG Search API is documented in [EPG Search API](./epg-search-api.md).
- ADR-0028 documents the future content classification architecture and explicitly avoids modeling genre as a single plain string.
- Phase 46.1 introduces the first source-aware genre domain foundation with `GenreClassification`, `GenreCollection` and `ContentClassificationSource`.
- Phase 46.2 introduces `GenreResolver` and `GenreResolutionResult` as the first deterministic source resolution model while preserving all genre evidence.
- Phase 46.3 introduces the genre resolution JSON contract with canonical genre IDs, original provider values and evidence serialization.
- Phase 46.4 introduces `CanonicalGenreRegistry` as the first mapping layer from multilingual provider labels to stable canonical genre IDs.
- Phase 46.5 introduces `GenreLocalization` as the first German and English label layer for canonical genre IDs.
- Phase 46.6 introduces localized genre resolution JSON while preserving stable canonical IDs.
- Phase 46.7 documents the complete genre architecture, including canonical IDs, localized labels and future language file layout.
- Phase 46.12 documents the current content rating API and JSON contract.
- Phase 46.17 documents the current person metadata API and JSON contract.
- Content classification is planned as source-aware evidence for genres, content ratings, keywords, collections, user tags and folder hints.
- Future genre, FSK/content-rating, profile, policy and TV frontend work should build on ADR-0028.
- Documentation phase consistency should remain aligned around Phase 46.7 as latest completed phase and Phase 46.8 as next focus.

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
- Recording actions use backend-native recording identity through `backendNativeId`.
- Recording rename, delete and move are validated end-to-end against a real VDR.
- `DomainRefreshPolicy` classifies Events / EPG as a heavy domain.
- `SnapshotRefreshPlanner` no longer creates automatic full EPG refresh work for EventsChanged.
- EPG search operates over selective event windows and does not require a persistent full EPG mirror.
- ADR-0028 defines source-aware content classification for future genre, rating, metadata and policy work.
- Genre architecture now uses stable canonical IDs, source evidence and localized labels.

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
GET /api/vdr/recordings/query
GET /api/vdr/changes
GET /api/vdr/live
```

Selective EPG REST APIs:

```text
GET /api/epg/now-next
GET /api/epg/now-next?channelId={channelId}&from={unixTime}
GET /api/epg/time-window?channelId={channelId}&from={unixTime}&timespan={seconds}
GET /api/epg/channel-window?channelId={channelId}&from={unixTime}&timespan={seconds}&limit={count}
GET /api/epg/search?query={text}&channelId={channelId}&from={unixTime}&timespan={seconds}&limit={count}&offset={count}&sort={field}&order={asc|desc}
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
make test-docs
make test-phase
make daemon
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
Phase 46.35 - Recording Character Search
```

The next step is to extend content classification from genres toward content ratings such as FSK-style age classification.

Important boundaries:

- keep VDR as the source of truth for VDR-owned state
- keep classification as enrichment and evidence, not a replacement for VDR data
- preserve source identity for genres and later content ratings
- avoid persistent full EPG mirroring
- avoid policy enforcement before a dedicated policy architecture exists
- keep metadata provider details behind provider boundaries

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
