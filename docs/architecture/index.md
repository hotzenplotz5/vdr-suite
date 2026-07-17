# Architecture Documentation

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)
- [Current State](../CURRENT.md)
- [Strict Roadmap](../planning/roadmap.md)

---

## Purpose

This section contains the stable architecture of VDR-Suite.

Implementation progress belongs in development documents. Long-term decisions belong in ADRs. Planned dependency order belongs in the planning section.

---

## Canonical Target Architecture

- [Target Platform Architecture](target-platform-architecture.md)
- [Domain Dependency Map](../planning/domain-dependency-map.md)
- [Implementation Dependency Map](../planning/implementation-dependency-map.md)
- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)

The Target Platform Architecture is the canonical diagram set for the contract package accepted through ADR-0049. The dependency maps distinguish domain ownership from runtime implementation order.

A target diagram is not evidence of completed runtime behavior. Use [Current Architecture State](../development/current-architecture-state.md) and [Completed Phases](../development/completed-phases.md) for implementation truth.

---

## Core Platform

- [VDR-Suite Core Platform Model](vdr-suite-core-platform-model.md)
- [Suite Components](suite-components.md)
- [Media Platform Comparison](media-platform-comparison.md)

The Core Platform Model is an earlier conceptual foundation. Where its older future wording differs from the accepted ADR-0038 through ADR-0049 package, the canonical Target Platform Architecture and active ADRs take precedence.

---

## Metadata Platform

- [Metadata Identity Foundation](metadata-identity-foundation.md)
- [ADR-0038: Suite Metadata Database and External Provider Strategy](../adr/ADR-0038-suite-metadata-database-and-external-provider-strategy.md)
- [Recording Metadata Roadmap](../planning/tvscraper-recording-metadata-roadmap.md)

The identity foundation defines the Phase 61.1 Suite-owned Entity, Assignment and Target identity boundary. Provider acquisition, persistence, resolver behavior and artwork storage remain later Phase 61 slices.

---

## Backend Architecture

- [VDR Backends](vdr-backends.md)
- [VDR Domain Model](vdr-domain-model.md)
- [RESTfulAPI Integration](restfulapi-integration.md)
- [External Project Analysis](external-project-analysis.md)
- [Recording Actions Architecture](recording-actions-architecture.md)
- [Recording Action Validation API](recording-action-validation-api.md)
- [Recording Action Execution API](recording-action-execution-api.md)
- [Recording Action Safety and Capability Model](recording-action-safety-capability-model.md)
- [Recording Action Source Constraints](recording-action-source-constraints.md)
- [Recording Action Real Backend Smoke Test Plan](recording-action-real-backend-smoke-test-plan.md)
- [Recording Action Real HTTP Client Gap](recording-action-real-http-client-gap.md)

---

## Runtime Architecture

- [REST API Runtime](rest-api-runtime.md)
- [Daemon REST Runtime](daemon-rest-runtime.md)
- [HTTP Server Boundary](http-server-boundary.md)
- [Test HTTP Server](test-http-server.md)

---

## Snapshot Architecture

- [Snapshot Architecture](snapshot-architecture.md)
- [Snapshot Access Architecture](snapshot-access-architecture.md)
- [Partial Snapshot Refresh Architecture](partial-snapshot-refresh-architecture.md)
- [Snapshot Change Feed Architecture](snapshot-change-feed-architecture.md)

---

## Event Architecture

- [Internal Event Dispatch Architecture](internal-event-dispatch-architecture.md)

---

## Historical Notes

- [Phase 8.94 Snapshot Cache Integration Plan](phase-8.94-snapshot-cache-integration-plan.md)
- [Phase 8.94 Runtime Wiring Notes](phase-8.94-runtime-wiring-notes.md)

Historical implementation notes are retained for reference. They do not override later accepted ADRs or the target diagram set.

---

## Architecture Decisions

See:

- [Architecture Decision Records](../adr/index.md)
- [Architecture-scoped Decision Records](adr/index.md)

---

## Related Documents

- [Current Architecture State](../development/current-architecture-state.md)
- [Current Project Status](../development/current-status.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)

---

## Rules

- stable architecture belongs here;
- target architecture and current implementation state remain explicitly separate;
- implementation progress belongs in development;
- dependency order belongs in planning;
- historical implementation notes remain historical;
- long-term decisions belong in ADRs;
- plugins, Agents, providers and frontends do not bypass the ownership boundaries in the target architecture.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
