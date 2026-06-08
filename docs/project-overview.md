# VDR-Suite Project Overview

Navigation:

- ../README.md
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

## Documentation Navigation

Primary navigation targets:

- README.md
- docs/index.md
- planning/index.md
- development/index.md
- architecture/index.md
- adr/index.md

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

## Next Major Step

### Phase 13 - Live Update Transport

Goal:

Expose snapshot change feed updates through a real-time transport.

- Server Sent Events (SSE)
- WebSocket

---

## Documentation Rule

Every documentation area should provide links back to:

- README.md
- docs/index.md
- docs/project-overview.md
- local section index
