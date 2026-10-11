# VDR-Suite Project Overview

> **English-language introduction.** This is a concise reader-facing overview, not a second current-status or architecture authority. Use [canonical project overview](project-overview.md), [CURRENT](CURRENT.md) and the [current architecture](development/current-architecture-state.md) for current source-backed facts.

## Navigation

- [README](../README.md)
- [Documentation Index](index.md)
- [Project Overview](project-overview.md)

---

VDR-Suite – Modern Platform for VDR

What is VDR-Suite?

VDR-Suite is a modern open-source platform built around the Video Disk Recorder (VDR).

The goal is not to replace VDR.

Instead, VDR-Suite provides a modern backend layer that complements VDR with contemporary APIs, services and client applications.

VDR remains responsible for TV reception, recordings, timers and live television.

Why VDR-Suite?

Many existing VDR components were designed long before:

- modern web applications
- mobile clients
- multi-server environments
- real-time APIs

became common requirements.

VDR-Suite aims to bridge that gap.

Goals

- Modern backend architecture
- Clear interfaces
- Multiple client applications
- Multi-VDR support
- Real-time updates
- Extensibility
- Long-term maintainability

Multi-VDR Support

VDR-Suite is designed with multiple VDR instances in mind.

Examples include:

- primary VDR server
- secondary VDR server
- remote locations
- vacation homes
- development systems

The long-term goal is centralized management of multiple VDR systems.

Modern Clients

Backend and frontend are intentionally separated.

Potential clients include:

- Windows applications
- Web frontends
- Android apps
- iOS apps
- Smart TV applications
- future OSD integrations

All clients can access the same backend services.

Real-Time Updates

VDR-Suite supports real-time events and status updates.

This allows clients to react immediately to changes without continuously reloading complete datasets.

Efficiency

A major development goal is to avoid unnecessary full refresh operations.

Selective EPG refresh, targeted event queries and resource-conscious snapshot processing are now established platform capabilities. Numbered phases through Phase 69 are completed for their accepted bounded scope. Cross-cutting Multiuser/Device integration is tracked separately, and Phase 70 has not started. See [Current State](CURRENT.md) for the authoritative live position.

This becomes increasingly important in multi-VDR environments.

Not a Plex or Jellyfin Clone

VDR-Suite is not intended to replace Plex, Jellyfin or Kodi.

Its focus remains on the traditional strengths of VDR:

- DVB
- Live TV
- Recordings
- Timers
- EPG

The goal is to modernize access to VDR, not to replicate other media platforms.

Current Status

Implemented features already include:

- backend foundation
- REST APIs
- VDR adapter layer
- snapshot architecture
- cache architecture
- change tracking
- live updates
- backend-scoped identity and permission enforcement
- selective refresh strategies
- Streaming Gateway and MediaSessions for Live TV and Recordings
- responsive Media Home, Teletext/HbbTV and the Legacy OSD bridge
- VDR-native Recording marks/editing integration
- Public-v1 API plus separately gated Device playback endpoints
- extensive test infrastructure

Long-Term Vision

VDR-Suite aims to become a modern platform on top of VDR.

VDR remains the core system.

VDR-Suite provides the modern services, APIs and client integration around it.

---

## Back

- [Back to README](../README.md)
- [Back to Documentation Index](index.md)
- [Back to Project Overview](project-overview.md)
