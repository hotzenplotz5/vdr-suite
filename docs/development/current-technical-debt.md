# VDR-Suite – Current Technical Debt

This document contains the current technical debt section that was split out of `docs/development/current-status.md` during Phase 10.21.1.

---

## Current Known Technical Debt

Current change-state parsing inside `RestfulApiVdrAdapter` uses a small local integer-field parser.

This is acceptable for the current minimal endpoint shape, but may later be replaced by a dedicated mapper if the endpoint grows or more RESTfulAPI JSON parsing is consolidated.

Phase 10.19 extends runtime diagnostics with a dedicated read-only summary endpoint at `GET /api/runtime/summary`. The existing `GET /api/runtime` endpoint still exposes the existing measurements JSON format. Persistence, rolling averages and broader diagnostics API hardening are not implemented yet.

The real daemon test showed that recordings are currently the dominant initial snapshot cost in the tested setup. Future optimization should investigate whether recordings can be refreshed or summarized more incrementally instead of repeatedly fetching large `/recordings.json` responses.

Documentation state:

- `docs/development/phase-10-runtime-diagnostics-measurement-collection.md` documents the Phase 10.10 to Phase 10.19 diagnostics measurement collection, retention, summaries, serialization and REST endpoint state.
- `docs/adr/007-platform-api-strategy.md` documents the platform API direction.
- `docs/development/current-status.md` documents Phase 10.19 and the current runtime diagnostics summary endpoint foundation.
