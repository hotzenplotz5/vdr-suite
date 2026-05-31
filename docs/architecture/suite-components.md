# VDR-Suite – Component Architecture

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
* System status reporting

## Owns

* SQLite database
* Repositories
* Services
* Workers
* REST API
* Adapters

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

# REST API Ownership

The REST API belongs to the daemon.

Current routes:

* GET /api/dashboard
* GET /api/jobs
* GET /api/recordings
* GET /api/metadata

Future routes:

* GET /api/metadata/{recordingId}
* POST /api/jobs
* POST /api/recordings/{id}/actions
* GET /api/system/status

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

Repositories

↓

SQLite

---

# Design Decisions

## Decision 1

The daemon is the only backend.

Reason:

* one source of truth
* reusable by all clients
* easier testing
* easier maintenance

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

## Decision 4

Adapters isolate external tools.

Reason:

* cleaner architecture
* easier migration
* easier replacement of external tools

---

# Next Major Milestone

Phase 8.0

vdr-suite-daemon foundation

Initial goals:

* daemon bootstrap
* database initialization
* repository initialization
* service initialization
* REST initialization
* worker integration preparation

Expected result:

A runnable vdr-suite-daemon process that becomes the foundation for all future frontends and integrations.

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
