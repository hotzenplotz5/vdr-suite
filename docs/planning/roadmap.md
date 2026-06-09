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
- prepared for future multi-VDR environments
- suitable for multiple future clients and integrations

VDR remains the primary backend domain and source of truth.

VDR-Suite complements VDR. It does not replace it.

---

## Current Position

```text
Completed implementation state
Phase 13.7e - Snapshot Cache Generation Tracking

Current cleanup
Documentation and roadmap synchronization after Phase 13.7e

Next implementation step
Phase 13.8 - Live Transport Foundation
```

Phase 13.7e completed the runtime-owned snapshot change feed chain up to cache generation tracking.

Phase 13.8 should introduce a live transport foundation above the existing snapshot change feed without moving polling, change detection, snapshot generation or feed generation into the transport layer.

---

## Phase 13.8 - Live Transport Foundation

Goal:

Introduce a transport boundary for live update delivery.

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

Expected result:

- frontend-friendly live update foundation
- preserved backend neutrality
- preserved snapshot architecture
- no direct frontend coupling to polling internals
- no RESTfulAPI-specific logic outside the VDR adapter layer

Architecture rule:

Live transport consumes the snapshot change feed.

Live transport must not become the owner of:

- snapshot generation
- change detection
- feed generation
- backend-specific event parsing

---

## Phase 14 - Multi-VDR Backend Routing

Goal:

Prepare read-side architecture for multiple VDR instances.

Planned direction:

- backend identity aware routing
- multiple VDR instances
- backend federation preparation
- permission-aware architecture foundation

---

## Phase 15 - Frontend API Hardening

Goal:

Stabilize API contracts for future frontends.

Planned direction:

- filtering
- pagination
- stable response contracts
- capability-aware responses
- frontend-independent API behavior

---

## Phase 16 - Image and Preview Stream Validation

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

---

## Roadmap Rule

This file describes direction, not detailed implementation history.

Completed implementation detail belongs in [Completed Phases](../development/completed-phases.md).

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
