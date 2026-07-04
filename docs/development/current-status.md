# VDR-Suite Current Project Status

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)
- [Phase Map](../planning/phase-map.md)
- [Roadmap](../planning/roadmap.md)
- [Lazy Recording Loading](../planning/lazy-recording-loading.md)
- [Startup Snapshot Runtime Rule](startup-snapshot-runtime.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](completed-phases.md)

---

## Purpose

This document tracks the current verified technical state of VDR-Suite.

Implementation history belongs in [Completed Phases](completed-phases.md).

Future planning belongs in [Roadmap](../planning/roadmap.md).

---

## Latest Verified Implementation Slice

Status: abgeschlossen.

```text
Phase 58.90b - Stable Channel Sorter
```

Commit:

```text
2f66168d
```

Tag:

```text
v1.58.90b-stable-channel-sorter
```

Verified result:

- backend channel move API exists and is guarded by backend access policy
- frontend module `Kanäle sortieren` is available
- channel drag works on desktop and touch devices
- dragging starts only on the left `↕` handle
- normal vertical scrolling remains possible
- the experimental post-move focus restore is intentionally not part of the stable state

---

## Project

VDR-Suite is a service-oriented backend architecture for VDR recordings, metadata management, job processing, dashboard services, JSON export, REST APIs, Web UI, OSD integration and future integration of VDR-Rectools.

VDR remains the primary backend domain and source of truth.

---

## Current Branch

```text
main
```

---

<!-- PROJECT_PROGRESS_START -->
## Project State Snapshot

This is a verified implementation-state snapshot, not a product-completion percentage.

### Verified foundations

- Core runtime and daemon foundation
- VDR backend adapter and RESTfulAPI integration foundation
- Backend registry and multi-backend runtime foundation
- Snapshot cache, snapshot access and change-feed foundation
- REST routing and JSON response boundaries
- Recording query foundation
- Recording action validation and guarded execution foundation
- Selective EPG query and EPG search foundation
- Backend-scoped persistent EPG database foundation
- Content classification and person metadata foundations
- Recording person and character search foundations
- SearchTimer backend, validation, planning and workflow foundations
- SearchTimer safety gates, readback verification and production mutation policy foundations
- Live parity discovery foundation
- Real VDR acceptance manifest and runner foundation
- Daemon lifecycle hardening for duplicate bind failures and SIGTERM shutdown
- Recording operations audit and safety policy foundation
- RESTfulAPI event stream change-hint foundation
- Channel move API and stable frontend channel sorter foundation

### Verified real-runtime evidence

- Real VDR acceptance currently passes 20/20 safe and dry-run probes.
- Duplicate daemon start on an occupied HTTP port exits cleanly with status 1 instead of aborting.
- SIGTERM stops the daemon cleanly without `kill -9` and releases port 18080.
- GitHub Actions verification is required before runtime-related phases are considered complete.
- Phase 58.39 verifies bounded live EPG input for channel cards via the now-next EPG route.
- Phase 58.58 verifies RESTfulAPI SSE-driven change hints: vdr-suite connects to RESTfulAPI `/eventstream` on `vdr_port + 1`, receives `vdr-change` hints and turns them into `/api/vdr/changes` entries for timers and recordings.
- Phase 58.90a verifies guarded channel move API coverage.
- Phase 58.90b verifies stable channel sorting on desktop and touch devices.

### Guarded or deliberately incomplete areas

- SearchTimer production mutation remains gated and closed by default.
- Recording operations real-backend write probes remain explicitly gated.
- Lazy recording loading is still a required follow-up for large real recording catalogs and multi-backend scaling.
- The persistent EPG database foundation is present, but EPG synchronization service, daemon scheduling and frontend consumers are not wired to it yet.
- Suite-owned metadata database and external scraper/provider strategy are planned but not yet implemented as the final metadata product.
- Authentication, authorization, per-backend permissions and read-only secondary-site policy are still planned beyond the current access-mode foundation.
- Web, Windows, Android, iOS and TV frontends remain planned product layers; the current web frontend is a Phase 58 foundation, not the final client product.

### Current active focus

```text
Phase 58 - Frontend and Live Parity
```

### Later strategic milestones

- Multi-site backend federation and permission hardening
- Frontend and live-parity foundation
- EPG synchronization service and SSE/change-state triggered background synchronization
- Suite metadata database and external provider integration
- Safe production-grade recording operations

Progress source: ../planning/project-progress.md
<!-- PROJECT_PROGRESS_END -->

---

## Phase 58.90b - Stable Channel Sorter

Status: implemented, runtime-tested, committed and tagged.

Commit:

- `2f66168d` Phase 58.90b: add stable channel sorter

Tag:

- `v1.58.90b-stable-channel-sorter`

Summary:

- Added the frontend module `Kanäle sortieren`.
- Channel sorting is separated from the normal channel browser.
- Drag and drop uses pointer events and works on desktop and touch devices.
- Drag starts only on the left `↕` handle so normal list scrolling remains available.
- Channel movement uses the guarded backend move API.
- The experimental post-move focus restore is deliberately excluded from the stable state.

Runtime proof:

- Manual touch test passed on Android.
- Manual desktop test passed.
- A later focus-restore experiment was rolled back and is not part of the tagged state.

---

## Phase 58.90a - Channel Move API

Status: abgeschlossen.

- Backend-API fuer Kanalverschiebung ergaenzt.
- SVDRP MOVC <sourceNumber> <targetNumber> wird ueber VDR-Suite gekapselt.
- Dry-Run und reale reversible Verschiebung wurden erfolgreich getestet.
- Read-only Backends werden durch BackendAccessPolicy blockiert.
- Next stable frontend slice: Phase 58.90b stable channel sorter.

---

## Phase 58.58 - RESTfulAPI Event Stream Change Hints

Status: implemented, runtime-tested, committed and tagged.

Commit:

- `361d0e9f` Phase 58.58: add RESTfulAPI event stream change hints

Tag:

- `v1.58.58-restfulapi-eventstream-change-hints`

Summary:

- Added `RestfulApiEventStreamClient`.
- The client connects to RESTfulAPI `/eventstream` on `vdr_port + 1`.
- Incoming `vdr-change` SSE events set an atomic external change hint.
- The existing HTTP listener tick consumes that hint and triggers `pollVdrAndUpdateChangeFeed()`.
- Snapshot and change-feed mutation remain on the existing runtime path.
- The previous 5-second polling path remains active as fallback.

Runtime proof:

- vdr-suite connected to RESTfulAPI event stream on `127.0.0.1:8003`.
- vdr-suite logged RESTfulAPI event stream change hints.
- `/api/vdr/changes` produced new entries for `timers` and `recordings`.

---

## Current Verified State

Latest completed major implementation block:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

Latest completed Phase 58 implementation slice:

```text
Phase 58.90b - Stable Channel Sorter
```

Current documentation consolidation state:

```text
Phase 58.90c - Documentation Consolidation
```

Current implementation focus:

```text
Phase 58 - Frontend and Live Parity
```

Required planned follow-up:

```text
Continue frontend/live-parity consolidation after the stable channel sorter.
```

Completed foundations:

```text
Core Platform Foundation
VDR Backend Foundation
Multi-Backend Foundation
Snapshot Runtime Foundation
Change Feed Foundation
Live Transport Foundation
Recording Query Foundation
Recording Action Safety Foundation
SearchTimer Backend and Workflow Foundation
Multi-Site Backend Administration and Permissions
Stable Channel Move and Sorter Slice
```
