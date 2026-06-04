# ADR-005 Stream Provider Strategy

## Status

Accepted

## Context

VDR-Suite is evolving towards a backend-neutral media architecture.

Future environments may include:

- VDR
- IPTV
- HLS streams
- M3U playlists
- RTSP streams
- TVHeadend
- Jellyfin
- Internet radio
- future media providers

The architecture must avoid coupling live playback and recording functionality to a single backend implementation.

---

## Decision

VDR-Suite shall introduce the concept of a Stream Provider.

Media streams are treated as backend-neutral resources.

The architecture shall not assume DVB as the only source of media streams.

---

## Stream Source

Future stream sources may include:

- DVB channels
- IPTV channels
- M3U playlists
- HLS streams
- RTSP streams
- Jellyfin streams
- TVHeadend streams

Example concept:

StreamSource

- id
- name
- provider
- url
- type

---

## Stream Provider

Future implementations may expose:

- stream discovery
- stream metadata
- stream playback URLs

Possible providers:

- VDR
- IPTV
- TVHeadend
- Jellyfin

---

## Recording Integration

Future recording jobs may operate on stream sources.

Examples:

- DVB recording
- IPTV recording
- HLS recording
- RTSP recording

Recording implementation details remain backend-specific.

Recording scheduling remains backend-neutral.

---

## Consequences

Benefits:

- backend-neutral stream architecture
- future IPTV support
- future TVHeadend support
- future Jellyfin support
- reusable recording architecture

Tradeoffs:

- additional abstraction layer
- future provider implementations required

---

## Related ADRs

- ADR-001 Backend Identity Strategy
- ADR-002 Backend Federation Strategy
- ADR-003 Backend Capability Strategy
- ADR-004 Backend Lifecycle Strategy
