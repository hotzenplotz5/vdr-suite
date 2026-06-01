# VDR-Suite – Current Project Status

## Project

VDR-Suite

Goal:

Modern service-oriented backend architecture for VDR recordings, metadata management, job processing, dashboard services, JSON export, REST API, Web UI, OSD integration and future integration of VDR-Rectools.

---

## Current Branch

phase-2-actions

## Latest Tag

v1.15-phase8-restfulapi-adapter

## Latest Commit

19761f0

Phase 8.9: add RESTfulAPI VDR adapter foundation

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
```

---

## Current Architecture

```text
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
```

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

```text
Database
↓
Repositories
↓
Dashboard Services
↓
DashboardFacade
```

### VDR Integration Architecture

```text
RuntimeConfig
↓
VdrConfig
↓
VdrAdapterFactory
↓
IVdrAdapter
├── ExternalVdrAdapter
├── MockVdrAdapter
└── future adapters
↓
VdrStatus
```

RESTfulAPI Architecture Decision

RESTfulAPI will be integrated as a future IVdrAdapter implementation.

IVdrAdapter
├── ExternalVdrAdapter
├── MockVdrAdapter
├── RestfulApiVdrAdapter
├── SvdrpVdrAdapter
└── PluginBridgeVdrAdapter

RESTfulAPI must remain behind the adapter boundary.

The daemon, REST API controllers, dashboard services and recording services must not call RESTfulAPI directly.

RESTfulAPI Boundary

Future target architecture:

RestfulApiVdrAdapter
↓
IHttpClient
↓
vdr-plugin-restfulapi

HTTP transport must be separated from VDR adapter logic.

The first RESTfulAPI endpoint to map should be:

/info.json

Target mapping:

/info.json
↓
RestfulApiVdrAdapter
↓
VdrStatus
Important Phase 8.7 Decision

No HTTP communication was implemented in Phase 8.7.

Phase 8.7 is architecture validation only.

Next Recommended Phase
Phase 8.8: introduce HTTP client abstraction

Expected scope:

IHttpClient
HttpRequest
HttpResponse
MockHttpClient
tests

No real network communication yet.

Project Rules
Prefer complete files.
Use nano only.
Never use cat <<EOF.
Run make test before code commits.
Update current-status.md after every phase.
Create milestone tags.
Keep daemon and service layers backend-independent.
Do not reinvent existing VDR plugin functionality.

---

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

### VdrAdapterFactory

Features:

* Central adapter creation
* Backend selection layer
* Returns IVdrAdapter instances
* Foundation for future backend expansion

Supported adapter modes:

* external
* mock
* restfulapi

Future adapter modes:

* svdrp
* plugin

### MockVdrAdapter

Features:

* Mock backend implementation
* Deterministic test backend
* No VDR dependency
* Multi-backend architecture validation
* Unit test coverage

Purpose:

Allows testing of higher layers without
requiring a running VDR instance.

### RestfulApiVdrAdapter

Features:

* RESTfulAPI adapter foundation
* Implements IVdrAdapter
* Uses IHttpClient boundary
* Uses MockHttpClient in tests
* Maps mocked /info.json response into VdrStatus
* Keeps RESTfulAPI details behind adapter boundary

Status:

Foundation implemented.

Out of scope:

* Real HTTP communication
* JSON parser
* Channel mapping
* Timer mapping
* Recording mapping
* EPGSearch mapping
* SearchTimer mapping

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

### v1.1-phase6-rest-skeleton

Initial REST API foundation and project structure.

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

### v1.8-phase8-vdr-config

VdrConfig architecture.

### v1.9-phase8-vdr-adapter-interface

IVdrAdapter abstraction layer.

### v1.10-phase8-vdr-adapter-factory

VdrAdapterFactory implementation.

### v1.11-phase8-vdr-mock-backend

Mock backend implementation.

Added:

* MockVdrAdapter
* Factory support for mock mode
* Multi-backend validation
* Dedicated mock adapter test

### v1.12-phase8-vdr-backend-architecture

Backend-independent VDR backend architecture.

Added:

* ADR-0006
* VDR backend architecture documentation
* RESTfulAPI backend strategy
* Future SVDRP backend strategy

### v1.13-phase8-restfulapi-architecture

RESTfulAPI integration architecture.

Added:

* ADR-0007
* RESTfulAPI adapter boundary documentation
* RESTfulAPI endpoint classification
* VdrStatus boundary definition
* Future domain object mapping strategy

### v1.14-phase8-http-abstraction

HTTP abstraction layer.

Added:

* IHttpClient
* HttpRequest
* HttpResponse
* MockHttpClient
* HTTP abstraction tests

### v1.15-phase8-restfulapi-adapter

RESTfulAPI VDR adapter foundation.

Added:

* RestfulApiVdrAdapter
* IHttpClient injection
* MockHttpClient based adapter test
* Mocked /info.json to VdrStatus mapping
* Factory support for restfulapi mode

---

# Last Completed Step

Phase 8.9: RESTfulAPI VDR adapter foundation

Phase 8 Status

Completed:

Phase 8.0: daemon foundation
Phase 8.1: external VDR adapter foundation
Phase 8.2: VdrConfig architecture
Phase 8.3: IVdrAdapter abstraction layer
Phase 8.4: VdrAdapterFactory
Phase 8.5: MockVdrAdapter multi-backend foundation
Phase 8.6: VDR backend architecture documentation
Phase 8.7: RESTfulAPI integration architecture analysis
Phase 8.8: HTTP abstraction layer
Phase 8.9: RESTfulAPI VDR adapter foundation

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

## Next Planned Step

Phase 8.10

Goal:

VDR domain model documentation.

Scope:

* Define backend-neutral VDR domain concepts
* Document VdrStatus boundary
* Document future VdrChannel
* Document future VdrEvent
* Document future VdrTimer
* Document future VdrSearchTimer
* Document future VdrRecording
* Keep RESTfulAPI JSON behind adapter boundary
* Prepare Phase 8.11 domain object implementation

Out of scope:

* New C++ domain headers
* IVdrAdapter method expansion
* JSON parser
* Real HTTP communication
* RESTfulAPI channel/timer/recording mapping
