# VDR-Suite – Component Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)

---

## Purpose

This document defines the major runtime components of the VDR-Suite ecosystem and their responsibilities.

The architecture follows a strict separation of concerns:

* vdr-suite-daemon owns backend logic.
* vdr-plugin-suite provides the VDR OSD frontend.
* Web, Windows, Android and iOS clients communicate through the REST API.
* SQLite belongs exclusively to the daemon.
* Business logic belongs exclusively to the daemon.

---

# Component Overview

## Backend

* vdr-suite-daemon

## VDR Frontend

* vdr-plugin-suite

## External Clients

* VDR-Suite Web
* VDR-Suite Windows
* VDR-Suite Android
* VDR-Suite iOS

## Integration Components

* RectoolsAdapter
* TvScraperAdapter (future)
* ArtworkAdapter (future)
* Media Adapters (future)

---

# vdr-suite-daemon

## Role

The daemon is the central backend process of VDR-Suite.

All business logic lives here.

The daemon can operate independently of VDR.

## Responsibilities

* SQLite access
* Repository management
* Service management
* Workflow execution
* Job queue management
* Worker processing
* Metadata management
* Artwork management
* REST API
* Adapter execution
* Backend registry ownership
* Snapshot generation and cache ownership
* Backend-aware read routing
* Runtime diagnostics
* System status reporting

## Owns

* SQLite database
* Repositories
* Services
* Workers
* REST API
* Adapters
* Backend registry
* Snapshot cache
* Change feed

## Must Not Own

* OSD rendering
* Remote control handling
* Browser UI rendering
* Mobile UI rendering

## Internal Structure

SQLite

↓

Repositories

↓

Services

↓

Workflow

↓

Workers

↓

REST API

---

# vdr-plugin-suite

## Role

The VDR plugin is the native OSD frontend for VDR users.

It is intentionally lightweight.

## Responsibilities

* OSD menus
* Remote control navigation
* Dashboard display
* Recording display
* Metadata display
* Job display
* Action triggering

## Communication

The plugin communicates only through the REST API.

VDR

↓

vdr-plugin-suite

↓

REST API

↓

vdr-suite-daemon

## Must Not Own

* SQLite
* Workers
* Metadata import
* Job execution
* Heavy processing logic

## Design Goal

The plugin remains a frontend only.

All backend functionality stays inside the daemon.

---

# VDR-Suite Web

## Role

Browser-based frontend.

## Responsibilities

* Dashboard
* Recording browser
* Metadata editor
* Artwork browser
* Job queue display
* System monitoring

## Communication

Browser

↓

Web Frontend

↓

REST API

↓

vdr-suite-daemon

## Must Not Own

* SQLite
* Workers
* Repository logic
* Service logic

---

# VDR-Suite Windows

## Role

Desktop management client.

## Responsibilities

* Recording management
* Metadata maintenance
* Job monitoring
* Bulk actions
* Administrative tasks

## Communication

Windows Client

↓

REST API

↓

vdr-suite-daemon

---

# VDR-Suite Android

## Role

Mobile monitoring and control client.

## Responsibilities

* Dashboard access
* Recording overview
* Job monitoring
* Notifications
* Quick actions

## Communication

Android Client

↓

REST API

↓

vdr-suite-daemon

---

# VDR-Suite iOS

## Role

Mobile monitoring and control client.

## Responsibilities

* Dashboard access
* Recording overview
* Job monitoring
* Notifications
* Quick actions

## Communication

iOS Client

↓

REST API

↓

vdr-suite-daemon

---

# Adapter Layer

## Purpose

Adapters isolate external tools from the core architecture.

The rest of the system should not care whether functionality comes from:

* internal C++ code
* shell scripts
* existing plugins
* third-party tools

## Current Adapter

### RectoolsAdapter

Status:

Placeholder implementation

Purpose:

Integration point for VDR-Rectools functionality.

## Future Adapters

* TvScraperAdapter
* ArtworkAdapter
* Pes2TsAdapter
* CutAdapter
* RepairAdapter
* ShrinkAdapter

---

# VDR Backend Layer

## Purpose

The VDR backend layer isolates VDR communication and backend identity from API controllers and frontend clients.

Implemented backend-facing components include:

* VdrConfig
* IVdrAdapter
* RestfulApiVdrAdapter
* MockVdrAdapter
* VdrService
* BackendNode
* BackendRegistry
* BackendRegistryService

## Ownership

The daemon owns backend configuration, backend registry state and backend-aware runtime wiring.

Frontend clients consume backend information through REST API boundaries only.

---

# Snapshot Runtime Layer

## Purpose

The snapshot runtime avoids repeated direct backend calls per API request.

Implemented snapshot-facing components include:

* VdrSnapshot
* VdrSnapshotBuilder
* PollingService
* SnapshotCache
* SnapshotCacheService
* SnapshotAccessService
* VdrSnapshotReadService
* VdrSnapshotReadJsonSerializer
* SnapshotChangeFeed
* SnapshotChangeFeedService

## Current State

The snapshot layer supports default-backend compatibility and multi-backend snapshot storage and lookup.

Backend-aware snapshot generation is implemented through `VdrSnapshotBuilder` assigning a stable backend ID.

The next runtime step is to connect backend registry iteration to daemon-owned polling.

---

# REST API Ownership

The REST API belongs to the daemon.

Current routes:

* GET /api/dashboard
* GET /api/jobs
* GET /api/recordings
* GET /api/metadata
* GET /api/runtime/diagnostics
* GET /api/runtime/summary
* GET /api/vdr/status
* GET /api/vdr/health
* GET /api/vdr/snapshot
* GET /api/vdr/capabilities
* GET /api/vdr/channels
* GET /api/vdr/timers
* GET /api/vdr/events
* GET /api/vdr/recordings
* GET /api/vdr/changes
* GET /api/backends
* GET /api/backends/default
* GET /api/backends/{backendId}/status
* GET /api/backends/{backendId}/health
* GET /api/backends/{backendId}/snapshot

Future routes:

* GET /api/metadata/{recordingId}
* POST /api/jobs
* POST /api/recordings/{id}/actions
* permission-aware backend routes
* live transport routes above the snapshot change feed

---

# Ownership Rules

## SQLite

Owned by:

* vdr-suite-daemon

Not owned by:

* vdr-plugin-suite
* Web Frontend
* Windows Client
* Android Client
* iOS Client

## Workers

Owned by:

* vdr-suite-daemon

## Business Logic

Owned by:

* vdr-suite-daemon

## OSD Rendering

Owned by:

* vdr-plugin-suite

## Backend Registry

Owned by:

* vdr-suite-daemon

Consumed by:

* REST API controllers
* future clients through REST responses

Not owned by:

* frontend clients
* VDR plugin frontend

## Snapshot Cache

Owned by:

* vdr-suite-daemon

Not owned by:

* REST controllers
* frontend clients
* live transport layer

## REST API Contract

Owned by:

* vdr-suite-daemon

Consumed by:

* vdr-plugin-suite
* Web Frontend
* Windows Client
* Android Client
* iOS Client

---

# Target Runtime Architecture

Web Frontend
Windows Client
Android Client
iOS Client
vdr-plugin-suite

↓

REST API

↓

ApiRouter

↓

Controllers

↓

Services

↓

Repositories / Backend Registry / Snapshot Read Services

↓

SQLite / Snapshot Cache / VDR Adapter Boundary

---

# Design Decisions

## Decision 1

The daemon is the only backend owner.

Reason:

* one source of truth
* reusable by all clients
* easier testing
* easier maintenance
* clear backend registry ownership
* clear snapshot ownership

## Decision 2

The VDR plugin is only a frontend.

Reason:

* smaller plugin
* fewer crashes
* no SQLite locking inside VDR
* easier future development

## Decision 3

The REST API is the integration boundary.

Reason:

* common interface
* multiple clients
* platform independence
* backend-aware routing without frontend-owned backend logic

## Decision 4

Adapters isolate external tools.

Reason:

* cleaner architecture
* easier migration
* easier replacement of external tools

## Decision 5

Backend registry and snapshot cache ownership remain inside the daemon.

Reason:

* prevents frontend-controlled polling
* preserves backend neutrality
* prepares permission-aware multi-VDR routing
* keeps runtime state changes centralized

---

# Current Major Milestone

Phase 15.9 is complete.

Implemented result:

* backend registry foundation
* backend registry service layer
* backend registry JSON serialization
* backend registry REST controller
* backend registry API routing
* backend-aware snapshot access
* backend-aware VDR controller methods
* backend snapshot API routes
* dynamic backend snapshot route parsing
* multi-snapshot cache foundation
* multi-backend snapshot cache access
* backend-aware snapshot builder

---

# Next Major Milestone

Phase 16.0 – Multi-Backend Polling Foundation

Initial goals:

* connect BackendRegistry to daemon-owned polling
* preserve default backend behavior
* build backend-tagged snapshots per backend
* update SnapshotCache per backend ID
* keep REST controllers independent from polling ownership
* avoid parallel polling until the single-threaded multi-backend model is clear

Expected result:

The daemon can prepare runtime snapshot refresh per registered backend while preserving existing `/api/vdr/...` default routes and enabling `/api/backends/{backendId}/...` routes to use runtime data.

---

# External VDR Integration Layer

VDR-Suite is designed as an orchestration layer above the existing
VDR ecosystem.

Preferred integration targets:

- vdr-plugin-restfulapi
- epgsearch
- scraper2vdr
- tvscraper
- vdr-rectools

The goal is to reuse mature implementations and focus development on:

- workflow management
- metadata aggregation
- dashboard services
- job processing
- user interfaces

rather than reimplementing existing VDR functionality.

---

## Back

- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
