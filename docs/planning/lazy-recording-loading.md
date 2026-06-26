# Lazy Recording Loading and Backend-Scoped Recording Refresh

## Navigation

- [Planning Index](index.md)
- [Roadmap](roadmap.md)
- [Project Progress](project-progress.md)
- [Current Project Status](../development/current-status.md)
- [ADR-0035](../adr/ADR-0035-lazy-recording-loading-and-backend-scoped-refresh.md)

---

## Purpose

This document keeps the lazy recording loading requirement visible until it is implemented.

The ordering relative to SearchTimer EPG cache work is flexible, but the requirement must not be lost.

---

## Problem

Reading VDR recordings can be expensive on real installations.

Large archives and multiple backends can make full recording reads take long enough to block daemon startup or make the frontend feel unavailable.

The current startup snapshot path avoids full EPG/event loading but still loads recordings through `buildSnapshotWithoutEvents()`.

That behavior is not the desired long-term runtime model.

---

## Target Behavior

Daemon startup must stay lightweight.

```text
startup:
  load only lightweight backend state
  do not load full recordings
  do not load full EPG/events
```

Recording pages must render before recordings are loaded.

```text
recording page:
  show backend list immediately
  show per-backend state
  load recordings only for the selected backend
  show loading state while work is running
```

Multiple backends must remain independent.

```text
backend A loading recordings:
  backend B remains usable
  backend C can still show its last known state
```

---

## Required Backend State

Each backend needs recording-load state such as:

```text
unknown
loading
ready
stale
unavailable
error
```

A ready cache with zero recordings is valid.

An unknown, loading, unavailable or error state must not be shown as a valid empty recording list.

---

## Future API Direction

Preferred future shape:

```text
GET  /api/backends/{backendId}/recordings/status
POST /api/backends/{backendId}/recordings/refresh
GET  /api/backends/{backendId}/recordings
```

The existing recording query API can remain, but it should consume the backend-scoped recording cache instead of triggering global startup work.

---

## Future UI Direction

The UI should show backend cards or a backend selector immediately.

For each backend, it should show:

```text
not loaded yet
loading
ready with count
stale
error/unavailable
```

Until meaningful progress values exist, an indeterminate spinner or loading bar is preferred over fake percentages.

---

## Action Refresh Direction

Recording actions should refresh only the affected backend.

```text
action on backend A:
  mark backend A recordings stale/loading
  read back backend A recordings
  update backend A cache
  do not refresh backend B/C
```

---

## Planned Implementation Steps

1. Introduce a startup/lightweight snapshot builder path that excludes recordings and full EPG/events.
2. Add backend-scoped recording cache status.
3. Add backend-scoped recording refresh service.
4. Add REST status and refresh endpoints.
5. Adapt recording query APIs to respect loading/ready/error semantics.
6. Add frontend loading-state support.
7. Refresh only the affected backend after recording actions.

---

## Related Documents

- [ADR-0035: Lazy Recording Loading and Backend-Scoped Refresh](../adr/ADR-0035-lazy-recording-loading-and-backend-scoped-refresh.md)
- [Current Project Status](../development/current-status.md)
- [Project Status Dashboard](../project-status-dashboard.md)
