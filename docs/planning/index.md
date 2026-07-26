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

Completed implementation belongs in [Completed Phases](../development/completed-phases.md) and dedicated closeout documents. Stable architecture belongs in the architecture section.

---

## Authoritative Planning Documents

### Execution Order

- [Strict Roadmap](roadmap.md)
- [Phase Map](phase-map.md)
- [Implementation Dependency Map](implementation-dependency-map.md)

The roadmap is read from top to bottom. The Phase Map owns compact numbering and status. The Implementation Dependency Map expands prerequisites, bounded slices and exit gates.

### Domain and Target Architecture

- [Domain Dependency Map](domain-dependency-map.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Architecture Decision Records](../adr/index.md)

The Domain Dependency Map defines cross-domain prerequisites. The Target Platform Architecture defines the accepted Control Plane, Agent, provider, media, OSD and public API boundaries.

### Architecture Gaps

- [Architecture Audit Gap Matrix](architecture-audit-gap-matrix.md)
- [Completed Architecture Source Audit](../development/architecture-source-audit-2026-07-15.md)

The gap matrix is the living architecture status register. The source audit is completed evidence. Accepted ADRs do not automatically close runtime gaps.

### Product and Ecosystem Parity

- [VDR Ecosystem Parity and Product Gap Roadmap](parity-audit-and-frontend-gap-roadmap.md)
- [Recording Metadata, External Scrapers and Suite Metadata Database Roadmap](tvscraper-recording-metadata-roadmap.md)
- [Lazy Recording Loading](lazy-recording-loading.md)

The parity document compares VDR-Suite with VDR Core, Live, epgsearch and RESTfulAPI. It does not replace the strict roadmap.

### Completion Evidence

- [Phase 61 and Performance Closeout](../development/phase-61-metadata-genre-performance-closeout.md)
- [Completed Phases](../development/completed-phases.md)
- [Completed Phases Latest Marker](../development/completed-phases-latest.md)

---

## Current Verified Position

```text
Latest completed runtime phase
Phase 61 - Suite Metadata and Genre Platform

Latest completed operational hardening block
Post-Phase 61 Performance Hardening (B1-B4)

Historical umbrella implementation track
Phase 58 - Frontend and Live Parity

Architecture contract package
ADR-0042 through ADR-0049, target diagrams and dependency maps completed

Next runtime implementation phase
Phase 62 - Identity, RBAC and Accountability Foundation
```

---

## Strict Future Sequence

```text
1. Phase 62 - Identity, RBAC and Accountability Foundation
2. Phase 63 - Backend Agent and Secure Multi-Site Runtime
3. Phase 64 - Timer Intent and Multi-Backend Orchestration
4. Phase 65 - Streaming Gateway and Media Sessions
5. Phase 66 - Legacy OSD Compatibility Bridge
6. Phase 67 - Public API and Client Compatibility Hardening
7. Phase 68 - Recommendation and Content Knowledge Graph
```

Phase 61 is completed and is not kept open by optional later provider adapters or broader diagnostics.

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

- The roadmap defines one strict forward order.
- The Phase Map owns phase numbers and compact status.
- The Implementation Dependency Map owns detailed implementation/test order.
- The Domain Dependency Map owns cross-domain prerequisites.
- The Architecture Audit Gap Matrix owns living implementation gaps.
- The parity document owns VDR Core, Live, epgsearch and RESTfulAPI comparison.
- Completed Phases and closeout documents own finished runtime implementation.
- Completed phases are not silently reopened by optional extensions.
- Avoid duplicate or conflicting phase sequencing across documents.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)