# VDR-Suite Completed Phases

## Navigation

- [README](../../README.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Completed Phases Latest Marker](completed-phases-latest.md)

---

## Purpose

This file is the compact entry point for completed implementation phases.

---

## Completed Milestones Overview

### Frontend and Live Parity Foundation Slices

Status: Current major phase in progress with completed subphases.

Completed subphases:

- Phase 58.38 - SearchTimer frontend cockpit and mobile UI polish.
- Phase 58.39 - Bounded live EPG for channel cards.
- Phase 58.40 - Backend-scoped persistent EPG database foundation.
- Phase 58.58 - RESTfulAPI event stream change hints.
- Phase 58.90a - Guarded channel move API.
- Phase 58.90b - Stable frontend channel sorter.

Key outcomes:

- Existing backend capabilities are visible in the frontend foundation.
- Channel-card EPG uses bounded now-next input instead of an unbounded full EPG dump.
- Persistent EPG event identity is backend-scoped as `backend_id + channel_id + event_id`.
- Channel move and sorting are represented as stable Phase 58 frontend/live-parity slices.
- The stable channel sorter uses handle-only pointer drag and preserves normal scrolling.

Note: Phase 58 as a major block is not marked complete here. The latest completed major implementation phase remains Phase 57 until the full Phase 58 block is closed consistently across the marker files.

---

### Multi-Site Backend Administration and Permissions

Status: Completed.

Completed phases:

- Phase 57 - Multi-Site Backend Administration and Permissions.
- Phase 57.9 - Completion Audit.

Key outcomes:

- Backend access modes are represented.
- Backend registry JSON exposes permission hints.
- Recording, timer and SearchTimer paths use backend access handling.

---

### Library Boundary, Packaging and Developer Documentation

Status: Completed.

Completed phases:

- Phase 56 - Library Boundary, Packaging and Developer Documentation.
- Phase 56.57 - Phase 56 Completion Audit.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Project Status](current-status.md)
- [Back to README](../../README.md)
