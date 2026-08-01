# VDR-Suite Documentation

## Start here

- [Current State](CURRENT.md) — verified implementation and runtime truth.
- [New Chat Handoff](NEW-CHAT-HANDOFF.md) — mandatory entry point for new work.
- [Current Project Status](development/current-status.md) — compact current branch and acceptance state.
- [Phase 62 Slice 2P Closeout](development/phase-62-slice-2p-query-cache-refresh-security-migration.md) — newest accepted route-family evidence.
- [Project Overview](project-overview.md) — compact product and architecture summary.
- [Project Status Dashboard](project-status-dashboard.md) — capability/status table.

## Planning and implementation status

- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Phase 62 Security and Identity Gap Matrix](planning/phase-62-security-identity-gap-matrix.md)
- [Phase 62 Runtime Evidence](development/phase-62-runtime-evidence.md)
- [Phase 62 Slice 1](development/phase-62-security-identity-foundation-slice-1.md)
- [Phase 62 Slice 2](development/phase-62-security-identity-foundation-slice-2.md)
- [Phase 62 Slice 2O](development/phase-62-slice-2o-native-fuzzy-refresh-security-migration.md)
- [Phase 62 Slice 2P](development/phase-62-slice-2p-query-cache-refresh-security-migration.md)
- [Implementation Dependency Map](planning/implementation-dependency-map.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Current Architecture State](development/current-architecture-state.md)

## Current marker

The latest completed numbered runtime phase is **Phase 61 — Suite Metadata and
Genre Platform**. Phase 62 — Identity, RBAC and Accountability Foundation is
active and incomplete, but repository implementation, all five CI jobs and real
yaVDR runtime acceptance are complete through **Slice 2P — Query-Scoped Cache
Refresh Security Migration** at code/runtime head
`173c929964dbb7aabd30c5e482c2e250b5785d92`.

PR #117 remains open, Draft and unmerged. Phase 63-67 runtime has not been
advanced.

## Completed history

- [Completed Phases](development/completed-phases.md)
- [Completed Phases Latest Marker](development/completed-phases-latest.md)
- [Completed Phase Archive](development/completed-phases/README.md)
- [Phase 61 Metadata, Genre and Performance Closeout](development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](development/post-phase-61-platform-runtime-closeout.md)

## Architecture

- [Architecture Documentation](architecture/index.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)
- [Security and Identity Foundation](architecture/security-identity-foundation.md)
- [Metadata-Backed Genre Browser](architecture/metadata-genre-browser.md)
- [Backend-Scoped Global Search](architecture/global-search.md)
- [Live Remote, Overlay and Legacy OSD Contract](architecture/live-remote-osd-contract.md)
- [RESTfulAPI Integration](architecture/restfulapi-integration.md)
- [Architecture Decision Records](adr/index.md)

## Development references

- [Development Documentation](development/index.md)
- [Developer Onboarding](development/developer-onboarding.md)
- [Build System State](development/build-system-state.md)
- [GitHub Actions Status Handoff](development/github-actions-status-handoff.md)
- [Person API](development/person-api.md)
- [Web Client API Contract Snapshot](development/web-client-api-contract-snapshot.md)

## Planning references

- [Planning Documentation](planning/index.md)
- [Domain Dependency Map](planning/domain-dependency-map.md)
- [TVScraper / Provider Strategy](planning/tvscraper-recording-metadata-roadmap.md)
- [Lazy Recording Loading](planning/lazy-recording-loading.md)

## Status model

- **CURRENT**: verified current merged-code truth plus explicitly identified active branch work.
- **PLANNED**: genuinely open work with an explicit target owner.
- **COMPLETED**: merged implementation with test/acceptance evidence.
- **HISTORICAL**: retained traceability that is not a current entry point.
- **SUPERSEDED**: replaced content with a named current successor.
- **DEFERRED**: intentionally postponed work with prerequisites.

Accepted ADRs remain architecture decisions. Their runtime implementation status
is tracked separately.

## Supporting documents

- [Project Principles](project-principles.md)
- [Project Glossary](project-glossary.md)
- [Database Design](database-design.md)
- [Build Requirements](build-requirements.md)
- [Dependencies](dependencies.md)
- [Community Documentation](community/index.md)
- [Legacy Roadmap Archive](roadmap/README.md)
