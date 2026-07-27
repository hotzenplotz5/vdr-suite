# VDR-Suite Roadmap

## Navigation

- [README](README.md)
- [Current State](docs/CURRENT.md)
- [Strict Roadmap](docs/planning/roadmap.md)
- [Phase Map](docs/planning/phase-map.md)
- [Implementation Dependency Map](docs/planning/implementation-dependency-map.md)
- [Completed History](docs/development/completed-phases.md)
- [Architecture Gap Matrix](docs/planning/architecture-audit-gap-matrix.md)

## Purpose

This root file is only a compact roadmap entry point. The authoritative execution order, prerequisites and exit criteria live in [docs/planning/roadmap.md](docs/planning/roadmap.md). Historical implementation detail belongs in [Completed History](docs/development/completed-phases.md).

## Current position

```text
Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Completed post-phase platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)

Next strict runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation
```

Phase 61 was merged through PR #100. PRs #102 through #108 completed the measured metadata and EPG hardening. PR #110 completed the current remote-control pressed-state and dispatch behaviour. PR #111 added the backend-scoped global search. These are completed implementation, not open Phase 61 work.

## Strict forward sequence

```text
Phase 62 - Identity, RBAC and Accountability Foundation
Phase 63 - Backend Agent and Secure Multi-Site Runtime
Phase 64 - Timer Intent and Multi-Backend Orchestration
Phase 65 - Streaming Gateway and Media Sessions
Phase 66 - Legacy OSD Compatibility Bridge
Phase 67 - Public API and Client Compatibility Hardening
Phase 68 - Recommendation and Content Knowledge Graph
```

Later phases may not bypass identity, authorization, accountability, generation fencing, stable Suite identity or provider-neutral boundaries by moving policy into a frontend, plugin or external provider.

## Roadmap rule

Completed phases are not reopened merely because optional extensions remain possible. Provider additions, broader diagnostics and competing frontend asset proposals must be classified explicitly as backlog, deferred work or a later bounded slice.