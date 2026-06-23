# VDR-Suite – Planning Milestones

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Planning Index](index.md)
- [Roadmap](roadmap.md)
- [Project Progress](project-progress.md)
- [Current Project Status](../development/current-status.md)
- [Completed Phases](../development/completed-phases.md)

---

## Purpose

This document is a lightweight planning entry point.

The authoritative current planning state is maintained in:

- [Roadmap](roadmap.md)
- [Project Progress](project-progress.md)
- [Current Project Status](../development/current-status.md)
- [Project Status Dashboard](../project-status-dashboard.md)

Historical implementation phase history is maintained in:

- [Completed Phases](../development/completed-phases.md)
- [Development Milestones](../development/milestones.md)

Older milestone sketches that described Phase 28 through early multi-backend work have been replaced by the current roadmap and completed-phase records.

---

## Current Position

Current completed implementation phase:

```text
Phase 49.30 - EPGSearch native fuzzy validation consolidation
```

Next major implementation milestone:

```text
Phase 50.0 - SearchTimer user workflow foundation
```

Current planning focus:

```text
SearchTimer user workflow foundation
```

---

## Active Planning Source

Use [Roadmap](roadmap.md) for the current milestone order and long-term direction.

Use [Project Progress](project-progress.md) for high-level progress percentages and the active major milestone.

Use [Current Project Status](../development/current-status.md) for the verified technical state before starting a new implementation phase.

---

## Current Implementation Direction

Phase 50.0 should move from backend capability validation into user-facing SearchTimer workflow foundations.

The next planning questions are:

- Which SearchTimer create/list/update/delete workflow contract is the minimum useful operator surface?
- Which epgsearch and Live-style options are supported now?
- Which unsupported options must stay explicit future scope?
- Which parts are read-only validation and which parts mutate VDR-owned SearchTimer state?
- How should backend identity remain visible for later multi-backend SearchTimer behavior?

---

## Deferred Planning Areas

The following remain planned after the SearchTimer user workflow foundation:

- Live-style information parity and frontend-ready API polish.
- SearchTimer automation and scheduled evaluation.
- Recommendation primitives.
- Cross-backend search and federation.
- Backend management and client administration.
- Web, TV and mobile frontend product layers.

---

## Maintenance Rule

Keep this file short. Detailed planning belongs in [Roadmap](roadmap.md), and completed implementation history belongs in [Completed Phases](../development/completed-phases.md).

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
