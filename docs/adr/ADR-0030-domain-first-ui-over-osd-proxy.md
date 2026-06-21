# ADR-0030: Domain-First UI Over OSD Proxy

## Status

Accepted

## Context

VDR, LIVE and vdr-plugin-restfulapi expose OSD and remote-control functionality.

LIVE uses VDR's internal data structures and presents many VDR workflows through a web interface. It is an important functional reference for VDR-Suite.

vdr-plugin-restfulapi also exposes OSD-related endpoints such as `/osd` and remote-control endpoints such as `/remote`. These endpoints can expose or drive the current VDR OSD state.

VDR-Suite has a different architectural goal.

VDR-Suite is not intended to proxy, mirror or re-skin the traditional VDR OSD. It is intended to model the underlying domain functions as backend-neutral services and present them through a modern, multi-backend-capable API and frontend.

## Decision

VDR-Suite will not treat the VDR OSD as the primary user interface model.

VDR-Suite will not copy VDR OSD menu hierarchies, remote-control workflows or screen layouts as its main product structure.

Instead, VDR-Suite will model the underlying functional domains directly:

- EPG
- schedules
- multi-schedules
- what's-on views
- channels and channel groups
- timers
- SearchTimers
- recordings
- recording actions
- backend capabilities
- backend permissions
- diagnostics
- administrative actions

RESTfulAPI OSD and remote-control endpoints may be used for diagnostics, compatibility, fallback behavior or temporary validation, but they must not define the primary VDR-Suite domain model or frontend architecture.

LIVE remains a functional reference, not a UI template.

## Consequences

### Positive

- VDR-Suite can build a modern web and mobile UI instead of reproducing VDR OSD screens.
- VDR-Suite can expose backend-neutral APIs that work across multiple VDR backends.
- VDR-Suite can support future non-VDR or federated media backends without inheriting VDR-specific OSD concepts.
- Functional parity with LIVE can be pursued domain by domain.
- SSE, snapshot and change-feed mechanisms can be designed around domain changes instead of OSD redraws.
- Permissions and read-only backends can be enforced at the domain-action level.

### Negative

- More domain modeling is required than with a simple OSD proxy.
- Some workflows exposed only through OSD/remote-control behavior must be rediscovered and modeled explicitly.
- RESTfulAPI OSD support cannot be used as a shortcut for the final user experience.

## Implementation Guidance

When a feature is discovered in VDR, LIVE or RESTfulAPI, the preferred path is:

1. identify the underlying domain function;
2. define or extend the backend-neutral VDR-Suite domain model;
3. add adapter support for the current backend;
4. expose the function through VDR-Suite REST APIs;
5. add snapshot, change-feed and SSE support when the data can change externally;
6. build the frontend from the VDR-Suite domain API.

The following mapping applies:

- OSD schedule screens become VDR-Suite EPG schedule APIs and views.
- OSD timer screens become VDR-Suite timer APIs and views.
- OSD recording screens become VDR-Suite recording and recording-action APIs and views.
- OSD SearchTimer screens become VDR-Suite SearchTimer APIs and views.
- OSD setup screens become VDR-Suite backend, capability and configuration APIs.
- Remote-control key sequences become explicit backend actions or workflow commands only where they remain necessary.

## Back

- [Back to ADR Index](index.md)
- [Back to Architecture Documentation](../architecture/index.md)
- [Back to Documentation Index](../index.md)
