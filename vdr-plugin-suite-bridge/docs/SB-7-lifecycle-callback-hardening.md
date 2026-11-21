# SB.7 Gold-Standard Lifecycle and Callback-Boundary Hardening

SB.7 hardens the existing read-only bridge without adding a new command,
capability, schema or transport.

## Ownership

This slice belongs entirely to `vdr-plugin-suite-bridge` because it changes the
VDR-process-local lifecycle and `cStatus` callback boundary.

The VDR-Suite Control Plane continues to own durable policy, identity, jobs,
authorization and public contracts. The Backend Agent continues to own local
transport selection and plugin invocation. ADR-0046 keeps public media sessions,
routing and Streaming Gateway behavior outside the plugin.

## Lifecycle state machine

The lifecycle is now explicit:

```text
constructed
  -> initialized
  -> started
  -> stopping
  -> stopped
```

`BeginStop()` accepts an initialized or started lifecycle and enters `stopping`.
Repeated calls while stopping or stopped are deterministic and accepted.
`CompleteStop()` accepts only `stopping`, or an already stopped lifecycle, and
finishes the transition.

The plugin stop order is:

1. enter `stopping`;
2. log the bounded stop-begin fact;
3. atomically deactivate status observation;
4. capture and log the final inactive snapshot outside callbacks;
5. complete the lifecycle transition;
6. log the bounded stopped fact.

Initialization or start cannot succeed after the stop sequence has begun.

## Callback boundary

The four native callbacks are:

- channel switch;
- recording state change;
- replay state change;
- Timer change.

Each callback now performs only:

1. argument discard without dereferencing or retention;
2. an atomic active-state check;
3. one relaxed atomic counter increment;
4. immediate return.

The callback body performs no:

- logging;
- serialization;
- allocation;
- locking;
- queue operation;
- network access;
- file or database work;
- waiting;
- Agent or Control Plane invocation.

Lifecycle, snapshot, local-contract and SVDRP logs remain outside callback
execution.

## Compatibility

Plugin version changes to `0.8.0`.

The following remain unchanged:

- capability schema version `1`;
- snapshot schema version `1`;
- local-contract schema version `1`;
- capability catalogue;
- `PLUG suitebridge SNAP` syntax;
- reply codes `900`, `504` and `451`;
- deterministic JSON field order and payload meaning;
- `mutations = disabled`.

Removing per-event callback logs is deliberate. Event counts remain observable
through `SNAP` and lifecycle boundary snapshots. The counters remain diagnostic
observations, not synchronization sequences.

## Non-goals

SB.7 does not add:

- a mutation;
- a new SVDRP command;
- a plugin service call;
- a listener or socket;
- a worker, thread or queue;
- a database;
- filesystem mutation;
- Timer or Recording write access;
- counter reset, overflow or resynchronization semantics;
- Backend Agent protocol framing;
- media streaming or Streaming Gateway behavior.

## Automated acceptance

`make check` must prove:

1. version `0.8.0` is extracted;
2. `Stopping` exists as an explicit state;
3. begin-stop and complete-stop transitions are deterministic;
4. initialization and start are rejected after stopping begins;
5. callback implementations contain no logging or external side effect;
6. each callback still records the correct atomic event family;
7. all SB.1 through SB.6 contracts and unit tests remain green;
8. the final VDR shared object builds and passes ELF validation.

## Required live VDR acceptance

The live test must prove:

1. plugin version `0.8.0` loads on VDR;
2. lifecycle reaches `initialized` and `started`;
3. a controlled channel switch increments the channel-switch counter;
4. no callback-side `status-event` log is emitted;
5. `PLUG suitebridge SNAP` remains schema- and byte-compatible;
6. shutdown logs `stopping` before monitor deactivation and `stopped` afterward;
7. the final inactive snapshot is deterministic;
8. plugin removal and VDR restart leave no Suite Bridge binary loaded;
9. the original channel and read-only VDR state are restored;
10. the repository worktree remains clean.
