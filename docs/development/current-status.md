# VDR-Suite – Current Project Status

## Project

VDR-Suite

Goal:

Modern service-oriented backend architecture for VDR recordings, metadata management, job processing, dashboard services, JSON export, REST API, Web UI, OSD integration and future integration of VDR-Rectools.

---

## Current Branch

phase-2-actions

## Latest Tag

v0.8-phase5-dashboard-facade

## Latest Commit

80045a7

Phase 5: add DashboardFacade

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

Features:

* Recording details
* Recording search

### MetadataService

Features:

* Metadata access

### ActionService

Features:

* Create RecordingAction objects

### JobService

Features:

* Create Job objects

### JobDashboardService

Features:

* Count total jobs
* Count pending jobs
* Count running jobs
* Count done jobs
* Count failed jobs
* Detect latest job id
* Detect latest job type
* Detect latest job status

### RecordingDashboardService

Features:

* Count total recordings
* Count recordings with metadata
* Count recordings without metadata
* Detect latest recording id
* Detect latest recording title

---

## Workflow Layer

Implemented:

### RecordingWorkflowService

Features:

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

Features:

* executeJob()
* processNextJob()

Lifecycle:

PENDING
→ RUNNING
→ DONE

---

## Adapter Layer

Implemented:

### RectoolsAdapter

Status:

Placeholder implementation

Purpose:

Future integration point for VDR-Rectools.

Currently no external execution.

---

## Dashboard Layer

Implemented:

### JobDashboardService

Purpose:

Read-only dashboard service for queue and job state.

Architecture:

JobRepository
↓
JobDashboardService
↓
DashboardSummary

Output model:

* totalJobs
* pendingJobs
* runningJobs
* doneJobs
* failedJobs
* latestJobId
* latestJobType
* latestJobStatus

### RecordingDashboardService

Purpose:

Read-only dashboard service for recording and metadata overview.

Architecture:

RecordingRepository
+
MetadataRepository
↓
RecordingDashboardService
↓
RecordingDashboardSummary

Output model:

* recordingsTotal
* recordingsWithMetadata
* recordingsWithoutMetadata
* latestRecordingId
* latestRecordingTitle

### DashboardFacade

Purpose:

Single read-only facade for future dashboard consumers.

Architecture:

JobDashboardService
+
RecordingDashboardService
↓
DashboardFacade
↓
DashboardOverview

Output model:

* jobs
* recordings

Used for future:

* CLI dashboard
* JSON export
* REST API
* Web UI
* VDR OSD

---

## Current Architecture

Execution path:

Recording
↓
RecordingWorkflowService
↓
Job
↓
JobRepository
↓
SQLite
↓
WorkerSimulator
↓
DONE

Dashboard path:

JobRepository
↓
JobDashboardService
↓
DashboardSummary
+
RecordingRepository
+
MetadataRepository
↓
RecordingDashboardService
↓
RecordingDashboardSummary
↓
DashboardFacade
↓
DashboardOverview

Future execution path:

Recording
↓
RecordingWorkflowService
↓
Job
↓
JobRepository
↓
SQLite
↓
RectoolsAdapter
↓
VDR-Rectools

Future external access path:

DashboardFacade
↓
DashboardJsonSerializer
↓
REST API / Web UI / CLI / OSD

---

## Existing Tags

v0.1-phase0

Project foundation and documentation.

v0.2-phase1-core

Core architecture and persistence.

v0.3-phase1-workflow

Workflow layer.

v0.4-phase2-worker

Worker lifecycle.

v0.5-phase2-queue

Queue-style processing.

v0.6-phase3-dashboard

Job dashboard service.

v0.7-phase4-recording-dashboard

Recording dashboard service.

v0.8-phase5-dashboard-facade

Dashboard facade.

---

# Last Completed Step

Phase 5

DashboardFacade

Implemented files:

* core/recordings/include/DashboardOverview.h
* core/recordings/include/DashboardFacade.h
* core/recordings/src/DashboardFacade.cpp
* core/recordings/tests/test_dashboard_facade.cpp

Updated files:

* Makefile

Verified:

* make clean
* make test

Result:

All tests passed.

---

# Next Planned Step

Phase 6.0

DashboardJsonSerializer

Goals:

* Convert DashboardOverview to JSON
* No HTTP server yet
* No REST routing yet
* No external dependencies if possible
* Stable output for future CLI, REST API, Web UI and OSD

Planned architecture:

DashboardFacade
↓
DashboardOverview
↓
DashboardJsonSerializer
↓
JSON string

Potential output:

```json
{
  "jobs": {
    "totalJobs": 12,
    "pendingJobs": 6,
    "runningJobs": 0,
    "doneJobs": 4,
    "failedJobs": 2,
    "latestJobId": 12,
    "latestJobType": "RENAME",
    "latestJobStatus": "PENDING"
  },
  "recordings": {
    "recordingsTotal": 2,
    "recordingsWithMetadata": 1,
    "recordingsWithoutMetadata": 1,
    "latestRecordingId": 2,
    "latestRecordingTitle": "Tagesschau"
  }
}
