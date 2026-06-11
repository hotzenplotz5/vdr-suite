# VDR-Suite Roadmap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Planning Index](index.md)

---

## Purpose

This roadmap describes the forward direction of VDR-Suite.

Current project status belongs to:

- [Current Project Status](../development/current-status.md)
- [Project Status Dashboard](../project-status-dashboard.md)

Completed implementation history belongs to:

- [Completed Phases](../development/completed-phases.md)

---

## Roadmap Principles

VDR-Suite remains:

- VDR-centered
- backend-neutral
- daemon-driven
- snapshot-oriented
- service-oriented
- prepared for multi-VDR environments
- suitable for multiple future clients and integrations

VDR remains the primary backend domain and source of truth.

VDR-Suite complements VDR. It does not replace it.

---

## Current Position

```text
Completed implementation state
Phase 19.3 - Snapshot Change Feed REST Validation

Current cleanup
Documentation and roadmap synchronization after Phase 19.3

Next implementation step
Phase 20.0 - Live Transport Foundation
```

Phase 16 completed the multi-backend polling and runtime context foundation. Backend-aware polling, backend polling coordination, backend runtime contexts, daemon runtime context migration, registry-driven context creation and backend-aware snapshot change feed support are implemented.

Phase 17 completed the initial multi-backend read-side visibility through Phase 17.3. SnapshotAccessService can return all cached backend snapshots, VdrSnapshotReadService exposes multi-backend snapshot lists, VdrSnapshotReadJsonSerializer serializes multi-backend snapshot summaries, `GET /api/vdr/snapshots` exposes these summaries through REST and the route is covered by the API router regression test.

Phase 18 completed opt-in real VDR and RESTfulAPI validation through Phase 18.4. Real RESTfulAPI integration, real snapshot building, real change-state handling, real polling initial snapshot generation and repeated polling stability are validated outside the default fast test set.

Phase 18 completed opt-in real VDR and RESTfulAPI validation through Phase 18.4. Real RESTfulAPI integration, real snapshot building, real change-state handling, real polling initial snapshot generation and repeated polling stability are validated outside the default fast test set.

---

## Implemented Foundation Since Phase 14

The following roadmap intentions have moved from planning into implemented foundation work:

```text
Phase 14.8 - BackendRegistry runtime integration
Phase 14.9 - BackendRegistry service layer
Phase 15.0 - BackendRegistry JSON serializer
Phase 15.1 - BackendRegistry controller
Phase 15.2 - BackendRegistry API routing
Phase 15.3 - Backend-aware snapshot read service
Phase 15.4 - Backend-aware VDR controller methods
Phase 15.5 - Backend snapshot API routes
Phase 15.6 - Dynamic backend snapshot route parsing
Phase 15.7 - Multi-snapshot cache foundation
Phase 15.8 - Multi-backend snapshot cache access
Phase 15.9 - Backend-aware snapshot builder
Phase 16.0 - Backend-aware snapshot cache service updates
Phase 16.1 - Backend-aware polling service
Phase 16.2 - Backend polling coordinator
Phase 16.3 - Backend runtime context foundation
Phase 16.4 - Daemon runtime context migration
Phase 16.5 - Coordinator runtime integration
Phase 16.6 - Runtime context collection
Phase 16.7 - Runtime context factory
Phase 16.8 - Runtime context creation from registry
Phase 16.9 - Backend-aware snapshot change feed
Phase 17.0 - Multi-backend snapshot read foundation
Phase 17.1 - Multi-backend snapshot summary serialization
Phase 17.2 - Multi-backend snapshots REST endpoint
Phase 17.3 - Multi-backend REST endpoint tests
Phase 18.0 - Real RESTfulAPI integration validation
Phase 18.1 - Real snapshot builder validation
Phase 18.2 - Real change-state validation
Phase 18.3 - Real polling initial snapshot validation
Phase 18.4 - Real polling stability validation
Phase 19.0 - Snapshot change feed service validation
Phase 19.1 - Polling to change feed runtime validation
Phase 19.2 - Multi-backend change feed aggregation
Phase 19.3 - Snapshot change feed REST validation
Phase 19.3 - Snapshot change feed REST validation
```

Completed implementation detail belongs in [Completed Phases](../development/completed-phases.md).

---

## Phase 18 - Real VDR and RESTfulAPI Integration Validation

Status:

Completed through Phase 18.4.

Goal:

Validate the snapshot runtime and RESTfulAPI adapter against an actual VDR system once the multi-backend REST contracts are stable.

Completed validation:

- RESTfulAPI reachability against one local VDR
- status, channels, timers, events and recordings mapping with real data
- snapshot generation from real RESTfulAPI responses
- real change-state handling
- daemon-owned polling against a real VDR
- repeated polling stability without generating false change events

Real VDR tests remain reserved for:

- RESTfulAPI against an actual VDR
- SSE event streams
- polling against an actual VDR
- snapshot runtime against an actual VDR
- multi-VDR or multi-server scenarios

Architecture rule:

Real VDR tests must stay opt-in and outside the default fast test set.

GitHub Actions must remain independent from a running VDR.

---

## Phase 19 - Snapshot Change Feed Validation

Goal:

Validate the existing snapshot change feed end-to-end before introducing live transport.

Planned direction:

- validate snapshot change feed creation from detected VDR changes
- validate backend-aware feed entries
- validate sequence number and snapshot generation behavior
- validate that unchanged polling cycles do not create feed entries
- validate the REST read boundary for `GET /api/vdr/changes`
- keep validation transport-independent
- keep real VDR validation opt-in and outside the default fast test set

Architecture rule:

Snapshot change feed validation must happen before SSE or WebSocket work.

Live transport must consume the snapshot change feed later. It must not become the owner of polling, snapshot generation, change detection or feed generation.

---

## Phase 20 - Live Transport Foundation

Goal:

Introduce a transport boundary for live update delivery above the snapshot change feed.

Expected direction:

- define the transport-facing boundary above `SnapshotChangeFeed`
- keep transport independent from polling internals
- keep transport independent from RESTfulAPI adapter details
- preserve daemon ownership of runtime state
- prepare Server Sent Events without committing WebSocket design too early

Candidate transports:

- Server Sent Events (SSE)
- WebSocket
- long polling fallback

Architecture rule:

Live transport consumes the snapshot change feed.

Live transport must not become the owner of:

- snapshot generation
- change detection
- feed generation
- backend-specific event parsing

---

## Phase 21 - Image and Preview Stream Validation

Goal:

Validate how VDR-Suite should expose image or preview stream data to future clients.

Planned direction:

- define image and preview stream use cases
- separate metadata images from live stream previews
- avoid coupling stream delivery to snapshot change transport
- evaluate HTTP streaming boundaries
- prepare frontend-independent preview contracts

Important boundary:

Live update transport is not the same as image or media streaming.

Live transport should notify clients that something changed.

Image, preview stream and media stream handling should define how clients can request media-oriented data.

---

## Phase 22 - Frontend API Hardening

Goal:

Stabilize API contracts for future frontends after the snapshot change feed and live transport foundations are validated.

Planned direction:

- filtering
- pagination
- stable response contracts
- capability-aware responses
- frontend-independent API behavior
- backend-aware response contracts
- frontend-safe error models

---

## Phase 22 - Frontend API Hardening

Goal:

Stabilize API contracts for future frontends after the snapshot change feed and live transport foundations are validated.

Planned direction:

- filtering
- pagination
- stable response contracts
- capability-aware responses
- frontend-independent API behavior
- backend-aware response contracts
- frontend-safe error models

---

## Future Product Layers

Possible later layers:

- Web frontend
- Windows frontend
- Android frontend
- iOS frontend
- OSD integration
- stream provider integration
- media library expansion
- authentication and authorization
- permission-aware backend federation

---

## Roadmap Rule

This file describes direction, not detailed implementation history.

Completed implementation detail belongs in [Completed Phases](../development/completed-phases.md).

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
