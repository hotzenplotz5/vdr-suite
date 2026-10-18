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

This document is a historical milestone pointer plus links to canonical living plans; it is **not** the active milestone register.

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

## Historical Phase-54.3 milestone snapshot (not current)

The Phase-54.3 material below records an earlier planning checkpoint. It is preserved for traceability, **not** as a second active-milestone source. The numbered implementation programme has since completed through Phase 69; the next Phase 70 has not started. For current work and gates consult [CURRENT](../CURRENT.md) and the [strict roadmap](roadmap.md).

Implementation focus recorded at that earlier checkpoint:

```text
Phase 54.3 - SearchTimer warm EPG cache implementation
```

Follow-up recorded at that checkpoint:

```text
Lazy Recording Loading and Backend-Scoped Recording Refresh
```

Planning focus recorded at that checkpoint:

```text
SearchTimer preview performance first; recording startup performance must remain tracked as a required follow-up.
```

---

## Current authoritative planning sources

Use [Roadmap](roadmap.md) for the current milestone order and long-term direction.

Use [Lazy Recording Loading](lazy-recording-loading.md) for the mandatory no-startup-recording-load requirement.

Use [Project Progress](project-progress.md) for high-level progress percentages and the active major milestone.

Use [Current Project Status](../development/current-status.md) for the verified technical state before starting a new implementation phase.

---

## Historical implementation direction (Phase 54.3)

At this earlier checkpoint the plan stated that Phase 54.3 should finish SearchTimer preview warm EPG cache and cache-status semantics. That statement is historical and must not be used as a new implementation instruction.

The required recording follow-up must ensure:

- daemon startup does not synchronously load recordings for all configured backends
- recording lists are loaded backend-by-backend on demand
- the recording page can render before recording data is available
- the UI exposes loading, ready, stale and error states per backend
- recording actions refresh only the affected backend after execution

---

## Historically deferred planning areas

The following items were listed as deferred after the then-active SearchTimer preview work. Their **current** completion or deferral must be checked in the roadmap and current-status authority:

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

Do not advance the historical Phase-54.3 snapshot when a current milestone changes. Update [CURRENT](../CURRENT.md) and the [binding roadmap](roadmap.md) at their established gates; validate relevant documentation and phase checks on the exact intended repository commit.

---

## Back

- [Back to Planning Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
