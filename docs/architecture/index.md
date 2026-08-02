# Architecture Documentation

## Purpose

This section documents stable VDR-Suite architecture. Current implementation truth belongs in [Current Architecture State](../development/current-architecture-state.md); future order belongs in planning; long-term decisions belong in ADRs.

A target diagram or accepted ADR is not evidence that its runtime is complete.

## Canonical target architecture

- [Target Platform Architecture](target-platform-architecture.md)
- [Domain Dependency Map](../planning/domain-dependency-map.md)
- [Implementation Dependency Map](../planning/implementation-dependency-map.md)
- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [ADR Index](../adr/index.md)

## Active security and identity architecture

- [Security and Identity Foundation](security-identity-foundation.md)
- [Phase 62 Security and Identity Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Phase 62 Slice 1](../development/phase-62-security-identity-foundation-slice-1.md)

The first Phase 62 slice establishes request identity, centralized backend-scoped permission decisions and append-only pre-dispatch accountability for the Remote mutation. Production identity lifecycle, complete route migration, revision/idempotency and outbox work remain open.

## Implemented metadata and discovery architecture

- [Metadata Identity Foundation](metadata-identity-foundation.md)
- [Suite Metadata Platform Schema v1](metadata-platform-schema-v1.md)
- [Metadata-Backed Genre Browser](metadata-genre-browser.md)
- [Backend-Scoped Global Search](global-search.md)
- [Live / TVScraper EPG Genre Comparison](live-tvscraper-epg-genre-comparison.md)
- [ADR-0038: Suite Metadata Database and External Provider Strategy](../adr/ADR-0038-suite-metadata-database-and-external-provider-strategy.md)
- [Post-Phase-61 Provider Strategy](../planning/tvscraper-recording-metadata-roadmap.md)

Phase 61 is completed. Optional providers, imports, broader artwork processing and diagnostics are post-Phase-61 backlog or deferred strategy.

## Backend and control architecture

- [VDR Backends](vdr-backends.md)
- [VDR Domain Model](vdr-domain-model.md)
- [RESTfulAPI Integration](restfulapi-integration.md)
- [Live Remote, Overlay and Legacy OSD Compatibility Contract](live-remote-osd-contract.md)
- [Suite Bridge Agent Handshake](suite-bridge-agent-handshake.md)
- [Suite Bridge Local SVDRP Transport](suite-bridge-svdrp-transport.md)
- [Suite Bridge Read-Only Observation Lifecycle](suite-bridge-observation-lifecycle.md)
- [Suite Bridge Embedded Agent Runtime](suite-bridge-embedded-agent-runtime.md)

The implemented RemoteAction/LiveOverlay path is not the future Streaming Gateway or Legacy OSD bridge. Secure remote Agents remain planned Phase 63 work.

## Recording architecture

- [Recording Actions Architecture](recording-actions-architecture.md)
- [Recording Action Validation API](recording-action-validation-api.md)
- [Recording Action Execution API](recording-action-execution-api.md)
- [Recording Action Safety and Capability Model](recording-action-safety-capability-model.md)
- [Recording Action Source Constraints](recording-action-source-constraints.md)
- [Lazy Recording Loading](../planning/lazy-recording-loading.md)

Recordings 2 is the delivered Recording browser and detail/action owner. Historical legacy browser notes must not be interpreted as an active parallel runtime.

## Runtime, snapshot and event architecture

- [REST API Runtime](rest-api-runtime.md)
- [Daemon REST Runtime](daemon-rest-runtime.md)
- [HTTP Server Boundary](http-server-boundary.md)
- [Snapshot Architecture](snapshot-architecture.md)
- [Snapshot Access Architecture](snapshot-access-architecture.md)
- [Partial Snapshot Refresh Architecture](partial-snapshot-refresh-architecture.md)
- [Snapshot Change Feed Architecture](snapshot-change-feed-architecture.md)
- [Internal Event Dispatch Architecture](internal-event-dispatch-architecture.md)

## Core platform context

- [VDR-Suite Core Platform Model](vdr-suite-core-platform-model.md)
- [Suite Components](suite-components.md)
- [Media Platform Comparison](media-platform-comparison.md)

## Historical notes

- [Architecture History](history/README.md)
- [Phase 8.94 Snapshot Cache Integration Plan](phase-8.94-snapshot-cache-integration-plan.md)
- [Phase 8.94 Runtime Wiring Notes](phase-8.94-runtime-wiring-notes.md)

## Related current documents

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Current Architecture State](../development/current-architecture-state.md)
- [Completed Phases](../development/completed-phases.md)
- [Strict Roadmap](../planning/roadmap.md)
