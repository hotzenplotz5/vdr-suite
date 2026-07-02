# VDR-Suite Project State Snapshot

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Planning Index](index.md)
- [Roadmap](roadmap.md)
- [Lazy Recording Loading](lazy-recording-loading.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Current Project Status](../development/current-status.md)

---

## Purpose

This file is the source for the high-level project state block copied into README, Current Status and Project Status Dashboard.

It is intentionally **not** a percentage-based product progress tracker.

VDR-Suite has many completed foundations, but it is not production-complete. Some domains are deliberately read-only, dry-run-only or guarded until later safety phases.

The generated state blocks are written by:

    tools/update_project_progress.py

---

<!-- PROJECT_STATE_SNAPSHOT_START -->
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
- Content classification and person metadata foundations
- Recording person and character search foundations
- SearchTimer backend, validation, planning and workflow foundations
- SearchTimer safety gates, readback verification and production mutation policy foundations
- Live parity discovery foundation
- Real VDR acceptance manifest and runner foundation
- Daemon lifecycle hardening for duplicate bind failures and SIGTERM shutdown
- Recording operations audit and safety policy foundation

### Verified real-runtime evidence

- Real VDR acceptance currently passes 20/20 safe and dry-run probes.
- Duplicate daemon start on an occupied HTTP port exits cleanly with status 1 instead of aborting.
- SIGTERM stops the daemon cleanly without `kill -9` and releases port 18080.
- GitHub Actions verification is required before runtime-related phases are considered complete.\n- Phase 58.39 verifies bounded live EPG input for channel cards via the now-next EPG route.

### Guarded or deliberately incomplete areas

- SearchTimer production mutation remains gated and closed by default.
- Recording operations real-backend write probes remain explicitly gated.
- Lazy recording loading is still a required follow-up for large real recording catalogs and multi-backend scaling.
- Suite-owned metadata database and external scraper/provider strategy are planned but not yet implemented as the final metadata product.
- Authentication, authorization, per-backend permissions and read-only secondary-site policy are still planned.
- Web, Windows, Android, iOS and TV frontends remain planned product layers; the current web frontend is a Phase 58 foundation, not the final client product.

### Current active focus

```text
Phase 58 - Frontend and Live Parity
```

### Later strategic milestones

- Multi-site backend federation and permissions
- Frontend and live-parity foundation
- Suite metadata database and external provider integration
- EPG cache database and SSE/change-state triggered background synchronization\n- Safe production-grade recording operations

Progress source: {{PROJECT_PROGRESS_SOURCE_LINK}}
<!-- PROJECT_STATE_SNAPSHOT_END -->

---

## Maintenance Rule

Update this file when the verified project state changes.

After editing this file, run:

    python3 tools/update_project_progress.py
    make test-docs
    make test-phase

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Planning Index](index.md)
