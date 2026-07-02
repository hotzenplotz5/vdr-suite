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
- Installed systemd unit foundation for vdr-suite-daemon

### Verified real-runtime evidence

- Real VDR acceptance currently passes 20/20 safe and dry-run probes.
- Duplicate daemon start on an occupied HTTP port exits cleanly with status 1 instead of aborting.
- SIGTERM stops the daemon cleanly without `kill -9` and releases port 18080.
- GitHub Actions verification is required before runtime-related phases are considered complete.
- Phase 58.39 verifies bounded live EPG input for channel cards via the now-next EPG route.

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

## Current Verified State

Latest completed major implementation phase:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

Current documentation consolidation state:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

Next major implementation milestone:

```text
Phase 58 - Frontend and Live Parity
```

Required planned follow-up:

```text
Frontend and Live Parity
```

Completed foundations:

```text
Core Platform Foundation
VDR Backend Foundation
Multi-Backend Foundation
Snapshot Runtime Foundation
Change Feed Foundation
Live Transport Foundation
Capability Foundation
Recording Query Foundation
Recording Action Foundation
EPG Search Foundation
Backend-Scoped Persistent EPG Database Foundation
Content Classification Foundation
Person Metadata Foundation
Recording Person Search Foundation
Recording Character Search Foundation
SearchTimer User Workflow Foundation
SearchTimer Runtime Mutation Policy
SearchTimer Warm EPG Cache Architecture
SearchTimer Preview EPG Input Status Contract
SearchTimer Discovery RESTfulAPI Runtime Wiring
SearchTimer Discovery Shared Decoder Cleanup
Real VDR Acceptance Foundation
Daemon Runtime Lifecycle Hardening
Documentation Handoff Verification
Recording Operations Audit and Safety Policy
Installed systemd Unit Foundation
```

Current foundation in progress:

```text
Frontend and Live Parity
```

Direct GitHub documentation synchronization should still be followed locally by functional or runtime-specific checks only when the change requires real local behaviour validation.

---

## Verification Summary

- Snapshot-based read architecture is completed for the current domain set.
- Backend registry, backend-aware snapshots and multi-backend snapshot summaries are implemented.
- Change feed and live transport foundations are implemented.
- Selective EPG REST APIs and the EPG search API are implemented.
- Recording query and recording action foundations are implemented.
- ADR-0028 documents the source-aware content classification architecture.
- Person metadata, recording-person search and recording-character search foundations are implemented.
- `make install` now stages the vdr-suite-daemon systemd unit without enabling it automatically.
