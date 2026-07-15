# VDR-Suite New Chat Handoff

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Current State](CURRENT.md)
- [Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Parity Audit and Frontend Gap Roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
- [ADR Index](adr/index.md)
- [GitHub Actions Status Handoff](development/github-actions-status-handoff.md)

---

## Purpose

This file is the compact project handoff that a new chat should read first.

It does not replace specialized implementation, acceptance or CI handoffs.

When GitHub Actions status matters, use [GitHub Actions Status Handoff](development/github-actions-status-handoff.md).

Detailed chronological phase history belongs in [Completed Phases](development/completed-phases.md).

---

## Required First Reading

A new chat should start with these files in this order:

1. [Current State](CURRENT.md)
2. [Phase Map](planning/phase-map.md)
3. [Roadmap](planning/roadmap.md)
4. [Current Project Status](development/current-status.md)
5. [ADR Index](adr/index.md)
6. [Current Architecture State](development/current-architecture-state.md)
7. [Parity Audit and Frontend Gap Roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
8. [Client API and Frontend Module Boundary Plan](development/client-api-frontend-module-boundary-plan.md)
9. [RESTfulAPI Integration Architecture](architecture/restfulapi-integration.md)
10. [EPGSearch Capability Matrix](development/epgsearch-capability-matrix.md)
11. [Completed Phases](development/completed-phases.md) only when historical detail is required
12. [GitHub Actions Status Handoff](development/github-actions-status-handoff.md) when CI state matters

---

## Current Repository Truth

Latest completed major project block:

```text
Phase 57 - Multi-Site Backend Administration and Permissions
```

Previous completed major project block:

```text
Phase 56 - Library Boundary, Packaging and Developer Documentation
```

Current umbrella implementation track:

```text
Phase 58 - Frontend and Live Parity
```

Latest completed implementation slice:

```text
Phase 60.14k - Recording Detail UX Polish
```

Next planned implementation slice:

```text
Phase 60.15 - Recording Metadata and Poster Preparation
```

The completed 59.x and 60.x slices are already-used frontend API, module, platform, Recording cache and Recording UX implementation ranges under the continuing frontend track.

Future major milestones therefore use:

```text
Phase 61 - Suite Metadata Database and External Providers
Phase 62 - Recommendation and Content Knowledge Graph
```

Do not renumber completed implementation history.

---

## Current Architecture Package

The source audit covered:

- VDR Core
- epgsearch
- Live
- RESTfulAPI
- Streamdev
- TVScraper
- scraper2vdr
- osd2web
- epg2vdr
- epgd

The first accepted follow-up ADR package is:

```text
ADR-0038 - Suite Metadata Database and External Provider Strategy
ADR-0039 - Backend Agent and Control Plane Boundary
ADR-0040 - Backend Lifecycle, Generation, Lease and Health
ADR-0041 - Authentication, Agent Trust and Multi-Site Transport
```

Core conclusions:

- VDR remains the native source of truth.
- VDR-Suite remains an external orchestration and platform layer.
- RESTfulAPI, SVDRP, Streamdev, TVScraper, epgsearch and OSD integrations remain behind adapter or provider boundaries.
- Multi-site production architecture uses a Control Plane and local Backend Agents.
- Backend Agents do not receive direct central database access.
- Remote sites do not expose VDR-internal plugin ports as public platform APIs.
- Stable BackendId, backend generation, lease and health are separate concepts.
- Read-only backend policy remains server-enforced.
- Metadata is normalized into suite-owned entities while acquisition remains provider-based.

---

## Verified Runtime Foundation

The repository already contains verified foundations for:

- daemon and REST runtime
- RESTfulAPI adapter boundary
- BackendNode and BackendRegistry
- backend-aware snapshot storage and reads
- snapshot change feed and SSE foundation
- runtime diagnostics
- Recording action request, preview, validation, planning and execution boundaries
- guarded real-backend Recording action probes
- Timer action boundaries
- backend-neutral SearchTimer workflows
- backend-scoped EPG cache and queries
- lazy SQLite-backed Recording cache
- Web Client API wrapper
- frontend module ownership and registry foundations
- read-only backend access handling

Do not describe these areas as wholly missing.

---

## Guarded or Incomplete Areas

The following remain incomplete or intentionally closed:

- user authentication and role-based authorization
- Agent enrollment and secure remote-site transport implementation
- backend generation, lease and health runtime model
- expected revision and idempotency for mutations
- production Job claim, retry, verification and rollback model
- TimerIntent, TimerAssignment, scheduler and reconciler
- canonical ProgramEvent identity above native backend events
- suite metadata database implementation
- artwork asset service
- Streaming Gateway
- Legacy OSD compatibility bridge
- public `/api/v1` compatibility contract
- audit and security event model
- final Web, Windows, Android, iOS and TV clients

---

## Next Work

Immediate implementation slice:

```text
Phase 60.15 - Recording Metadata and Poster Preparation
```

Next architecture package:

```text
ADR-0042 - Safe Mutation, Revision and Idempotency Contract
ADR-0043 - Job Claim, Retry and Saga Execution Model
ADR-0044 - Timer Intent, Assignment and Native Timer Model
ADR-0045 - Canonical EPG Event Identity and Provenance
```

Do not begin runtime implementation of those new concepts before the relevant ADR is accepted and mapped against existing code.

---

## Project Workflow Rules

- Architecture and cause analysis come before code changes.
- Inspect current repository state instead of relying on old chat summaries.
- Use the next free canonical ADR number from `docs/adr/index.md`.
- Do not create duplicate or lowercase ADR sequences.
- Keep RESTfulAPI behind adapter boundaries.
- Keep frontend modules free of direct backend fetch ownership.
- Preserve backend identity in all multi-backend reads and actions.
- Read-only policy must be enforced by backend services, not only by hidden UI controls.
- Recording and Timer writes remain guarded and explicit.
- Real destructive probes remain closed by default.
- Run the relevant documentation, phase and architecture checks after documentation changes.

---

## Documentation Verification

For documentation and ADR changes, run at least:

```bash
make test-docs
make test-phase-map-coverage
make test-phase
```

Also run any ADR-index or documentation reachability checks included by `make test-docs`.

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
- [Back to Current State](CURRENT.md)
