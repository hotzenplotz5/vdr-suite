# VDR-Suite – Architecture Index

This document is the entry point for architecture documentation.

The central documentation index is:

- [Documentation Index](../index.md)

---

## Core Platform

- [Core Platform Model](vdr-suite-core-platform-model.md)
- [Suite Components](suite-components.md)
- [Media Platform Comparison](media-platform-comparison.md)

---

## VDR Backend Architecture

- [VDR Backends](vdr-backends.md)
- [VDR Domain Model](vdr-domain-model.md)
- [RESTfulAPI Integration](restfulapi-integration.md)
- [External Project Analysis](external-project-analysis.md)

---

## REST and HTTP Runtime

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

## Event and Change Architecture

- [Internal Event Dispatch Architecture](internal-event-dispatch-architecture.md)
- [Snapshot Change Feed Architecture](snapshot-change-feed-architecture.md)

---

## Historical Phase-Specific Architecture Notes

- [Phase 8.94 Snapshot Cache Integration Plan](phase-8.94-snapshot-cache-integration-plan.md)
- [Phase 8.94 Runtime Wiring Notes](phase-8.94-runtime-wiring-notes.md)

---

## Architecture Decision Records

Architecture decisions are tracked separately in:

- [ADR Index](../adr/index.md)

---

## Maintenance Rules

When adding a new architecture document:

- link it from this architecture index
- link it from the central [Documentation Index](../index.md) if it is generally relevant
- create or update an ADR if the document contains a long-term architectural decision
- keep phase-specific notes separate from stable architecture documents
