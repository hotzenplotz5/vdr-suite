# VDR-Suite – Architecture Decision Records

This index lists accepted architecture decision records.

The central documentation index is:

- [Documentation Index](../index.md)

---

## Foundation ADRs

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

---

## Future-Facing Backend ADRs

- [ADR-001: Backend Identity Strategy](adr-001-backend-identity-strategy.md)
- [ADR-002: Backend Federation Strategy](adr-002-backend-federation-strategy.md)
- [ADR-003: Backend Capability Strategy](adr-003-backend-capability-strategy.md)
- [ADR-004: Backend Lifecycle Strategy](adr-004-backend-lifecycle-strategy.md)
- [ADR-005: Stream Provider Strategy](adr-005-stream-provider-strategy.md)
- [ADR-006: Internal Event Dispatch Strategy](adr-006-internal-event-dispatch-strategy.md)
- [ADR-007: Platform API Strategy](007-platform-api-strategy.md)

---

## Historical / Superseded ADRs

- [ADR-0011: VDR Source Model](ADR-0011-vdr-source-model.md) – superseded by [ADR-0011: VDR Source Model Architecture](ADR-0011-vdr-source-model-architecture.md)

---

## Maintenance Rules

When adding an ADR:

- add it to this index
- link it from [Documentation Index](../index.md) when it is relevant to current architecture
- keep superseded ADRs visible for history, but mark them clearly
- avoid creating duplicate ADR numbers for unrelated decisions
