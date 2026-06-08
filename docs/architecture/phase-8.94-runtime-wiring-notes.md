# Phase 8.94 Runtime Wiring Notes

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)

---

## Objective

Connect SnapshotCache ownership to daemon runtime components.

## Ownership Model

DaemonRuntime
  -> SnapshotCache
  -> SnapshotCacheService
  -> PollingService

## Responsibilities

SnapshotCache
- stores latest snapshot
- no refresh logic

SnapshotCacheService
- service boundary
- future extension point

PollingService
- polling
- change detection
- refresh decisions
- snapshot rebuild triggering

DaemonRuntime
- object ownership
- lifecycle management

## Explicitly Out Of Scope

- federation runtime
- SSE runtime
- WebSocket runtime
- user management
- permissions
- clustering
---

## Back

- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
