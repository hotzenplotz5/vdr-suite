# VDR-Suite – Current Technical Debt

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)
- [Current State](../CURRENT.md)
- [ADR-0056 Playback Semantics](../adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md)

---

This document tracks implementation debt that is not itself a current phase-completion requirement unless a later accepted contract promotes it.

---

## Current Known Technical Debt

### Legacy RESTfulAPI change-state parsing

Current change-state parsing inside `RestfulApiVdrAdapter` uses a small local integer-field parser.

This is acceptable for the current minimal endpoint shape, but may later be replaced by a dedicated mapper if the endpoint grows or more RESTfulAPI JSON parsing is consolidated.

### Runtime diagnostics consolidation

Phase 10.19 extended runtime diagnostics with a dedicated read-only summary endpoint at `GET /api/runtime/summary`. The existing `GET /api/runtime` endpoint still exposes the existing measurements JSON format. Persistence, rolling averages and broader diagnostics API hardening are not implemented yet.

A later media-specific read-only diagnostic projection is separately permitted by ADR-0056, but it must project stable MediaSession/playback semantics and must never become lifecycle/provider/capability authority.

### Recording snapshot cost

The real daemon test showed that recordings are currently the dominant initial snapshot cost in the tested setup. Future optimization should investigate whether recordings can be refreshed or summarized more incrementally instead of repeatedly fetching large `/recordings.json` responses.

### Shared fMP4/MSE browser primitives

The accepted Phase-65 browser implementation currently contains overlapping transport-neutral helpers in the continuous-fMP4 and HLS/fMP4 paths, including MP4 box inspection, codec/MIME derivation and SourceBuffer operations.

A bounded cleanup may extract shared primitives such as:

- init-segment inspection;
- codec/MIME derivation;
- safe SourceBuffer append/remove operations;
- buffered-range calculations.

This debt must **not** be solved by merging continuous-stream and HLS manifest/segment lifecycles into a universal player. The two transports keep different ownership/backpressure behavior under the same persistent playback owner.

This cleanup is not a Phase-65.D completion gate. ADR-0056 requires normalized playback semantics, canonical owner lifecycle publication, continuity/discontinuity semantics and classified failures first; helper deduplication may be performed only as a bounded maintenance change with focused regression value.

---

## Documentation state

- `docs/development/phase-10-runtime-diagnostics-measurement-collection.md` documents the Phase 10.10 to Phase 10.19 diagnostics measurement collection, retention, summaries, serialization and REST endpoint state.
- `docs/adr/007-platform-api-strategy.md` documents the historical platform API direction.
- `docs/adr/ADR-0056-playback-presentation-timeline-continuity-failure-semantics.md` owns the current playback semantic architecture.
- `docs/development/phase-65d-playback-semantics-consolidation.md` owns the bounded Phase-65.D implementation sequencing.

---

## Back

- [Back to Development Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
