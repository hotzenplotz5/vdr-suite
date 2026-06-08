# VDR-Suite Architecture Decision Records

Return:

- ../index.md
- ../project-overview.md

---

## Purpose

This index lists accepted architecture decision records.

ADRs document long-term architecture decisions.

Stable architecture overviews belong in:

- ../architecture/index.md

Current project state belongs in:

- ../development/current-status.md

---

## ADR Numbering Policy

The canonical ADR numbering series is:

```text
ADR-0001
ADR-0002
...
ADR-0016
```

New ADRs must continue this canonical series.

The older lowercase files `adr-001` through `adr-007` are kept as a historical secondary series because they are already part of repository history and may be linked from older documents or commits.

Do not add new ADRs to the lowercase `adr-00x` series.

Next canonical ADR number:

```text
ADR-0017
```

---

## Canonical ADRs

- [ADR-0001: Monorepo](ADR-0001-monorepo.md)
- [ADR-0002: SQLite](ADR-0002-sqlite.md)
- [ADR-0003: REST API](ADR-0003-rest-api.md)
- [ADR-0004: C++17](ADR-0004-cpp17.md)
- [ADR-0005: External VDR Integration Strategy](ADR-0005-external-vdr-integration-strategy.md)
- [ADR-0006: VDR Backend Architecture](ADR-0006-vdr-backend-architecture.md)
- [ADR-0007: RESTfulAPI Adapter Boundary](ADR-0007-restfulapi-adapter-boundary.md)
- [ADR-0008: Real HTTP Server Strategy](ADR-0008-real-http-server-strategy.md)
- [ADR-0009: HTTP Server Factory Strategy](ADR-0009-http-server-factory-strategy.md)
- [ADR-0010: Library-First VDR Architecture](ADR-0010-library-first-vdr-architecture.md)
- [ADR-0011: VDR Source Model Architecture](ADR-0011-vdr-source-model-architecture.md)
- [ADR-0012: Source Capability Model](ADR-0012-source-capability-model.md)
- [ADR-0013: Permission Model](ADR-0013-permission-model.md)
- [ADR-0014: Recording Identity Strategy](ADR-0014-recording-identity-strategy.md)
- [ADR-0015: Timer Operation Boundary](ADR-0015-timer-operation-boundary.md)
- [ADR-0016: Snapshot Change Feed Architecture](ADR-0016-snapshot-change-feed-architecture.md)

---

## Historical Secondary ADR Series

These files are retained for historical context.

They should not be used as the numbering model for new ADRs.

- [adr-001: Backend Identity Strategy](adr-001-backend-identity-strategy.md)
- [adr-002: Backend Federation Strategy](adr-002-backend-federation-strategy.md)
- [adr-003: Backend Capability Strategy](adr-003-backend-capability-strategy.md)
- [adr-004: Backend Lifecycle Strategy](adr-004-backend-lifecycle-strategy.md)
- [adr-005: Stream Provider Strategy](adr-005-stream-provider-strategy.md)
- [adr-006: Internal Event Dispatch Strategy](adr-006-internal-event-dispatch-strategy.md)
- [adr-007: Platform API Strategy](007-platform-api-strategy.md)

---

## Superseded ADRs

- [ADR-0011: VDR Source Model](ADR-0011-vdr-source-model.md) - superseded by [ADR-0011: VDR Source Model Architecture](ADR-0011-vdr-source-model-architecture.md)

---

## Rules

When adding an ADR:

- use the next canonical ADR number
- add it to the canonical ADR section
- link stable architecture summaries from docs/architecture
- keep superseded ADRs visible for history
- do not create new lowercase adr-00x files
- avoid duplicate ADR numbers for unrelated decisions
