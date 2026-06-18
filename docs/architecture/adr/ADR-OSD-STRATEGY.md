# ADR: Frontend OSD Strategy

## Status
Accepted

## Context
VDR-Suite is evolving beyond a traditional VDR web frontend. Future clients are expected to include:

- Web browsers
- Android phones and tablets
- Android TV devices
- Desktop clients

A key question is whether VDR-Suite should mirror the classic VDR OSD or render its own user interface layer.

## Decision
VDR-Suite will render its own OSD and user interface layer.

The classic VDR OSD will not be mirrored.

Video playback is treated as a separate concern and may be implemented using pluggable player backends.

Examples include:

- Android Media3 / ExoPlayer
- libVLC
- mpv
- Browser-based HLS/DASH players

The user interface and OSD layer must remain independent from the underlying playback engine.

## Consequences
### Advantages

- Modern UI design independent of VDR OSD limitations.
- Consistent user experience across web, mobile and TV clients.
- Easier future support for multiple playback engines.
- Cleaner separation between playback and management functionality.

### Trade-offs

- UI functionality must be implemented by VDR-Suite.
- Existing VDR OSD workflows are not automatically inherited.

## Long-term Vision

VDR provides data, timers, recordings and streams.

VDR-Suite provides:

- EPG UI
- Timer management
- Recording management
- Live TV navigation
- Playback overlays
- Future mobile and TV experiences

The OSD becomes a VDR-Suite capability rather than a mirrored VDR capability.

## Navigation

- [Project README](../../../README.md)

## Back


