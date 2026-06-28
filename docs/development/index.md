# Development Documentation

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Development Index](index.md)

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
- [Development Status Documentation](status/index.md)

These documents describe the verified current state of the project.

---

## Current Development Direction

- [Roadmap](../planning/roadmap.md)
- [Planning Milestones](../planning/milestones.md)
- [TVScraper and Recording Metadata Roadmap](../planning/tvscraper-recording-metadata-roadmap.md)
- [Completed Phases](completed-phases.md)
- [Milestones](milestones.md)

Current completed phase:

```text
Phase 55.6 - Recording operations audit and safety policy
```

Next implementation focus:

```text
Phase 56 - Library Boundary, Packaging and Developer Documentation
```

Current architecture validation:

- [Phase 21.0 - Real VDR Runtime Polling Findings](phase-21.0-real-vdr-runtime-polling-findings.md)
- [Phase 21.1 - RESTfulAPI Event Stream Strategy](phase-21.1-restfulapi-event-stream-strategy.md)
- [ADR-0021: Selective Backend Query Strategy](../adr/ADR-0021-selective-backend-query-strategy.md)
- [ADR-0025: Configurable Metadata Provider Architecture](../adr/ADR-0025-configurable-metadata-provider-architecture.md)
- [ADR-0028: Content Classification Architecture](../adr/ADR-0028-content-classification-architecture.md)
- [ADR-0031: Person Catalog and External Filmography Architecture](../adr/ADR-0031-person-catalog-and-external-filmography.md)
- [ADR-0032: EPGSearch Regex Mode Safety](../adr/ADR-0032-epgsearch-regex-mode-safety.md)
- [ADR-0033: EPGSearch Fuzzy Mode Decision](../adr/ADR-0033-epgsearch-fuzzy-mode-decision.md)
- [ADR-0034: SearchTimer Warm EPG Cache and Change Invalidation](../adr/ADR-0034-searchtimer-warm-epg-cache-and-change-invalidation.md)
- [ADR-0036: TVScraper Recording Metadata Integration Strategy](../adr/ADR-0036-tvscraper-recording-metadata-integration.md)
- [Timer Contract Gap Analysis](timer-contract-gap-analysis.md)
- [Real Recording Action End-to-End Validation](real-recording-action-e2e-validation.md)
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
- [Live / EPGSearch Feature Inventory](live-feature-inventory.md)
- [Live Plugin Parity Source Audit](live-plugin-parity-source-audit.md)
- [Live Parity Discovery Domain Foundation](live-parity-discovery-domain-foundation.md)
- [Live Parity Discovery JSON Contract](live-parity-discovery-json-contract.md)
- [Live Parity Discovery REST Controller Contract](live-parity-discovery-rest-controller-contract.md)
- [Live Parity Discovery Service Contract](live-parity-discovery-service-contract.md)
- [Live Parity Discovery Controller Service Integration](live-parity-discovery-controller-service-integration.md)
- [Live Parity Discovery Router Contract](live-parity-discovery-router-contract.md)
- [Live Parity Discovery Daemon Wiring](live-parity-discovery-daemon-wiring.md)
- [Live Parity Discovery HTTP Smoke Contract](live-parity-discovery-http-smoke-contract.md)
- [Live Parity Discovery RESTfulAPI Provider Contract](live-parity-discovery-restfulapi-provider-contract.md)
- [Live Parity Discovery Foundation Completion](live-parity-discovery-foundation-completion.md)
- [Phase 55.0 - VDR-Suite Feature-Parity and Adapter Audit](phase-55-vdr-parity-adapter-audit.md)
- [Phase 55.1 - SearchTimer Update RESTfulAPI Contract Fix](phase-55.1-searchtimer-update-restfulapi-contract.md)
- [Phase 55.2 - SearchTimer JSON Array Payload Parity](phase-55.2-searchtimer-json-array-payload-parity.md)
- [Phase 55.3 - Timer Form Body URL Encoding](phase-55.3-timer-form-body-url-encoding.md)
- [Phase 55.4 - RESTfulAPI SearchTimer Discovery Provider](phase-55.4-searchtimer-discovery-restfulapi-provider.md)
- [Phase 55.4b - Shared JSON String Decoder](phase-55.4b-shared-json-string-decoder.md)
- [Phase 55.4c - SearchTimer Discovery Runtime Wiring](phase-55.4c-searchtimer-discovery-runtime-wiring.md)
- [Phase 55.4d - SearchTimer Discovery Shared Decoder Cleanup](phase-55.4d-searchtimer-discovery-shared-decoder.md)
- [Phase 55.5m - Documentation Consolidation and Roadmap Alignment](phase-55.5m-documentation-consolidation.md)
- [Phase 55.5n - Roadmap Historical Coverage Alignment](phase-55.5n-roadmap-historical-coverage.md)
- [Phase 55.5o - Phase Map and Roadmap Simplification](phase-55.5o-phase-map-and-roadmap-simplification.md)
- [Phase 55.6 - Recording Operations Audit and Safety Policy](phase-55.6-recording-operations-audit-and-safety-policy.md)
- [Phase 56.0 - Completed Phases Archive Split Plan](phase-56.0-completed-phases-archive-split-plan.md)
- [Phase 56.20 - Library, Daemon and API Boundary Audit](phase-56.20-library-daemon-api-boundary-audit.md)
- [SearchTimer Automation Foundation Planning](searchtimer-automation-foundation-planning.md)
- [SearchTimer Automation Evaluation Plan Model](searchtimer-automation-evaluation-plan-model.md)
- [SearchTimer Automation Match Candidate Model](searchtimer-automation-match-candidate-model.md)
- [SearchTimer Automation Duplicate Detection Model](searchtimer-automation-duplicate-detection-model.md)
- [SearchTimer Automation Candidate Timer Proposal Model](searchtimer-automation-candidate-timer-proposal-model.md)
- [SearchTimer Automation Dry-Run Result Serializer](searchtimer-automation-dry-run-result-serializer.md)
- [SearchTimer Automation Read-Only Service Boundary](searchtimer-automation-read-only-service-boundary.md)
- [SearchTimer Automation REST Preview Contract](searchtimer-automation-rest-preview-contract.md)
- [SearchTimer Automation Daemon Scheduling Plan](searchtimer-automation-daemon-scheduling-plan.md)
- [SearchTimer Automation Safety Review](searchtimer-automation-safety-review.md)

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
