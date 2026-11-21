# ADR-0035: Lazy Recording Loading and Backend-Scoped Refresh

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Lazy Recording Loading Planning](../planning/lazy-recording-loading.md)

---

Status: Accepted

Date: 2026-06-26

---

## Context

VDR-Suite supports multiple VDR backends and already has snapshot-backed read APIs for recordings.

Recordings are a heavy runtime domain. On real installations, reading the recording list can take a long time, especially when a backend has a large archive or when multiple backends are configured.

The daemon startup path must not synchronously load the recordings list for all configured backends. Otherwise startup and multi-backend availability can be blocked for seconds or minutes before the HTTP/API layer is useful.

The existing `VdrSnapshotBuilder::buildSnapshotWithoutEvents()` name is misleading for this requirement: it avoids loading EPG events, but still loads recordings. That behavior is not the intended long-term startup behavior.

---

## Decision

VDR-Suite must treat recordings as a heavy, on-demand domain.

Daemon startup and initial backend polling must not require loading the full recording list of every backend.

Recording lists must be loaded backend-by-backend when the user opens or refreshes the recording view for a specific backend.

The frontend-facing recording page must be able to render before recordings are available and show a loading state for each backend.

Recording loading state must be explicit. Missing recording input must not be silently interpreted as an empty recording list.

---

## Required Runtime Semantics

Startup snapshot behavior:

```text
startup snapshot:
  load lightweight backend state only
  do not load recordings
  do not load full EPG/events
```

Recording page behavior:

```text
recording view opens:
  show backend cards or backend selection immediately
  show recording status per backend
  load recordings only for the selected backend
```

Backend refresh behavior:

```text
refresh recordings for backend A:
  only backend A is loading
  backend B remains usable
  slow or offline backend A must not block all recording views
```

---

## Recording Cache Status

Recording cache state must be tracked per backend.

The status vocabulary should include at least:

```text
unknown
loading
ready
stale
unavailable
error
```

A ready cache with zero recordings is a valid empty result.

An unknown, loading, unavailable or error state is not a valid empty result.

---

## API Direction

The recording API should evolve toward explicit backend-scoped refresh and status endpoints.

Possible future shape:

```text
GET  /api/backends/{backendId}/recordings/status
POST /api/backends/{backendId}/recordings/refresh
GET  /api/backends/{backendId}/recordings
```

The existing recording query API may continue to exist, but it must not force a full recording load for every backend during daemon startup.

---

## Frontend Direction

The frontend must not assume recordings are already present at page load.

Required frontend behavior:

```text
backend card visible immediately
recording list initially empty because status is unknown/loading
show spinner or indeterminate progress while loading
show count and data only after ready
show backend-specific error state when loading fails
```

A percentage progress bar is optional and should only be used if the backend can provide meaningful progress. Until then, an indeterminate loading indicator is preferred.

---

## Action and Readback Direction

Recording actions such as delete, move, rename or external processing must refresh only the affected backend after execution.

They must not trigger a global recording refresh across all configured backends.

Expected flow:

```text
execute action on backend A
mark backend A recordings stale/loading
read back backend A recordings
update backend A cache
leave backend B unchanged
```

---

## Consequences

Positive consequences:

- Faster daemon startup.
- Multi-backend installations do not block on the slowest recording backend.
- Recording views can give accurate loading feedback.
- Read-only and slow remote backends can coexist with local backends.
- Recording actions can refresh narrowly and predictably.

Trade-offs:

- Recording read APIs need explicit cache status semantics.
- The frontend must handle unknown/loading states.
- Snapshot builder naming and startup behavior need cleanup.
- Tests must distinguish unknown recording state from a valid empty recording list.

---

## Implementation Notes

A future implementation phase should introduce a lightweight startup snapshot path, for example:

```text
buildStartupSnapshot()
```

or:

```text
buildLightweightSnapshot()
```

This path must exclude recordings and full EPG/events.

A separate recording refresh service should own backend-scoped recording loading, state tracking and measurement.

Runtime measurements for recording loading must remain visible so large archives and slow remote backends can be diagnosed.

---

## Related Decisions

- ADR-0018: Incremental Snapshot Synchronization
- ADR-0021: Selective Backend Query Strategy
- ADR-0034: SearchTimer Warm EPG Cache and Change Invalidation

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to ADR Index](index.md)
