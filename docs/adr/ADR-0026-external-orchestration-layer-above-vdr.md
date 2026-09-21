# ADR-0026: External Orchestration Layer Above VDR

## Status

Proposed

## Context

A recurring architectural question is why VDR-Suite exists as a standalone
application when VDR, RESTfulAPI and other VDR plugins already provide
substantial functionality.

A plugin-only architecture would place all additional functionality directly
inside the VDR process.

However, VDR-Suite is intended to support use cases beyond a single VDR
instance:

- Multiple VDR backends.
- Backend-specific permissions.
- Read-only and read-write backend separation.
- Unified frontend access.
- Future Windows, web and mobile clients.
- Metadata provider federation.
- Recording workflow orchestration.
- External processing tools such as VDR-Rectools.
- Background jobs and long-running tasks.
- Backend capability abstraction.
- Additional safety layers around destructive operations.

The project therefore needs a clear statement whether VDR-Suite should be
implemented primarily as a VDR plugin or as an external orchestration layer.

## Decision

VDR-Suite shall remain an external orchestration layer above VDR.

VDR itself remains the source of truth for:

- Channels.
- EPG data.
- Timers.
- Recordings.
- Live recording state.

RESTfulAPI and other VDR-side plugins are considered transport and integration
layers that expose VDR functionality.

VDR-Suite does not replace VDR.

VDR-Suite consumes VDR functionality and provides:

- Multi-backend federation.
- Unified APIs.
- Backend capability abstraction.
- Additional validation and safety checks.
- Metadata aggregation.
- Cross-system orchestration.
- External client support.
- Workflow and automation services.

Whenever functionality already exists inside VDR, RESTfulAPI or EPGSearch,
VDR-Suite should prefer integration over reimplementation.

## Consequences

### Positive

- Clear separation between VDR runtime and orchestration logic.
- Support for multiple VDR installations.
- Easier integration of external applications.
- Safer execution of destructive actions.
- Better support for future frontend technologies.
- Reduced coupling to VDR internals.

### Negative

- Additional architectural layer.
- Additional maintenance effort.
- Some functionality may initially appear duplicated because VDR-Suite
  consumes data already available through RESTfulAPI.

### Long-Term Direction

Preferred architecture:

Client
-> VDR-Suite
-> RESTfulAPI / other adapters
-> VDR

VDR remains authoritative.

VDR-Suite coordinates and extends VDR functionality but does not become a
replacement for the VDR core.

## Alternatives Considered

### Plugin-Centric Architecture

Implement most new functionality directly as VDR plugins.

Rejected because:

- Tight coupling to VDR internals.
- More difficult multi-backend support.
- More difficult external client support.
- Increased risk of impacting the VDR runtime process.

### VDR Replacement Architecture

Move core scheduling, timer and recording responsibility into VDR-Suite.

Rejected because:

- Conflicts with the established VDR ecosystem.
- Duplicates mature VDR functionality.
- Increases long-term maintenance burden.

## Related ADRs

- ADR-0020 Multi-Source Federation Architecture
- ADR-0021 Selective Backend Query Strategy
- ADR-0025 Configurable Metadata Provider Architecture


## Navigation

- [ADR Index](index.md)
- [Development Index](../development/index.md)

## Back

- [Documentation Index](../index.md)
