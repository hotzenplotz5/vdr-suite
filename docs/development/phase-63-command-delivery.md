# Phase 63 Slice 3 — Durable Agent Command Delivery Contract

## Status

**Binding contract for the next bounded Phase-63 slice.**

This is a contract-only slice created after the read-only Observation and
Snapshot Ingestion foundation was implemented, accepted on the real yaVDR host
and merged for both `backend-health` and `channels`.

Merged foundation at contract creation:

```text
main commit: 39ed86fc3a425697f738f8f555394d54e4e1a684
main tree: e03bb84951cef7ec5f6b2f338ba456116cd766a2
completed lifecycle slice: Phase 63 Slice 1
completed read-only ingestion slice: Phase 63 Slice 2
next bounded slice: durable Agent command delivery
```

This document adds no command route, queue, database schema, Agent inbox,
result outbox, native executor, provider selection, packaging change or
installed runtime effect. A separate bounded Draft runtime PR is required.

## Why this slice is next

Phase 63 cannot move directly from read-only observations to TimerIntent or
another VDR mutation. The accepted architecture requires this order:

```text
Agent lifecycle
  -> complete read-only observation continuity
  -> durable command inbox and result outbox
  -> fenced native operations
  -> explicit local provider ownership and selection
  -> protected writes
  -> Phase 64 TimerIntent orchestration
```

Slice 2 now proves authenticated generation-/instance-fenced state publication,
complete baselines, exact-next changes, idempotent replay, explicit resync and
restart-safe persistence. The next missing boundary is durable command delivery
that can survive duplicate transport, process restart and lost responses without
speculative redispatch.

## Goal

Define one durable Control-Plane-to-Agent command transport that:

- keeps the Control Plane authoritative for operations, jobs and scheduling;
- binds every assignment to one current Backend, Agent, Agent instance, backend
  generation, job attempt and claim epoch;
- makes `commandId` and the normalized request fingerprint stable across
  transport retries;
- durably records Agent receipt before acknowledging acceptance;
- preserves an explicit local execution boundary before any future native side
  effect;
- persists bounded results for reconnect and equivalent replay;
- rejects conflicting duplicates, stale generations, stale attempts, expired
  authority and unsupported payload versions;
- treats uncertain dispatch as `outcome_unknown` and requires reconciliation
  before any redispatch;
- exposes no public Agent or provider endpoint and executes no VDR mutation in
  this contract slice.

## Governing contracts

This slice narrows and composes the already accepted contracts:

- [ADR-0042 — Safe Mutation, Revision and Idempotency Contract](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md);
- [ADR-0043 — Job Claim, Retry and Saga Execution Model](../adr/ADR-0043-job-claim-retry-saga-execution-model.md);
- [ADR-0040 — Backend Lifecycle, Generation, Lease and Health](../adr/ADR-0040-backend-lifecycle-generation-lease-health.md);
- [ADR-0041 — Authentication, Agent Trust and Multi-Site Transport](../adr/ADR-0041-authentication-agent-trust-multi-site-transport.md);
- [ADR-0049 — Audit and Security Event Model](../adr/ADR-0049-audit-security-event-model.md);
- [Phase 63 Slice 2 — Read-only Observation and Snapshot Ingestion Foundation](phase-63-observation-ingestion.md).

The identities `operationId`, `jobId`, `attemptId`, `commandId`,
`backendGeneration`, `agentInstanceId` and `claimEpoch` are distinct and must
never be collapsed into one transport sequence or database row identity.

## Authority boundary

The Control Plane owns:

- caller intent and `operationId`;
- authorization, read-only policy and capability decisions;
- idempotency scope and normalized request fingerprint;
- durable job, attempt, claim and retry state;
- assignment eligibility and deadline policy;
- final operation interpretation and reconciliation;
- accountability before remote dispatch and after every outcome.

The Backend Agent owns only bounded site-local transport and execution evidence:

- durable receipt of an assigned command;
- exact command fingerprint and payload version;
- local execution-boundary evidence;
- bounded receipt and result persistence for reconnect;
- one explicitly supported local command adapter;
- local verification facts where the command-specific contract permits them.

The Agent is not a second global scheduler. It does not invent caller
operations, claim unrelated jobs, choose a newer backend generation, extend a
deadline, bypass read-only policy or decide cross-site retry policy.

VDR remains authoritative for VDR-native state and execution.

## Transport direction

The Agent remains outbound-only toward the Control Plane.

The later runtime may use authenticated bounded polling, heartbeat-coupled
assignment or another explicitly documented outbound HTTPS exchange, but it
must not require an inbound public listener on the Agent.

Requirements:

- existing `vdr-suite-agent/1` technical authentication only;
- HTTPS verification remains mandatory;
- redirects, proxy inheritance and netrc remain disabled;
- browser sessions, CSRF credentials and user credentials are rejected;
- requests and responses are independently bounded;
- no private provider URL, credential or local secret path is returned;
- normal diagnostics do not print payloads, authorization headers or tokens;
- no public Agent/provider endpoint is introduced.

Exact route serialization is implementation-owned, but Agent-only command,
receipt and result routes must remain under the protected Agent protocol
boundary and cannot become public client API.

## Command envelope

A remotely assigned command carries at least:

```text
protocolVersion
requestId
correlationId
operationId when applicable
jobId
attemptId
claimEpoch
commandId
backendId
agentId
agentInstanceId
backendGeneration
commandType
payloadVersion
payload
requestFingerprint
resourceType when applicable
resourceId when applicable
expectedRevision when applicable
verificationPolicy
assignedAt
deadline
```

Rules:

- `protocolVersion` is exactly `vdr-suite-agent/1` for this slice;
- `commandId` is stable for one logical Agent assignment;
- `requestFingerprint` is derived from the complete normalized command envelope
  excluding transport timestamps and formatting;
- payloads contain values only, never executable code, raw VDR pointers, locks,
  transport sessions or credentials;
- every string, object, array, nesting depth, item count and total body size is
  bounded before durable storage;
- unknown fields, duplicate object keys, invalid UTF-8, numeric overflow and
  unsupported `payloadVersion` fail closed;
- `claimToken` is not copied into the Agent command payload;
- one command cannot silently change Backend, resource, operation, attempt,
  claim epoch or intended action on retry.

## Capability and command-type boundary

Read-only `adapters` and `observationDomains` do not authorize command
execution.

The later runtime must add a separately bounded declaration of supported
`commandType` values. A command is assignable only when the current accepted
Agent instance and backend generation explicitly advertise that exact command
type.

This contract does not authorize any production mutation command. Before a
Recording, Timer, SearchTimer, Remote, configuration or metadata command is
implemented, that domain requires a separate command-specific contract covering
stable identity, revision, idempotency, native executor, verification and real
system acceptance.

A transport-runtime PR may prove delivery only with an explicitly documented
non-mutating acceptance fixture or separately contracted non-mutating command.
It must not smuggle a production write through a generic payload.

## Control Plane durable state

Before an assignment is exposed to the Agent, the Control Plane must have
durably established the relevant operation/job/attempt state and required
pre-dispatch accountability evidence.

At minimum, command delivery needs Suite-owned records for:

- stable `commandId`;
- owning `jobId`, `attemptId` and `claimEpoch`;
- Backend, Agent and backend-generation binding;
- Agent-instance binding or explicit assignment disposition;
- command type, payload version and normalized fingerprint;
- assignment state and deadline;
- receipt identity and receipt outcome;
- result identity, result category and bounded diagnostics;
- reconciliation state when dispatch may have occurred;
- timestamps and request/correlation/accountability linkage.

Repository classes own SQLite. HTTP handlers, Agent transport and command
adapters issue no direct SQL.

A database failure cannot expose an unrecorded assignment, advance an attempt,
acknowledge a receipt or finalize a result partially.

## Agent durable inbox

The Agent stores an assigned command before returning a durable receipt.

The protected local inbox records at least:

```text
commandId
requestFingerprint
jobId
attemptId
claimEpoch
backendId
agentId
agentInstanceId
backendGeneration
commandType
payloadVersion
deadline
receiptState
dispatchState
resultState
```

Rules:

- local state remains root-/service-private under the established Agent state
  directory contract;
- identical duplicate delivery returns the existing receipt or result;
- the same `commandId` with a different fingerprint is rejected as a conflicting
  duplicate;
- inbox persistence failure rejects delivery and does not claim acceptance;
- an unsupported command or payload version is durably rejected before dispatch;
- restart reconstructs receipt, dispatch and result state from protected
  storage, not process memory;
- commands are never copied into world-readable logs, evidence or process
  arguments;
- local retention must cover reconnect and Control Plane reconciliation.

## Receipt contract

A receipt proves only what the Agent has durably recorded. It is not proof that
a native side effect occurred.

A receipt carries at least:

```text
commandId
requestFingerprint
jobId
attemptId
claimEpoch
backendId
agentId
agentInstanceId
backendGeneration
receiptCategory
receivedAt
reasonCode
```

Minimum receipt categories are:

```text
accepted
duplicate
rejected
expired
stale
unsupported
conflict
```

Equivalent receipt replay is idempotent. A conflicting replay is rejected and
recorded. The Control Plane accepts a state-changing receipt only for the
current Backend, Agent, generation, job attempt and claim epoch.

## Local execution boundary

Before any future command adapter starts a side effect, the Agent must durably
record the execution boundary defined by ADR-0043:

```text
not_started
starting
accepted_by_executor
effect_reported
```

The transition to `starting` is committed before invoking the local/native
executor.

Consequences:

- crash or disconnect while `not_started` may permit a safe later assignment
  only under Control Plane policy;
- crash or disconnect at `starting` or later cannot cause blind re-execution;
- a stale Agent process cannot overwrite a newer local execution record;
- an expired Agent lease or command deadline prevents starting a new side
  effect;
- after `starting`, bounded local verification may finish and persist a result,
  but the Control Plane still owns final reconciliation;
- transport timeout never becomes proof of failure.

This contract PR contains no native executor and crosses no VDR lock boundary.

## Result outbox

The Agent persists a bounded result before attempting delivery to the Control
Plane.

A result carries at least:

```text
commandId
requestFingerprint
jobId
attemptId
claimEpoch
backendId
agentId
agentInstanceId
backendGeneration
dispatchState
verificationState
resultCategory
errorCategory
retryClassification
executorReference when safe
boundedDiagnostics
completedAt
```

Rules:

- identical result replay is idempotent;
- a conflicting result for the same command identity is rejected;
- result delivery retries the exact persisted result after ambiguous transport;
- stale-generation or stale-attempt results may be retained as reconciliation
  evidence but cannot directly finalize current state;
- result acknowledgement is persisted before local outbox retirement;
- no result alone authorizes automatic redispatch;
- `outcome_unknown` enters reconciliation and never directly schedules the
  original mutation again;
- required readback or event verification remains separate from executor
  acknowledgement.

## Lease, generation, deadline and claim fencing

The Agent may begin a command only when all required fences are current:

```text
authenticated Agent identity matches
AND accepted agentInstanceId matches
AND backendGeneration matches
AND Agent execution lease authorizes new work
AND command deadline permits dispatch
AND advertised commandType is still supported
AND local duplicate/fingerprint check passes
```

The Control Plane accepts receipt/result state changes only when:

```text
backendId matches
AND agentId matches
AND backendGeneration matches
AND jobId and attemptId match
AND claimEpoch is current
AND commandId and fingerprint match
```

A newer backend generation, Agent instance or claim epoch fences older work.
Fenced evidence remains append-only for diagnostics/reconciliation but cannot
become current authority.

## Reconnect and uncertain outcome

Reconnect order is deterministic:

1. authenticate and reconcile Agent identity, instance, generation and lease;
2. upload outstanding persisted results;
3. reconcile outstanding receipts and commands already at `starting` or later;
4. resolve conflicting or stale assignments;
5. only then expose new eligible assignments.

If the Control Plane cannot prove that dispatch remained `not_started`, the job
or operation enters `waiting_reconciliation` / `outcome_unknown`. It must use
receipt identity, result replay and authoritative domain readback before any
redispatch.

No continuity is guessed from timestamps, process uptime or missing rows.

## Authorization and accountability

Remote dispatch preserves the Phase-62 decision order.

Before assignment:

- authenticate the caller or trusted system actor;
- resolve explicit Backend and current generation;
- authorize the domain action and enforce read-only policy;
- verify command capability, stable resource identity and expected revision;
- resolve idempotency and durable operation identity;
- persist required pre-dispatch AccountabilityEvent/outbox evidence;
- create/claim the durable job attempt and command assignment.

After receipt/result/reconciliation, append bounded outcome evidence linked to:

```text
requestId
correlationId
actorId
operationId
jobId
attemptId
claimEpoch
commandId
backendId
agentId
agentInstanceId
backendGeneration
commandType
receiptCategory
dispatchState
verificationState
resultCategory
errorCategory
```

Credentials, tokens, Authorization headers, cookies, CSRF values, private URLs,
raw payload dumps and unrestricted native diagnostics are excluded.

## Failure behaviour

- command transport failure does not stop VDR;
- command transport failure does not disable existing direct adapters;
- Agent inbox/outbox failure fails the command closed;
- database failure cannot advance Control Plane command state;
- malformed input creates no partial inbox, receipt or result;
- duplicate transport does not duplicate execution;
- stale Agent processes and stale job claims cannot finalize current work;
- backoff is bounded and never spins;
- Agent shutdown does not wait indefinitely for the Control Plane;
- no unknown outcome is silently converted to success, failure or safe retry.

## Required automated coverage for the runtime PR

The separate runtime implementation must cover at least:

- valid bounded assignment and durable receipt;
- identical assignment replay returning the existing receipt;
- conflicting duplicate command rejection;
- unsupported command type and payload version rejection;
- stale backend generation and stale Agent instance rejection;
- expired lease and expired deadline preventing new dispatch;
- current attempt/claim-epoch acceptance;
- stale attempt/claim-epoch receipt and result fencing;
- inbox persistence failure before acknowledgement;
- restart-safe receipt state;
- durable `starting` boundary before executor invocation;
- crash/restart at `not_started` versus `starting`;
- result persisted before transport;
- identical result replay idempotency;
- conflicting result replay rejection;
- lost-response retry of the exact persisted result;
- reconnect ordering before new assignments;
- database rollback without partial cursor/job/command advancement;
- required pre-dispatch accountability failure preventing assignment;
- no browser/user authentication on Agent routes;
- no public Agent/provider endpoint;
- no native VDR mutation or domain command introduced by the transport
  foundation;
- repository/HTTP SQLite boundary;
- build, packaging, documentation and Make inventory.

## Real yaVDR acceptance direction

This contract-only PR changes no installed behaviour, so it requires no local
installation or real yaVDR acceptance.

The separate runtime PR must use exact-head guarded acceptance that:

- preserves the existing enrolled Agent ID and credential generation;
- installs byte-identical candidate daemon, Agent and administration binaries;
- uses only an explicitly non-mutating acceptance fixture or a separately
  contracted non-mutating command;
- proves durable receipt before acknowledgement;
- proves identical duplicate delivery and result replay;
- proves conflicting duplicate rejection;
- proves Agent restart with inbox/outbox persistence;
- proves deliberate lost-response recovery without duplicate execution;
- proves stale generation, stale attempt and expired-deadline fencing;
- proves reconnect reconciliation before new assignment;
- leaves VDR-native fingerprints unchanged;
- leaves existing observation cursors and Agent-owned facts unchanged except for
  independently expected heartbeat/read-only observation progress;
- restores original configuration;
- leaves VDR, daemon and Agent active;
- retains root-only secret-scanned evidence;
- performs no enrollment, revocation, Agent replacement or manual SQLite
  inspection.

## Hard exclusions

This contract-only slice does not implement:

- any Agent command HTTP route or client loop;
- database command/inbox/outbox tables;
- a universal Operation/Job/Attempt runtime;
- native VDR execution;
- Recording, Timer, SearchTimer, Remote, configuration or metadata mutation;
- provider ownership or provider selection;
- public Agent/provider URLs;
- TimerIntent or Phase 64;
- Streaming Gateway or media sessions;
- OSD snapshots or remote input;
- migration of existing local mutation endpoints;
- frontend/client command UI;
- replacement of direct-adapter `BackendNode.online` authority.

## Exit criterion

This contract slice is complete when:

- the contract and Slice-2 closeout are present and CI-guarded;
- documentation and Make inventory pass on one exact Draft-PR head;
- the durable diff contains only documentation, static guard and Make test
  registration;
- no runtime, API, schema, packaging, plugin or installed-behaviour change is
  present;
- the PR remains Draft until separate explicit approval.

Merging this contract authorizes only a separate bounded Draft runtime PR for
durable command delivery. It does not authorize a native domain command,
provider selection, protected writes or Phase 64.

## Related documents

- [Phase 63 Slice 2 Closeout](phase-63-slice-2-closeout.md)
- [Phase 63 Slice 1 Closeout](phase-63-slice-1-closeout.md)
- [Phase 63 Observation and Snapshot Ingestion](phase-63-observation-ingestion.md)
- [Phase 63 Channel Observation Ingestion](phase-63-channel-observation-ingestion.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Implementation Dependency Map](../planning/implementation-dependency-map.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
