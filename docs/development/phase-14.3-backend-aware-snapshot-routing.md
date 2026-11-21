# Phase 14.3 - Backend-Aware Snapshot Read Routing Boundary

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Roadmap](../planning/roadmap.md)

---

## Purpose

This document defines the implementation direction for Phase 14.3.

Phase 14.3 prepares the snapshot read layer for backend-aware routing while preserving the existing single-backend behavior.

This is an architecture planning document. It does not introduce code by itself.

---

## Current Verified Baseline

Phase 14.2 completed backend identity lookup on the existing snapshot cache boundary.

Current state:

- `VdrSnapshot` carries `backendId` with default value `default`
- snapshot change feed entries carry `backendId`
- snapshot change feed JSON serializes `backendId`
- snapshot read metadata can serialize `backendId`
- `SnapshotCacheService` exposes `backendId()`
- `SnapshotAccessService` is still single-cache and single-snapshot oriented
- `ISnapshotAccessService` currently exposes only `hasSnapshot()` and `snapshot()`

---

## Architectural Problem

The project now has backend identity attached to snapshot-related data, but the read-side access boundary cannot yet answer backend-scoped read questions.

Current access shape:

```text
hasSnapshot()
snapshot()
```

Future multi-backend access shape needs to support the question:

```text
Which backend should this read operation target?
```

Without this boundary, future controllers or REST endpoints would be forced to know too much about cache layout.

---

## Phase 14.3 Goal

Introduce a backend-aware snapshot read routing boundary above the current snapshot cache access layer.

The first implementation should remain backward compatible and continue to route the existing single-backend runtime to `default`.

---

## Proposed Boundary

Phase 14.3 should introduce backend-aware read methods without removing the existing single-backend methods immediately.

Candidate interface direction:

```text
bool hasSnapshotForBackend(backendId)
const VdrSnapshot* snapshotForBackend(backendId)
```

The exact C++ naming must follow the existing repository style after the affected files are read again immediately before implementation.

---

## Routing Semantics

Initial behavior should be deliberately small:

```text
default -> existing SnapshotCacheService snapshot
matching cached backendId -> existing SnapshotCacheService snapshot
unknown backendId -> nullptr / false
```

This keeps Phase 14.3 testable without introducing a multi-cache registry yet.

---

## Non-Goals

Phase 14.3 must not introduce:

- persistent backend registry
- multi-cache storage
- remote backend connection management
- permission enforcement
- authentication
- frontend changes
- REST endpoint expansion unless explicitly scoped later
- destructive operation routing

---

## Architecture Boundaries

Backend-aware snapshot routing:

- consumes backend identity already present in snapshots
- remains above `SnapshotCacheService`
- does not own polling
- does not own snapshot generation
- does not own change detection
- does not parse backend-specific adapter data
- does not replace the future `BackendRegistry`

---

## Expected Implementation Shape

Recommended implementation order:

1. Add failing tests to `test_snapshot_access_service.cpp` for backend-aware snapshot lookup.
2. Extend `ISnapshotAccessService` with backend-aware read methods while keeping existing methods.
3. Implement the methods in `SnapshotAccessService` using the current single `SnapshotCacheService`.
4. Verify that existing single-backend tests continue to pass.
5. Update architecture documentation after implementation.

---

## Expected Tests

Minimum tests:

- default backend lookup returns the existing snapshot
- explicit matching backend lookup returns the existing snapshot
- unknown backend lookup returns no snapshot
- empty cache lookup remains false/nullptr
- existing `hasSnapshot()` and `snapshot()` behavior remains unchanged

---

## Next Phase After 14.3

Phase 14.4 should likely introduce the first `BackendRegistry` or `BackendNode` domain boundary.

That should only happen after snapshot read routing has a stable backend-aware access surface.

---

## Back

- [Back to Development Index](index.md)
- [Back to Current Project Status](current-status.md)
- [Back to Roadmap](../planning/roadmap.md)
