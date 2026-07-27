# VDR-Suite Project Overview

## Purpose

VDR-Suite is a VDR-centred, domain-first platform for modern Web, mobile, desktop and TV clients. It complements rather than forks or replaces VDR internals.

```text
VDR
  remains native authority for devices, schedules, timers, recordings, replay and plugins

VDR-Suite
  owns external domain models, backend scope, policy, orchestration,
  persistent read models and client-facing contracts
```

## Current project position

Baseline reconciled on 2026-07-27 against `origin/main` commit `44ae3102ab202ee0dfc974ee0bc9624b9219ad2d`.

```text
Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Completed post-phase platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)

Next strict runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation
```

## Implemented product surface

VDR-Suite currently provides:

- backend-aware status, channels, EPG, recordings and timers;
- EPG timeline and channel-day programme navigation;
- Recordings 2 as the sole delivered Recording browser, including guarded actions;
- SearchTimer list, preview, validation and controlled mutation foundations;
- persistent provider-neutral metadata, people, artwork and Genres;
- Recording and EPG Genre browsing with backend isolation;
- TVScraper-backed details behind Suite-owned routes;
- backend-scoped global search over persisted Recording/EPG titles, subtitles and people;
- VDR remote actions and live-overlay status;
- server-enforced read-only backend mode;
- modular frontend ownership through `VdrSuiteClientApi`;
- packaging, staging and real-system acceptance workflows.

## Architecture strengths

- provider data is evidence with provenance, not hidden Suite authority;
- RESTfulAPI, SVDRP, Streamdev, TVScraper and SuiteBridge remain private adapters/providers;
- browsers consume Suite-owned contracts rather than backend-specific URLs;
- normal Genre and search GET paths use query-only SQLite reads and perform no provider resolution;
- stable backend scope and read-only policy are enforced server-side;
- mutation safety and readback foundations are stronger than direct plugin passthrough;
- target ADRs are kept separate from current runtime completion status.

## What is not complete

VDR-Suite is not yet a full replacement for every Live, epgsearch or RESTfulAPI surface. Major remaining work includes:

- production user identity, scoped RBAC and accountability;
- secure Backend Agents and multi-site command fencing;
- central TimerIntent orchestration and reconciliation;
- Streaming Gateway and media sessions;
- legacy OSD compatibility bridge;
- stable versioned `/api/v1`, common errors and ETags;
- exact remaining epgsearch edge semantics;
- later recommendation and knowledge-graph behaviour.

## Strict next step

```text
Phase 62 - Identity, RBAC and Accountability Foundation
```

Phase 62 introduces actor identities, server-side authorization, request/correlation context, append-only accountability evidence and a transactional outbox before later remote privileged operations.

## Authoritative navigation

- [Current State](CURRENT.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Project Status](development/current-status.md)
- [Current Architecture State](development/current-architecture-state.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Completed History](development/completed-phases.md)
- [Architecture Documentation](architecture/index.md)
- [ADR Index](adr/index.md)