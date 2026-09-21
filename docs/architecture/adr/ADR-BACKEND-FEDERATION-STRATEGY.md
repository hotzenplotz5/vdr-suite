# ADR: Backend Federation Strategy

## Status
Accepted

## Context
VDR-Suite currently integrates VDR through adapter abstractions and backend identities.

Future deployments may contain multiple backend technologies.

Examples:

- VDR
- TVHeadend
- Future TV backends

The system should avoid coupling all backend functionality to VDR-specific implementations.

## Decision
VDR-Suite will support multiple backend types through backend federation.

Backend implementations should be represented through adapter abstractions and capability mapping.

TVHeadend is considered a future supported backend type.

## Backend Mapping
Examples:

| VDR-Suite Domain | TVHeadend |
|------------------|-----------|
| Channel | Channel |
| Event | EPG Event |
| Timer | DVR Entry |
| Recording | DVR Recording |
| Stream | HTTP / HTSP Stream |

## Capability Model
Backends expose capabilities rather than assuming feature parity.

Examples:

- EPG
- Timers
- Recordings
- Streaming
- Recording Actions
- Timer Actions

The frontend should adapt to backend capabilities.

## Consequences
### Advantages

- Multiple backend technologies can coexist.
- Unified frontend experience.
- Reuse of query, snapshot and UI infrastructure.
- Cleaner long-term architecture.

### Trade-offs

- Additional adapter implementations are required.
- Capability differences must be handled explicitly.

## Example Deployment

House A:
- VDR

House B:
- TVHeadend

VDR-Suite:
- Single UI
- Unified recording library
- Unified timer management
- Unified EPG experience

## Long-term Vision

VDR-Suite evolves into a backend-agnostic TV management platform while maintaining first-class support for VDR.

## Navigation

- [Project README](../../../README.md)

## Back


