# SB.3 Native VDR Status Events

SB.3 adds the first read-only observation path from VDR Core into the Suite
bridge.

## Native integration

`SuiteBridgeStatusMonitor` derives from VDR's `cStatus` interface and observes:

- channel switches;
- recording start and stop notifications;
- replay start and stop notifications;
- timer changes.

The monitor is registered for the lifetime of the plugin object but ignores all
callbacks until `cPluginSuiteBridge::Start()` activates it. `Stop()` deactivates
the monitor before completing the plugin lifecycle transition.

## Data minimization

The bridge does not retain VDR object pointers, recording names, replay names or
file paths. Each accepted callback increments one atomic counter and writes a
structured log line containing only scalar state needed for diagnostics.

## Concurrency boundary

VDR may invoke status callbacks from different execution contexts. The initial
observer therefore uses only:

- one atomic active flag;
- one relaxed atomic counter per event family;
- no queue;
- no lock;
- no allocation;
- no worker thread.

## Capability transition

Capability schema version remains `1`.

`status-events` changes from `planned` to `available` only because the native
callbacks, activation boundary, counters, source contracts, unit tests and live
acceptance path now exist.

`snapshots`, `local-contract` and `mutations` remain unavailable.
