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
Phase 15.9 - Backend-aware Snapshot Builder

Current cleanup
Documentation and roadmap synchronization after Phase 15.9

Next implementation step
Phase 16.0 - Multi-Backend Polling Foundation
```

Phase 15 completed the read-side multi-backend foundation. Backend registry, backend registry service, backend registry JSON serialization, backend registry REST exposure, dynamic backend routes, multi-snapshot cache storage, backend-aware snapshot access and backend-aware snapshot building are implemented.

Phase 16 should connect this foundation to runtime polling without weakening daemon ownership, snapshot boundaries or backend neutrality.

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
```

Completed implementation detail belongs in [Completed Phases](../development/completed-phases.md).

---

## Phase 16.0 - Multi-Backend Polling Foundation

Goal:

Connect `BackendRegistry` to daemon-owned polling and snapshot generation.

Planned direction:

- keep polling daemon-owned
- keep snapshot generation backend-neutral
- introduce backend-aware polling preparation
- create snapshots with stable backend identity
- update `SnapshotCache` per backend
- preserve existing default-backend behavior
- avoid parallelism until the single-threaded multi-backend model is clear
- prepare later real-world validation with more than one VDR backend

Expected result:

- runtime can iterate backend registry entries
- each backend can produce a backend-tagged snapshot
- snapshot cache can hold runtime snapshots by backend ID
- existing `/api/vdr/...` default routes remain compatible
- `/api/backends/{backendId}/...` routes become backed by runtime data

Architecture rule:

Polling remains below the daemon runtime boundary.

Frontend routes must not own polling, backend selection or snapshot generation.

---

## Phase 16.1+ - Multi-Backend Runtime Validation

Goal:

Validate the runtime model before introducing concurrency or production-grade federation behavior.

Planned direction:

- test default-backend behavior after multi-backend polling preparation
- add controlled mock multi-backend runtime tests
- define when a real VDR is required for validation
- validate RESTfulAPI-backed polling against one real VDR
- validate two-backend scenarios when a second VDR or controlled mock backend is available

Real VDR tests should be reserved for:

- RESTfulAPI against an actual VDR
- SSE event streams
- polling against an actual VDR
- snapshot runtime against an actual VDR
- multi-VDR or multi-server scenarios

---

## Phase 17 - Live Transport Foundation

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

## Phase 18 - Frontend API Hardening

Goal:

Stabilize API contracts for future frontends.

Planned direction:

- filtering
- pagination
- stable response contracts
- capability-aware responses
- frontend-independent API behavior
- backend-aware response contracts
- frontend-safe error models

---

## Phase 19 - Image and Preview Stream Validation

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
