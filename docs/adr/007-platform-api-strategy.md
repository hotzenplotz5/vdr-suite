# ADR-007: Platform API Strategy

## Status

Accepted

## Context

VDR-Suite is no longer only a direct application layer around VDR data.

The implemented architecture already contains several platform-like building blocks:

- REST API controllers
- backend adapter boundary
- daemon-owned snapshot runtime
- `SnapshotCache`
- `SnapshotCacheService`
- `SnapshotAccessService`
- change-state polling
- `VdrChangeEvent`
- `SnapshotRefreshPlanner`
- `SnapshotUpdatePlan`
- runtime logging abstraction
- backend identity, federation, capability, lifecycle and stream-provider ADRs

These components make VDR-Suite useful not only for its own future frontend, but also as a possible integration layer for other clients and tools.

Potential future consumers include:

- VDR-Suite Web UI
- VDR-Suite desktop clients
- VDR-Suite mobile clients
- VDR OSD integration
- `vdr-plugin-live`
- Kodi integrations
- Android applications
- Home Assistant integrations
- automation scripts
- monitoring tools
- third-party VDR management tools

Without an explicit strategy, the API could accidentally become shaped only by the first internal frontend implementation. That would make later third-party or multi-client use harder.

## Decision

VDR-Suite shall be treated as a VDR-centered backend platform with a stable integration API, not only as a single frontend application.

The VDR-Suite API should be designed as a reusable service boundary for multiple clients.

This does not mean that all APIs must be public, versioned or complete immediately. It means that new API design must avoid unnecessary coupling to a single UI, a single frontend technology or one local-only deployment shape.

## Principles

- VDR remains the primary domain and source of truth.
- VDR-Suite does not become a Plex clone.
- VDR-Suite may become a stable middleware layer for VDR-based systems.
- API controllers should stay backend-independent.
- API shapes should be stable enough for multiple clients.
- Frontends should consume capabilities instead of checking concrete backend types.
- Backend identity must be explicit and stable.
- Multi-VDR support must remain possible.
- Permission-aware clients must remain possible.
- Snapshot-backed reads should be preferred over repeated live backend calls.
- Runtime diagnostics should evolve without leaking internal implementation details into public API contracts.

## API Direction

Future API design should distinguish between:

1. Internal service boundaries
2. Stable client-facing REST API
3. Future live update transports such as SSE or WebSocket
4. Future diagnostics or monitoring APIs
5. Future administrative APIs

Possible long-term API areas include:

```text
/api/v1/overview
/api/v1/recordings
/api/v1/timers
/api/v1/channels
/api/v1/events
/api/v1/jobs
/api/v1/backends
/api/v1/capabilities
/api/v1/streams
```

Later areas may include:

```text
/api/v1/search
/api/v1/users
/api/v1/permissions
/api/v1/diagnostics
/api/v1/live
```

These paths are directional examples, not implementation commitments for the current phase.

## Relationship to Existing ADRs

This ADR builds on:

- ADR-001 Backend Identity Strategy
- ADR-002 Backend Federation Strategy
- ADR-003 Backend Capability Strategy
- ADR-004 Backend Lifecycle Strategy
- ADR-005 Stream Provider Strategy
- ADR-006 Internal Event Dispatch Strategy

Together, these decisions define VDR-Suite as a backend-neutral, capability-aware, event-capable and future multi-backend platform while keeping VDR as the core domain.

## Consequences

Positive consequences:

- future clients can reuse the same backend API
- frontend development can remain decoupled from backend internals
- integrations such as Live, Kodi, Android apps or automation tools remain possible
- API design can be reviewed before it becomes frontend-specific
- multi-VDR and permission-aware designs remain aligned with the API direction

Trade-offs:

- API design requires more discipline
- backward compatibility becomes more important over time
- internal service models and public API models may need to remain separate
- versioning strategy will become necessary later

## Non-Goals

This ADR does not implement:

- a public plugin API
- a final REST API versioning policy
- a permission system
- user management
- SSE or WebSocket transport
- diagnostics endpoints
- multi-VDR routing
- compatibility with any specific third-party frontend

## Current Phase Boundary

For Phase 10, this ADR only documents the strategic direction.

Runtime logging and diagnostics are still being developed. No new API endpoint is introduced by this ADR.

## Next Steps

- Keep current REST API shapes backend-independent.
- Avoid frontend-specific API coupling.
- Continue runtime logging and diagnostics work.
- Revisit API versioning before exposing broader client-facing APIs.
- Consider a dedicated API architecture document once the next REST API expansion begins.
