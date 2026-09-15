# VDR-Suite Project Overview

## Navigation

- [README](../README.md)
- [Current State](CURRENT.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Strict Roadmap](planning/roadmap.md)
- [Target Platform Architecture](architecture/target-platform-architecture.md)

## Purpose

VDR-Suite is a VDR-centred, domain-first platform for modern Web, mobile, desktop and TV clients. It complements VDR instead of forking or replacing VDR's native runtime responsibilities.

This overview contains stable capability/ownership direction. Volatile branch/PR/CI facts belong in [Current State](CURRENT.md).

## Platform ownership

```text
VDR
  remains native authority for devices, schedules, timers,
  recordings, replay, OSD and plugin execution

VDR-Suite Control Plane
  owns Suite identity, authorization, policy, orchestration,
  durable read models, reconciliation and client-facing semantics

Backend Agent
  owns bounded site-local observation, command delivery,
  provider ownership/selection, fencing and cleanup

Private adapters/providers
  RESTfulAPI, SVDRP, Streamdev, TVScraper, SuiteBridge and
  later broadcast/compatibility providers remain implementation details
```

Clients consume Suite-owned contracts. Reachable private providers never become public security or compatibility boundaries by accident.

## Durable implemented platform

The implemented platform includes:

- daemon-owned SQLite with repository-owned SQL and explicit persistence boundaries;
- backend identity/scope and server-enforced access policy;
- Channels, EPG, Recordings, metadata, people, artwork, Genres and search read models;
- Recordings 2 browsing/detail/actions plus VDR-native marks/cutting integration;
- persistent actor identity, browser-session security, backend-scoped authorization and accountability;
- Backend Agent enrollment, generation fencing, observations, durable commands/results and provider ownership;
- protected writes with idempotency/fences, expected revisions, unknown-outcome handling and authoritative readback;
- TimerIntent, TimerAssignment and NativeTimerBinding orchestration;
- an authenticated Streaming Gateway / MediaSession plane for Recording and Live TV;
- normalized first-party playback contracts and persistent client playback ownership;
- a responsive Media Home/Browse experience projecting existing Channel/EPG/Recording/Metadata/Artwork truth;
- post-Phase-66 Home performance/correctness hardening without a second Home-specific owner.

## Current product direction

Completed product flows include:

- choose a channel/programme and watch Live TV;
- browse/play/seek/resume completed Recordings on supported profiles;
- create backend-neutral Timer intent with managed native fulfillment;
- browse responsive Home Now/Next, newly recorded, Movies/Genres and Series hierarchy;
- use canonical TVScraper/native artwork and manual Series hierarchy/artwork overrides where explicitly configured;
- edit Recording marks/cuts through VDR-native authority under Suite safety boundaries.

The next strict numbered product domain is Phase 67 Broadcast Companion Services: Teletext and HbbTV. Phase 67 has not started.

## Key architectural rules

- VDR remains authoritative for VDR-native runtime state/execution.
- Suite IDs and backend-native IDs are never interchangeable.
- Authorization and capability are separate decisions.
- Provider reachability does not grant provider authority.
- Possible mutation dispatch followed by timeout does not authorize blind retry.
- Browser/TV/native clients do not construct private provider URLs as application contracts.
- Home projects existing domain truth; it does not own a second metadata, artwork or playback database.
- Teletext/HbbTV must be domain-first and must not become raw plugin/browser command tunnels.

## Execution order

Binding numbered phase order and completion gates live in the [Strict Roadmap](planning/roadmap.md). Current phase status lives in [Current State](CURRENT.md). Historical implementation proof lives in closeouts such as the [Post-Phase-66 Home Rebuild Closeout](development/post-phase66-home-rebuild-closeout.md).

## Authoritative navigation

- [Current State](CURRENT.md)
- [Current Project Status](development/current-status.md)
- [Post-Phase-66 Home Rebuild Closeout](development/post-phase66-home-rebuild-closeout.md)
- [Current Architecture State](development/current-architecture-state.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [Completed History](development/completed-phases.md)
- [Architecture Decision Records](adr/index.md)

## Back

- [Back to Documentation Index](index.md)
- [Back to README](../README.md)
