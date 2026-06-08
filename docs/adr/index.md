# Architecture Decision Records (ADR)

Navigation:

- ../../README.md
- ../index.md
- ../project-overview.md
- index.md

---

## Purpose

This section contains long-term architecture decisions.

Stable architecture descriptions belong in:

- ../architecture/index.md

Current implementation progress belongs in:

- ../development/current-status.md

---

## ADR Numbering Policy

Canonical ADR sequence:

ADR-0001
ADR-0002
...
ADR-0016

Next available ADR:

ADR-0017

The historical lowercase adr-001 to adr-007 files remain for repository history and compatibility.

No new ADRs should be created in the lowercase series.

---

## Canonical ADRs

- ADR-0001 Monorepo
- ADR-0002 SQLite
- ADR-0003 REST API
- ADR-0004 C++17
- ADR-0005 External VDR Integration Strategy
- ADR-0006 VDR Backend Architecture
- ADR-0007 RESTfulAPI Adapter Boundary
- ADR-0008 Real HTTP Server Strategy
- ADR-0009 HTTP Server Factory Strategy
- ADR-0010 Library-First VDR Architecture
- ADR-0011 VDR Source Model Architecture
- ADR-0012 Source Capability Model
- ADR-0013 Permission Model
- ADR-0014 Recording Identity Strategy
- ADR-0015 Timer Operation Boundary
- ADR-0016 Snapshot Change Feed Architecture

---

## Historical ADR Series

Retained for historical reference:

- adr-001 Backend Identity Strategy
- adr-002 Backend Federation Strategy
- adr-003 Backend Capability Strategy
- adr-004 Backend Lifecycle Strategy
- adr-005 Stream Provider Strategy
- adr-006 Internal Event Dispatch Strategy
- adr-007 Platform API Strategy

---

## Superseded ADRs

- ADR-0011 VDR Source Model (superseded)

---

## Related Documents

- ../architecture/index.md
- ../development/current-architecture-state.md
- ../development/current-status.md
- ../planning/roadmap.md

---

## Rules

- use the next canonical ADR number
- keep historical ADRs available
- keep superseded ADRs visible
- avoid duplicate ADR numbers
- link architecture summaries from docs/architecture

---

## Return Paths

- ../../README.md
- ../index.md
- ../project-overview.md
- index.md