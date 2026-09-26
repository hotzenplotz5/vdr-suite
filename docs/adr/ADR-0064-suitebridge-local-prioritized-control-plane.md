# ADR-0064: SuiteBridge Local Prioritized Control Plane

## Navigation

- [ADR Index](index.md)
- [SuiteBridge Control-Plane Audit](../architecture/suitebridge-control-plane-audit.md)
- [Suite Bridge Local SVDRP Transport](../architecture/suite-bridge-svdrp-transport.md)
- [Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [Safe Mutation Contract](ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [Mutation Complexity and Reuse](ADR-0063-mutation-complexity-proportionality-reuse.md)

---

## Status

Accepted architecture decision.

Date: 2026-09-26

Implementation status: **Foundation + initial Critical Control slices + Legacy OSD Interactive slice implemented; later operation families remain staged.**

Implementation record:
[SuiteBridge local control-plane implementation](../architecture/suitebridge-local-control-plane-implementation.md).

This decision is a prerequisite architecture gate before the next productive
Phase-69 mutation slice. It does not reopen accepted Phase-69 work and does not
change the public API.

## Context

VDR-Suite currently reaches most SuiteBridge operations through VDR's local
SVDRP server. The Backend Agent opens one bounded TCP transaction for each typed
request and the SuiteBridge plugin handles the command synchronously through
`cPlugin::SVDRPCommand()`.

The transport itself is bounded: connect, I/O and total operation deadlines are
enforced, replies are size/line bounded and no arbitrary command tunnel exists.
Those properties remain valuable.

The architectural problem is below that transport boundary.

VDR executes plugin SVDRP commands from its dedicated SVDRP server-handler
thread. That handler services the accepted SVDRP connections serially. A slow
plugin command therefore occupies the common VDR SVDRP command-execution path
until the plugin returns. Independent TCP connections do not provide independent
plugin-command execution.

SuiteBridge has grown from a small read-only diagnostic surface into a private
native integration plane containing:

- capability and status discovery;
- Live provider discovery, open/close and status;
- Legacy OSD observation and input;
- Teletext reads;
- HbbTV discovery, runtime control, presentation and media reads;
- native Timer create/delete/modify operations;
- Recording marks reads and modifications;
- Recording cut state and cut admission;
- TVScraper EPG/Recording metadata, artwork and type classification;
- native probe and mutation readback/fencing.

Those operations do not have equal latency or execution characteristics.

A concrete production incident exposed the consequence: a slow synchronous
`RMETA` / TVScraper request occupied the SVDRP handler for about three seconds.
Other local SuiteBridge requests then waited behind it and some exhausted their
own short transport deadlines. PR #350 correctly made an already-running Live
session treat such transport uncertainty as indeterminate rather than terminal,
but it deliberately did not remove the shared head-of-line blocking.

The repository audit also shows that different command families require
different native safety boundaries:

- native Timer mutations acquire VDR's Timer write lock with a bounded
  1000-millisecond acquisition timeout;
- Recording marks/cut operations use the VDR Recording read boundary and may
  perform filesystem/index/cutter work;
- EPG metadata/artwork can detach an event before TVScraper work, while some
  diagnostic/type paths still perform synchronous provider work around native
  VDR state;
- Recording metadata must consume the real `cRecording*` synchronously while
  the Recording read lock is held because the TVScraper service contract takes
  that native object;
- Teletext and HbbTV invoke other VDR plugins synchronously through service
  calls;
- Legacy OSD input terminates at VDR's native remote-input primitive.

The solution therefore cannot be “run every command in parallel”. Transport
isolation and VDR execution correctness are separate concerns.

## Decision

### 1. SVDRP is no longer the target universal production transport

VDR-Suite will retain the existing typed SuiteBridge/SVDRP path for:

- compatibility with older SuiteBridge deployments;
- controlled diagnostics and operator inspection;
- staged rollout and rollback;
- commands that have not yet migrated.

SVDRP is not the target transport for the complete production SuiteBridge
control plane.

New production hardening must not add another latency-sensitive SuiteBridge
domain to the common SVDRP path merely because the path already exists.

### 2. Add one private local SuiteBridge control endpoint

The SuiteBridge plugin will expose one **local-only** control endpoint for the
Backend Agent.

The preferred Linux transport is an `AF_UNIX` message-oriented socket. The
implementation slice must evaluate `SOCK_SEQPACKET` first because message
boundaries, bounded frames and deterministic one-request/one-reply semantics
match the control-plane contract. If the supported VDR deployment matrix proves
that `SOCK_SEQPACKET` is unsuitable, `SOCK_STREAM` may be used only with an
equally explicit bounded frame protocol.

This is not a public server and does not change the client path:

```text
client
  -> VDR-Suite public API
  -> Control Plane
  -> authenticated Backend Agent
  -> private local SuiteBridge control endpoint
  -> typed SuiteBridge operation
  -> VDR / provider boundary
```

The endpoint:

- binds only in a private runtime directory;
- has no TCP/IP listener;
- is never exposed outside the backend host;
- is protected by filesystem ownership/mode and deployment-specific Agent
  access;
- verifies local peer credentials where the platform provides `SO_PEERCRED`;
- carries no user authentication or public authorization model;
- accepts only protocol-defined typed operations.

This narrowly refines the older “no plugin-owned listener” rule in the
SuiteBridge plugin strategy: general/public listeners remain prohibited, while
one private local bounded Agent endpoint is now an accepted plugin
responsibility.

### 3. The wire protocol is typed, versioned and bounded

Every request envelope has, at minimum:

- protocol major/version;
- operation identifier from a closed registry;
- request/correlation identity;
- absolute monotonic deadline or equivalent bounded remaining budget;
- bounded operation payload.

Every response has, at minimum:

- matching request identity;
- typed transport/execution result;
- operation-specific reply code/disposition;
- bounded payload/evidence.

The protocol has hard maximum frame sizes and rejects malformed, unknown,
oversized or expired requests before native execution.

There is no equivalent of:

```text
runSvdrp(text)
callPlugin(name, payload)
executeNative(command)
```

Priority is assigned from the server-side operation registry. A caller cannot
self-promote a background metadata operation to the critical class.

### 4. Admission is bounded and priority-aware

The endpoint uses finite queues with reserved capacity for latency-sensitive
control work. Exact queue depths are implementation constants selected and
tested in the implementation slice; this ADR deliberately does not invent
untested production numbers.

The required service classes are:

#### Critical control

Operations whose delay can invalidate or destabilize an already-active
time-sensitive session or controller.

Initial members:

- Live provider capability/status/open/close;
- native probe/readback needed for current ownership/fencing;
- current Legacy OSD control dispatch when a valid controller lease exists.

Critical capacity cannot be consumed by metadata prefetch or bulk/background
work.

#### Interactive control/read

User-visible request/response work where bounded latency matters but temporary
delay is not itself session-fatal.

Initial members include:

- Legacy OSD snapshot;
- Teletext page/service reads;
- HbbTV discovery, status/control and presentation metadata;
- Recording marks/cut-state reads;
- capability/status discovery used by interactive flows.

#### Native mutation

Typed Timer, Recording-mark and Recording-cut mutations remain a distinct
execution class even if their user-visible priority is interactive.

They preserve all existing operation identity, generation, revision,
idempotency, replay and unknown-outcome rules. Queueing never authorizes a
mutation.

#### Background/provider

Potentially slow enrichment and bulk/provider work, including:

- `RMETA`;
- `META`;
- `ARTW`;
- `ETYPES`;
- comparison/diagnostic metadata work.

Background/provider work cannot consume reserved critical capacity.

Scheduling must be starvation-safe. Strict permanent priority starvation is not
accepted; reserved critical capacity plus bounded fair service is preferred.

### 5. Transport concurrency does not imply native execution concurrency

The implementation separates **admission/scheduling** from **execution lanes**.

At minimum the design must isolate:

1. short native control work;
2. side-effecting native mutation work;
3. potentially blocking external-plugin/provider work.

A slow TVScraper call must not occupy the execution resource reserved for Live
liveness/control.

Mutation execution is serialized unless a later operation-specific proof
demonstrates safe parallelism. Existing VDR locks remain authoritative.

External VDR-plugin service calls are not assumed thread-safe merely because
the new transport can accept requests concurrently. Each provider family must
have an explicit execution-lane/thread-safety decision before parallel service
calls are enabled.

If a VDR operation requires main-thread execution, it must use a bounded
main-thread handoff with deadline/staleness checks. No implementation may infer
main-thread safety or worker-thread safety from this ADR alone.

### 6. Deadlines are enforced before and after queueing

A request may wait only within its bounded deadline.

The server checks the deadline:

- at admission;
- before dequeue/native execution;
- before any optional main-thread handoff.

Expired read-only requests are rejected without execution.

For a mutation, a caller-side timeout after native execution has begun is an
**unknown outcome**, not permission to redispatch. ADR-0042/0043 semantics
remain authoritative.

The implementation must expose distinct local result categories for at least:

- success;
- typed native rejection;
- stale/fence rejection;
- queue full / overloaded;
- deadline expired before execution;
- provider unavailable;
- transport failure;
- outcome unknown after possible side effect.

### 7. Backpressure is explicit, not hidden in socket timeouts

Queue saturation returns a bounded typed overload result. The server does not:

- allocate an unbounded request queue;
- block the accept/read loop waiting for queue space;
- silently drop accepted mutations;
- let background metadata consume all workers;
- convert overload into an authoritative “resource absent/terminal” fact.

PR #350's Live liveness rule remains valid after migration: inability to obtain
fresh control-plane evidence is indeterminate unless an authoritative terminal
condition is observed.

### 8. Existing VDR locking and provider contracts are preserved

The new endpoint changes transport scheduling, not native authority.

Examples:

- Timer CREATE/DELETE/MODIFY keep their bounded VDR Timer write-lock and
  authoritative readback/reconciliation rules.
- Recording marks and cut keep Recording identity, in-use, revision and cutter
  preconditions.
- Recording metadata keeps the native `cRecording*` lifetime/lock contract
  until TVScraper provides a safer detached identity contract.
- detached EPG metadata paths continue releasing schedule locks before slow
  provider/filesystem work where already implemented.
- Teletext/HbbTV provider calls remain behind their typed adapters.

Long-running work must still be represented as Suite-owned operations/jobs
where appropriate. The plugin does not become a workflow engine.

### 9. Migrate incrementally with one selected transport per operation

Migration is capability-driven.

For each migrated operation family:

1. the Agent discovers the dedicated local-control capability and protocol;
2. the Agent selects exactly one local transport for that request;
3. successful dedicated-control support is preferred;
4. legacy SVDRP remains a pre-dispatch fallback only while compatibility is
   required;
5. a mutation is never retried through SVDRP after the dedicated transport
   returns an unknown outcome.

SVDRP commands may remain available for diagnostics/rollback during migration.
Their continued existence is not evidence that production code may issue both
paths concurrently.

### 10. No Phase-69 public-contract change is implied

This is internal infrastructure hardening.

It does not:

- add or remove a public `/api/v1` resource;
- change accepted Phase-69.C resource semantics;
- alter actor authorization;
- change Timer/Recording/MediaSession ownership;
- alter public error or idempotency contracts.

The next productive Phase-69 mutation slice may resume only after the
dedicated-control foundation required by the operation has either been
implemented and accepted or explicitly shown unnecessary for that operation.

## Operation migration order

The implementation should migrate by risk reduction, not by source-file order.

### Foundation

- protocol framing/version negotiation;
- peer validation;
- bounded queues/deadlines;
- typed capability/status request;
- deterministic shutdown and rollback;
- metrics for queue depth, rejection, queue wait and execution duration.

### Critical control first

- Live provider discovery/status/open/close;
- native probe/current fencing reads required by Live/control paths.

This is the path that directly removes the failure coupling exposed before
PR #350.

### Interactive control next

- Legacy OSD control/snapshot;
- HbbTV control/status/presentation metadata;
- Teletext reads.

### Native mutation families

- Timer create/delete/modify;
- marks modify;
- cut admission.

These migrate only with existing mutation safety and unknown-outcome semantics
unchanged.

### Background/provider last

- Recording/EPG TVScraper metadata;
- artwork;
- EPG type bulk snapshot;
- diagnostic comparison.

Moving these last is intentional: the architectural benefit is already obtained
once they can no longer block critical control, and their provider/thread
contracts need the most careful lane analysis.

## Observability requirements

The local control plane must make contention diagnosable without logging
sensitive payloads.

Required bounded metrics/facts include:

- admitted/rejected count by operation family/service class;
- queue depth/high-water mark by class;
- queue wait duration;
- native execution duration;
- deadline-expired-before-execution count;
- overload count;
- provider/transport failure count;
- active worker/lane state;
- protocol/schema mismatch count.

Logs identify operation family, request identity and bounded reason code, not
raw metadata bodies, OSD contents or credentials.

## Alternatives considered

### Keep universal SVDRP and only increase timeouts

Rejected. Larger client timeouts do not remove the single-handler head-of-line
blocking and merely increase how long latency-sensitive work can wait.

### Open multiple SVDRP TCP connections

Rejected as an architectural fix. The current Agent already creates independent
connections, while VDR's server handler still executes their commands through
the shared serial handler.

### Put a priority queue only in the Backend Agent

Rejected as insufficient. It can order requests before they reach VDR but
cannot preempt or isolate a slow command already executing in VDR's SVDRP
handler, nor protect unrelated local SVDRP users.

### Run every SuiteBridge command in a generic worker pool

Rejected. VDR locks, native pointer lifetimes, side effects and third-party
plugin Service contracts have operation-specific thread-safety requirements.

### Replace SuiteBridge with RESTfulAPI

Rejected. ADR-0001 remains valid: SuiteBridge owns native lifecycle/readback and
typed gaps that broad HTTP polling cannot truthfully cover.

### Add a generic local RPC/plugin-service gateway

Rejected. It would recreate the prohibited unbounded command tunnel under a new
transport.

## Consequences

Positive:

- slow metadata/provider work no longer has to block Live control;
- overload becomes explicit instead of manifesting mainly as unrelated SVDRP
  timeouts;
- latency classes and queue capacity become testable architecture;
- VDR lock/mutation correctness remains explicit;
- migration can be incremental and reversible;
- public API contracts remain unchanged.

Costs:

- the plugin gains a small local listener, scheduler and bounded worker/lane
  lifecycle;
- shutdown ordering and socket permissions become packaging/runtime concerns;
- thread-safety of provider service calls must be audited rather than assumed;
- two local transports coexist during migration;
- implementation needs contention and overload acceptance tests, not only
  happy-path command tests.

## Acceptance gates for implementation

A runtime implementation is not accepted until it proves:

1. the endpoint is local-only and inaccessible as a TCP/public listener;
2. peer/file permissions reject unauthorized local callers;
3. malformed/oversized/unknown frames are rejected without native execution;
4. all queues are finite and overload is deterministic;
5. a deliberately blocked background/provider request does not prevent a
   critical Live status request from completing inside its tested budget;
6. deadline-expired queued reads never execute;
7. mutations preserve existing replay/fence/unknown-outcome contracts;
8. native Timer write-lock timeout remains bounded;
9. Recording pointer/lock lifetime contracts remain valid;
10. provider service concurrency matches explicit provider-specific proof;
11. clean plugin stop closes the endpoint and drains/cancels work
    deterministically;
12. SVDRP fallback/rollback remains available during staged migration;
13. no public API route or representation changes;
14. repository architecture, docs, packaging and fast regression checks pass;
15. controlled real-yaVDR acceptance proves the contention scenario that
    motivated this decision.

## Related

- [SuiteBridge Control-Plane Audit](../architecture/suitebridge-control-plane-audit.md)
- [Suite Bridge Local SVDRP Transport](../architecture/suite-bridge-svdrp-transport.md)
- [ADR-0039 Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040 Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0042 Safe Mutation, Revision and Idempotency](ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0043 Job Claim, Retry and Saga Execution](ADR-0043-job-claim-retry-saga-execution-model.md)
- [ADR-0063 Mutation Complexity Proportionality and Reuse](ADR-0063-mutation-complexity-proportionality-reuse.md)
