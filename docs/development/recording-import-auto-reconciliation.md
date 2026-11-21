# Recording import automatic reconciliation

## Proven incident (2026-09-24)

Repository base: `main` at `94dcffa8fc77b9087472ec09056853819515308a`
(PR #334). Its accepted head `c4b0be094f69d74e68787f3487d678af4c783550`
passed [VDR-Suite CI #9143, run 35967848509](https://github.com/hotzenplotz5/vdr-suite/actions/runs/35967848509).

Read-only yaVDR evidence, without browser reload, cache mutation or daemon restart:

- Running daemon PID 49994 started on September 23 at 21:49:13 CEST.
- SHA-256 of both the running executable and installed daemon:
  `486cf3abfb6bb6dc3c0c1e037cf2c6529919bbbb82ce038683df7577f162c470`.
  It matches the binary in the existing Phase-68.G acceptance worktree
  (checkout head `7ae51d090cbe06570b8a70e787137232df83f124`).
  This is binary provenance evidence, not proof of a fresh clean rebuild.
- The unrelated primary checkout is on `work/manual-series-assignment-grouping`
  at `9d4d88e1a1b904ce85a2024e7955bb0e63ca21c8` with uncommitted changes;
  it was not changed.
- Native RESTfulAPI reports 1017 recordings. The persisted Suite cache has 1011.
- The cache status is `stale`, with `HTTP request timed out`.
  Last successful refresh: 06:23:33 UTC; last failed attempt: 06:53:24 UTC.
- RESTfulAPI change hints arrived at 08:50:40 and 08:50:46 CEST.
  Recording refreshes began at 08:52:57 and 08:53:14 and both timed out
  exactly ten seconds later. Subsequent periodic metadata work continued,
  but did not reload the recording inventory.
- A read-only full `/recordings.json` request succeeded in 10.774108 seconds
  (HTTP 200, 4,645,627 bytes). A subsequent request also returned 1017 recordings.
- SQLite lock errors occurred during the preceding EPG refresh. They are
  additional observed contention, not established as the cause of the HTTP timeout.

## Failure chain

Native changes reach the daemon through polling and RESTfulAPI dirty hints.
The inventory queue removes the ordinary one-shot hint in `takePending()`.
If the provider request or repository replacement then fails, the worker
previously marked the cache stale but did not retain outstanding work.
The polling snapshot can already have consumed the native version independently
of the persistent browse cache. With no further native change, there is then
no guaranteed future inventory retry.

Periodic metadata enrichment reads the existing persistent recording inventory;
it cannot discover recordings missing from that inventory.

The normal HTTP deadline remains ten seconds. Increasing it globally would
also affect unrelated backend requests and would not repair lost work.
This change instead retains failed read work with a steady-clock retry schedule:
5, 10, 20, 40, then at most one attempt per 60 seconds per backend.
Fresh hints are coalesced, unrelated backends remain eligible, success resets
the delay, and hints arriving during a successful read survive.
This retries read-only reconciliation, never a native mutation.

The existing commit-before-feed boundary remains:
successful repository replacement -> completion queue -> polling-thread
SnapshotChangeFeed -> SSE. Failure emits no false recording completion.

## Frontend composition

Recordings 2 already subscribes through the canonical Client API and also
revalidates its folder view every 30 seconds. Those requests could only read
the stale persistent inventory in this incident.

The retained Home Recording Discovery owner had no Recording SSE subscription.
Its ready-generation and warm Series shortcuts could therefore keep an open
Home unchanged even after a successful backend update.
It now consumes the same cache-committed `recordings` domain, invalidates its
existing projections and calls its existing generation-fenced refresh path.

The subscription coalesces bursts and in-flight hints, filters backend/domain
and duplicate sequences, catches up after a daemon sequence reset, closes on
Home exit/hidden state, and catches up on return. It creates no alternate
Recording, metadata, playback or HTTP owner and adds no periodic page reload.

## SuiteBridge observation boundary

The existing `cStatus::Recording(..., On)` callback records native recording
start/stop, not external inventory imports. A reliable additional SuiteBridge
import hint would require observing VDR's recording-list state, with its own
protocol/lifecycle compatibility review. That optional extension is outside
this bounded failure-recovery fix; it cannot replace retry after a failed read.

## Validation and acceptance boundary

- Deterministic queue tests cover failed reads without another native hint,
  capped backoff, independent backends, in-flight hints and success reset.
- The runtime guard covers repository rejection and both exception paths,
  plus polling admission and completion publication.
- The Home regression runs the full production source through install/lazy-load,
  canonical Client API feed delivery, actual refresh calls and DOM projection,
  including hidden/return, backend switch, replay and restart handling.
- Existing Recordings 2, Home retention and discovery regressions remain required.
- Hosted final-head CI is required before acceptance.
- No daemon/plugin installation, restart, browser reload or live cache repair
  is performed by this PR. Real installed-candidate acceptance is still separate:
  after a separately authorized deployment, an external import must converge
  automatically in the persistent cache and already-open Home/Recordings 2.
