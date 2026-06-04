# Phase 8.94 – Snapshot Cache Integration Plan

## Goal

Integrate the existing SnapshotCache and SnapshotCacheService into the polling runtime.

## Architectural Direction

Current:

VdrSnapshotBuilder -> PollingService -> internal snapshot storage

Target:

VdrSnapshotBuilder -> PollingService -> SnapshotCacheService -> SnapshotCache

## Scope

- PollingService writes snapshots into SnapshotCacheService
- PollingService reads current snapshot from SnapshotCacheService
- DaemonRuntime owns SnapshotCache and SnapshotCacheService
- No SSE runtime
- No WebSocket runtime
- No federation runtime
- No permission model
- No user management

## Expected Benefits

- Single daemon-owned snapshot source
- Cleaner separation of polling and storage responsibilities
- Preparation for future snapshot-backed API responses
- Preparation for future multi-backend cache routing

## Verification

make test-snapshot-cache
make test-snapshot-cache-service
make test-polling-service
make daemon
