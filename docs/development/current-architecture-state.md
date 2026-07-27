# VDR-Suite Current Architecture State

## Purpose

This document describes implemented architecture on current `main`. Target contracts that are accepted but not implemented remain in ADRs and the target architecture; open gaps remain in the Architecture Audit Gap Matrix.

Baseline: `44ae3102ab202ee0dfc974ee0bc9624b9219ad2d` on 2026-07-27.

## Ownership model

```text
VDR
  -> native devices, schedules, timers, recordings, replay, OSD and plugin execution

VDR-Suite
  -> backend identity and scope
  -> domain services and policy
  -> persistent read models
  -> guarded operations
  -> client-facing REST and VdrSuiteClientApi contracts

Private adapters/providers
  -> RESTfulAPI, SVDRP, Streamdev, TVScraper, SuiteBridge
```

Frontend modules do not call private backend protocols or provider databases directly.

## Backend and snapshot foundation

```text
BackendNode
  -> BackendRegistry
  -> BackendRegistryService
  -> backend-scoped adapter/service calls
  -> backend-scoped snapshots and caches
  -> change detection and change feed
  -> Suite REST / Client API
```

Implemented properties include stable backend IDs, backend-scoped reads, capability reporting, server-enforced read-only access mode, snapshot/change sequencing and SSE/live-update foundations. Secure remote Agent identity, generation, lease and command fencing remain Phase 63 work.

## Recordings 2 architecture

Recordings are a lazy, on-demand domain and are not synchronously loaded for every backend at daemon startup.

```text
backend-scoped Recording fetch
  -> Recording cache worker
  -> vdr_recording_cache and native metadata/person relations
  -> Recording repositories/services/controllers
  -> VdrSuiteClientApi
  -> Recordings 2 browser/cards/details/actions
```

Recordings 2 is the sole delivered Recording browser and owns folder state, cards, detail navigation, metadata, people, artwork, Genre integration and guarded rename/move/trash workflows. Legacy Recording browser scripts are historical and must not be reintroduced as a second owner.

## Metadata and Genre architecture

```text
Recording cache worker / EPG cache worker
  -> backend-scoped metadata target bindings
  -> native, provider and derived evidence
  -> canonical Genre identities and assignment states
  -> indexed Genre read model

Genre GET
  -> GenreBrowserApiRuntime
  -> controller/service/repository
  -> dedicated query-only SQLite connection
  -> VdrSuiteClientApi
  -> Genre frontend
  -> existing Recordings 2 / EPG detail owner
```

Phase 61 implemented persistence, people relations, Genre aliases, unknown/unclassified values, multiple Genres, explicit active/missing/unknown/stale/conflict states, EPG browse classes and backend isolation. Provider acquisition is asynchronous. Normal Genre reads do not call TVScraper, TMDB, IMDb, RESTfulAPI, SVDRP or SuiteBridge.

## EPG architecture

The EPG runtime separates authoritative backend cache refresh from metadata enrichment:

- backend/channel/time-scoped event cache;
- bounded authoritative reconciliation of disappeared native event IDs;
- persistent TVScraper public metadata and normalized people relations;
- persistent Genre/media-type/browse-class evidence;
- existing EPG timeline and detail owner;
- query-only Genre and global-search reads.

Suite-owned cache identity remains backend-native and backend-scoped. Similar title/time values are not silently promoted to canonical cross-provider event identity. A richer canonical ProgramEvent/provenance model remains partial and is required for later TimerIntent orchestration.

## Global search architecture

```text
Frontend search dialog
  -> VdrSuiteClientApi.fetchClientGlobalSearch()
  -> GET /api/search
  -> GlobalSearchApiRuntime
  -> GlobalSearchController
  -> GlobalSearchService
  -> GlobalSearchRepository
  -> existing SQLite database with PRAGMA query_only=ON
```

The implemented first slice searches one selected backend across persisted Recording and EPG titles, subtitles and people. It uses bounded EPG time windows, independent set-based title/person candidates, deterministic pagination and no provider resolution. Recordings 2 and the existing EPG detail owner remain the destination owners.

## Remote and live-overlay architecture

```text
Frontend remote module
  -> VdrSuiteClientApi
  -> LiveRemoteApiRuntime
  -> backend-neutral RemoteAction / LiveOverlay service contracts
  -> private backend adapter
```

Read-only/capability checks remain server-backed. PR #110 isolates pressed-state to one button and uses an internal in-flight guard rather than globally disabling keys. Streaming and legacy OSD are separate future domains and are not implied by the existing live overlay.

## SearchTimer and Timer architecture

Implemented foundations include SearchTimer domain values, discovery, preview cache, validation, RESTfulAPI command mapping, native capability handling and controlled mutation/readback paths. These do not yet constitute central multi-backend Timer intent orchestration.

Phase 64 must introduce durable `TimerIntent`, `TimerAssignment`, `NativeTimerBinding`, scheduler and reconciler semantics after identity/accountability and Agent prerequisites.

## Current persistence boundaries

- SQLite remains the central Suite-owned metadata/read-model database.
- Domain repositories own SQL; controllers and frontend modules do not.
- Dedicated query-only connections serve Genre and global-search GET paths where configured.
- Provider databases and filesystem paths are not public Suite identities.
- Provider JSON may be normalized during asynchronous persistence, not repeatedly parsed from normal search GET paths.
- ADR-0050 defines the domain-repository SQLite boundary for future work.

## Implemented safety boundaries

- backend scope on persisted/query paths;
- server-enforced read-only backend mode;
- guarded Recording validation/preview/execution/readback;
- allowlisted remote actions;
- operation IDs and duplicate-dispatch prevention in the current remote flow;
- no frontend-owned authorization;
- no provider lookup during documented query-only reads;
- bounded native/plugin work before asynchronous database/network processing.

## Important incomplete architecture

| Area | Current state | Roadmap owner |
| --- | --- | --- |
| Actor identities and scoped RBAC | Missing production model | Phase 62 |
| Append-only accountability/outbox | Missing | Phase 62 |
| Secure Backend Agent lifecycle | Contract/foundation only | Phase 63 |
| Universal revision/idempotency | Partial | Phase 62/63 mutation gates |
| Durable job claims/retry/sagas | Partial | Phase 62/63 foundations |
| TimerIntent orchestration | Missing | Phase 64 |
| Streaming Gateway | Missing | Phase 65 |
| Legacy OSD bridge | Missing | Phase 66 |
| Stable `/api/v1` and compatibility | Partial | Phase 67 |
| Recommendations / knowledge graph | Not implemented | Phase 68 |

## Related documents

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Architecture Index](../architecture/index.md)
- [Metadata-Backed Genre Browser](../architecture/metadata-genre-browser.md)
- [Backend-Scoped Global Search](../architecture/global-search.md)
- [Live Remote and OSD Contract](../architecture/live-remote-osd-contract.md)
- [Architecture Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Strict Roadmap](../planning/roadmap.md)