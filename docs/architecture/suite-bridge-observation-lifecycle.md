# Suite Bridge Read-Only Observation Lifecycle

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Architecture Index](index.md)
- [Suite Bridge Agent Handshake](suite-bridge-agent-handshake.md)
- [Suite Bridge Local SVDRP Transport](suite-bridge-svdrp-transport.md)
- [Plugin Roadmap](../../vdr-plugin-suite-bridge/docs/ROADMAP.md)
- [Shared Suite Bridge Handoff](../../vdr-plugin-suite-bridge/docs/VDR-SUITE-HANDOFF.md)

---

## Status

SB.10c completed.

Focused automated acceptance passed at:

```text
10e82701f5633681b96df13d39ee0c05783ff68c
```

Controlled live VDR acceptance and complete rollback passed at:

```text
7362ecec0d103e1e4659b80476ea5ad321d413e2
```

The plugin runtime contract remained unchanged.

---

## Purpose

SB.10c turns the accepted one-shot Suite Bridge handshake into a bounded,
read-only observation lifecycle owned by the Backend Agent.

The lifecycle:

1. discovers the local plugin through `CAPS 1`;
2. accepts only the exact supported schemas and required read-only capabilities;
3. reads an initial `SNAP` baseline;
4. polls later snapshots without repeating discovery while continuity is trusted;
5. rediscovers after transport or protocol uncertainty;
6. retains the last valid bounded baseline;
7. reports freshness and reconnect state;
8. stops without leaving a worker thread alive.

The plugin remains unchanged.

---

## Architectural Position

```text
Suite Bridge observation worker
  -> deterministic observation service
      -> staged handshake service
          -> accepted typed local transport
              -> VDR SVDRP
                  -> vdr-plugin-suite-bridge
```

The observation lifecycle composes the SB.10a handshake and SB.10b transport.
It does not widen the transport command boundary.

Only these plugin-local requests remain possible:

```text
PLUG suitebridge CAPS 1
PLUG suitebridge SNAP
```

---

## Ownership

Owned by the Backend Agent:

- local compatibility state;
- read-only poll scheduling;
- reconnect backoff;
- last successful observation time;
- freshness classification;
- last valid plugin-local baseline;
- safe diagnostic deltas;
- worker start and stop.

Not owned by this slice:

- daemon runtime wiring;
- backend generation or lease persistence;
- public REST resources;
- user authorization;
- durable audit;
- VDR domain snapshots for channels, EPG, Timers or Recordings;
- mutations.

Daemon integration remains SB.10d.

---

## State Vocabulary

```text
not_configured
connecting
plugin_missing
legacy_or_unknown
incompatible
compatible
snapshot_current
snapshot_stale
transport_degraded
overflowed
offline
```

Meaning:

- `not_configured`: the typed transport has no usable local configuration;
- `connecting`: discovery or reconnect work is in progress;
- `plugin_missing`: VDR replied that `suitebridge` is not loaded;
- `legacy_or_unknown`: the plugin does not provide the required discovery command;
- `incompatible`: identity, schema, capability or snapshot contract is rejected;
- `compatible`: discovery succeeded and the initial snapshot is pending;
- `snapshot_current`: the latest valid baseline is inside the freshness window;
- `snapshot_stale`: the last valid baseline exceeded the stale threshold;
- `transport_degraded`: the latest attempt failed but the baseline is still fresh;
- `overflowed`: the current snapshot is valid but diagnostic deltas are unsafe;
- `offline`: no current successful observation remains inside the offline window.

These states describe the plugin-local observation channel. They are not the
Control Plane backend lifecycle or backend generation.

---

## Default Timing Contract

```text
snapshot poll interval:       5 seconds
snapshot stale threshold:    15 seconds
offline threshold:           60 seconds
initial reconnect delay:      1 second
maximum reconnect delay:     30 seconds
```

Reconnect delay doubles after consecutive failures:

```text
1, 2, 4, 8, 16, 30, 30, ... seconds
```

A successful snapshot resets the failure counter and returns scheduling to the
normal five-second poll interval.

A missing configuration is not retried in a tight loop. A new configured
service instance is required.

---

## Polling Contract

Initial connection or recovery:

```text
CAPS 1
SNAP
```

Normal trusted polling:

```text
SNAP
```

Any transport, protocol or continuity uncertainty forces the next attempt back
to discovery before another snapshot is accepted.

No optimistic capability fallback exists. The `mutations` state remains false
for every outcome, including a future plugin payload that advertises mutation
availability.

---

## Baseline and Delta Contract

The service retains at most one accepted plugin-local baseline.

A diagnostic delta is available only when:

- both snapshots are active;
- both snapshots use the same non-empty `counter_epoch`;
- neither snapshot reports overflow;
- every counter is monotonic.

Delta fields are:

```text
total
channelSwitch
recording
replaying
timerChange
```

Epoch change:

- replaces the baseline;
- exposes no delta for that transition;
- does not imply a backend generation change.

Overflow:

- retains the valid snapshot;
- enters `overflowed`;
- disables diagnostic deltas.

Counter regression inside one epoch:

- rejects the candidate snapshot;
- retains the last valid baseline;
- exposes no unsigned subtraction;
- enters degraded, stale or offline state according to freshness.

---

## Freshness Contract

The service records bounded steady-clock timestamps for:

- last attempt;
- last successful snapshot;
- last state change;
- next scheduled attempt.

At exactly the stale threshold, the baseline becomes `snapshot_stale`.
At exactly the offline threshold, the observation becomes `offline`.

The last valid baseline remains available for diagnostics while stale or
offline. Its age and state must remain explicit to later consumers.

---

## Worker and Shutdown Contract

The deterministic service owns no thread, mutex, condition variable, socket,
sleep or runtime dependency.

A separate thin worker owns:

- one joinable thread;
- one condition variable;
- interruptible waits;
- idempotent `start()` and `stop()`;
- a thread-safe published snapshot copy.

The worker never detaches. `stop()` wakes a scheduled wait immediately and joins
the thread. If a typed transport operation is already running, its accepted
SB.10b operation deadline remains the upper bound before the worker can finish.

---

## Diagnostic Boundary

Observation diagnostics are bounded to 256 bytes and line breaks are removed.
The lifecycle does not publish:

- raw SVDRP payloads;
- VDR credentials;
- arbitrary command text;
- database data;
- public API responses.

---

## Source Boundary

SB.10c source is restricted to `core/agent/` plus its Make and architecture
contracts.

It has no dependency on:

- `DaemonRuntime`;
- `BackendRuntimeContext`;
- RESTfulAPI;
- SQLite;
- filesystem state;
- the mutation-specific `SvdrpChannelMoveExecutor`;
- plugin source files.

The plugin version, commands, capability catalogue and schema versions remain
unchanged.

---

## Acceptance Requirements

Automated acceptance must prove:

- exact initial `CAPS 1` then `SNAP` ordering;
- later snapshot-only polling;
- rediscovery after failure;
- separate not-configured, missing-plugin and legacy states;
- no snapshot request after incompatible discovery;
- exact bounded reconnect sequence;
- backoff reset after success;
- stale and offline boundaries;
- retention of the last valid baseline;
- epoch replacement;
- overflow delta suppression;
- same-epoch counter-regression rejection;
- hard mutation disablement;
- idempotent worker start and stop;
- interruptible reconnect waiting;
- no surviving worker after stop;
- no daemon, RESTfulAPI, database or plugin coupling;
- strict Make inventory closure.

## Acceptance Result

Status: `completed`

Automated acceptance proved:

- strict Make inventory closure;
- retained SB.10a and SB.10b regressions;
- deterministic polling, reconnect, freshness and delta behavior;
- exact stale and offline thresholds;
- epoch replacement, overflow suppression and counter-regression rejection;
- hard mutation disablement;
- interruptible worker shutdown with no surviving thread;
- complete documentation and architecture checks;
- a clean synchronized worktree.

Controlled live acceptance on VDR `2.7.9`, API version `11`, proved:

- plugin version `0.10.0` loaded as `libvdr-suitebridge.so.11`;
- installed object SHA-256 `a84c4571e951da94de2c0b5f9badf2c74034fe94b0c43483dfa9d9345d513b5d`;
- retained handshake result `status=ready`;
- initial observation command sequence `CAPS,SNAP`;
- trusted follow-up polling through one additional `SNAP`;
- observation state `snapshot_current`;
- live epoch `9587ed0c461a89827c75a26fc56d11c6`;
- live total `4`;
- `counter_overflow=false`;
- clean worker stop;
- unchanged channel, Timer, Recording and `setup.conf` state;
- complete plugin removal, VDR restart and rollback;
- no Suite Bridge object remained mapped;
- a clean synchronized repository.

No plugin source, command, capability, schema or mutation state changed.

---

## Back

- [Back to Architecture Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
