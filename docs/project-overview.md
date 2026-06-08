# VDR-Suite Project Overview

Navigation:

- index.md
- planning/index.md
- development/index.md
- architecture/index.md
- adr/index.md

---

## Purpose

This page is the primary landing page for the VDR-Suite project.

It provides a high-level overview of project status, architecture direction and future development.

---

## Project Status

```text
Current Major Phase
Phase 12 - Snapshot Change Feed Foundation

Next Major Phase
Phase 13 - Live Update Transport
```

Current activity:

```text
Documentation consolidation and roadmap cleanup
```

---

## Phase Ladder

```text
Phase 0-7   Core backend, database, jobs, REST and daemon foundations
Phase 8     External VDR and RESTfulAPI integration architecture
Phase 9     Snapshot architecture foundation
Phase 10    Runtime diagnostics and measurement architecture
Phase 11    Snapshot read APIs
Phase 12    Snapshot change feed foundation
Phase 13    Live update transport
Phase 14    Multi-VDR backend routing
Phase 15    Frontend API hardening
Phase 16    Image and preview stream validation
```

Current implementation state:

```text
Phase 12 complete
Phase 13 next
```

---

## Vision

VDR-Suite modernizes the VDR ecosystem through a service-oriented backend architecture.

The project adds:

- daemon-owned state
- snapshot-based data access
- REST APIs
- future multi-client support
- future multi-VDR support

VDR remains the authoritative backend.

---

## Architecture Principles

- VDR-centered
- backend-neutral
- daemon-driven
- snapshot-oriented
- service-oriented
- multi-client ready
- multi-VDR ready

---

## Implemented Foundations

- SQLite persistence
- recording and metadata services
- job and action services
- dashboard services
- REST API layer
- HTTP abstraction layer
- VDR adapter architecture
- RESTfulAPI integration
- snapshot runtime
- snapshot cache and access services
- change detection and refresh planning
- runtime diagnostics
- snapshot read APIs
- snapshot change feed

---

## Progress Overview

```text
Backend Foundation        95 percent
Snapshot Runtime         100 percent
Snapshot Read APIs        90 percent
Snapshot Change Feed     100 percent
Live Update Transport      0 percent
Multi-VDR Architecture    15 percent
Frontend Contracts        25 percent
Authentication             0 percent
```

These values are orientation estimates only.

---

## Next Major Step

### Phase 13 - Live Update Transport

Goal:

Expose snapshot change feed updates through a real-time transport.

Candidate technologies:

- Server Sent Events (SSE)
- WebSocket

Architecture rule:

The transport layer consumes snapshot change feed events and does not own change detection or snapshot generation.

---

## Documentation Hubs

### Planning

- planning/index.md

### Development

- development/index.md

### Architecture

- architecture/index.md

### ADRs

- adr/index.md

---

## Documentation Rule

Use:

- project-overview.md for orientation
- development/current-status.md for verified status
- development/completed-phases.md for history
- planning/roadmap.md for future planning