# Development Documentation

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Current State](../CURRENT.md)

---

## Purpose

Development status, implementation progress and technical project history.

This index separates current project state from historical phase-specific records.

---

## Current Project State

Authoritative sources:

- [Current Project Status](current-status.md)
- [GitHub Actions Status Handoff](github-actions-status-handoff.md)
- [Current Architecture State](current-architecture-state.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Current Technical Debt](current-technical-debt.md)
- [Build System State](build-system-state.md)
- [CI Test Strategy](ci-test-strategy.md)
- [Runtime Diagnostics Status](runtime-diagnostics-status.md)
- [Runtime Diagnostics Documentation](runtime-diagnostics/README.md)
- [Startup Snapshot Runtime Rule](startup-snapshot-runtime.md)
- [SearchTimer Preview EPG Cache Strategy](searchtimer-preview-epg-cache-strategy.md)
- [Development Status Documentation](status/index.md)

These documents describe the verified current state of the project.

---

## Current Development Direction

- [Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [Planning Milestones](../planning/milestones.md)
- [TVScraper and Recording Metadata Roadmap](../planning/tvscraper-recording-metadata-roadmap.md)
- [Completed Phases](completed-phases.md)
- [Completed Phases Latest Marker](completed-phases-latest.md)
- [Milestones](milestones.md)

Latest completed major implementation phase:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

Current implementation focus:

```text
Phase 58 - Frontend and Live Parity
```

Current Phase 58 foundation notes:

- Phase 58.38 - SearchTimer frontend cockpit and mobile UI polish.
- Phase 58.39 - Bounded live EPG for channel cards.
- Phase 58.40 - Backend-scoped persistent EPG database foundation.

Do not move global latest-completed markers to Phase 58 until the full Phase 58 major block is closed consistently across the marker files.

---

## Current Architecture Validation

- [ADR-0021: Selective Backend Query Strategy](../adr/ADR-0021-selective-backend-query-strategy.md)
- [ADR-0025: Configurable Metadata Provider Architecture](../adr/ADR-0025-configurable-metadata-provider-architecture.md)
- [ADR-0028: Content Classification Architecture](../adr/ADR-0028-content-classification-architecture.md)
- [ADR-0031: Person Catalog and External Filmography Architecture](../adr/ADR-0031-person-catalog-and-external-filmography.md)
- [ADR-0032: EPGSearch Regex Mode Safety](../adr/ADR-0032-epgsearch-regex-mode-safety.md)
- [ADR-0033: EPGSearch Fuzzy Mode Decision](../adr/ADR-0033-epgsearch-fuzzy-mode-decision.md)
- [ADR-0034: SearchTimer Warm EPG Cache and Change Invalidation](../adr/ADR-0034-searchtimer-warm-epg-cache-and-change-invalidation.md)
- [ADR-0035: Lazy Recording Loading and Backend-Scoped Refresh](../adr/ADR-0035-lazy-recording-loading-and-backend-scoped-refresh.md)
- [ADR-0036: TVScraper Recording Metadata Integration Strategy](../adr/ADR-0036-tvscraper-recording-metadata-integration.md)
- [ADR-0037: Packaging, Install Layout and API Boundary](../adr/ADR-0037-packaging-install-api-boundary.md)
- [Startup Snapshot Runtime Rule](startup-snapshot-runtime.md)
- [SearchTimer Preview EPG Cache Strategy](searchtimer-preview-epg-cache-strategy.md)
- [Live / EPGSearch Feature Inventory](live-feature-inventory.md)
- [Live Plugin Parity Source Audit](live-plugin-parity-source-audit.md)
- [Live Parity Discovery Foundation Completion](live-parity-discovery-foundation-completion.md)

---

## Getting Started

- [Developer Onboarding](developer-onboarding.md)
- [Architecture Map](architecture-map.md)
- [Testing Guide](testing-guide.md)
- [CI Test Strategy](ci-test-strategy.md)
- [Coding Standards](coding-standards.md)
- [Documentation Standards](documentation-standards.md)
- [Backend Development Guide](backend-development-guide.md)
- [Contributor Guide](contributor-guide.md)
- [Release Process](release-process.md)

---

## Progress Tracking

- [Completed Phases](completed-phases.md)
- [Completed Phases Latest Marker](completed-phases-latest.md)
- [Completed Phases Archive](completed-phases/README.md)
- [Milestones](milestones.md)

Implementation history and completed work.

---

## Future Planning

- [Roadmap](../planning/roadmap.md)
- [Planning Milestones](../planning/milestones.md)
- [TVScraper and Recording Metadata Roadmap](../planning/tvscraper-recording-metadata-roadmap.md)

Planned work and future direction.

---

## Historical Phase Records

These documents are historical implementation notes or phase-specific architecture records. They are intentionally kept for traceability and should not be read as the current implementation focus.

Use [Completed Phases](completed-phases.md) and [Completed Phases Archive](completed-phases/README.md) as compact entry points before opening old phase-specific files.

Key historical records and active reference documents:

- [Phase 8 - Architecture Guardrails](phase-8-architecture-guardrails.md)
- [Phase 21.0 - Real VDR Runtime Polling Findings](phase-21.0-real-vdr-runtime-polling-findings.md)
- [Phase 21.1 - RESTfulAPI Event Stream Strategy](phase-21.1-restfulapi-event-stream-strategy.md)
- [Phase 21.3 - Selective RESTfulAPI EPG Validation](phase-21.3-selective-restfulapi-epg-validation.md)
- [Phase 44 Recording Action Runtime Completion](phase-44-recording-action-runtime-completion.md)
- [Phase 45.0 - EPG Search Architecture](phase-45-epg-search-architecture.md)
- [EPG Search API](epg-search-api.md)
- [SearchTimer Backend Contract](searchtimer-backend-contract.md)
- [SearchTimer Real Payload Validation](searchtimer-real-payload-validation.md)
- [SearchTimer Completeness Audit](searchtimer-completeness-audit.md)
- [SearchTimer epgsearch / Live Compatibility Analysis](searchtimer-epgsearch-live-compatibility-analysis.md)
- [SearchTimer Real VDR Compatibility Report](searchtimer-real-vdr-compatibility-report.md)
- [SearchTimer Feature Gap Analysis](searchtimer-feature-gap-analysis.md)
- [SearchTimer Completeness Re-Audit](searchtimer-completeness-reaudit.md)
- [Real VDR Regression Coverage Audit](real-vdr-regression-coverage-audit.md)
- [Real Recording Action Regression Audit](real-recording-action-regression-audit.md)
- [Live Feature Inventory](live-feature-inventory.md)
- [Live Plugin Parity Source Audit](live-plugin-parity-source-audit.md)
- [SearchTimer Preview EPG Cache Strategy](searchtimer-preview-epg-cache-strategy.md)
- [EPGSearch Capability Matrix](epgsearch-capability-matrix.md)
- [EPGSearch Native Fuzzy Real-Backend Validation](epgsearch-native-fuzzy-real-backend-validation.md)
- [SearchTimer User Workflow Foundation](searchtimer-user-workflow-foundation.md)
- [SearchTimer Workflow Foundation Completion](searchtimer-workflow-foundation-completion.md)
- [EPGSearch Result Model Audit](epgsearch-result-model-audit.md)
- [EPGSearch Query Alignment Audit](epgsearch-query-alignment-audit.md)
- [EPGSearch Test Coverage Audit](epgsearch-test-coverage-audit.md)
- [Genre Architecture](genre-architecture.md)
- [Content Rating API](content-rating-api.md)
- [Person API](person-api.md)
- [Real VDR Person Metadata Validation](real-vdr-person-metadata-validation.md)

---

## Supporting Development Documents

- [Architecture Map](architecture-map.md)
- [Current Architecture State](current-architecture-state.md)
- [Current Technical Debt](current-technical-debt.md)
- [Build System State](build-system-state.md)
- [Testing Guide](testing-guide.md)
- [CI Test Strategy](ci-test-strategy.md)
- [Coding Standards](coding-standards.md)
- [Documentation Standards](documentation-standards.md)
- [Backend Development Guide](backend-development-guide.md)
- [Contributor Guide](contributor-guide.md)

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
