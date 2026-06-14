# VDR-Suite Project Overview

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Planning Documentation](planning/index.md)
- [Development Documentation](development/index.md)
- [Architecture Documentation](architecture/index.md)
- [Architecture Decision Records](adr/index.md)

---

## Purpose

This page is the primary landing page for the VDR-Suite project.

It provides a high-level overview of project status, architecture direction and future development.

---

## Project Status

```text
Current Completed Phase
Phase 28.12 - Recording Query API Documentation

Current Implementation Focus
Phase 29.0 - Multi-Backend Recording Identity Foundation
```

Current activity:

```text
Prepare backend identity for recording domain objects before adding backend-specific recording query filters.
```

Authoritative status documents:

- [Current Project Status](development/current-status.md)
- [Current Architecture State](development/current-architecture-state.md)
- [Completed Phases](development/completed-phases.md)
- [Roadmap](planning/roadmap.md)

---

## Phase Ladder

```text
Phase 0-7    Core backend, database, jobs, REST and daemon foundations
Phase 8      VDR backend architecture foundation
Phase 9-13   Snapshot runtime, diagnostics, read APIs and change feed
Phase 14-17  Backend registry and multi-backend snapshot visibility
Phase 18-21  Real VDR validation and selective EPG query foundation
Phase 22-24  Heavy-domain policy and EPG query architecture
Phase 25-27  EPG REST boundary, startup decoupling and capability-aware runtime
Phase 28     Recording query API foundation
Phase 29     Multi-backend recording identity foundation
```

Current implementation state:

```text
Phase 28.12 complete
Phase 29.0 next
```

---

## Current Architecture Direction

VDR remains the primary backend domain and source of truth.

VDR-Suite complements VDR with daemon-owned runtime state, backend-neutral service boundaries, REST APIs, capability-aware runtime behavior and future client-facing contracts.

The current recording query API supports title, path, start-time, duration, sorting and paging through `GET /api/vdr/recordings/query`.

Backend-specific recording filters are intentionally deferred until backend identity is represented in the `VdrRecording` domain model.

---

## Documentation Navigation

Primary navigation targets:

- [README](../README.md)
- [Documentation Index](index.md)
- [Planning Documentation](planning/index.md)
- [Development Documentation](development/index.md)
- [Architecture Documentation](architecture/index.md)
- [Architecture Decision Records](adr/index.md)

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
- [Back to Planning Documentation](planning/index.md)
- [Back to Development Documentation](development/index.md)
