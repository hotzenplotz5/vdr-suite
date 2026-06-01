# ADR-0006: VDR Backend Architecture

## Status

Accepted

## Context

VDR-Suite currently supports multiple VDR integration modes.

Implemented:

- MockVdrAdapter
- ExternalVdrAdapter

Future integrations are expected:

- RESTfulAPI backend
- SVDRP backend
- Additional VDR communication layers

The daemon, dashboard and service layers must remain independent of the transport mechanism.

## Decision

All VDR integrations must implement IVdrAdapter.

The application accesses VDR exclusively through:

RuntimeConfig
→ VdrConfig
→ VdrAdapterFactory
→ IVdrAdapter

Backend implementations are hidden behind the adapter interface.

Current implementations:

- MockVdrAdapter
- ExternalVdrAdapter

Planned implementations:

- RestfulApiVdrAdapter
- SvdrpVdrAdapter

## Consequences

Benefits:

- Backend-independent architecture
- Easy testing using MockVdrAdapter
- Multiple VDR communication methods
- Future frontend independence

Tradeoffs:

- Additional abstraction layer
- More adapter implementations to maintain

## Notes

RESTfulAPI is treated as a backend implementation and not as a core dependency of VDR-Suite.

HTTP communication will be introduced in a later phase.
