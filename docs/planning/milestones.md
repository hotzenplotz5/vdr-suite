# VDR-Suite – Planning Milestones

This document describes forward-looking planning milestones.

Historical implementation phases and tag history are tracked in:

- [Development Milestones](../development/milestones.md)
- [Completed Phases](../development/completed-phases.md)
- [Current Project Status](../development/current-status.md)

The roadmap is tracked in:

- [Roadmap](roadmap.md)

---

## Current Position

Current completed implementation phase:

```text
Phase 11.6: Complete snapshot read domain JSON serialization
```

Current transition:

```text
Documentation cleanup
↓
Phase 12.0 Snapshot Change Feed Architecture
```

---

## Milestone M0 – Project Foundation

Status: Completed

Result:

- repository structure established
- documentation structure created
- ADR process introduced
- initial architecture documented

---

## Milestone M1 – Core Backend Foundation

Status: Completed

Result:

- SQLite foundation
- recording and metadata services
- job and action foundations
- dashboard service foundations
- REST API foundation

---

## Milestone M2 – VDR Backend Foundation

Status: Completed

Result:

- VDR configuration model
- adapter abstraction
- mock backend
- RESTfulAPI integration boundary
- backend architecture ADRs

---

## Milestone M3 – Snapshot Runtime Foundation

Status: Completed

Result:

- VDR snapshot model
- snapshot builder
- polling service
- snapshot cache
- snapshot access service
- partial refresh planning
- change detection foundation

---

## Milestone M4 – Runtime Diagnostics Foundation

Status: Completed

Result:

- runtime measurement boundary
- runtime diagnostics service
- runtime diagnostics JSON serialization
- runtime diagnostics REST endpoints
- bounded diagnostics retention
- runtime configuration cleanup

---

## Milestone M5 – Snapshot Read API Foundation

Status: Completed

Result:

- snapshot read service
- snapshot read JSON serializer
- snapshot-backed controller and router paths
- HTTP server coverage
- domain JSON for status, channels, timers, events and recordings
- full `make test` verification after Phase 11.6

---

## Milestone M6 – Documentation Navigation Cleanup

Status: Active

Goal:

Make the documentation easy to navigate from both `README.md` and `docs/index.md`.

Completion criteria:

- README links to current status, roadmap, milestones and documentation index
- `docs/index.md` links all major documentation areas
- roadmap reflects the real Phase 11.6 state
- planning milestones distinguish future planning from implementation history
- development milestones remain focused on historical tags and verified implementation milestones
- known ADR duplicates are marked or resolved
- no intentionally maintained document is unreachable from the documentation index

---

## Milestone M7 – Snapshot Change Feed Architecture

Status: Planned

Goal:

Define the internal architecture for snapshot change feeds before adding live transport.

Completion criteria:

- existing `VdrChangeState`, `VdrChangeEvent` and `ChangeDetectionService` are reviewed
- change feed responsibilities are documented
- feed generation is separated from HTTP, SSE and WebSocket transport
- multi-VDR and backend identity implications are documented
- first implementation phase is planned without frontend coupling

---

## Milestone M8 – Snapshot Change Feed Implementation

Status: Planned

Goal:

Implement a backend-internal change feed service and expose a first read-only REST endpoint.

Completion criteria:

- change feed service implemented
- tests verify feed generation
- REST endpoint added for read-only change access
- API does not require direct RESTfulAPI calls
- design remains compatible with future SSE/WebSocket transport

---

## Milestone M9 – Live Update Transport

Status: Planned

Goal:

Expose snapshot change events to future frontends through a live-update transport.

Completion criteria:

- SSE or WebSocket transport selected through architecture review
- live transport remains a consumer of the change feed, not the generator
- transport is tested independently from polling
- future multi-client usage is considered

---

## Milestone M10 – Multi-VDR Read Architecture

Status: Planned

Goal:

Prepare read-side architecture for multiple VDR instances and future federation.

Completion criteria:

- backend identity requirements are reflected in read models where needed
- snapshot ownership is source/backend aware
- routing strategy for multiple VDR instances is documented
- permission-aware future architecture remains possible

---

## Milestone M11 – Frontend Contract Hardening

Status: Planned

Goal:

Stabilize API contracts for future Web, Windows, Android, iOS and OSD frontends.

Completion criteria:

- read API contracts documented
- error response strategy documented
- filtering and pagination strategy reviewed
- capability-aware API strategy considered
- frontend-independent API behavior verified

---

## Milestone M12 – First Frontend-Oriented Product Layer

Status: Future

Goal:

Start the first real user-facing product layer after backend read, diagnostics and change-feed foundations are stable.

Possible directions:

- Web frontend
- Windows frontend
- OSD integration
- Android frontend
- iOS frontend

Final frontend ordering should be decided after Phase 12 and API contract hardening.
