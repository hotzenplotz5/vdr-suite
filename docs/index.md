# VDR-Suite Documentation

## Start here

- [Current State](CURRENT.md) — verified implementation and runtime truth.
- [New Chat Handoff](NEW-CHAT-HANDOFF.md) — mandatory entry point for new work.
- [Post-Slice-2W New Chat Prompt](development/phase-62-post-slice-2w-new-chat-prompt.md) — ready-to-copy continuation prompt.
- [Current Project Status](development/current-status.md) — compact current branch and acceptance state.
- [Phase 62 Slice 2W Runtime Closeout](development/phase-62-slice-2w-runtime-closeout.md) — newest accepted runtime evidence.
- [Project Overview](project-overview.md) — compact product and architecture summary.
- [Project Status Dashboard](project-status-dashboard.md) — capability/status table.

## Planning and implementation status

- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Phase 62 Security and Identity Gap Matrix](planning/phase-62-security-identity-gap-matrix.md)
- [Phase 62 Slice 2W Contract](development/phase-62-slice-2w-browser-session-retention-cleanup.md)
- [Phase 62 Runtime Evidence through Slice 2V](development/phase-62-runtime-evidence.md)
- [Development Documentation](development/index.md)
- [Implementation Dependency Map](planning/implementation-dependency-map.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Current Architecture State](development/current-architecture-state.md)

## Current marker

The latest completed numbered runtime phase is **Phase 61 — Suite Metadata and
Genre Platform**. Phase 62 — Identity, RBAC and Accountability Foundation is
active and incomplete, but repository implementation, all five CI jobs and real
yaVDR runtime acceptance are complete through **Slice 2W — Browser-Session
Terminal Retention Cleanup**.

```text
Accepted source/runtime head:
bb8609151313c613d403b88b1b4c3f55453a93e2

Source CI:
VDR-Suite CI #6834
Run ID 30745952119
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30745952119

Runtime marker:
PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE=PASS

Installed/running daemon SHA-256:
7775804306bf70eca6ef23474605467381162cfc9d5b874cdb187840ca8bc571

Runtime report SHA-256:
e0fbe1689b2f48e75bb4ae6836b227d7da92e08d53b009ac1c2cb371a36c74ea
```

PR #117 remains open, Draft and unmerged. No next Phase-62 implementation slice
is selected yet. Phase 63-67 runtime has not been advanced.

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
