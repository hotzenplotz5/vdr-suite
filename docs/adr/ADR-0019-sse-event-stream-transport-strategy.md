# ADR-0019: SSE Event Stream Transport Strategy

## Status

Accepted

## Context

VDR-Suite already contains snapshot-based state synchronization and a snapshot change feed architecture.

Future clients (Windows, Web, Android and iOS) require low-latency updates without repeatedly requesting complete snapshots.

Several transport mechanisms were considered:

- Periodic polling
- Long polling
- Server-Sent Events (SSE)
- WebSockets

The architecture must remain backend-neutral and must not couple clients directly to a specific VDR backend.

## Decision

VDR-Suite adopts Server-Sent Events (SSE) as the primary live-update transport.

Rules:

- Snapshots remain the authoritative state source.
- Snapshot Change Feed remains the authoritative change source.
- SSE transports change-feed entries to clients.
- Clients may reconnect at any time.
- Clients must be able to rebuild state from snapshots plus feed events.
- WebSocket support may be added later without changing the snapshot architecture.

## Consequences

Benefits:

- Simple HTTP-based transport.
- Native browser support.
- Easy reverse-proxy integration.
- Lower complexity than WebSockets.
- Compatible with snapshot synchronization.

Trade-offs:

- Unidirectional transport.
- Not suitable for bidirectional control channels.

## Related ADRs

- ADR-0016 Snapshot Change Feed Architecture
- ADR-0017 Live Transport Boundary
- ADR-0018 Incremental Snapshot Synchronization
