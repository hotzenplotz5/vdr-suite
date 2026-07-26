# VDR-Suite

## Navigation

- [Current State](docs/CURRENT.md)
- [Documentation Index](docs/index.md)
- [Roadmap](docs/planning/roadmap.md)
- [Phase Map](docs/planning/phase-map.md)
- [Phase 61 and Performance Closeout](docs/development/phase-61-metadata-genre-performance-closeout.md)
- [VDR Ecosystem Parity](docs/planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture Decision Records](docs/adr/index.md)

---

## Current Verified State

Latest completed implementation phase:

```text
Phase 61 - Suite Metadata and Genre Platform
```

Latest completed operational hardening block:

```text
Post-Phase 61 Performance Hardening (B1-B4)
```

Historical umbrella implementation track:

```text
Phase 58 - Frontend and Live Parity
```

Next implementation focus:

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```

Phase 61 was merged through PR #100 and accepted on the real yaVDR system. The subsequent PRs #102 through #108 optimized EPG candidate selection, Genre persistence, Recording synchronization, EPG window indexing, no-op event upserts and TVScraper type-snapshot scheduling.

---

## What Is Implemented

VDR-Suite currently provides:

- a daemon-owned SQLite runtime and backend-neutral REST layer;
- backend-aware VDR status, channels, EPG, recordings and timers;
- lazy Recording browsing with metadata, people, artwork and guarded actions;
- persistent Recording and EPG Genre assignments and a metadata-backed Genre browser;
- TVScraper-backed EPG details without direct browser/provider coupling;
- SearchTimer list, preview, validation and controlled mutation foundations;
- server-enforced read-only backend policy;
- remote-control and live-overlay foundations;
- Web Client API wrappers and modular frontend ownership;
- packaging, install staging and real-system acceptance workflows.

The project is not yet a complete replacement for every Live, epgsearch or RESTfulAPI surface. Streaming, legacy OSD, production RBAC, secure Backend Agents and the versioned public `/api/v1` contract remain later phases.

---

## Architecture Direction

VDR remains the native runtime authority. VDR-Suite owns the external domain, policy, orchestration, metadata read models and client-facing contracts.

Accepted architecture packages include ADR-0038 through ADR-0049, the target platform architecture and the domain and implementation dependency maps.

Read [Current State](docs/CURRENT.md), the [Strict Roadmap](docs/planning/roadmap.md) and the [VDR Ecosystem Parity](docs/planning/parity-audit-and-frontend-gap-roadmap.md) before planning new implementation work.

---

## Next Work

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```

Phase 62 introduces actor identities, scoped roles and permissions, centralized authorization decisions and append-only accountability evidence before later secure multi-site command dispatch.