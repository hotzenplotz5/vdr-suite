# VDR-Suite – Planning Milestones

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Planning Index](index.md)
- [Roadmap](roadmap.md)
- [Lazy Recording Loading](lazy-recording-loading.md)
- [Project Progress](project-progress.md)
- [Current Project Status](../development/current-status.md)
- [Completed Phases](../development/completed-phases.md)

---

## Purpose

This document is a lightweight planning entry point.

The authoritative current planning state is maintained in:

- [Roadmap](roadmap.md)
- [Lazy Recording Loading](lazy-recording-loading.md)
- [Project Progress](project-progress.md)
- [Current Project Status](../development/current-status.md)
- [Project Status Dashboard](../project-status-dashboard.md)

Historical implementation phase history is maintained in:

- [Completed Phases](../development/completed-phases.md)
- [Development Milestones](../development/milestones.md)

Older milestone sketches that described Phase 28 through early multi-backend work have been replaced by the current roadmap and completed-phase records.

---

## Current Position

Current implementation focus:

```text
Phase 54.3 - SearchTimer warm EPG cache implementation
```

Required planned follow-up:

```text
Lazy Recording Loading and Backend-Scoped Recording Refresh
```

Current planning focus:

```text
SearchTimer preview performance first; recording startup performance must remain tracked as a required follow-up.
```

---

## Active Planning Source

Use [Roadmap](roadmap.md) for the current milestone order and long-term direction.

Use [Lazy Recording Loading](lazy-recording-loading.md) for the mandatory no-startup-recording-load requirement.

Use [Project Progress](project-progress.md) for high-level progress percentages and the active major milestone.

Use [Current Project Status](../development/current-status.md) for the verified technical state before starting a new implementation phase.

---

## Current Implementation Direction

Phase 54.3 should finish the SearchTimer preview warm EPG cache and cache-status semantics.

The required recording follow-up must ensure:

- daemon startup does not synchronously load recordings for all configured backends
- recording lists are loaded backend-by-backend on demand
- the recording page can render before recording data is available
- the UI exposes loading, ready, stale and error states per backend
- recording actions refresh only the affected backend after execution

---

## Deferred Planning Areas

The following remain planned after the active SearchTimer preview performance work:

- Lazy recording loading and backend-scoped recording refresh.
- Live-style information parity and frontend-ready API polish.
- SearchTimer automation and scheduled evaluation.
- Recommendation primitives.
- Cross-backend search and federation.
- Backend management and client administration.
- Web, TV and mobile frontend product layers.

---

## Maintenance Rule

Keep this file short. Detailed planning belongs in [Roadmap](roadmap.md), [Lazy Recording Loading](lazy-recording-loading.md), and completed implementation history belongs in [Completed Phases](../development/completed-phases.md).

When the active milestone changes, update the current position here and then run:

```text
make test-docs
make test-phase
```

---

## Back

- [Back to Planning Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
