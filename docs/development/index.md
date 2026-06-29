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
- [ADR-0037: Packaging, Install Layout and API Boundary](../adr/ADR-0037-packaging-install-api-boundary.md)
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
- [Phase 56.21 - Build Boundary Map](phase-56.21-build-boundary-map.md)
- [Phase 56.22 - VDR Source Boundary Plan](phase-56.22-vdr-source-boundary-plan.md)
- [Phase 56.23 - HTTP Boundary Plan](phase-56.23-http-boundary-plan.md)
- [Phase 56.24 - HTTP Source Group Split](phase-56.24-http-source-group-split.md)
- [Phase 56.25 - Public and Internal API Boundaries](phase-56.25-public-internal-api-boundaries.md)
- [Phase 56.26 - REST API Developer Boundary Guide](phase-56.26-rest-api-developer-boundary-guide.md)
- [Phase 56.27 - Service API Developer Boundary Guide](phase-56.27-service-api-developer-boundary-guide.md)
- [Phase 56.28 - Backend Adapter Developer Boundary Guide](phase-56.28-backend-adapter-developer-boundary-guide.md)
- [Phase 56.29 - Packaging Boundary Guide](phase-56.29-packaging-boundary-guide.md)
- [Phase 56.30 - Package Candidate Source Inventory](phase-56.30-package-candidate-source-inventory.md)
- [Phase 56.31 - Public Header Inventory](phase-56.31-public-header-inventory.md)
- [Phase 56.32 - Package Dependency Graph](phase-56.32-package-dependency-graph.md)
- [Phase 56.33 - Recording Action Source Group Split](phase-56.33-recording-action-source-group-split.md)
- [Phase 56.34 - Recording Action Target Migration](phase-56.34-recording-action-target-migration.md)
- [Phase 56.35 - Recording Action Core Target Migration](phase-56.35-recording-action-core-target-migration.md)
- [Phase 56.36 - Remaining ACTIONS_SRC Audit](phase-56.36-remaining-actions-src-audit.md)
- [Phase 56.37 - Recording Action Policy Target Migration](phase-56.37-recording-action-policy-target-migration.md)
- [Phase 56.38 - Recording Action Validation Target Migration](phase-56.38-recording-action-validation-target-migration.md)
- [Phase 56.39 - Recording Action Preview Source Group Split](phase-56.39-recording-action-preview-source-group.md)
- [Phase 56.40 - RESTfulAPI Executor Source Group Split](phase-56.40-restfulapi-executor-source-group.md)
- [Phase 56.41-56.44 - ACTIONS_SRC Batch Migration](phase-56.41-56.44-actions-src-batch.md)
- [Phase 56.45 - Remaining ACTIONS_SRC Re-Audit](phase-56.45-remaining-actions-src-reaudit.md)
- [Phase 56.46 - Recording Action REST Controller Source Group Split](phase-56.46-rest-controller-source-group.md)
- [Phase 56.47 - Recording Action Executor Adapter Source Group Split](phase-56.47-executor-adapter-source-group.md)
- [Phase 56.48 - VDR Timer Action Parser Source Group Split](phase-56.48-vdr-timer-parser-source-group.md)
- [Phase 56.49 - Legacy Recording Action Test Source Group Split](phase-56.49-legacy-action-tests-source-group.md)
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
- [SearchTimer Runtime Mutation Policy Wiring](searchtimer-runtime-mutation-policy-wiring.md)
- [SearchTimer Verified Execution Result Integration](searchtimer-verified-execution-result-integration.md)
- [SearchTimer Title-Only RESTfulAPI Field Mapping](searchtimer-title-only-restfulapi-field-mapping.md)
- [SearchTimer Title-Only Request Parser Contract](searchtimer-title-only-request-parser-contract.md)
- [SearchTimer Title-Only REST Controller Contract](searchtimer-title-only-rest-controller-contract.md)
- [SearchTimer Title-Only Update Parser Contract](searchtimer-title-only-update-parser-contract.md)
- [SearchTimer Title-Only Update Controller Contract](searchtimer-title-only-update-controller-contract.md)
- [SearchTimer Title-Only Workflow Request Contract](searchtimer-title-only-workflow-request-contract.md)
- [SearchTimer Title-Only Workflow Command Mapper Contract](searchtimer-title-only-workflow-command-mapper-contract.md)
- [SearchTimer Title-Only Workflow Execution Dispatch Contract](searchtimer-title-only-workflow-execution-dispatch-contract.md)
- [SearchTimer Title-Only Workflow Completion Audit](searchtimer-title-only-workflow-completion-audit.md)
- [SearchTimer Preview EPG Cache Strategy](searchtimer-preview-epg-cache-strategy.md)
- [EPGSearch Capability Matrix](epgsearch-capability-matrix.md)
- [EPGSearch Native Fuzzy Real-Backend Validation](epgsearch-native-fuzzy-real-backend-validation.md)
- [SearchTimer User Workflow Foundation](searchtimer-user-workflow-foundation.md)
- [SearchTimer Workflow Validation API](searchtimer-workflow-validation-api.md)
- [SearchTimer yaVDR Smoke-Test Execution Report](searchtimer-yavdr-smoke-test-execution-report.md)
- [SearchTimer yaVDR Local API Smoke Harness](searchtimer-yavdr-api-smoke-harness.md)
- [SearchTimer yaVDR Local API Smoke Harness Execution Report](searchtimer-yavdr-api-smoke-harness-execution-report.md)
- [SearchTimer Mandatory Backend Readback Verification Plan](searchtimer-backend-readback-verification-plan.md)
- [SearchTimer Backend Readback Verification Result Model](searchtimer-backend-readback-verification-result-model.md)
- [SearchTimer Create Readback Verification Service](searchtimer-create-readback-verification-service.md)
- [SearchTimer Update Readback Verification Service](searchtimer-update-readback-verification-service.md)
- [SearchTimer Delete Absence Verification Plan](searchtimer-delete-absence-verification-plan.md)
- [SearchTimer Delete Absence Verification Service](searchtimer-delete-absence-verification-service.md)
- [SearchTimer Readback Services Dispatch Integration](searchtimer-readback-services-dispatch-integration.md)
- [SearchTimer Verified Execution REST Contract](searchtimer-verified-execution-rest-contract.md)
- [SearchTimer End-to-End Verified Execution Test](searchtimer-end-to-end-verified-execution-test.md)
- [SearchTimer User Workflow Foundation Completion](searchtimer-workflow-foundation-completion.md)
- [EPGSearch Result Model Audit](epgsearch-result-model-audit.md)
- [EPGSearch Query Alignment Audit](epgsearch-query-alignment-audit.md)
- [EPGSearch Test Coverage Audit](epgsearch-test-coverage-audit.md)
- [Genre Architecture](genre-architecture.md)
- [Content Rating API](content-rating-api.md)
- [Person API](person-api.md)
- [Real VDR Person Metadata Validation](real-vdr-person-metadata-validation.md)

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

- [Phase 8 - Architecture Guardrails](phase-8-architecture-guardrails.md)
- [Phase 8.37 - VDR Source Model Architecture Status](phase-8.37-vdr-source-model-status.md)
- [Phase 8.38 - Minimal Source Domain Object Gate](phase-8.38-minimal-source-domain-object-gate.md)
- [Phase 8.38 - SourceRegistry Architecture](phase-8.38-source-registry-architecture.md)
- [Phase 8.38 - SourceType Architecture Decision](phase-8.38-source-type-decision.md)
- [Phase 8.39 - ContentIdentity Architecture](phase-8.39-content-identity-architecture.md)
- [Phase 8.40 - Action Target Architecture](phase-8.40-action-target-architecture.md)
- [Phase 8.41 - Platform Action Architecture](phase-8.41-platform-action-architecture.md)
- [Phase 8.42 - Capability Resolver Architecture](phase-8.42-capability-resolver-architecture.md)
- [Phase 9 Runtime Validation Result](phase-9-runtime-validation-result.md)
- [Phase 9.6 - Local RESTfulAPI Integration Test](phase-9.6-local-restfulapi-integration.md)
- [Phase 9.7 - Local Snapshot Runtime Integration Test](phase-9.7-local-snapshot-runtime-integration.md)
- [Phase 9.8 - Local Partial Refresh Validation](phase-9.8-local-partial-refresh-validation.md)
- [Phase 10 Runtime Diagnostics Measurement Collection](phase-10-runtime-diagnostics-measurement-collection.md)
- [Phase 11 Snapshot Read APIs](phase-11-snapshot-read-apis.md)
- [Phase 14.3 - Backend-Aware Snapshot Routing](phase-14.3-backend-aware-snapshot-routing.md)
- [Phase 21.0 - Real VDR Runtime Polling Findings](phase-21.0-real-vdr-runtime-polling-findings.md)
- [Phase 21.1 - RESTfulAPI Event Stream Strategy](phase-21.1-restfulapi-event-stream-strategy.md)
- [ADR-0021: Selective Backend Query Strategy](../adr/ADR-0021-selective-backend-query-strategy.md)
- [Phase 21.3 - Selective RESTfulAPI EPG Validation](phase-21.3-selective-restfulapi-epg-validation.md)

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
