# VDR-Suite Roadmap

This roadmap describes the current development direction of VDR-Suite.

Detailed implementation status lives in:

- [Current Project Status](../development/current-status.md)
- [Planning Milestones](milestones.md)
- [Development Milestones](../development/milestones.md)
- [Documentation Index](../index.md)

---

## Roadmap Principles

VDR-Suite remains:

- VDR-centered
- backend-neutral
- daemon-driven
- snapshot-oriented
- service-oriented
- prepared for future multi-VDR environments
- suitable for multiple future clients and integrations

VDR remains the primary backend domain and source of truth.

VDR-Suite complements VDR. It does not replace it.

---

## Completed Foundation

### Phase 0 – Phase 7

Status: Completed

Result:

- project foundation
- persistence layer
- action and queue system
- dashboard services
- REST API foundation
- CLI foundation

---

## Completed VDR Backend Architecture

### Phase 8 – VDR Backend Architecture

Status: Completed

### Phase 9 – Snapshot Runtime Validation

Status: Completed

### Phase 10 – Runtime Diagnostics and Runtime Wiring

Status: Completed

### Phase 11 – Snapshot Read APIs

Status: Completed

Implemented snapshot read endpoints:

```text
GET /api/vdr/status
GET /api/vdr/channels
GET /api/vdr/timers
GET /api/vdr/events
GET /api/vdr/recordings
```

### Phase 12 – Snapshot Change Feed Foundation

Status: Completed through Phase 12.3

Implemented:

```text
Phase 12.0 Snapshot Change Feed Architecture
Phase 12.1 Snapshot Change Feed Model and Service
Phase 12.2 Snapshot Change Feed JSON Serializer
Phase 12.3 Snapshot Change Feed REST Controller
```

Result:

- transport-independent change feed architecture
- change feed service layer
- change feed serialization
- REST controller integration
- controller test coverage

---

## Current Roadmap Position

Current completed phase:

```text
Phase 12.3 Snapshot Change Feed REST Controller
```

Current transition:

```text
Phase 12 complete
↓
Roadmap cleanup
↓
Phase 13 Live Update Transport
```

---

## Next Planned Architecture Phase

### Phase 13 – Live Update Transport

Goal:

Expose snapshot change feed information through a transport mechanism while keeping feed generation transport-independent.

Candidate transports:

- Server Sent Events (SSE)
- WebSocket

Expected result:

- live change delivery
- frontend-friendly update channel
- preserved backend neutrality
- preserved snapshot architecture

---

## Later Roadmap

### Phase 14 – Multi-VDR Backend Routing

Planned direction:

- backend identity aware routing
- multiple VDR instances
- backend federation preparation
- permission-aware architecture foundation

### Phase 15 – Frontend API Hardening

Planned direction:

- filtering
- pagination
- stable API contracts
- capability-aware responses

### Later Product Phases

Possible future layers:

- Web frontend
- Windows frontend
- Android frontend
- iOS frontend
- OSD integration
- stream provider integration
- media library expansion

---

## Important Architectural Rule

Live transport remains separate from:

- snapshot generation
- change detection
- feed generation

This separation keeps SSE, WebSocket, desktop clients, web clients and future multi-VDR environments compatible with the same backend architecture.
