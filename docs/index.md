# VDR-Suite Documentation

## Start here

- [Current State](CURRENT.md) — verified implementation and runtime truth.
- [New Chat Handoff](NEW-CHAT-HANDOFF.md) — mandatory entry point for new work.
- [Current Project Status](development/current-status.md) — compact branch and acceptance state.
- [Phase 62 Final Closeout](development/phase-62-closeout.md) — completed phase scope and retirement decision.
- [Slice 2X Runtime Closeout](development/phase-62-slice-2x-runtime-closeout.md) — final real-yaVDR evidence.
- [Project Overview](project-overview.md) — compact product and architecture summary.
- [Project Status Dashboard](project-status-dashboard.md) — capability/status table.
- [Project Principles](project-principles.md) — binding product and engineering principles.

## Planning and implementation status

- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Phase 62 Final Gap Matrix](planning/phase-62-security-identity-gap-matrix.md)
- [Development Documentation](development/index.md)
- [Implementation Dependency Map](planning/implementation-dependency-map.md)
- [Architecture Audit Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Current Architecture State](development/current-architecture-state.md)

## Current marker

The latest completed numbered runtime phase is **Phase 62 — Identity, RBAC and Accountability Foundation**. The next strict runtime phase is **Phase 63 — Backend Agent and Secure Multi-Site Runtime**, which is planned but not started.

```text
Final accepted runtime head:
4762583d5b5170866838ed9f03b928adbf39f99e

Source CI:
VDR-Suite CI #6884
Run ID 30752351218
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30752351218

Runtime marker:
PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS

Installed/running daemon SHA-256:
488edade196cedfb92d5393a8725b39c5f5cdfd3265e2b15bab6aadfbe7ef5f5

Runtime report SHA-256:
bf165416b5ad041f44b2514182dac582a7f1060bf1ae8cc584964f3fc5a98bdf

Durable evidence:
/var/backups/vdr-suite-phase62-slice2x-20260802T145043Z-4762583d5b51
```

PR #117 remains open, Draft and unmerged pending explicit approval for PR metadata and merge actions. Phase 63-67 runtime has not been advanced.

## Completed history

- [Completed Phases](development/completed-phases.md)
- [Completed Phases Latest Marker](development/completed-phases-latest.md)
- [Completed Phase Archive](development/completed-phases/README.md)
- [Phase 62 Final Closeout](development/phase-62-closeout.md)
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
- **COMPLETED**: implementation with test and real-system evidence.
- **HISTORICAL**: retained traceability that is not a current entry point.
- **SUPERSEDED**: replaced content with a named current successor.
- **DEFERRED**: intentionally postponed work with prerequisites.

Accepted ADRs remain architecture decisions. Their runtime implementation status is tracked separately.
