# SuiteBridge Control-Plane Audit

## Status

Repository audit completed against:

```text
main=dcf4af097dee3e7762eb18152bd005d40ca54f3e
PR #350=merged
open PRs=0 at audit start
date=2026-09-26
```

Binding decision: [ADR-0064](../adr/ADR-0064-suitebridge-local-prioritized-control-plane.md).

This document records the implementation truth found by the audit. It is not a
Phase-69 product slice and does not change runtime behavior.

## Question

The audit answers one narrow architecture question:

> Should the internal SuiteBridge/VDR production control plane continue to use
> VDR SVDRP as the universal transport for all native SuiteBridge domains?

Answer: **no**.

SVDRP remains a useful typed compatibility/diagnostic transport, but the current
single VDR SVDRP command-execution path creates avoidable head-of-line coupling
between latency-sensitive control operations and slow provider/native work.
ADR-0064 therefore selects a dedicated private local bounded/prioritized control
endpoint as the migration target.

## Current topology

```text
Control Plane / daemon
        |
        | Agent-owned typed adapters
        v
Backend Agent SuiteBridgeSvdrpTransport
        |
        | one bounded TCP connection per request
        | 127.0.0.1:6419 by default
        v
VDR SVDRP server handler
        |
        | PLUG suitebridge <typed command>
        v
cPluginSuiteBridge::SVDRPCommand()
        |
        +-- native VDR state / mutations
        +-- TVScraper plugin services
        +-- osdteletext plugin services
        +-- vdr-plugin-web HbbTV services
        +-- Live provider state
        +-- Legacy OSD input/state
```

The Agent transport is not itself an unbounded or persistent transport:

| Property | Current value |
| --- | --- |
| default host | `127.0.0.1` |
| default port | `6419` |
| connect timeout | `1000 ms` |
| per-I/O timeout | `1000 ms` |
| total operation timeout | `3000 ms` |
| maximum greeting bytes | `1024` |
| maximum command reply bytes | `131072` |
| maximum reply lines | `64` |
| requests per TCP connection | one |
| transport-owned retries | none |
| transport-owned worker threads | none |

Each request creates a non-blocking close-on-exec socket, validates the VDR
`220` greeting, sends one typed command, parses one bounded reply and closes
the socket.

## VDR serialization boundary

The decisive boundary is inside VDR, not in the Agent socket client.

VDR invokes plugin `SVDRPCommand()` from its dedicated SVDRP server-handler
thread. The server handler maintains the accepted server connections and
processes them from that one handler loop. Plugin command execution is
synchronous with respect to the connection-processing call.

Consequences:

1. separate Agent TCP connections are not independent VDR command workers;
2. while one SuiteBridge command is executing, another accepted SVDRP
   connection may wait for the handler to return;
3. a three-second provider call can therefore consume the entire useful timeout
   budget of another one-second I/O phase before that second command itself is
   serviced;
4. increasing the number of client sockets does not fix the execution
   serialization;
5. a priority queue only in the Backend Agent cannot preempt a command already
   running in VDR's SVDRP handler.

This is the structural coupling exposed by the incident preceding PR #350.

## Complete SuiteBridge operation inventory

The following inventory is derived from the live plugin dispatcher, Agent
transport implementations and command-specific services at the audit baseline.

### Capability, status and native probe

| Wire operation | Purpose | Agent production use | Execution character |
| --- | --- | --- | --- |
| `CAPS [1]` | SuiteBridge capability discovery | yes | short immutable/read-only |
| `SNAP` | local status/counter snapshot | yes | short atomic/read-only |
| `NCAP 1` | typed native probe capability | yes | short read-only |
| `NPROBE EXEC ...` | fenced native probe execution | yes | typed native operation |
| `NPROBE READ ...` | probe readback | yes | short readback |

`CAPS` and `SNAP` are advertised through SVDRP help. Native-probe commands
are private typed Agent/plugin contracts.

### Live provider

| Wire operation | Purpose | Latency sensitivity |
| --- | --- | --- |
| `NLCAP 1` | Live provider capability/current provider facts | critical |
| `NLIVE OPEN 1 ...` | open one fenced Live provider lease | critical |
| `NLIVE STATUS 1 ...` | read current Live lease/provider state | critical |
| `NLIVE CLOSE 1 ...` | close one Live provider lease | critical |

PR #350 changed the consumer semantics only: transport failure while checking an
already-running session is now indeterminate rather than authoritative terminal
evidence. These commands still traverse the shared SVDRP handler at this
baseline.

### Legacy OSD

| Wire operation | Purpose | Native boundary |
| --- | --- | --- |
| `OSDSNAP` | bounded semantic OSD frame | monitor snapshot |
| `OSDINPUT 1 ...` | fenced allowlisted input | ends at `cRemote::Put()` |

OSD input is not a generic key/SVDRP tunnel. Existing surface identity, OSD
epoch, controller lease, revision and deadline fences remain part of the
operation contract.

### Teletext

| Wire operation | Purpose | Provider boundary |
| --- | --- | --- |
| `TTXC 1` | Teletext provider capabilities | `cPluginManager::CallFirstService` |
| `TTXP 1 ...` | one page/subpage read | osdteletext service call |

These are interactive provider calls and therefore should not share an
execution resource with slow background metadata once the dedicated plane is
implemented.

The later Teletext implementation audit of the pinned
`vdr-plugin-osdteletext` provider contract proved that page reads copy one
bounded snapshot under the provider mutex, release that mutex, then render and
normalize a fixed 25x40 cell matrix locally. No network/filesystem I/O or
borrowed VDR-native pointer crosses the service call. Teletext therefore shares
the serial External Plugin Interactive lane with HbbTV rather than adding a
separate worker.

### HbbTV

| Wire operation | Purpose | Provider boundary |
| --- | --- | --- |
| `HBBAPPS 1 <channel>` | discover applications | vdr-plugin-web service |
| `HBBRUN LAUNCH/STATUS/INPUT/CLOSE 1 ...` | runtime control | vdr-plugin-web service |
| `HBBPRES META 1 ...` | presentation metadata | vdr-plugin-web service |
| `HBBPRES CHUNK 1 ...` | bounded presentation chunk | vdr-plugin-web service |
| `HBBMEDIA 1 ...` | media source/read data | vdr-plugin-web service |

The adapter invokes the provider synchronously through VDR's plugin Service
mechanism. The new transport must not assume that arbitrary concurrent Service
calls are safe without provider-specific proof.

The later HbbTV implementation audit proved a narrower execution contract:
the provider's discovery and media reads are mutex-protected bounded state
reads; runtime state is serialized and UI effects retain the provider's
existing VDR remote/main-context scheduling; presentation can perform
BGRA-to-RGBA conversion and QOI encoding while holding its presentation mutex.
Consequently HbbTV is assigned one dedicated serial External Plugin Interactive
worker. This isolates HbbTV provider latency from Critical and Legacy OSD
without enabling concurrent calls into `vdr-plugin-web`.

### Native Timer mutation families

| Wire operation | Purpose | Native synchronization |
| --- | --- | --- |
| `NTCREATE CAP 1` | CREATE capability | short discovery |
| `NTCREATE EXEC ...` | create managed native Timer | `cTimers::GetTimersWrite(..., 1000)` |
| `NTDEL CAP 1` | DELETE capability | short discovery |
| `NTDEL EXEC ...` | delete exact managed Timer | `cTimers::GetTimersWrite(..., 1000)` |
| `NTMOD CAP 1 <kind>` | MODIFY capability | short discovery |
| `NTMOD EXEC ...` | modify exact managed Timer | `cTimers::GetTimersWrite(..., 1000)` |

All three mutation families already have operation identity/fingerprint,
backend-generation and provider/capability fencing at the Agent/plugin
boundary. They return explicit stale/rejected/applied-unverified/outcome-unknown
style results.

The VDR mutation callback itself has a separate bounded Timer write-lock
acquisition. A dedicated control-plane worker must preserve that boundary rather
than replacing it with an external mutex or unbounded wait.

### Recording marks and cut

| Wire operation | Purpose | Native/filesystem boundary |
| --- | --- | --- |
| `RMARKS <recording-key>` | canonical native marks read | Recording identity/read state |
| `RCUT <recording-key>` | cut preconditions/readback state | Recording read lock + marks/cutter state |
| `NMARKS CAP 2 modify` | marks-mutation capability | short discovery |
| `NMARKS EXEC ...` | add/delete/move/reset/replace marks | Recording read lock + marks/index/filesystem |
| `NCUT CAP 1 start` | cutter capability | short discovery |
| `NCUT EXEC ...` | enqueue one fenced cut | Recording read lock + `RecordingsHandler.Add(ruCut,...)` |

Marks mutation may load/save the marks file, inspect the recording index and
emit `cStatus::MsgMarksModified` while operating on a recording resolved under
the VDR Recording boundary. Cut admission performs repeated precondition
inspection and then queues the VDR cutter operation.

These are typed native mutations but not appropriate work for a generic
unbounded parallel pool.

### EPG/TVScraper provider reads

| Wire operation | Purpose | Important boundary |
| --- | --- | --- |
| `ARTW <channel> <event>` | preferred artwork | detached event then TVScraper/image work |
| `META <channel> <event>` | EPG metadata | detached event then TVScraper/serialization |
| `MCOMPARE <channel> <event>` | Live-vs-detached diagnostic comparison | diagnostic synchronous provider work |
| `ETYPES <from> <until> <offset> <limit>` | bounded type page | Channels/Schedules locks + real-event TVScraper classification |

`ARTW` and `META` deliberately copy/detach the selected event while holding
the schedule read lock and release the lock before TVScraper/filesystem/JSON
work.

`ETYPES` is different: it re-resolves real locked VDR events and calls the
type adapter on those events. It is bounded by time window/page size, but one
provider lookup can still be slower than a native status read.

`MCOMPARE` is an advertised diagnostic command. No normal Agent production
transport caller was found during this audit; it should remain diagnostic unless
a future slice explicitly assigns product ownership.

### Recording TVScraper metadata

| Wire operation | Purpose | Important boundary |
| --- | --- | --- |
| `RMETA <recording-key>` | Recording metadata/people/artwork evidence | `LOCK_RECORDINGS_READ` + synchronous TVScraper service |

The Recording path cannot currently detach the object before the provider call.
The code explicitly keeps the borrowed `cRecording*` only while the VDR
Recording-list read lock is held and synchronously calls TVScraper with that
pointer.

This is the highest-value example of why transport isolation must not be
implemented by simply copying existing handlers into arbitrary parallel worker
threads. The native pointer lifetime contract must stay correct even when
`RMETA` is placed in a low-priority/provider execution lane.

## Dispatcher inventory

At the baseline, `cPluginSuiteBridge::SVDRPCommand()` attempts handlers in this
order:

```text
OSDINPUT
native probe
Live capability
Live source
Teletext
HbbTV
native Timer CREATE
native Timer DELETE
native Timer MODIFY
Recording marks MODIFY
Recording cut
CAPS
OSDSNAP
ARTW
META
MCOMPARE
ETYPES
RMETA
RMARKS
RCUT
SNAP
```

This order controls command recognition only. It is **not** priority scheduling:
once VDR calls the dispatcher for one command, that handler runs synchronously
until it returns.

## Current timeout interaction

The Agent transport has one total operation deadline of 3000 ms and 1000 ms
per connect/I/O phase.

A command that reaches `SVDRPCommand()` may continue executing inside VDR even
after a different client has exhausted its own waiting deadline. The transport
deadline bounds the Agent's wait; it is not a cancellation mechanism for a
plugin command already executing.

For read operations this causes stale/unavailable observations.

For mutation operations it is more important: loss of the reply after possible
native execution must remain `outcome_unknown`; timeout is never evidence that
the mutation did not occur.

## Thread and lock conclusions

### Proven safe to state

- SuiteBridge `SVDRPCommand()` is not VDR's main thread; it is reached through
  the SVDRP server-handler thread.
- Existing plugin callbacks such as status observation remain separately bounded
  and non-blocking; the control-plane redesign must not move network waits into
  those callbacks.
- Timer mutations already use VDR's state-key/write-lock API with a finite
  acquisition timeout.
- Recording operations use explicit VDR Recording locking/native object
  lifetimes.
- provider adapters synchronously invoke other VDR plugins.

### Not proven and therefore not assumed

- that all VDR plugin `Service()` implementations are re-entrant;
- that TVScraper can safely serve concurrent `GetScraperVideo` requests from
  arbitrary new worker threads;
- that Teletext/HbbTV providers may be called concurrently with themselves;
- that every existing SuiteBridge command is safe on VDR's main thread;
- that every existing command is safe on any arbitrary worker thread;
- that a socket/client timeout can cancel a VDR-side operation.

These unknowns directly shape ADR-0064's execution-lane requirement.

## Failure coupling demonstrated by PR #350 incident

The observed chain was:

```text
slow RMETA / TVScraper
  -> SuiteBridge SVDRPCommand remains busy
  -> common VDR SVDRP handler cannot service another SuiteBridge connection
  -> short Agent SuiteBridge request hits transport timeout
  -> Live liveness check lacks fresh evidence
```

Before PR #350 the final condition could be interpreted too aggressively by the
Live session reaper. PR #350 corrected that interpretation.

The remaining architectural problem is the common serialized execution path.
Fixing the consumer's failure semantics is necessary but does not supply
latency isolation.

## Required new control-plane properties

The migration target in ADR-0064 must provide all of the following:

| Property | Requirement |
| --- | --- |
| locality | Unix-domain/local host only; never public |
| request surface | closed typed operation registry |
| frame size | hard bounded |
| queues | finite per service class/lane |
| priority | server-owned, operation-derived |
| critical capacity | reserved from metadata/background work |
| fairness | no permanent starvation |
| deadlines | checked at admission and before execution |
| overload | typed explicit rejection |
| mutations | no blind retry after possible effect |
| provider calls | lane/thread-safety decision per provider |
| VDR locks | existing native lock APIs remain authoritative |
| callbacks | remain bounded/non-blocking |
| shutdown | deterministic; no orphan accepted work |
| fallback | staged SVDRP compatibility path |
| observability | queue wait and execution latency by operation family |

## Recommended execution decomposition

The architecture decision deliberately distinguishes queue priority from
threading.

A suitable implementation decomposition is:

```text
AF_UNIX endpoint
   |
   +-- bounded admission + protocol validation
   |
   +-- critical native-control lane
   |     Live / native probe / current OSD dispatch
   |
   +-- interactive control lane
   |     OSD snapshot/input
   |
   +-- external-plugin interactive lane
   |     HbbTV (serial provider Service calls)
   |     Teletext (same serial provider lane after audit)
   |
   +-- native-mutation lane
   |     Timer / marks / cut, serialized unless proven otherwise
   |
   +-- background metadata/provider lane
         RMETA / META / ARTW / ETYPES / diagnostics
```

The exact number of threads and queue depths is intentionally deferred to the
implementation slice because those values require contention tests on supported
VDR/provider builds.

The invariants are not deferred:

- background work must not consume the critical lane;
- mutation safety must not depend on arbitrary worker concurrency;
- provider re-entrancy must be proven before enabling same-provider concurrent
  calls;
- a request that expires in a queue must not later execute as a normal read.

## Migration map

### Stage 0 — current state

All listed operations use typed SuiteBridge/SVDRP.

### Stage 1 — local control foundation

Add:

- versioned local wire protocol;
- local peer validation;
- bounded admission queues;
- deadline/result model;
- lifecycle/shutdown behavior;
- local capability negotiation;
- metrics.

Keep runtime operations on SVDRP until the foundation is accepted.

### Stage 2 — critical Live/control isolation

Migrate:

- `NLCAP`;
- `NLIVE OPEN/STATUS/CLOSE`;
- native probe/readback required by current control authority.

Acceptance must include a deliberately blocked/slow background provider request
while Live status continues to complete through the new lane.

### Stage 3 — interactive surfaces

Migrate OSD, Teletext and HbbTV after provider/thread boundaries are proven.

### Stage 4 — mutations

Migrate Timer and Recording mutations without altering their durable
Control-Plane/Agent operation semantics.

A mutation transport switch is selected before execution. Unknown outcome never
triggers cross-transport replay.

### Stage 5 — metadata/background

Migrate TVScraper metadata/artwork/type work last and use it as the primary
backpressure/overload stress workload.

### Stage 6 — SVDRP production retirement review

After all production callers have migrated and rollback evidence exists, decide
whether each legacy SVDRP command remains operator diagnostics or can be removed.
That is a separate compatibility decision.

## Phase-69 relation

Phase 69 remains **Public API and Client Compatibility Hardening**.

This audit:

- does not revert or rewrite any accepted 69.C work;
- does not create a public route;
- does not change public ETag/precondition/idempotency semantics;
- does not implement the next Phase-69 mutation;
- establishes an internal prerequisite decision before more productive mutation
  surface is added.

The Phase-69 implementation may resume from its accepted state after the
relevant internal control-plane prerequisite is explicitly closed.

## Non-goals

This audit does not propose:

- a VDR fork;
- a second public API;
- replacing RESTfulAPI;
- a raw RPC tunnel;
- a plugin database;
- plugin-owned durable jobs/retries;
- generic parallel execution of all VDR operations;
- undoing PR #350;
- increasing timeouts as the architectural fix.

## Evidence locations

Primary current repository evidence:

- `core/agent/include/SuiteBridgeSvdrpTransport.h`
- `core/agent/src/SuiteBridgeSvdrpTransport.cpp`
- `core/agent/src/SuiteBridgeSvdrp*Transport.cpp`
- `vdr-plugin-suite-bridge/suitebridge_svdrp.cpp`
- `vdr-plugin-suite-bridge/suitebridge_epg_command_handler.cpp`
- `vdr-plugin-suite-bridge/suitebridge_epg_type_snapshot_command.cpp`
- `vdr-plugin-suite-bridge/suitebridge_recording_metadata_command.cpp`
- `vdr-plugin-suite-bridge/suitebridge_native_timer_*_vdr.cpp`
- `vdr-plugin-suite-bridge/suitebridge_recording_marks_modify_vdr.cpp`
- `vdr-plugin-suite-bridge/suitebridge_recording_cut_vdr.cpp`
- `vdr-plugin-suite-bridge/suitebridge_teletext_adapter.cpp`
- `vdr-plugin-suite-bridge/suitebridge_hbbtv_adapter.cpp`
- `vdr-plugin-suite-bridge/docs/ADR-0001-plugin-role-and-native-integration-strategy.md`
- `docs/architecture/suite-bridge-svdrp-transport.md`
- `docs/adr/ADR-0039-backend-agent-control-plane-boundary.md`
- `docs/adr/ADR-0042-safe-mutation-revision-idempotency-contract.md`
- `docs/adr/ADR-0063-mutation-complexity-proportionality-reuse.md`

External VDR source documentation was used only to confirm the VDR
SVDRP-server-handler threading/connection-processing model. The repository's own
runtime evidence remains the authority for VDR-Suite behavior.
