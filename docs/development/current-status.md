# VDR-Suite – Current Project Status

## Project

VDR-Suite

Goal:

Modern service-oriented backend architecture for VDR recordings, metadata management, job processing, dashboard services and future integration of VDR-Rectools.

---

## Current Branch

phase-2-actions

## Latest Tag

v0.6-phase3-dashboard

## Latest Commit

2493ae7

Phase 3: add JobDashboardService

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

---

## Repository Layer

Implemented:

### RecordingRepository

Features:

* Load recordings
* Search recordings

### MetadataRepository

Features:

* Load metadata

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

Current output model:

* totalJobs
* pendingJobs
* runningJobs
* doneJobs
* failedJobs
* latestJobId
* latestJobType
* latestJobStatus

Used for future:

* CLI dashboard
* REST API
* Web UI
* VDR OSD

---

## Current Architecture

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

Dashboard:

JobRepository
↓
JobDashboardService
↓
DashboardSummary

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

Future dashboard path:

JobDashboardService
+
RecordingDashboardService
↓
DashboardFacade
↓
CLI / REST API / Web UI / OSD

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

---

# Last Completed Step

Phase 3

JobDashboardService

Implemented files:

* core/recordings/include/DashboardSummary.h
* core/recordings/include/JobDashboardService.h
* core/recordings/src/JobDashboardService.cpp
* core/recordings/tests/test_job_dashboard_service.cpp

Updated files:

* Makefile
* docs/development/current-status.md

Verified:

* make clean
* make test

Result:

All tests passed.

---

# Next Planned Step

Phase 4

RecordingDashboardService

Goals:

* Recording overview
* Recording statistics
* Metadata coverage overview
* Shared backend service for:

  * CLI
  * REST API
  * Web UI
  * OSD

Potential output:

* recordingsTotal
* tsRecordings
* pesRecordings
* recordingsWithMetadata
* recordingsWithoutMetadata
* latestRecordingId
* latestRecordingTitle

Planned architecture:

RecordingRepository
+
MetadataRepository
↓
RecordingDashboardService
↓
RecordingDashboardSummary

---

# Long-Term Goals

* Real RectoolsAdapter integration
* REST API
* Web Frontend
* VDR OSD integration
* Unified recording management
* Metadata services
* Background worker processing
* Dashboard facade
* Modern VDR Suite architecture
