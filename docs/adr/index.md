# Architecture Decision Records (ADR)

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Current State](../CURRENT.md)
- [Project Overview](../project-overview.md)
- [Architecture Documentation](../architecture/index.md)
- [Strict Roadmap](../planning/roadmap.md)

---

## Purpose

This section contains long-term architecture decisions.

Stable architecture descriptions belong in:

- [Architecture Documentation](../architecture/index.md)

Current implementation progress belongs in:

- [Current State](../CURRENT.md)
- [Current Project Status](../development/current-status.md)

The Strict Roadmap owns future phase sequencing. ADRs own stable architecture and may declare prerequisites, but accepted architecture does not by itself authorize or complete runtime work.

---

## ADR Numbering Policy

Canonical ADR sequence currently runs through:

```text
ADR-0054
```

Latest accepted ADRs at the end of the sequence:

- [ADR-0051: Manual Recording Metadata Assignment](ADR-0051-manual-recording-metadata-assignment.md)
- [ADR-0052: Manual Recording Cast Ingestion and Search Integration](ADR-0052-manual-recording-cast-ingestion-search.md)
- [ADR-0053: Client Playback Engine and Media Adaptation Strategy](ADR-0053-client-playback-engine-media-adaptation-strategy.md)
- [ADR-0054: Broadcast Companion Services — Teletext and HbbTV](ADR-0054-broadcast-companion-teletext-hbbtv.md)

Next available canonical ADR:

```text
ADR-0055
```

Rules:

- Use the next canonical `ADR-00xx` number for new ADRs.
- Do not create new lowercase `adr-00x` files.
- Do not create new legacy numeric files such as `007-*` or `008-*`.
- Do not list duplicate active/proposed ADR numbers.
- A Proposed ADR is not an implemented or accepted runtime boundary.
- If an old ADR is superseded or has a numbering conflict, keep it visible only in the historical/superseded sections and state the replacement.

---

## Active Canonical ADRs

### Repository, Storage and Language Foundation

- [ADR-0001: Monorepo for VDR-Suite](ADR-0001-monorepo.md)
- [ADR-0002: SQLite as Central Metadata Database](ADR-0002-sqlite.md)
- [ADR-0003: REST API as External Interface](ADR-0003-rest-api.md)
- [ADR-0004: C++17 Minimum Standard](ADR-0004-cpp17.md)
- [ADR-0050: Domain Repository SQLite Boundary](ADR-0050-domain-repository-sqlite-boundary.md)

### VDR Backend and Runtime Architecture

- [ADR-0005: External VDR Integration Strategy](ADR-0005-external-vdr-integration-strategy.md)
- [ADR-0006: VDR Backend Architecture](ADR-0006-vdr-backend-architecture.md)
- [ADR-0007: RESTfulAPI Adapter Boundary](ADR-0007-restfulapi-adapter-boundary.md)
- [ADR-0008: Real HTTP Server Strategy](ADR-0008-real-http-server-strategy.md)
- [ADR-0009: HTTP Server Factory Strategy](ADR-0009-http-server-factory-strategy.md)
- [ADR-0010: Library First VDR Architecture](ADR-0010-library-first-vdr-architecture.md)
- [ADR-0011: VDR Source Model Architecture](ADR-0011-vdr-source-model-architecture.md)
- [ADR-0012: Source Capability Model](ADR-0012-source-capability-model.md)
- [ADR-0013: Permission Model](ADR-0013-permission-model.md)
- [ADR-0014: Recording Identity Strategy](ADR-0014-recording-identity-strategy.md)
- [ADR-0015: Timer Operation Boundary](ADR-0015-timer-operation-boundary.md)
- [ADR-0016: Snapshot Change Feed Architecture](ADR-0016-snapshot-change-feed-architecture.md)
- [ADR-0017: Live Transport Boundary](ADR-0017-live-transport-boundary.md)
- [ADR-0018: Incremental Snapshot Synchronization](ADR-0018-incremental-snapshot-synchronization.md)
- [ADR-0019: SSE Event Stream Transport Strategy](ADR-0019-sse-event-stream-transport-strategy.md)
- [ADR-0020: Multi-Source Federation Architecture](ADR-0020-multi-source-federation-architecture.md)
- [ADR-0021: Selective Backend Query Strategy](ADR-0021-selective-backend-query-strategy.md)
- [ADR-0022: LIVE Functional Reference Strategy](ADR-0022-live-functional-reference-strategy.md)
- [ADR-0023: LIVE Superset Strategy](ADR-0023-live-superset-strategy.md)
- [ADR-0024: Recording Action Transport Mapping](ADR-0024-recording-action-transport-mapping.md)

### Metadata, Search and UI Strategy

- [ADR-0025: Configurable Metadata Provider Architecture](ADR-0025-configurable-metadata-provider-architecture.md)
- [ADR-0026: External Orchestration Layer Above VDR](ADR-0026-external-orchestration-layer-above-vdr.md)
- [ADR-0027: VDR-First Implementation With Future Media Federation](ADR-0027-vdr-first-implementation-with-future-media-federation.md)
- [ADR-0028: Content Classification Architecture](ADR-0028-content-classification-architecture.md)
- [ADR-0029: Backend-Neutral SearchTimer Architecture](ADR-0029-backend-neutral-searchtimer-architecture.md)
- [ADR-0030: Domain-First UI Over OSD Proxy](ADR-0030-domain-first-ui-over-osd-proxy.md)
- [ADR-0031: Person Catalog and External Filmography Architecture](ADR-0031-person-catalog-and-external-filmography.md)
- [ADR-0032: EPGSearch Regex Mode Safety](ADR-0032-epgsearch-regex-mode-safety.md)
- [ADR-0033: EPGSearch Fuzzy Mode Decision](ADR-0033-epgsearch-fuzzy-mode-decision.md)
- [ADR-0034: SearchTimer Warm EPG Cache and Change Invalidation](ADR-0034-searchtimer-warm-epg-cache-change-invalidation.md)
- [ADR-0035: Lazy Recording Loading and Backend-Scoped Refresh](ADR-0035-lazy-recording-loading-and-backend-scoped-refresh.md)
- [ADR-0036: TVScraper Recording Metadata Integration Strategy](ADR-0036-tvscraper-recording-metadata-integration.md)
- [ADR-0038: Suite Metadata Database and External Provider Strategy](ADR-0038-suite-metadata-database-and-external-provider-strategy.md)
- [ADR-0051: Manual Recording Metadata Assignment](ADR-0051-manual-recording-metadata-assignment.md)
- [ADR-0052: Manual Recording Cast Ingestion and Search Integration](ADR-0052-manual-recording-cast-ingestion-search.md)

### Packaging and Install Boundary

- [ADR-0037: Packaging, Install Layout and API Boundary](ADR-0037-packaging-install-api-boundary.md)

### Control Plane, Trust, Mutation Safety, Orchestration, Media, Compatibility, Public API and Audit

- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0043: Job Claim, Retry and Saga Execution Model](ADR-0043-job-claim-retry-saga-execution-model.md)
- [ADR-0044: Timer Intent, Assignment and Native Timer Model](ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0045: Canonical EPG Event Identity and Provenance](ADR-0045-canonical-epg-event-identity-provenance.md)
- [ADR-0046: Streaming Gateway and Media Session Boundary](ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0053: Client Playback Engine and Media Adaptation Strategy](ADR-0053-client-playback-engine-media-adaptation-strategy.md)
- [ADR-0054: Broadcast Companion Services — Teletext and HbbTV](ADR-0054-broadcast-companion-teletext-hbbtv.md)
- [ADR-0047: Legacy OSD Compatibility Bridge](ADR-0047-legacy-osd-compatibility-bridge.md)
- [ADR-0048: Public API Versioning, Error and Compatibility Contract](ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [ADR-0049: Audit and Security Event Model](ADR-0049-audit-security-event-model.md)

---

## Proposed Canonical ADRs

None currently.

---

## Superseded Canonical ADRs

- [ADR-0011: VDR Source Model](ADR-0011-vdr-source-model.md), superseded by [ADR-0011: VDR Source Model Architecture](ADR-0011-vdr-source-model-architecture.md)

---

## Historical ADRs

Historical lowercase and numeric ADRs are retained for repository history and compatibility:

- [ADR-001 Backend Identity Strategy](adr-001-backend-identity-strategy.md)
- [ADR-002 Backend Federation Strategy](adr-002-backend-federation-strategy.md)
- [ADR-003 Backend Capability Strategy](adr-003-backend-capability-strategy.md)
- [ADR-004 Backend Lifecycle Strategy](adr-004-backend-lifecycle-strategy.md), superseded in direction by [ADR-0040](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-005 Stream Provider Strategy](adr-005-stream-provider-strategy.md)
- [ADR-006 Internal Event Dispatch Strategy](adr-006-internal-event-dispatch-strategy.md)
- [ADR-007: Platform API Strategy](007-platform-api-strategy.md)
- [ADR-008: Runtime Observability Strategy](008-runtime-observability-strategy.md)

---

## Numbering Cleanup

The former conflicting file:

```text
ADR-0037-suite-metadata-database-and-external-scraper-strategy.md
```

was replaced by the canonical:

- [ADR-0038: Suite Metadata Database and External Provider Strategy](ADR-0038-suite-metadata-database-and-external-provider-strategy.md)

`ADR-0037` remains assigned only to Packaging, Install Layout and API Boundary.

---

## Future-phase sequencing note

The Strict Roadmap owns phase numbering/order. Accepted ADR-0054 establishes the post-Phase-65 sequence:

```text
65 Streaming
66 Broadcast Companion: Teletext + HbbTV
67 Legacy OSD
68 Public API Hardening
69 Recommendation / Knowledge Graph
```

ADR-0054's sequencing clause supersedes only older future phase-number statements inside ADR-0047/ADR-0048. Their architecture decisions remain accepted.

---

## Related Documents

- [Current State](../CURRENT.md)
- [Architecture Documentation](../architecture/index.md)
- [Current Architecture State](../development/current-architecture-state.md)
- [Current Project Status](../development/current-status.md)
- [Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
