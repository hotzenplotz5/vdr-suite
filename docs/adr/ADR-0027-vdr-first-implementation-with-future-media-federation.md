# ADR-0027: VDR-First Implementation With Future Media Federation

## Status

Proposed

## Date

2026-06-18

## Context

VDR-Suite is currently being implemented as an orchestration layer above VDR.

The immediate technical work focuses on VDR domains such as channels, EPG,
timers, recordings and recording actions.

At the same time, future use cases may include non-VDR media sources such as
IPTV, M3U playlists, XMLTV EPG feeds, Xtream APIs, Stalker/MAG APIs, VOD
libraries, movie libraries and series libraries.

These future sources overlap with VDR concepts in some areas:

- Channels.
- EPG data.
- Media entries.
- Categories.
- Metadata.
- Stream URLs.

However, they do not necessarily expose VDR concepts such as cTimer,
cRecording, VDR recording directories, VDR timer flags or VDR-specific
recording actions.

The project needs to decide whether to generalize the architecture now for
future media-source federation or whether to continue implementing the VDR
model first.

## Decision

VDR-Suite shall remain VDR-first for the current implementation phases.

The project shall continue to implement VDR channels, VDR EPG, VDR timers, VDR
recordings and VDR recording actions using VDR semantics.

Future IPTV and media-source support is recognized as an architectural extension
target, but shall not be implemented prematurely in the VDR timer and EPG
foundation phases.

The architecture must avoid hard VDR-only dead ends at stable boundaries:

- Backend identity.
- Backend type.
- Backend capabilities.
- Backend permissions.
- Read-only and read-write separation.
- Adapter boundaries.
- Normalized read APIs.
- Metadata provider boundaries.

VDR-specific domain objects may remain VDR-specific where VDR semantics are
actually required.

Generic media-source abstractions shall only be introduced when a stable
cross-backend boundary is proven by real integration requirements.

## Consequences

### Positive

- Keeps the current implementation focused and testable.
- Avoids premature abstraction before the VDR domain is complete.
- Preserves the ability to support IPTV and other media sources later.
- Reduces the risk of weakening VDR-specific correctness for speculative future
  sources.
- Keeps Timer, EPG and Recording work grounded in VDR, RESTfulAPI, LIVE and
  VDR-Core behavior.

### Negative

- Future IPTV/media-source support will require additional domain work.
- Some VDR-first objects may later need adapters or normalization layers.
- The project must continue checking whether new VDR-specific decisions create
  unnecessary dead ends for future backends.

## Implementation Guidance

Current phases should continue VDR-first:

- VDR Timer read contract.
- VDR Timer operations.
- VDR EPG foundation.
- EPGSearch integration.
- Recording action safety and execution.
- Metadata provider integration.

Future media federation should be considered only at stable seams:

- Backend registry.
- Capability model.
- Adapter interfaces.
- Permission model.
- Metadata provider model.
- Client-facing API boundaries.

IPTV support, if implemented later, should likely use dedicated adapters such as:

- M3U/XMLTV adapter.
- Xtream adapter.
- Stalker adapter.
- Local media-library adapter.

IPTVnator is considered a useful client/reference for integration expectations,
but not the canonical backend model for VDR-Suite.

## Alternatives Considered

### Generalize Everything Now

Introduce generic media-source objects immediately and make VDR only one
implementation of a broader media model.

Rejected for now because:

- VDR semantics are not fully implemented yet.
- Timer and recording behavior is highly VDR-specific.
- Premature generalization would likely create weak abstractions.

### VDR-Only Forever

Declare VDR-Suite a strictly VDR-only system.

Rejected because:

- ADR-0026 defines VDR-Suite as an external orchestration layer.
- Future backend federation is part of the long-term architectural direction.
- IPTV, media libraries and external metadata sources may become useful later.

## Related ADRs

- ADR-0020 Multi-Source Federation Architecture
- ADR-0021 Selective Backend Query Strategy
- ADR-0025 Configurable Metadata Provider Architecture
- ADR-0026 External Orchestration Layer Above VDR

## Navigation

- [ADR Index](index.md)
- [Development Index](../development/index.md)

## Back

- [Documentation Index](../index.md)
