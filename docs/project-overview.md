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
Phase 49.30 - EPGSearch native fuzzy validation consolidation

Current Implementation Focus
Phase 50.0 - SearchTimer user workflow foundation
```

Current activity:

```text
Turn the validated SearchTimer backend and native fuzzy capability foundation into a coherent user workflow foundation.
```

Authoritative status documents:

- [Current Project Status](development/current-status.md)
- [Current Architecture State](development/current-architecture-state.md)
- [Completed Phases](development/completed-phases.md)
- [Roadmap](planning/roadmap.md)

---

## Phase Ladder

```text
Phase 0-7     Core backend, database, jobs, REST and daemon foundations
Phase 8       VDR backend architecture foundation
Phase 9-13    Snapshot runtime, diagnostics, read APIs and change feed
Phase 14-29   Backend registry, multi-backend reads, selective EPG and recording query foundations
Phase 30-36   Recording action validation and execution foundation
Phase 45      EPG search foundation
Phase 46      Content classification and person metadata foundations
Phase 47      SearchTimer backend foundation and domain expansion
Phase 49      EPGSearch native fuzzy capability validation
Phase 50      SearchTimer user workflow foundation
```

Current implementation state:

```text
Phase 49.30 complete
Phase 50.0 next
```

---

## Current Architecture Direction

VDR remains the primary backend domain and source of truth.

VDR-Suite complements VDR with daemon-owned runtime state, backend-neutral service boundaries, REST APIs, capability-aware runtime behavior and future client-facing contracts.

The current architecture has working foundations for snapshot-backed VDR reads, backend registry, backend-aware snapshots, selective EPG reads, recording query, recording actions, SearchTimer backend access and native fuzzy capability reporting.

The next useful step is to turn SearchTimer backend capability and contract work into a practical user workflow foundation.

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
