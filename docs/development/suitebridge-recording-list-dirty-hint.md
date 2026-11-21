# SuiteBridge recording-list dirty hint

## Scope

This is a bounded follow-up to the recording import reconciliation incident
documented in `recording-import-auto-reconciliation.md`. PR #335 repaired the
authoritative cache retry and frontend invalidation path. This slice adds one
supplemental native hint; it does not replace RESTfulAPI polling, the recording
cache repository, or the cache-commit-before-SSE boundary.

## Source proof

VDR 2.7.9 exposes two different facts:

- `cStatus::Recording(..., On)` reports recording start/stop activity only.
  It does not report arbitrary additions to the recordings inventory.
- `cRecordings` is state-keyed. VDR's video-directory scanner takes the
  recordings write lock while incorporating newly discovered recording
  directories, so the recordings state revision advances when an external
  import becomes part of VDR's native list.

Therefore the existing SuiteBridge `recording` counter is not reinterpreted as
an inventory counter.

## Bounded design

SuiteBridge 0.14.1 adds a dedicated `RecordingList` counter and carries it as
`recording_list` in snapshot/local-contract schema 4.

The plugin observes only VDR's native `cRecordings` state key:

1. the plugin's VDR main-loop hook calls the status monitor;
2. the monitor performs `cRecordings::GetRecordingsRead(stateKey, 1)`;
3. unchanged state returns without work;
4. a changed state is acknowledged by `cStateKey::Remove()`;
5. one saturating `RecordingList` counter increment is recorded.

The observer has no filesystem scan, network I/O, database access, allocation
queue, plugin-owned thread, sleep, or VDR mutation. The one-millisecond lock
timeout bounds waiting behind a recording-list writer.

## Agent and daemon path

The existing Agent observation worker reads the versioned SuiteBridge snapshot.
The new absolute counter participates in the existing epoch, overflow and
counter-regression rules.

The daemon consumes the absolute counter through an epoch-fenced one-shot
tracker. It deliberately does not repeatedly consume a still-visible Agent
delta.

On one accepted counter advance the daemon only calls the existing
`RecordingCacheRefreshQueue::request(backendId)`.

The authority chain remains:

```text
VDR cRecordings state
  -> SuiteBridge recording_list counter
  -> Agent snapshot observation
  -> one-shot daemon dirty hint
  -> existing RecordingCacheRefreshQueue
  -> authoritative RESTfulAPI inventory read
  -> persistent cache commit
  -> completion queue
  -> SnapshotChangeFeed / SSE
  -> frontend invalidation
```

A hint never publishes SSE directly and never writes the recording cache.
Retry/backoff from PR #335 remains authoritative when the inventory read fails.

## Compatibility

This is an explicit plugin-local wire-contract change:

- plugin version: `0.14.0 -> 0.14.1`
- discovery schema: unchanged at 1
- capability schema: unchanged at 1
- snapshot schema: `3 -> 4`
- local contract schema: `3 -> 4`
- generic mutation capability: unchanged

Agent and plugin must both support schema 4 for SuiteBridge observation to be
current. A temporary mixed-version deployment fails closed at the handshake;
the independent RESTfulAPI polling path remains available.

The Phase-68 closeout continues to document the last live-accepted 0.14.0
baseline until this follow-up has its own real-host acceptance.

## Acceptance

Hosted CI must prove plugin contract tests, schema parsing, observation deltas,
one-shot daemon consumption, architecture guards, full daemon build and
packaging.

Real yaVDR acceptance remains separate: after installing the coordinated plugin
and daemon candidate, an external vdr-rectools import must cause the native
recording-list counter to advance and the already-open VDR-Suite UI to converge
without browser reload or a future daemon restart.
