# Snapshot Access Architecture

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Architecture Index](index.md)

---

## Purpose

SnapshotAccessService provides a dedicated read layer above SnapshotCacheService.

Controllers and future API services should consume snapshots through this abstraction instead of accessing SnapshotCache directly.

## Architecture

```text
SnapshotCache
    ↓
SnapshotCacheService
    ↓
ISnapshotAccessService
    ↓
SnapshotAccessService
    ↓
REST Controllers
```

## Responsibilities

### SnapshotCache

Stores the current daemon-owned snapshot.

### SnapshotCacheService

Owns cache access and cache lifecycle operations.

### ISnapshotAccessService

Defines the read interface used by higher layers.

### SnapshotAccessService

Provides safe snapshot access.
Returns nullptr if no snapshot is available.

## Future Extensions

The abstraction allows future support for:

- multi-VDR access
- federation strategies
- capability-aware snapshot routing
- lifecycle-aware snapshot selection

without changing controller code.

---

## Back

- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
