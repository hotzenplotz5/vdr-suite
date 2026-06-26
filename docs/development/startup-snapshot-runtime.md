# Startup Snapshot Runtime Rule

## Purpose

This document records the runtime rule for the initial daemon polling snapshot.

The startup snapshot must make the daemon responsive without forcing heavy backend domains to load synchronously.

---

## Rule

The initial polling snapshot is a lightweight startup snapshot.

It may load:

- VDR status
- timers
- SearchTimer metadata
- channels

It must not load:

- recordings
- full EPG events

---

## Reason

Recordings and EPG/event data are heavy domains.

Loading them synchronously during daemon startup has two bad effects:

- daemon startup waits for backend-heavy transfers
- multi-backend setups would multiply that cost across all configured VDR instances

The daemon should become available first and then load heavy backend domains only when a caller or refresh plan actually needs them.

---

## Recording Boundary

Recordings are intentionally absent from the startup snapshot.

A recording page or recording API consumer must therefore not assume that an empty startup snapshot means "no recordings exist".

For recordings, future runtime behavior must distinguish:

- unknown or not loaded yet
- loading
- ready with zero recordings
- ready with recordings
- unavailable or error

---

## EPG Boundary

Full event loading is also excluded from the startup snapshot.

EPG and SearchTimer preview paths should prefer:

- selective EPG queries
- backend-scoped warm preview caches
- explicit input readiness metadata

SearchTimer preview already exposes the `epgInput` object so clients can tell whether match counts are authoritative.

---

## Current Implementation

`VdrSnapshotBuilder::buildStartupSnapshot()` builds the startup snapshot and intentionally skips recordings and events.

`PollingService` uses `buildStartupSnapshot()` for the initial poll.

Targeted tests verify that the startup snapshot reads only status, timers and channels from the VDR adapter in the no-SearchTimer-provider test setup, and does not call recording or event reads.

---

## Related Documents

- [Current Project Status](current-status.md)
- [Lazy Recording Loading](../planning/lazy-recording-loading.md)
- [ADR-0035 Lazy Recording Loading and Backend-Scoped Refresh](../adr/ADR-0035-lazy-recording-loading-and-backend-scoped-refresh.md)
- [SearchTimer Backend Contract](searchtimer-backend-contract.md)
