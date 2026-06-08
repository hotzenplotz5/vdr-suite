# Multi Client Vision

## Navigation

- [README](../../../README.md)
- [Documentation Index](../../index.md)
- [Project Overview](../../project-overview.md)

---

## Goal

VDR-Suite should not depend on a single frontend.

The backend must support multiple independent clients.

## Supported Client Types

### Windows Desktop

Example:

- Windows OSD Reference Client

Purpose:

- primary development frontend
- reference implementation

### Web Client

Example:

- successor to VDR Live

Purpose:

- browser access
- remote access
- administration

### Android

- recordings
- timers
- notifications
- remote control

### iOS

- recordings
- timers
- notifications
- remote control

### Smart TV

- living room frontend
- lightweight playback client

### Kodi Integration

- consume VDR-Suite APIs
- reuse Kodi playback capabilities

### Home Assistant

- automation
- monitoring
- notifications

## Architectural Rule

Clients communicate only with VDR-Suite APIs.

## Long-Term Goal

Create an ecosystem around a stable backend platform.
---

## Back

- [Back to Documentation Index](../../index.md)
- [Back to Project Overview](../../project-overview.md)
- [Back to README](../../../README.md)
