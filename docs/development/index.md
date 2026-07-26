# Development Documentation

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Planning Index](../planning/index.md)
- [Architecture Index](../architecture/index.md)

---

## Purpose

This section contains current technical status, implementation closeouts, developer guidance and historical phase records.

Use current-state and closeout documents first. Historical `phase-*` files remain implementation evidence, not the authoritative answer for current roadmap position.

---

## Current Project State

Authoritative sources:

- [Current Project Status](current-status.md)
- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [GitHub Actions Status Handoff](github-actions-status-handoff.md)
- [Current Architecture State](current-architecture-state.md)
- [Current Technical Debt](current-technical-debt.md)
- [Build System State](build-system-state.md)
- [Runtime Diagnostics Status](runtime-diagnostics-status.md)

Current verified markers:

```text
Latest completed runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Latest completed operational hardening block:
Post-Phase 61 Performance Hardening (B1-B4)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Next runtime implementation phase:
Phase 62 - Identity, RBAC and Accountability Foundation
```

---

## Latest Completion Evidence

- [Phase 61 Metadata, Genre and Performance Closeout](phase-61-metadata-genre-performance-closeout.md)
- [Completed Phases](completed-phases.md)
- [Completed Phases Latest Marker](completed-phases-latest.md)
- [Completed Phases Archive](completed-phases/README.md)

The Phase 61 closeout records PR #100, the B1-B4 performance PRs #102 through #108, production measurements and real yaVDR acceptance.

---

## Current Development Direction

- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Implementation Dependency Map](../planning/implementation-dependency-map.md)
- [Domain Dependency Map](../planning/domain-dependency-map.md)
- [VDR Ecosystem Parity](../planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)

Immediate implementation focus:

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```

---

## Architecture and Runtime Validation

Key current architecture and validation documents:

- [Architecture Source Audit - 2026-07-15](architecture-source-audit-2026-07-15.md)
- [Architecture Map](architecture-map.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Frontend Architecture and Ownership Contracts](frontend-architecture.md)
- [Client API and Frontend Module Boundary Plan](client-api-frontend-module-boundary-plan.md)
- [Web Client API Contract Snapshot](web-client-api-contract-snapshot.md)
- [CI Test Strategy](ci-test-strategy.md)
- [Startup Snapshot Runtime Rule](startup-snapshot-runtime.md)
- [SearchTimer Preview EPG Cache Strategy](searchtimer-preview-epg-cache-strategy.md)
- [Real VDR Regression Coverage Audit](real-vdr-regression-coverage-audit.md)
- [Real Recording Action End-to-End Validation](real-recording-action-e2e-validation.md)
- [Live Plugin Parity Source Audit](live-plugin-parity-source-audit.md)
- [SearchTimer epgsearch / Live Compatibility Analysis](searchtimer-epgsearch-live-compatibility-analysis.md)

---

## Major Historical Tracks

Detailed historical records remain available for:

- Recording action architecture and safety;
- EPG search and cache evolution;
- SearchTimer backend, preview, workflow and automation planning;
- Live parity discovery;
- adapter and RESTfulAPI compatibility hardening;
- packaging, source boundaries and install staging;
- frontend Client API and module extraction;
- Recording browser, metadata and artwork preparation;
- Phase 61 metadata-backed Genre runtime.

Use [Completed Phases](completed-phases.md) and the [Completed Phases Archive](completed-phases/README.md) to locate the relevant phase record.

---

## Developer Guidance

- [Developer Onboarding](developer-onboarding.md)
- [Build System State](build-system-state.md)
- [Build Requirements](../build-requirements.md)
- [Dependencies](../dependencies.md)
- [REST API Developer Boundary Guide](phase-56.26-rest-api-developer-boundary-guide.md)
- [Service API Developer Boundary Guide](phase-56.27-service-api-developer-boundary-guide.md)
- [Backend Adapter Developer Boundary Guide](phase-56.28-backend-adapter-developer-boundary-guide.md)
- [Packaging Boundary Guide](phase-56.29-packaging-boundary-guide.md)

---

## Documentation Rules

- Current state belongs in `docs/CURRENT.md` and `current-status.md`.
- Strict future order belongs in the roadmap and dependency map.
- Completed implementation belongs in Completed Phases and closeout documents.
- Historical phase files must not override newer current-state markers.
- Completed phases are not reopened by optional future extensions.
- Ecosystem parity claims require source, test or live evidence.

Verification:

```bash
make test-phase-map-coverage
make test-docs
make test-phase
```

---

## Back

- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to README](../../README.md)