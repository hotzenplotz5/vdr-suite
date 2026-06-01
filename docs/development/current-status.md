# VDR-Suite – Current Project Status

## Project

VDR-Suite

Goal:

Modern service-oriented backend architecture for VDR recordings, metadata management, job processing, dashboard services, JSON export, REST API, Web UI, OSD integration and future integration of VDR-Rectools.

---

## Current Branch

phase-2-actions

## Latest Tag

v1.8-phase8-vdr-config

## Latest Commit

9056486

Phase 8.1: add external VDR adapter foundation

---

# Implemented Components

## Database Layer

Implemented:

* SQLite schema
* Sample test data
* Database wrapper
* Automated database tests

Files:

* database/schema/vdr-suite.sql
* database/testdata/sample-data.sql
* core/sqlite/

---

## Domain Layer

Implemented:

### Recording

* Recording
* RecordingDetails

### Metadata

* Metadata
* Artwork

### Jobs

* Job
* RecordingAction

### Dashboard

* DashboardSummary
* RecordingDashboardSummary
* DashboardOverview

---

## Repository Layer

Implemented:

### RecordingRepository

Features:

* Load recordings
* Search recordings
* Load recording by id

### MetadataRepository

Features:

* Load metadata
* Load metadata for recording

### JobRepository

Features:

* Insert jobs
* Load jobs
* Update job status
* Get next pending job

---

## Service Layer

Implemented:

### RecordingService

* Recording details
* Recording search

### MetadataService

* Metadata access

### ActionService

* Create RecordingAction objects

### JobService

* Create Job objects

### JobDashboardService

* Job statistics
* Queue statistics
* Latest job information

### RecordingDashboardService

* Recording statistics
* Metadata coverage
* Latest recording information

### DashboardFacade

* Unified dashboard access
* Single entry point for consumers

### DashboardJsonSerializer

* DashboardOverview → JSON conversion

---

## Workflow Layer

Implemented:

### RecordingWorkflowService

* createActionJob()
* createCheckJob()
* createShrinkJob()
* createRepairJob()
* createCutJob()
* createPes2TsJob()
* createRenameJob()

---

## Worker Layer

Implemented:

### WorkerSimulator

Lifecycle:

```text
PENDING
→ RUNNING
→ DONE
```

Functions:

* executeJob()
* processNextJob()

---

## Adapter Layer

Implemented:

### RectoolsAdapter

Status:

Placeholder implementation

Purpose:

Future integration point for VDR-Rectools.

---

## Dashboard Layer

Implemented:

### JobDashboardService

Output:

* totalJobs
* pendingJobs
* runningJobs
* doneJobs
* failedJobs
* latestJobId
* latestJobType
* latestJobStatus

### RecordingDashboardService

Output:

* recordingsTotal
* recordingsWithMetadata
* recordingsWithoutMetadata
* latestRecordingId
* latestRecordingTitle

### DashboardFacade

Output:

* DashboardOverview

### DashboardJsonSerializer

Output:

* JSON string

---

## CLI Layer

Implemented:

### Dashboard CLI

File:

* apps/dashboard/main.cpp

Build:

```bash
make dashboard-cli
```

Run:

```bash
/tmp/vdr-suite-dashboard
```

Output:

```json
{
  "jobs": {
    "totalJobs": 2,
    "pendingJobs": 1
  },
  "recordings": {
    "recordingsTotal": 2
  }
}

---

## Current Architecture


vdr-suite-daemon
├── RuntimeConfig
├── Database
├── Repository Layer
├── Service Layer
├── Workflow Layer
├── Worker Layer
├── Dashboard Layer
└── REST Layer

Clients

Dashboard CLI
Web UI (planned)
OSD Frontend (planned)
Mobile Apps (planned)

---

## REST API

Implemented:

* GET /api/dashboard
* GET /api/jobs
* GET /api/recordings
* GET /api/metadata

Controllers:

* DashboardController
* JobsController
* RecordingsController
* MetadataController

Router:

* ApiRouter

---

## Daemon Layer

Implemented:

### RuntimeConfig

Features:

* Central runtime configuration
* Database path configuration

### DaemonApp

Features:

* Application entry point
* Runtime startup
* Runtime shutdown

### DaemonRuntime

Features:

* Runtime lifecycle management
* Signal handling (SIGINT/SIGTERM)
* Database lifecycle
* Dashboard runtime wiring

Current Runtime:

Database
↓
Repositories
↓
Dashboard Services
↓
DashboardFacade

RuntimeConfig
↓
VdrConfig
↓
IVdrAdapter
↓
ExternalVdrAdapter
↓
VdrStatus

Build:

make daemon

Run:

/tmp/vdr-suite-daemon

## VDR Integration Layer

Implemented:

### VdrStatus

Features:

* VDR status model
* Host information
* Port information
* Runtime state information

### ExternalVdrAdapter

Features:

* Initial external VDR abstraction
* Adapter foundation for future integrations
* Status retrieval API
* Unit test coverage
* Configurable adapter initialization
* VdrConfig integration

### VdrConfig

Features:

* Dedicated VDR configuration object
* Default VDR runtime configuration
* Adapter configuration abstraction
* Unit test coverage

Current Status:

Static configuration placeholder

Planned:

* RuntimeConfig integration
* SVDRP client
* RESTfulAPI client
* Channel management
* Timer management
* EPG integration

### IVdrAdapter

Features:

* Backend abstraction layer
* Common VDR adapter contract
* Backend-independent integration point
* Foundation for multiple VDR backends

Supported future backends:

* RESTfulAPI
* SVDRP
* Plugin Bridge
* Mock/Test Adapters

---

## Existing Tags

### v0.1-phase0

Project foundation and documentation.

### v0.2-phase1-core

Core architecture and persistence.

### v0.3-phase1-workflow

Workflow layer.

### v0.4-phase2-worker

Worker lifecycle.

### v0.5-phase2-queue

Queue-style processing.

### v0.6-phase3-dashboard

Job dashboard service.

### v0.7-phase4-recording-dashboard

Recording dashboard service.

### v0.8-phase5-dashboard-facade

Dashboard facade.

### v0.9-phase6-json

Dashboard JSON serialization.

### v1.0-phase6-cli

Dashboard CLI application.

### v1.2-phase7-router

API router.

### v1.3-phase7-jobs-api

Jobs API route.

### v1.4-phase7-recordings-api

Recordings API route.

### v1.5-phase7-metadata-api

Metadata API route.

### v1.6-phase8-daemon-foundation

### v1.7-phase8-vdr-adapter-foundation

External VDR integration foundation:

* VdrStatus
* ExternalVdrAdapter
* Initial VDR domain
* Adapter unit test

Daemon foundation:

* DaemonApp
* RuntimeConfig
* Signal handling
* Database lifecycle
* Dashboard runtime wiring

---

# Last Completed Step

Phase 8.0

Daemon Foundation

Implemented:

* DaemonApp
* RuntimeConfig
* DaemonRuntime
* Signal handling
* Database lifecycle
* Dashboard runtime wiring

Result:

vdr-suite-daemon now provides a central runtime layer
responsible for startup, shutdown and lifecycle management.

---

Phase 8.1

REST Server Lifecycle

Goals:

* Runtime-owned ApiRouter
* Runtime-owned Controllers
* Long-running REST service
* Foundation for Web UI and OSD frontend integration

Phase 8.2

VDR Configuration Layer

Implemented:

* VdrConfig
* Configurable ExternalVdrAdapter
* VDR test integration in Makefile
* Dedicated VDR unit tests

Result:

VDR configuration is now separated from the adapter implementation
and prepared for future SVDRP and RESTfulAPI integrations.

Phase 8.3

VDR Adapter Abstraction Layer

Implemented:

* IVdrAdapter
* ExternalVdrAdapter implements IVdrAdapter
* Interface-based unit tests
* Backend-independent adapter architecture

Result:

VDR-Suite can now support multiple VDR backends
through a common abstraction layer.

No network communication has been implemented yet.

Future adapters may use:

* RESTfulAPI
* SVDRP
* Plugin Bridge

---

# Long-Term Goals

* REST API
* Web Frontend
* VDR OSD integration
* Real RectoolsAdapter integration
* Unified recording management
* Metadata services
* Background worker processing
* Modern VDR Suite architecture
