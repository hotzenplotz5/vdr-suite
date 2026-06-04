# VDR-Suite – Current Project Status

## Introduction

New contributors should start with:

- `docs/introduction/vdr-suite-vision.md`
- `README.md`

The vision document explains why VDR-Suite exists, the long-term goals of the project, the snapshot architecture philosophy and the future direction of the platform.

---

## Project

VDR-Suite

Goal:

Modern service-oriented backend architecture for VDR recordings, metadata management, job processing, dashboard services, JSON export, REST API, Web UI, OSD integration and future integration of VDR-Rectools.

VDR remains the primary backend domain and source of truth.

---

## Current Branch

`phase-2-actions`

---

## Current Verified Head

`0e4d231`

Latest milestone tag:

`v1.97-phase8-makefile-modularization`

Latest completed phase:

Phase 8.98: Makefile source modularization.

Verified locally with:

- `make clean`
- `make test`

Important architecture note:

Phase 8.98 reduces the root `Makefile` by moving source-group definitions into dedicated `mk/*.mk` include files while preserving all public target names and build behavior.

The repository was build- and test-clean at `0e4d231` before this documentation update.

---

## Current Architecture State

VDR-Suite is moving from direct live RESTfulAPI access per API request toward a daemon-owned snapshot and change-detection architecture.

Current implemented chain:

RESTfulAPI
    ↓
/change-state.json
    ↓
RestfulApiVdrAdapter
    ↓
VdrChangeState
    ↓
VdrService
    ↓
PollingService
    ↓
ChangeDetectionService
    ↓
VdrChangeEvent
    ↓
SnapshotRefreshDecisionService
    ↓
VdrSnapshotBuilder
    ↓
SnapshotCacheService
    ↓
SnapshotCache
    ↓
SnapshotAccessService
    ↓
VdrOverviewService
    ↓
VdrController

Purpose:

- keep RESTfulAPI behind the adapter boundary
- avoid repeated live RESTfulAPI calls per API request
- keep refresh decisions inside daemon-owned services
- keep snapshot storage separate from polling logic
- prepare efficient polling based on lightweight change-state checks
- prepare future multi-VDR and permission-aware designs
- keep API controllers backend-independent
- avoid premature federation, SSE, WebSocket, user-management or cluster runtime implementation

---

## Build System State

The top-level `Makefile` now delegates source groups to modular include files under `mk/`.

Implemented build modules:

- `mk/common.mk`
- `mk/recording-sources.mk`
- `mk/action-job-sources.mk`
- `mk/rest-sources.mk`
- `mk/daemon-sources.mk`
- `mk/http-sources.mk`
- `mk/vdr-sources.mk`
- `mk/vdr-tests.mk`

The public targets remain stable:

- `make test`
- `make clean`
- `make daemon`
- `make dashboard-cli`
- `make prepare-test-db`

---

(remaining content unchanged intentionally; introduction added at top)