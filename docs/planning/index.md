# Planning Documentation

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Development Index](../development/index.md)
- [Architecture Index](../architecture/index.md)

---

## Purpose

This section describes future VDR-Suite work, dependency order and open product or architecture gaps.

Completed implementation belongs in [Completed Phases](../development/completed-phases.md). Completed audit evidence belongs in the development section. Stable architecture belongs in the architecture section.

---

## Authoritative Planning Documents

### Execution Order

- [Strict Roadmap](roadmap.md)
- [Phase Map](phase-map.md)
- [Implementation Dependency Map](implementation-dependency-map.md)

The roadmap is read from top to bottom. The Phase Map owns compact phase numbering. The Implementation Dependency Map expands each planned phase into prerequisite, slice and exit-gate order.

### Domain and Target Architecture

- [Domain Dependency Map](domain-dependency-map.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Architecture Decision Records](../adr/index.md)

The Domain Dependency Map defines which Suite-owned concepts depend on which stable identities and policies. The Target Platform Architecture defines the canonical Control Plane, Agent, plugin, media, OSD and public API boundaries accepted through ADR-0049.

### Architecture Gaps

- [Architecture Audit Gap Matrix](architecture-audit-gap-matrix.md)
- [Completed Architecture Source Audit](../development/architecture-source-audit-2026-07-15.md)

The gap matrix is the living status register for the architecture audit. The source-audit document is the immutable evidence snapshot. Accepted ADRs and dependency maps do not automatically close runtime gaps.

### Product and Ecosystem Parity

- [Parity Audit and Frontend Gap Roadmap](parity-audit-and-frontend-gap-roadmap.md)
- [Recording Metadata, External Scrapers and Suite Metadata Database Roadmap](tvscraper-recording-metadata-roadmap.md)
- [Lazy Recording Loading](lazy-recording-loading.md)

The parity document focuses on RESTfulAPI, Live, epgsearch, VDR-Core and frontend product coverage. It does not define future architecture phase order.

### Supporting Planning

- [Phase 57 Local Server Permission Model](phase-57-local-server-permission-model.md)
- [Project State Snapshot](project-progress.md)
- [Milestones](milestones.md)

---

## Current Verified Position

```text
Latest completed major project block
Phase 57 - Multi-Site Backend Administration and Permissions

Latest completed implementation slice
Phase 60.15 - Recording Metadata and Poster Preparation

Architecture contract package
ADR-0042 through ADR-0049, target diagrams and dependency maps completed

Next runtime implementation phase
Phase 61 - Suite Metadata Database and External Provider Integration
```

---

## Strict Future Sequence

```text
1. Phase 61 - Suite Metadata Platform
2. Phase 62 - Identity, RBAC and Audit
3. Phase 63 - Backend Agent and Multi-Site Runtime
4. Phase 64 - Timer Intent and Orchestration
5. Phase 65 - Streaming Gateway
6. Phase 66 - Legacy OSD Bridge
7. Phase 67 - Public API and Client Hardening
8. Phase 68 - Recommendation and Knowledge Graph
```

The completed architecture package remains the prerequisite baseline for every phase above.

---

## Related Status Documents

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Current Project Status](../development/current-status.md)
- [Current Architecture State](../development/current-architecture-state.md)
- [Completed Phases](../development/completed-phases.md)
- [Completed Phases Latest Marker](../development/completed-phases-latest.md)

---

## Documentation Rules

- The roadmap defines one strict future execution order.
- The Phase Map owns phase numbers and compact status.
- The Implementation Dependency Map expands the required implementation and test order.
- The Domain Dependency Map owns cross-domain prerequisite direction.
- The Architecture Audit Gap Matrix owns living implementation-gap status.
- The older parity matrix owns product and ecosystem parity questions only.
- Completed Phases owns finished runtime implementation.
- The completed source audit owns the 2026-07-15 evidence baseline.
- Accepted ADRs and diagrams do not imply completed runtime behavior.
- Avoid duplicate or conflicting phase sequencing across documents.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
