# Recording Actions Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)

---

## Purpose

This document defines the architecture boundary for future recording actions.

Phase 30.0 does not implement destructive recording operations.
It defines the rules required before move, rename, delete, cut, repair or metadata refresh can be exposed through APIs or frontends.

## Core Rule

Recording actions must never be implemented as direct frontend-to-filesystem operations.

All recording actions must pass through explicit backend-aware service, validation, capability, permission and job boundaries.

## Existing Foundation

VDR-Suite already contains an early action and job foundation:

ActionService
-> RecordingAction
-> JobService
-> Job
-> JobRepository
-> Worker or Adapter boundary

This foundation is intentionally minimal.
It is not yet sufficient for real recording operations because it does not carry backend identity, action payloads, dry-run information, capability checks or permission decisions.

## Required Future Boundary

Future recording actions should use this conceptual chain:

REST/API request
-> RecordingActionRequest
-> Recording identity resolution
-> Validation / dry-run planning
-> Backend capability check
-> Permission check
-> RecordingActionPlan
-> Job boundary
-> Backend or adapter execution
-> Result / diagnostics / audit trail

The job boundary is mandatory for destructive or long-running operations.

## Backend Identity

Recording actions must be backend-aware.

Rules:

- a recording action must know which backend owns the target recording
- backend=default compatibility may be preserved for legacy single-backend behavior
- empty backend identity may only be interpreted through an explicit legacy/default rule
- multi-backend actions must not guess ownership from title, path or display labels
- cross-backend operations are out of scope until explicitly designed

## Destructive Actions

Destructive or state-changing actions include:

- move
- rename
- delete
- cut
- repair
- metadata refresh execution

These must require:

- explicit validation
- explicit action request
- explicit backend ownership
- capability check
- permission check
- job creation
- auditable result state

## Dry-Run and Validation

Every destructive action should support dry-run or validation before execution.

Dry-run must not mutate VDR, VDR-Suite metadata, the filesystem or Rectools state.

## Capability Boundary

Recording actions must not assume that every backend can perform every action.

Future capability names may include:

- recordings.action.move
- recordings.action.rename
- recordings.action.delete
- recordings.action.cut
- recordings.action.repair
- recordings.action.metadata.refresh
- recordings.action.dry-run

## Permission Boundary

Recording actions must be permission-ready.

The architecture must allow future policies such as:

- view-only backend
- read-only backend
- admin-only delete
- operator-only repair
- metadata-only permission
- per-backend permissions
- per-client permissions

## Rectools Boundary

VDR-Rectools remains an external execution system.

Rules:

- no controller should call Rectools directly
- no frontend should call Rectools directly
- Rectools execution should remain behind an adapter or job boundary
- Rectools failures must be represented as job/action results
- placeholder execution must not be mistaken for production execution

## Recording Domain Separation

VDR recordings and VDR-Suite metadata recordings are related but not identical.

Rules:

- VDR remains the source of truth for real recordings
- VDR-Suite metadata remains complementary
- recording action target identity must come from the backend-aware VDR recording model
- metadata refresh may update VDR-Suite metadata, but must not redefine VDR ownership
- local metadata IDs must not be used as the only identity for real backend actions

## API Boundary

Future REST APIs should expose action intent, not filesystem operations.

Possible future endpoints may include:

- POST /api/vdr/recordings/actions/validate
- POST /api/vdr/recordings/actions
- GET /api/vdr/recordings/actions/{actionId}
- GET /api/jobs/{jobId}

Exact routes remain future work.

## Phase 30.1 Request Model

Phase 30.1 introduces the first backend-aware recording action request domain model.

The request model is intentionally not an execution model.
It describes the requested action intent and keeps execution behind later validation, capability, permission and job boundaries.

The request model carries:

- backend identity
- backend-owned recording identity
- action type
- dry-run flag
- action parameters

The default request behavior is dry-run oriented.
Real execution remains out of scope until a later phase defines validation results, action plans and job payload boundaries.

## Non-Goals

Phase 30.0 does not:

- move files
- delete recordings
- rename recordings
- cut recordings
- repair recordings
- call Rectools for real execution
- change /api/vdr/recordings
- implement frontend action buttons
- implement final authentication or authorization

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
