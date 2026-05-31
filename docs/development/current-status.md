# VDR-Suite – Current Project Status

## Project

VDR-Suite

Goal:

Modern service-oriented backend architecture for VDR recordings, metadata management, job processing, dashboard services, JSON export, REST API, Web UI, OSD integration and future integration of VDR-Rectools.

---

## Current Branch

phase-2-actions

## Latest Tag

v1.0-phase6-cli

## Latest Commit

e31269c

Phase 6.1: add Dashboard CLI

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
SQLite
↓
Repositories
↓
Services
↓
Workflow
↓
Worker
↓
DashboardServices
↓
DashboardFacade
↓
DashboardOverview
↓
DashboardJsonSerializer
↓
DashboardCli
```

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

---

# Last Completed Step

## Phase 6.1

Dashboard CLI

Implemented:

* apps/dashboard/main.cpp

Updated:

* Makefile

Verified:

```bash
make clean
make test
make dashboard-cli
/tmp/vdr-suite-dashboard
```

Result:

All tests passed.

Dashboard JSON successfully generated from live database data.

---

# Next Planned Step

## Phase 6.2

REST Skeleton

Goals:

* Introduce API layer
* Introduce DashboardController
* Reuse DashboardFacade
* Reuse DashboardJsonSerializer
* No external HTTP framework yet
* No VDR plugin integration yet

Planned architecture:

```text
DashboardController
↓
DashboardFacade
↓
DashboardOverview
↓
DashboardJsonSerializer
↓
JSON
```

Future:

```text
GET /api/dashboard
```

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
