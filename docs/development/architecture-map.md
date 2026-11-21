# Architecture Map

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Architecture Index](../architecture/index.md)
- [Planning Index](../planning/index.md)

---

## Purpose

This document helps new developers understand which documents should be read first when learning the VDR-Suite architecture.

It separates current implementation truth, accepted target architecture, domain dependencies and future implementation order.

---

## Recommended Reading Order

1. [Developer Onboarding](developer-onboarding.md)
2. [Current State](../CURRENT.md)
3. [Current Project Status](current-status.md)
4. [Current Architecture State](current-architecture-state.md)
5. [Target Platform Architecture](../architecture/target-platform-architecture.md)
6. [Domain Dependency Map](../planning/domain-dependency-map.md)
7. [Implementation Dependency Map](../planning/implementation-dependency-map.md)
8. [Strict Roadmap](../planning/roadmap.md)
9. [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)
10. [Architecture Index](../architecture/index.md)
11. [ADR Index](../adr/index.md)
12. [Completed Phases](completed-phases.md) when historical implementation detail is needed

---

## Core Concepts

- Control Plane and Backend Agent ownership
- Actor identity, authorization and backend access policy
- stable Suite identities versus backend-native references
- backend generation, lease, health and capability degradation
- snapshots, observations, revisions and event sequences
- safe mutations, idempotency and authoritative readback
- durable operations, jobs, attempts, sagas and reconciliation
- Suite-owned metadata, artwork and provenance
- ProgramEvent and BackendEventRef separation
- TimerIntent, TimerAssignment and NativeTimerBinding
- MediaSession, route, access grant and provider lease
- Legacy OSD viewer/controller separation and sequencing
- public `/api/v1` versus Agent, media, OSD and plugin contracts
- append-only AccountabilityEvent versus logs and diagnostics

---

## Current Versus Target Rule

```text
Current Architecture State
  describes implemented repository behavior

Target Platform Architecture
  describes accepted future ownership and contract boundaries

Domain Dependency Map
  describes conceptual prerequisite direction

Implementation Dependency Map and Roadmap
  describe future runtime order

Completed Phases
  records finished implementation only
```

Do not describe a target box, ADR or dependency-map entry as implemented until repository evidence, tests and the relevant phase exit criteria prove it.

---

## Back

- [Back to Development Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
