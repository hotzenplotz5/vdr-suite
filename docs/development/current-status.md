# VDR-Suite – Current Project Status

## Project

VDR-Suite

Goal:

Modern service-oriented backend architecture for VDR recordings, metadata management, job processing and future integration of VDR-Rectools.

---

## Current Branch

phase-2-actions

## Latest Tag

v0.5-phase2-queue

## Latest Commit

f887ebc

Phase 2: add queue style job processing

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

Future:

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

---

# Next Planned Step

Phase 3

JobDashboardService

Goals:

* Job overview
* Queue monitoring
* Job statistics
* Shared backend service for:

  * CLI
  * REST API
  * Web UI
  * OSD

Potential methods:

* getAllJobs()
* getPendingJobs()
* getRunningJobs()
* getFinishedJobs()
* getJobStatistics()

---

# Long-Term Goals

* Real RectoolsAdapter integration
* REST API
* Web Frontend
* VDR OSD integration
* Unified recording management
* Metadata services
* Background worker processing
* Modern VDR Suite architecture
