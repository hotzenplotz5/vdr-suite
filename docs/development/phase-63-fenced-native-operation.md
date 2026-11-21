# Phase 63 Slice 4 — Fenced Native Operation Contract

## Status

**Binding contract for the next bounded Phase-63 slice.**

This is a contract-only slice created after durable Agent command delivery was
implemented, accepted on the real yaVDR host and merged.

Merged foundation at contract creation:

```text
main commit: 271254a5e5baf83f4a32e974da3d6bec7e33064b
main tree: 4b4b3c89498bf15397d27dffbf1cbcb114673825
completed lifecycle slice: Phase 63 Slice 1
completed read-only ingestion slice: Phase 63 Slice 2
completed durable command-delivery slice: Phase 63 Slice 3
next bounded slice: fenced native operation execution
```

This document adds no plugin command, local transport method, Agent executor,
command capability, schema, packaging change or installed runtime effect. A
separate bounded Draft runtime PR is required.

## Why this slice is next

The accepted Phase-63 order is:

```text
Agent lifecycle
  -> complete read-only observation continuity
  -> durable command inbox and result outbox
  -> fenced native operations
  -> explicit local provider ownership and selection
  -> protected writes
  -> Phase 64 TimerIntent orchestration
```

The merged command-delivery runtime proves durable assignment, receipt, result,
replay, restart and generation fencing with `probe.noop`, but that command does
not cross a VDR-native executor boundary. The next missing proof is one typed,
bounded and side-effect-free native operation that reaches the local
SuiteBridge/VDR boundary without authorizing any production mutation.

Moving directly to a Timer, Recording, SearchTimer, Remote, configuration or
metadata write would combine three unresolved concerns at once:

- native executor and VDR lock/thread safety;
- provider ownership and selection;
- domain mutation identity, revision and authoritative readback.

This slice isolates only the first concern.

## Goal

Define one fenced native operation foundation that:

- reuses the merged durable Agent command-delivery runtime;
- introduces exactly one command type, `vdr.native.probe`;
- crosses the Agent-to-local-adapter-to-SuiteBridge-to-VDR execution boundary;
- is explicitly side-effect-free and leaves `mutations=disabled`;
- persists `starting` before local dispatch;
- proves executor acceptance separately from authoritative readback;
- rejects stale generation, Agent instance, command capability and local
  capability state;
- makes identical replay idempotent and conflicting replay fail closed;
- preserves `outcome_unknown` and reconciliation after ambiguous local dispatch;
- exposes no public Agent, provider, plugin or SVDRP endpoint;
- does not select a provider for normal domain reads or writes;
- does not authorize a production VDR mutation.

## Governing contracts

This slice narrows and composes:

- [ADR-0040 — Backend Lifecycle, Generation, Lease and Health](../adr/ADR-0040-backend-lifecycle-generation-lease-health.md);
- [ADR-0041 — Authentication, Agent Trust and Multi-Site Transport](../adr/ADR-0041-authentication-agent-trust-multi-site-transport.md);
- [ADR-0042 — Safe Mutation, Revision and Idempotency Contract](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md);
- [ADR-0043 — Job Claim, Retry and Saga Execution Model](../adr/ADR-0043-job-claim-retry-saga-execution-model.md);
- [ADR-0049 — Audit and Security Event Model](../adr/ADR-0049-audit-security-event-model.md);
- [Suite Bridge ADR-0001 — Plugin Role and Native Integration Strategy](../../vdr-plugin-suite-bridge/docs/ADR-0001-plugin-role-and-native-integration-strategy.md);
- [Phase 63 Slice 3 — Durable Agent Command Delivery](phase-63-command-delivery.md);
- [Phase 63 Slice 3 Runtime](phase-63-command-delivery-runtime.md).

The identities `operationId`, `jobId`, `attemptId`, `claimEpoch`, `commandId`,
`agentInstanceId`, `backendGeneration`, `pluginInstanceEpoch` and
`nativeExecutionSequence` are distinct.

## Exact command boundary

The only remotely assignable command introduced by the later runtime is:

```text
commandType = vdr.native.probe
payloadVersion = 1
verificationPolicy = readback_required
sideEffectClass = none
```

The normalized payload contains only:

```text
probeSchema = 1
probeNonce = one opaque bounded non-secret value
```

Rules:

- `probeNonce` is part of the request fingerprint and is stable across exact
  retries;
- the payload contains no command text, plugin name chosen by a caller, native
  pointer, path, URL, credential, lock, script, shell fragment or arbitrary
  JSON extension;
- unknown fields, duplicate keys, invalid UTF-8, oversized values and any
  payload version other than `1` fail closed before native dispatch;
- no `resourceType`, `resourceId` or `expectedRevision` is manufactured because
  the probe mutates no domain resource;
- the command remains bound to the existing `operationId`, `jobId`, `attemptId`,
  `claimEpoch`, `commandId`, Backend, Agent instance and backend generation;
- the command deadline must permit both dispatch and bounded readback;
- `claimToken` never crosses into the Agent command or local native request.

`vdr.native.probe` is not a generic command namespace. It does not authorize a
future command merely because the future command also reaches VDR.

## Authority boundary

The Control Plane remains authoritative for:

- caller or trusted system intent;
- authorization and backend read-only policy;
- durable operation, job, attempt and claim state;
- idempotency and normalized request fingerprint;
- assignment eligibility and deadline;
- final interpretation of success, failure or unknown outcome;
- retry and reconciliation policy;
- accountability before dispatch and after outcome interpretation.

The Backend Agent owns only:

- durable receipt and protected local command state;
- current Agent-instance and backend-generation validation;
- exact command capability publication;
- local capability negotiation;
- durable `starting` before local dispatch;
- bounded typed adapter invocation;
- persisted native receipt, result and readback evidence;
- exact replay of already persisted evidence.

SuiteBridge owns only the bounded VDR-process-local operation and epoch-scoped
native receipt evidence. VDR remains authoritative for VDR process state.

The Agent is not a global scheduler. SuiteBridge is not an operation store,
retry scheduler, public API or policy engine.

## Local provider boundary

The acceptance-only local adapter kind for this command is exactly
`suitebridge`.

This does **not** establish provider ownership or provider selection for
channels, EPG, recordings, timers, SearchTimers, metadata, Remote, Live or any
other Suite domain.

Rules:

- the Agent may advertise `vdr.native.probe` only after a successful local
  SuiteBridge capability negotiation for the exact native-probe schema;
- a configured SuiteBridge local transport is used only for this command;
- there is no fallback to free-form SVDRP, RESTfulAPI, shell execution,
  `svdrpsend`, plugin service tunnelling or another provider;
- a missing, incompatible or stale SuiteBridge capability yields
  `capability_unavailable` before native dispatch;
- existing direct-adapter availability and `BackendNode.online` authority remain
  unchanged;
- no private local endpoint, socket, port or provider URL is returned to the
  Control Plane or public clients.

Provider ownership and selection remain a separate later Phase-63 slice.

## SuiteBridge capability contract

The later runtime must extend truthful local capability negotiation with an
independent native-operation capability axis.

At minimum the negotiated facts include:

```text
nativeOperation = vdr.native.probe
nativeOperationSchema = 1
sideEffectClass = none
mutations = disabled
localProviderKind = suitebridge
pluginInstanceEpoch
```

Requirements:

- plugin software version, capability schema, local-contract schema and native
  operation schema remain independent compatibility axes;
- `mutations=disabled` remains true before, during and after the probe;
- a capability is descriptive and does not bypass Control-Plane authorization;
- capability disappearance or epoch replacement fences pending dispatch;
- the Agent rechecks the exact capability immediately before committing
  `starting`;
- the packaged Agent configuration keeps command types disabled by default;
- enabling `vdr.native.probe` for acceptance is explicit, temporary and restored.

A future mutation capability requires a new accepted domain-specific contract.

## Typed local request

The Agent-to-SuiteBridge adapter sends one typed request with at least:

```text
nativeProtocolVersion
nativeOperation
nativeOperationSchema
commandId
requestFingerprint
backendId
agentId
agentInstanceId
backendGeneration
probeNonce
```

Exact local wire spelling is implementation-owned, but the public C++ boundary
must not accept free command text.

The serializer must be allowlisted and deterministic. Each field is bounded.
The request contains no credential, authorization header, cookie, CSRF value,
claim token, public actor token, native pointer, process argument or file path.

The local transport remains loopback-/local-only, uses one bounded invocation,
validates framing and reply codes, closes deterministically and never becomes a
public listener.

## SuiteBridge execution rules

The plugin implementation for the later runtime must:

- accept only the exact typed native operation and schema;
- validate `commandId` and `requestFingerprint` before VDR dispatch;
- reserve an epoch-scoped bounded receipt entry before invoking the native
  execution step;
- reject the same `commandId` with a different fingerprint as conflict;
- return the stored receipt for an identical duplicate without executing the
  native step again;
- execute only bounded side-effect-free VDR state capture;
- retain no raw VDR pointer or lock after the bounded call;
- perform no network, database, filesystem mutation, shell or process execution;
- never block a VDR callback waiting for the Agent or Control Plane;
- keep the receipt ledger fixed-capacity and in-memory;
- replace the receipt ledger when `pluginInstanceEpoch` changes;
- expose no durable audit store or retry loop.

The epoch-scoped ledger closes the lost-local-response replay window for the
probe. It is not sufficient idempotency for a production mutation because it is
not durable across VDR restart. Every later write requires its own stable
resource identity, revision, native idempotency and authoritative readback
contract.

## Native receipt and result

The typed native receipt carries at least:

```text
commandId
requestFingerprint
nativeOperation
nativeOperationSchema
pluginInstanceEpoch
nativeExecutionSequence
receiptCategory
acceptedAt
sideEffectClass = none
```

Minimum native receipt categories are:

```text
accepted
duplicate
rejected
stale
unsupported
conflict
```

The typed native result carries at least:

```text
commandId
requestFingerprint
nativeOperation
nativeOperationSchema
pluginInstanceEpoch
nativeExecutionSequence
resultCategory
vdrActive
mutationsState = disabled
sideEffectObserved = false
boundedDiagnostics
completedAt
```

Timestamps from the plugin are evidence only and never extend the command,
Agent or claim deadline.

An identical duplicate returns the same plugin instance epoch, native execution
sequence and normalized result. A conflicting duplicate changes no state and
fails closed.

## Durable execution boundary

The Agent preserves the merged dispatch states:

```text
not_started
starting
accepted_by_executor
effect_reported
```

Required order:

1. validate current Agent identity, instance, backend generation, lease, command
   deadline and claim epoch;
2. negotiate and recheck exact local SuiteBridge native-probe capability;
3. persist the protected local command and receipt state;
4. commit `starting` durably;
5. invoke the typed local adapter;
6. persist `accepted_by_executor` only after a valid native receipt;
7. persist `effect_reported` only after a valid bounded native result;
8. perform authoritative readback;
9. persist the final Agent result before Control-Plane transport.

No local call occurs before `starting` is durable.

A stale Agent process or obsolete backend generation cannot overwrite a newer
local execution record. A lease or deadline that expires before step 4 prevents
native dispatch. Expiry after step 4 does not erase evidence or imply failure.

## Readback and verification

`vdr.native.probe` uses `readback_required` even though it is side-effect-free.
The purpose is to prove executor acknowledgement and authoritative verification
as separate facts.

Readback must verify at least:

- exact `commandId` and request fingerprint;
- exact native operation and schema;
- same `pluginInstanceEpoch` as the accepted native receipt;
- same `nativeExecutionSequence` as the accepted native receipt;
- `vdrActive = true`;
- `mutationsState = disabled`;
- `sideEffectObserved = false`;
- exact duplicate disposition when recovery replays the request.

A typed exact duplicate may serve as readback only when SuiteBridge returns the
stored receipt/result without executing again. A generic `SNAP`, log message,
process uptime or transport success alone is insufficient verification.

Verification states remain:

```text
pending
succeeded
failed
unknown
```

The job and operation cannot become `succeeded` until readback succeeds.

## Replay, restart and uncertain outcome

Equivalent Agent delivery keeps the same command identity and fingerprint.
Equivalent local replay keeps the same typed native request.

Recovery rules:

- crash while `not_started`: Control-Plane policy may safely create a later
  fenced attempt;
- crash or lost response at `starting`: enter `waiting_reconciliation`; do not
  invent `failed_before_dispatch`;
- exact local duplicate in the same plugin epoch: return the stored receipt and
  result without native re-execution;
- conflicting local duplicate: reject and retain original evidence;
- Agent restart: reload protected command state before new assignment;
- daemon restart: preserve Control-Plane command, receipt, result and
  reconciliation state;
- plugin/VDR restart: a new `pluginInstanceEpoch` fences old epoch-local receipt
  authority;
- unknown old-epoch dispatch: retain `outcome_unknown`; do not blindly execute
  the original command under the new epoch;
- because the probe has no side effect, an operator or explicit reconciliation
  policy may create a new probe operation with a new command identity; it does
  not rewrite history of the unknown original command.

No continuity is guessed from timestamps, PID, uptime, missing logs or missing
rows.

## Fencing matrix

Native dispatch is permitted only when all are current:

```text
authenticated Agent identity
AND accepted agentInstanceId
AND backendGeneration
AND Agent execution lease
AND command deadline
AND job attempt and claimEpoch
AND commandId and requestFingerprint
AND advertised commandType vdr.native.probe
AND local provider kind suitebridge
AND native operation schema 1
AND current pluginInstanceEpoch capability
AND protected local duplicate state
```

A native receipt/result changes current command state only when:

```text
commandId matches
AND requestFingerprint matches
AND backendId matches
AND agentId matches
AND agentInstanceId matches
AND backendGeneration matches
AND attemptId and claimEpoch match
AND native operation and schema match
AND pluginInstanceEpoch matches the accepted local dispatch
```

Stale evidence remains bounded diagnostic/reconciliation evidence. It cannot
become current authority.

## Authorization, read-only policy and accountability

The probe is non-mutating, but remote native dispatch is still a privileged
machine operation.

Before assignment the Control Plane must:

- authenticate the trusted caller;
- resolve the explicit Backend and current generation;
- authorize the exact native-probe action;
- enforce backend read-only policy according to the probe's explicitly allowed
  non-mutating classification;
- verify Agent and local native capability;
- resolve idempotency and durable operation identity;
- persist pre-dispatch AccountabilityEvent/outbox evidence;
- create and claim the durable job attempt and command assignment.

Outcome evidence links:

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
nativeOperation
nativeOperationSchema
pluginInstanceEpoch
nativeExecutionSequence
receiptCategory
resultCategory
verificationState
```

No secret, raw payload dump, Authorization header, cookie, CSRF value, private
URL, local path or unrestricted native diagnostic enters normal logs or
accountability context.

## Failure behaviour

- missing or incompatible SuiteBridge capability fails before native dispatch;
- plugin or local transport failure does not stop VDR;
- Agent command failure does not disable existing direct adapters;
- local state persistence failure rejects dispatch;
- failure to persist `starting` prevents local invocation;
- malformed local reply creates no accepted executor state;
- lost local response becomes reconciliation, not automatic failure;
- identical duplicate does not duplicate native execution;
- conflicting duplicate does not overwrite original evidence;
- stale generation, instance, epoch, attempt or claim cannot finalize current
  state;
- backoff is bounded and never spins;
- shutdown does not wait indefinitely for the plugin or Control Plane;
- no unknown outcome is converted silently to success, failure or safe retry.

## Required automated coverage for the runtime PR

The separate runtime implementation must cover at least:

- exact `vdr.native.probe` payload schema and bounds;
- unsupported command and payload versions;
- command capability absent before local negotiation;
- command capability publication only after exact native capability discovery;
- no fallback to generic SVDRP or another provider;
- stale Agent instance and backend generation rejection;
- expired lease, deadline, attempt and claim-epoch fencing;
- durable `starting` before local adapter invocation;
- local persistence failure before dispatch;
- valid native receipt and result parsing;
- identical local duplicate returning one native execution sequence;
- conflicting local duplicate rejection;
- result/readback identity and plugin-epoch matching;
- lost local response recovery through exact duplicate readback;
- Agent restart with protected command state;
- daemon restart with Control-Plane state;
- plugin epoch replacement fencing stale local evidence;
- `outcome_unknown` without blind cross-epoch execution;
- `mutations=disabled` before and after the probe;
- no Timer, Recording, SearchTimer, Remote, configuration or metadata mutation;
- no public endpoint, free command text, shell or arbitrary plugin-service call;
- repository/HTTP SQLite boundary;
- plugin, Agent, daemon, packaging, documentation and Make inventory.

## Real yaVDR acceptance direction

This contract-only PR changes no installed behavior and requires no real yaVDR
acceptance.

The separate runtime PR must use exact-head guarded acceptance that:

- preserves the enrolled Agent ID and credential generation;
- installs byte-identical candidate daemon, Agent, administration and
  SuiteBridge plugin binaries required by the runtime;
- verifies the exact API-versioned plugin shared object and load state;
- enables only `vdr.native.probe` temporarily;
- proves native capability negotiation while `mutations=disabled`;
- proves one baseline native execution and separate readback;
- proves equivalent replay without a second native execution sequence;
- proves deliberate lost local response recovery;
- proves conflicting duplicate rejection;
- proves daemon and Agent restart persistence;
- proves stale Agent generation and plugin epoch fencing;
- leaves channels, timers, SearchTimers, recordings and VDR configuration
  fingerprints unchanged;
- leaves existing observation cursors and Agent-owned facts unchanged except for
  independently expected heartbeat/read-only progress;
- restores original Agent and plugin configuration and local state;
- leaves VDR, daemon and Agent active;
- retains root-only secret-scanned evidence;
- performs no enrollment, revocation, Agent replacement or manual SQLite
  inspection.

## Hard exclusions

This contract-only slice does not implement or authorize:

- a plugin `vdr.native.probe` command or runtime handler;
- an Agent native executor or local transport method;
- any command capability in packaged configuration;
- Timer, Recording, SearchTimer, Remote, OSD input, configuration or metadata
  mutation;
- a universal native command, SVDRP tunnel or plugin-service tunnel;
- provider ownership or provider selection;
- replacement of RESTfulAPI or existing direct adapters;
- a public Agent, plugin, provider or local transport endpoint;
- `mutations=enabled`;
- plugin database, durable retry scheduler or public audit store;
- Phase 64 TimerIntent orchestration;
- Streaming Gateway, Legacy OSD control or media sessions;
- frontend/client native command UI;
- manual SQLite inspection.

## Exit criterion

This contract slice is complete when:

- this contract and the Slice-3 closeout are present and CI-guarded;
- documentation and Make inventory pass on one exact Draft-PR head;
- the durable diff contains only documentation, one static guard and Make test
  registration;
- no Agent, daemon, plugin, API, schema, packaging or installed behavior change
  is present;
- the PR remains Draft until separate explicit user approval.

Merging this contract authorizes only a separate bounded Draft runtime PR for
`vdr.native.probe`. It does not authorize provider selection, a production
native mutation, protected writes or Phase 64.

## Related documents

- [Phase 63 Slice 3 Closeout](phase-63-slice-3-closeout.md)
- [Phase 63 Durable Command Delivery Contract](phase-63-command-delivery.md)
- [Phase 63 Durable Command Delivery Runtime](phase-63-command-delivery-runtime.md)
- [Suite Bridge ADR-0001](../../vdr-plugin-suite-bridge/docs/ADR-0001-plugin-role-and-native-integration-strategy.md)
- [Suite Bridge Roadmap](../../vdr-plugin-suite-bridge/docs/ROADMAP.md)
- [Target Platform Architecture](../architecture/target-platform-architecture.md)
- [Implementation Dependency Map](../planning/implementation-dependency-map.md)
- [Strict Roadmap](../planning/roadmap.md)
