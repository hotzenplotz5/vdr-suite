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
callbacks until `cPluginSuiteBridge::Start()` activates it. `Stop()` begins the
explicit stopping lifecycle phase and deactivates the monitor before completing
the transition to `stopped`.

## Data minimization

The bridge does not retain VDR object pointers, recording names, replay names or
file paths. Each accepted callback performs only the active-state check and one
bounded atomic counter increment. It then returns immediately.

Callback arguments are not serialized, logged, queued or forwarded. Diagnostic
logging and local-contract serialization happen only outside the VDR callback
path through activation, deactivation or explicit snapshot requests.

## Concurrency boundary

VDR may invoke status callbacks from different execution contexts. The observer
therefore uses only:

- one atomic active flag;
- one relaxed atomic counter per event family;
- no queue;
- no lock;
- no allocation;
- no logging in the callback body;
- no worker thread.

The counters are diagnostic observations. They are not yet sequence numbers and
do not define overflow, reset or resynchronization semantics.

## Capability state

Capability schema version remains `1`.

`status-events` remains `available`. Later accepted slices also make `snapshots`
and `local-contract` available. `mutations` remains `disabled`.
