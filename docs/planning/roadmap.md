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
Completed Major Phase
Phase 12 - Snapshot Change Feed Foundation

Current Cleanup
Documentation and roadmap consolidation

Next Major Phase
Phase 13 - Live Update Transport
```

Phase 12 completed the transport-independent change feed foundation.

Phase 13 should add live update transport without moving change detection or feed generation into the transport layer.

---

## Phase 13 - Live Update Transport

Goal:

Expose snapshot change feed information through a live transport mechanism.

Candidate transports:

- Server Sent Events (SSE)
- WebSocket

Expected result:

- live change delivery
- frontend-friendly update channel
- preserved backend neutrality
- preserved snapshot architecture
- no direct frontend coupling to polling internals

Architecture rule:

Live transport consumes the snapshot change feed.

Live transport must not become the owner of:

- snapshot generation
- change detection
- feed generation

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

Phase 13 should notify clients that something changed.

Phase 16 should define how clients can request image or preview stream data.

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