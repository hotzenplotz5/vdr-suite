# ADR-0043: Job Claim, Retry and Saga Execution Model

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Current State](../CURRENT.md)
- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](ADR-0042-safe-mutation-revision-idempotency-contract.md)

---

## Status

Accepted

Date: 2026-07-16

---

## Context

VDR-Suite already contains an early local job foundation:

- a `jobs` table with `PENDING`, `RUNNING`, `DONE`, `FAILED` and `CANCELLED` states;
- a `Job` value object and `JobRepository`;
- a `JobService` and `RecordingWorkflowService`;
- a `WorkerSimulator` that selects the next pending job and moves it through `RUNNING` to `DONE`;
- Recording action job payloads;
- REST and dashboard read models for jobs.

This foundation proves basic persistence and queue-shaped control flow, but it is not a production execution model.

The current repository does not yet provide:

- an atomic claim operation;
- worker ownership and fencing;
- a renewable claim lease;
- durable attempt records;
- retry classification and scheduling;
- safe treatment of a crash after backend dispatch;
- cancellation boundaries;
- dependency handling;
- compensation for multi-step workflows;
- durable offline Agent command and result reconciliation;
- payload versioning;
- resource-level concurrency control;
- a clear relationship between jobs and ADR-0042 mutation operations.

The current `getNextPendingJob()` read followed by a later status update can allow more than one worker to observe the same pending job. The current `WorkerSimulator` also cannot distinguish:

```text
backend dispatch never started
backend rejected before mutation
backend may have executed but response was lost
backend executed and readback is still pending
backend result was verified
```

That distinction is mandatory before production remote writes, retries or multi-site Agent execution.

A distributed transaction across the Control Plane, Backend Agent, VDR plugin, VDR Core, metadata providers and filesystem is neither available nor desirable. VDR-Suite therefore needs durable local transactions, fenced work ownership, idempotent commands, authoritative readback and explicit saga compensation rather than pretending that external side effects are one atomic database transaction.

---

## Decision

VDR-Suite adopts a durable job execution model with:

- stable job identity;
- versioned immutable job payloads;
- atomic claim and renewable claim lease;
- monotonic claim fencing;
- immutable execution attempts;
- explicit dispatch and verification boundaries;
- persistent retry schedules;
- safe cancellation semantics;
- resource and backend concurrency keys;
- durable saga instances and compensation steps;
- Agent inbox and result-outbox reconciliation for remote execution;
- strict preservation of ADR-0042 operation identity and outcome semantics.

The job scheduler is owned by VDR-Suite. A Backend Agent may execute assigned work and persist local command receipts or results for reconnect safety, but it does not become an independent global scheduler. The VDR plugin executes only bounded VDR-native steps and never owns durable retries, sagas or cross-site policy.

---

## Core Identity Model

The model distinguishes four identities:

| Identity | Meaning |
| --- | --- |
| `operationId` | One externally meaningful logical mutation under ADR-0042. |
| `jobId` | One durable schedulable unit of work. |
| `attemptId` | One concrete execution attempt for a job. |
| `sagaInstanceId` | One durable multi-step workflow instance. |

These identities are not interchangeable.

### Operation

An operation represents the caller-visible intent and durable mutation outcome.

Examples:

- move one Recording;
- delete one native Timer;
- change one SearchTimer definition;
- assign metadata to one Recording.

The operation owns:

- actor and authorization context;
- idempotency key and normalized request fingerprint;
- target backend and resource identity;
- expected resource revision;
- final mutation-level state.

### Job

A job is a durable schedulable action that advances an operation or performs independent background work.

Examples:

- validate a Recording binding;
- dispatch a native Recording move;
- read back the Recording after dispatch;
- refresh metadata from one provider;
- reconcile an unknown Agent result.

One operation may require multiple jobs. A non-mutating maintenance job may have no `operationId`.

### Attempt

An attempt records one worker's claimed execution of one job. Retries create new attempts; they do not create a new caller intent.

### Saga instance

A saga instance coordinates multiple dependent jobs and compensations. A saga may advance one operation or a larger explicitly modelled workflow.

---

## Job Envelope

Every durable job carries at least:

| Field | Meaning |
| --- | --- |
| `jobId` | Stable VDR-Suite job identity. |
| `jobType` | Stable handler name. |
| `payloadVersion` | Version of the serialized job payload. |
| `payload` | Immutable normalized handler input. |
| `payloadFingerprint` | Stable fingerprint used to detect accidental mutation or incompatible replay. |
| `operationId` | Optional related ADR-0042 operation. |
| `sagaInstanceId` | Optional owning saga. |
| `sagaStepId` | Optional stable step identity inside the saga definition. |
| `backendId` | Optional target backend. |
| `backendGeneration` | Required for generation-bound backend execution. |
| `resourceType` | Optional target domain type. |
| `resourceId` | Optional stable Suite resource identity. |
| `expectedRevision` | Optional ADR-0042 resource revision. |
| `verificationPolicy` | Required verification mode where side effects occur. |
| `retryPolicyId` | Stable retry policy identifier and version. |
| `priority` | Scheduling priority within policy limits. |
| `notBefore` | Earliest eligible claim time. |
| `deadline` | Time after which new dispatch must not start. |
| `concurrencyKey` | Key preventing incompatible simultaneous work. |
| `createdAt` | Durable creation timestamp. |

The payload contains values, not executable code, raw pointers, VDR lock objects or transport sessions.

Credentials and secrets are referenced through protected configuration or credential identities. They are not copied into arbitrary job payloads.

---

## Payload Versioning

Job payloads are durable data and may outlive the process version that created them.

Rules:

- every handler has a stable `jobType`;
- every payload has an explicit `payloadVersion`;
- existing field meaning is never silently changed;
- additive optional fields require documented defaults;
- incompatible changes require a new payload version;
- a worker must reject an unsupported payload version before dispatch;
- deterministic upcasters may convert supported older payloads into the current in-memory representation;
- payload conversion must not change the original requested intent;
- the original serialized payload and fingerprint remain available for audit and recovery;
- queued jobs are included in migration and deployment compatibility checks.

A deployment must not remove the last worker capable of handling still-queued payload versions without first migrating or draining those jobs.

---

## Canonical Job States

The durable job lifecycle uses:

| State | Meaning |
| --- | --- |
| `queued` | Eligible now or in the future, with no active claim. |
| `claimed` | Atomically owned by one worker claim, but handler dispatch has not yet begun. |
| `running` | The claimed handler is actively executing or awaiting its bounded result. |
| `blocked` | A declared dependency, backend state or operator prerequisite is not currently satisfied. |
| `waiting_retry` | A known retryable outcome has a persisted next eligible time. |
| `waiting_reconciliation` | Backend dispatch may have happened and verification must precede redispatch. |
| `cancel_requested` | Cancellation is durable but has not yet reached a safe terminal boundary. |
| `succeeded` | The job's required execution and verification contract completed. |
| `failed_terminal` | The job cannot continue automatically and its outcome is known. |
| `cancelled` | Work stopped before an unsafe boundary or completed an explicitly verified cancellation path. |
| `dead_letter` | Recovery requires explicit operator action; evidence is retained. |

Terminal states are:

```text
succeeded
failed_terminal
cancelled
dead_letter
```

`dead_letter` is not a synonym for generic failure. It means automatic policy is exhausted, compensation failed, a payload is no longer executable, or an unresolved outcome requires operator intervention.

---

## Job State and Operation State Are Different

Internal job states must not replace the ADR-0042 mutation lifecycle.

Examples:

| Job fact | Required operation interpretation |
| --- | --- |
| queued before dispatch | `queued` |
| claimed before dispatch | normally `queued` or `dispatching`, depending on the recorded dispatch boundary |
| running before external dispatch | `dispatching` only when execution ownership is active |
| waiting retry with no prior dispatch | remains safely queued or dispatching according to the operation record |
| backend success, readback pending | `executed_unverified` |
| dispatch may have occurred | `outcome_unknown` |
| verified requested state | `succeeded` |
| verified no mutation was attempted | `failed_before_dispatch` |
| verified backend result does not satisfy intent | `failed_verified` |
| safe cancellation before dispatch | `cancelled` |

A job entering `dead_letter` does not invent a new public mutation state. The operation remains in the correct ADR-0042 state, such as `failed_before_dispatch`, `failed_verified` or `outcome_unknown`, with an operator-attention flag.

---

## Atomic Claim Model

Claiming work is a database state transition, not a read followed by a later update.

An eligible job may be claimed only through one atomic transaction or equivalent compare-and-swap operation that:

1. selects a job whose state, schedule, dependencies and concurrency constraints permit execution;
2. verifies that no valid claim exists;
3. increments its monotonic `claimEpoch`;
4. writes a new opaque `claimToken`;
5. records `claimOwnerId`, `claimedAt` and `claimLeaseExpiresAt`;
6. creates the attempt record;
7. changes the job to `claimed`;
8. returns the claimed job and attempt to exactly one worker.

The claim update must include the previous state and eligibility conditions in its write predicate.

A plain `SELECT ... WHERE status = 'PENDING' LIMIT 1` is not a production claim mechanism.

---

## Claim Lease and Fencing

A job claim lease authorizes one worker attempt for one job.

It is distinct from the Backend Agent lease in ADR-0040:

```text
Backend Agent lease
  authorizes one backend generation to represent and execute for a backend

Job claim lease
  authorizes one worker attempt to advance one durable job
```

Remote mutation execution requires both boundaries to be valid.

A claim includes:

```text
jobId
attemptId
claimOwnerId
claimEpoch
claimToken
claimLeaseIssuedAt
claimLeaseRenewedAt
claimLeaseExpiresAt
```

Rules:

- `claimEpoch` increases monotonically for every new claim of the job;
- all state-changing worker writes include `jobId`, `attemptId`, `claimEpoch` and `claimToken` in the write predicate;
- a stale worker cannot complete, retry, cancel or reschedule a job after another claim epoch exists;
- lease renewal is accepted only from the active owner before expiry;
- an expired claim is not resurrected; recovery creates a new claim epoch;
- failure to renew prevents starting a new external side effect;
- a late worker may submit diagnostic or reconciliation evidence, but stale evidence cannot directly overwrite the current job or operation state;
- lease durations and heartbeat intervals are configuration values, not protocol meaning.

Time comparisons use one authoritative persistence or scheduler clock abstraction. Tests use an injected deterministic clock.

---

## Attempt Record

Every claim creates an immutable attempt identity and an append-only execution history.

An attempt records at least:

```text
attemptId
jobId
attemptNumber
claimOwnerId
claimEpoch
startedAt
finishedAt
dispatchState
verificationState
resultCategory
errorCategory
retryClassification
backendId
backendGeneration
executorReference
boundedDiagnostics
```

`attemptNumber` is the ordered execution count for the job. Claim recovery that proves no handler execution started may be recorded separately from side-effect attempts, but history must never be erased.

Attempt evidence is append-only except for bounded fields that are explicitly completed by the active fenced owner.

---

## Dispatch Boundary

The worker records the dispatch boundary before calling an external or VDR-native executor.

Canonical dispatch states are:

```text
not_started
starting
accepted_by_executor
effect_reported
```

Meaning:

- `not_started`: no external side effect was attempted;
- `starting`: the durable record was committed immediately before the external call, so dispatch may or may not have reached the executor;
- `accepted_by_executor`: the executor acknowledged command acceptance, but final effect may still be unverified;
- `effect_reported`: the executor reported an effect or result, still subject to the declared verification policy.

The transition to `starting` must be durable before external dispatch.

This deliberately prefers a conservative unknown outcome over a false claim that nothing happened. A crash after `starting` triggers reconciliation before any redispatch.

---

## Verification State

Execution and verification are separate dimensions.

Canonical verification states are:

```text
not_required
pending
succeeded
failed
unknown
```

They implement the ADR-0042 policies:

- `none` maps to `not_required` only when the executor contract is authoritative and atomic;
- `readback_required` remains `pending` until authoritative state is compared;
- `event_confirmation` remains `pending` until the expected sequenced event is observed;
- `reconciliation_required` enters a durable reconciliation path.

A job must not become `succeeded` until its required verification state is `succeeded` or `not_required` under an accepted authoritative executor contract.

---

## Claim Expiry Recovery

When a claim expires, recovery uses the attempt's durable dispatch boundary:

| Last durable fact | Recovery |
| --- | --- |
| `not_started` | safely create a new claim when policy permits |
| `starting` | move to `waiting_reconciliation`; do not redispatch |
| `accepted_by_executor` | move to `waiting_reconciliation`; query receipt or authoritative state |
| `effect_reported` with verification pending | continue or schedule verification under a new fenced claim |
| verified success | finalize job and operation idempotently |
| verified failure with no requested effect | classify through retry policy |

If the system cannot prove that dispatch did not start, it treats the result as potentially executed.

---

## Retry Classification

Retry is based on a structured classification, not on arbitrary error strings.

Canonical classifications are:

| Classification | Meaning |
| --- | --- |
| `safe_before_dispatch` | No external side effect started; retry may be scheduled. |
| `safe_after_verified_no_effect` | Authoritative evidence proves the requested effect did not occur. |
| `transient_dependency` | Backend, provider, storage or rate-limit condition may recover without changing intent. |
| `conflict_requires_refresh` | Generation, revision, preview or identity changed; automatic redispatch is forbidden. |
| `permanent` | Validation, authorization, unsupported payload or non-recoverable executor failure. |
| `outcome_unknown` | Dispatch may have occurred; reconciliation is mandatory before retry. |
| `cancelled` | Execution stopped at a safe cancellation boundary. |

Rules:

- `outcome_unknown` never directly schedules the original mutation for redispatch;
- generation and revision conflicts require refreshed state and normally a new user operation;
- authorization and read-only denials are not retried by a worker;
- temporary capability or backend unavailability may block or retry only within an explicit policy and deadline;
- retryable errors retain the same `operationId`, idempotency key, request fingerprint, backend identity, resource identity and intended transition;
- a new attempt does not create a new logical mutation;
- a user intentionally choosing a different action after conflict creates a new operation and idempotency key.

---

## Retry Policy

Retry behavior is configured through a named and versioned policy.

A retry policy may define:

```text
policyId
policyVersion
maximumAttempts
initialDelay
maximumDelay
backoffFunction
jitterPolicy
retryableCategories
deadlineBehavior
operatorEscalation
```

Rules:

- `nextEligibleAt` is persisted;
- retries never use a tight in-process loop;
- backoff is bounded;
- jitter is deterministic under tests and bounded in production;
- an executor-provided retry hint may delay work but cannot bypass the configured maximum or deadline;
- maximum attempts and deadlines are separate limits;
- a deadline prevents new dispatch but does not erase an already unknown outcome;
- exhausted safe retries enter `failed_terminal` or `dead_letter` according to policy and operator requirements;
- destructive operations use more conservative defaults than read-only refresh jobs.

Exact delay values remain configuration and implementation details.

---

## Scheduling and Eligibility

A job is claimable only when all required conditions are true:

```text
state permits claim
AND notBefore <= now
AND nextEligibleAt <= now
AND deadline permits new dispatch
AND dependencies are satisfied
AND required backend state permits execution
AND concurrency key is available
AND retry policy is not exhausted
```

Priority influences ordering only among otherwise eligible jobs.

The scheduler must:

- avoid starvation;
- avoid one offline backend blocking unrelated backends;
- support backend and handler concurrency limits;
- preserve stable ordering for equal priority where practical;
- treat operator priority changes as auditable control actions;
- not bypass authorization, read-only, revision, generation or capability rules.

---

## Concurrency Keys

Every job that can conflict with another job declares a stable `concurrencyKey`.

Examples:

```text
backend:<backendId>:recording:<recordingId>
backend:<backendId>:timer:<timerId>
backend:<backendId>:searchtimer:<searchTimerId>
storage:<storageIdentity>
metadata-assignment:<recordingId>
```

Rules:

- only one incompatible active claim may own a concurrency key;
- read-only jobs may declare a shared or no lock mode where safe;
- resource mutation defaults to exclusive ownership;
- a backend-wide limit may coexist with a resource key;
- a path that changes during a Recording move is not the stable concurrency identity;
- claim expiry releases scheduler ownership only through fenced recovery rules;
- concurrency ownership is not held through an in-memory mutex alone.

---

## Cancellation

Cancellation is a durable request, not an assumption that work instantly stopped.

A cancellation request records:

```text
requestedBy
requestedAt
reason
scope
```

The worker checks cancellation:

- before external dispatch;
- between declared safe saga steps;
- during cooperative long-running work at bounded checkpoints;
- before scheduling another retry.

Rules:

- before dispatch, cancellation may transition directly to `cancelled`;
- after dispatch starts, cancellation cannot claim that no effect occurred;
- an executor may support cooperative cancellation, but the result still requires verification;
- if a side effect may have occurred, the job enters verification, reconciliation or compensation rather than false cancellation;
- operation state `cancelled` is used only at the safe boundary defined by ADR-0042;
- cancellation of a saga stops future forward steps at safe boundaries and may start compensation;
- cancellation requests and decisions are auditable;
- forcefully terminating a worker process is not a valid cancellation protocol.

---

## Saga Model

A saga coordinates a durable multi-step workflow without claiming a distributed transaction.

A saga definition is versioned and declares:

```text
sagaType
sagaVersion
steps
dependencies
forward job type for each step
compensation job type where available
idempotency requirements
irreversible boundary
completion policy
```

A saga instance records:

```text
sagaInstanceId
sagaType
sagaVersion
operationId when applicable
state
current dependency frontier
createdAt
updatedAt
failure reason
operator attention state
```

Canonical saga states are:

```text
pending
running
compensating
succeeded
compensated
partially_compensated
cancelled
manual_intervention
```

Each forward and compensation step is a normal durable job with its own attempts, claim fencing and verification policy.

---

## Saga Step Rules

Every saga step declares whether it is:

```text
idempotent
compensatable
irreversible
read_only
```

Rules:

- dependencies are explicit and durable;
- completed steps are never inferred only from process memory;
- compensation runs in reverse dependency order unless the saga definition explicitly proves another safe order;
- compensation is itself idempotent and verified;
- irreversible steps are placed as late as practical;
- a workflow containing an irreversible step must define its failure and operator-recovery behavior before implementation;
- an unknown forward outcome is reconciled before compensation;
- compensation never runs merely because a transport response was lost;
- failed compensation enters `partially_compensated` or `manual_intervention` with retained evidence;
- a compensated saga does not erase the original operation or audit history.

Examples of future saga-shaped work may include:

- prepare metadata asset, publish asset, assign asset, remove obsolete asset;
- cross-storage Recording transfer, verify destination, switch binding, retire source;
- multi-backend Timer intent assignment and reconciliation.

A simple single-backend Recording rename should remain a single mutation plus verification rather than being inflated into an unnecessary saga.

---

## Transaction Boundaries

The local database provides atomicity only for local durable state.

Required transaction boundaries include:

- operation and first job creation in one transaction, or an equivalent transactional outbox;
- atomic job claim and attempt creation;
- atomic claim renewal and fenced state update;
- atomic retry scheduling and attempt completion;
- atomic dependency release after a successful step;
- atomic saga state and next-step creation;
- atomic job result and corresponding operation transition where they share one database authority.

External VDR, Agent, provider, network and filesystem effects cannot be included in the SQLite transaction.

Safety across that boundary comes from:

- ADR-0042 idempotency;
- backend generation fencing;
- job claim fencing;
- durable dispatch boundary;
- executor receipt identity;
- authoritative readback;
- reconciliation;
- compensation where explicitly defined.

---

## Backend Agent Command Contract

A remote Agent command derived from a job carries at least:

```text
operationId when applicable
jobId
attemptId
claimEpoch
commandId
idempotencyKey when mutating
backendId
backendGeneration
resource identity and expected revision
payload type and version
verification policy
deadline
```

The Agent must reject a command when:

- backend identity does not match;
- backend generation is obsolete;
- its Backend Agent lease does not authorize new execution;
- the command deadline has expired;
- the command payload version is unsupported;
- the same command identity was already completed with a different fingerprint;
- local capability or safety policy forbids execution.

The Control Plane rejects state-changing results that do not match the current job attempt, claim epoch and backend generation.

Late or stale results may be retained as evidence for reconciliation but cannot directly finalize current state.

---

## Offline Agent and Reconnect Model

The Control Plane job queue remains authoritative.

A Backend Agent may maintain a durable local command inbox and result outbox to survive process or network interruption.

The local Agent store contains only assigned command receipts, execution boundary facts and results. It is not a second global scheduler and does not invent new caller operations.

Rules:

- a command is durably acknowledged before the Control Plane treats it as accepted by the Agent;
- the Agent stores the exact command identity and request fingerprint;
- a duplicate identical command returns the existing receipt or result;
- a duplicate command identity with a different fingerprint is rejected;
- if the Agent lease expires before dispatch, the Agent must not begin a new mutation;
- if dispatch already began, the Agent may complete bounded local verification and persist the result for reconnect;
- a disconnected Agent does not start queued future mutations after its execution authority or deadline expires;
- on reconnect, outstanding receipts and results are reconciled before the Control Plane considers redispatch;
- old-generation commands and results never become authoritative for a new generation;
- unknown outcomes remain `waiting_reconciliation` until state or command receipts prove the result;
- non-mutating offline work requires an explicit policy and must not delay VDR shutdown.

This model closes the architectural direction for durable offline queues while preserving Control Plane authority.

---

## VDR Plugin Boundary

The VDR plugin is not the job scheduler, retry engine or saga coordinator.

The plugin may:

- validate one bounded VDR-native request;
- resolve and copy native identities under the correct VDR lock;
- execute one bounded native step through a documented safe VDR boundary;
- return a deterministic local receipt or result;
- perform immediate bounded native readback where safe;
- publish local capability and verification facts.

The plugin must not:

- claim global jobs;
- store the authoritative durable retry schedule;
- run unbounded retry loops;
- hold VDR locks while waiting for Agent or Control Plane communication;
- block VDR status callbacks on command execution;
- own user authorization or cross-site policy;
- silently repeat a native mutation after an unknown outcome;
- become a second public API or Control Plane.

Long-running work belongs in Suite or Agent workers. VDR callbacks remain non-blocking. Plugin mutations remain disabled until the relevant ADR-0042 and ADR-0043 runtime foundations, generation fencing, revision checks, idempotency, readback and live VDR acceptance are implemented.

---

## Worker Shutdown and Restart

Workers stop safely through a bounded sequence:

1. stop claiming new jobs;
2. persist shutdown intent and stop starting new side effects;
3. finish or checkpoint bounded in-progress local work where policy permits;
4. renew claims only while the worker can still honor them;
5. release claims that are proven `not_started`;
6. preserve `starting` or later attempts for reconciliation;
7. exit without indefinite dependency on a backend or Agent.

On restart:

- expired claims are scanned;
- stale workers are fenced by claim epoch;
- `not_started` attempts may become claimable;
- `starting` or later attempts become reconciliation candidates;
- due retries are made eligible;
- saga dependency and compensation state is rebuilt from durable records, not process memory.

---

## Progress and Heartbeats

Progress is diagnostic and user-facing evidence, not mutation authority.

A running attempt may report bounded progress fields such as:

```text
phase
completedUnits
totalUnits
messageCode
lastProgressAt
```

Rules:

- progress updates are fenced by the active claim;
- progress text is bounded and does not contain secrets;
- progress does not prove backend mutation success;
- absence of progress is not by itself proof of failure while the lease remains valid;
- large logs and artifacts are stored outside the primary job row with retention rules;
- the final state depends on result and verification, not percentage.

---

## Error Categories

Job execution preserves structured categories suitable for later ADR-0048 API mapping and ADR-0049 audit events.

Minimum internal categories include:

```text
invalid_payload
unsupported_payload_version
dependency_unavailable
backend_offline
backend_generation_conflict
claim_lost
claim_expired
deadline_expired
resource_revision_conflict
capability_unavailable
forbidden
rate_limited
resource_busy
executor_rejected
executor_failed
verification_failed
outcome_unknown
compensation_failed
cancelled
```

Native or transport-specific detail may be retained as bounded diagnostics. Error strings alone do not control retries.

---

## Observability and Audit Correlation

Every job, attempt, Agent command and saga event carries correlation identity.

Important events include:

```text
job_created
job_claimed
claim_renewed
claim_expired
dispatch_starting
executor_accepted
verification_started
verification_succeeded
verification_failed
retry_scheduled
reconciliation_scheduled
cancel_requested
job_cancelled
job_succeeded
job_failed_terminal
job_dead_lettered
saga_step_started
saga_compensation_started
saga_manual_intervention
```

Logs and future audit events include relevant bounded fields:

```text
operationId
jobId
attemptId
sagaInstanceId
claimEpoch
backendId
backendGeneration
resourceType
resourceId
jobType
payloadVersion
resultCategory
errorCategory
```

Credentials, tokens, private key material, raw VDR pointers and unrestricted payload dumps are never logged.

---

## Existing Foundation Mapping

The existing implementation is retained as early evidence, not declared production-ready:

| Existing foundation | Role under ADR-0043 |
| --- | --- |
| `jobs` table | Legacy persistence foundation requiring migration. |
| `Job` | Early value model requiring stable identity, scheduling and claim fields. |
| `JobRepository` | Repository boundary to extend with atomic claim and fenced writes. |
| `JobService` | Early creation service to replace with versioned job construction. |
| `WorkerSimulator` | Test fixture and historical lifecycle demonstration, not the production worker. |
| `RecordingWorkflowService` | Early workflow entry point to adapt to operation and job transactions. |
| `RecordingActionJobPayload` | Domain payload foundation requiring operation, revision, generation and payload-version integration. |
| Jobs REST and dashboard views | Read-model foundation requiring stable public status mapping later through ADR-0048. |
| SearchTimer verification results | Evidence for executor-versus-readback separation. |
| Backend lifecycle and generation | Required Agent authority boundary from ADR-0040. |
| ADR-0042 operations and idempotency | Required mutation identity and final outcome authority. |

The legacy uppercase status vocabulary may be migrated as:

```text
PENDING   -> queued
RUNNING   -> running, after reconstruction of claim evidence
DONE      -> succeeded
FAILED    -> failed_terminal or dead_letter after classification
CANCELLED -> cancelled only when the safe boundary is proven
```

Migration must not invent missing attempt or dispatch evidence. Ambiguous legacy `RUNNING` work is treated conservatively and requires operator or reconciliation policy.

---

## Implementation Sequence

Implementation follows bounded slices:

1. introduce shared job, attempt, claim, dispatch, verification and retry value contracts;
2. add versioned schema migrations for jobs, attempts, job events and concurrency ownership;
3. implement atomic claim, claim renewal, expiry recovery and fenced repository writes;
4. add a real worker runtime with deterministic clock tests and graceful shutdown;
5. implement structured retry classification, persisted backoff and dead-letter handling;
6. integrate ADR-0042 operation and idempotency storage transactionally;
7. adapt Recording actions first, including readback and unknown-outcome reconciliation;
8. add cancellation requests and safe cancellation checkpoints;
9. add Agent command inbox, result outbox and reconnect reconciliation;
10. add versioned saga definitions, step dependencies and compensation execution;
11. adapt metadata, Timer and SearchTimer workflows in their ordered domain phases;
12. expose versioned public job, operation and error views through ADR-0048;
13. emit actor, claim, dispatch, retry, reconciliation and compensation audit events through ADR-0049.

No production remote mutation worker may bypass the operation, claim, fencing and reconciliation sequence.

---

## Required Tests

The runtime implementation must include at least:

### Repository and claim tests

- two workers racing for one job produce exactly one valid claim;
- stale `claimToken` and `claimEpoch` writes fail;
- claim renewal works only for the active owner;
- expired claims cannot be resurrected;
- concurrency keys prevent incompatible claims;
- transaction rollback leaves neither an orphan attempt nor a half-claimed job.

### Crash-boundary tests

- crash before `starting` permits safe retry;
- crash immediately after `starting` schedules reconciliation;
- crash after executor receipt does not redispatch blindly;
- late stale-worker completion cannot finalize a newer attempt;
- restart rebuilds due retries and saga dependencies from durable state.

### Retry tests

- structured error category selects the correct policy;
- persisted backoff and maximum attempts are enforced;
- deadlines block new dispatch without erasing unknown outcomes;
- revision and generation conflicts do not automatically retry;
- deterministic jitter is testable;
- one offline backend does not block another backend.

### Cancellation tests

- cancellation before dispatch becomes safely cancelled;
- cancellation after dispatch requires verification or compensation;
- cooperative cancellation cannot bypass final readback;
- stale workers cannot acknowledge cancellation.

### Saga tests

- dependencies release exactly once;
- compensation runs in the declared safe reverse order;
- unknown forward outcome blocks compensation until reconciliation;
- compensation retry is idempotent;
- failed compensation enters manual intervention with complete evidence;
- irreversible boundaries are enforced.

### Agent and plugin boundary tests

- obsolete backend generations reject commands and results;
- duplicate identical Agent command returns the existing receipt;
- same command identity with a different fingerprint is rejected;
- reconnect reconciles receipts before redispatch;
- plugin callback paths remain non-blocking;
- plugin execution does not hold VDR locks during Agent communication;
- no write capability is advertised before complete live acceptance.

---

## Rules

- A job is claimed atomically.
- Every new claim increments a monotonic fencing epoch.
- Worker writes are fenced by job, attempt, claim epoch and token.
- Backend Agent lease and job claim lease remain separate.
- Durable dispatch state is written before external execution.
- If dispatch may have started, reconciliation precedes redispatch.
- Retry classification is structured and persisted.
- Retry preserves the same operation identity and idempotency scope.
- Revision and generation conflicts require refresh, not blind retry.
- Cancellation is terminal only at a proven safe boundary.
- Saga steps and compensations are durable jobs.
- Unknown outcomes are reconciled before compensation.
- The Control Plane owns the authoritative queue.
- An Agent may persist command receipts and results, but not invent global operations.
- The VDR plugin executes bounded native steps and owns no global retry loop.
- Every durable payload is versioned.
- Failed and dead-letter records are retained for recovery and audit.
- Existing job foundations are not marked production-ready by this ADR alone.

---

## Consequences

Positive:

- prevents two workers from executing the same job concurrently;
- fences stale workers and obsolete Agent generations;
- makes crash recovery deterministic;
- prevents destructive blind retries after lost responses;
- supports remote and offline Agent reconciliation;
- separates caller-visible operation state from internal scheduler mechanics;
- provides a safe foundation for metadata, Recording, Timer and SearchTimer workflows;
- supports explicit compensation without pretending to provide distributed transactions;
- keeps VDR-native execution narrow and non-blocking.

Trade-offs:

- requires substantial schema and repository migration;
- introduces more durable states and operator tooling;
- reconciliation can delay final user-visible results;
- workers and Agents require clock, lease and fencing tests;
- saga definitions and compensation need careful domain design;
- legacy job rows may not contain enough evidence for automatic recovery;
- durable inbox and outbox storage increases Agent implementation complexity.

---

## Non-Goals

This ADR does not define:

- exact retry delays or lease durations;
- one mandatory queue product outside the existing persistence architecture;
- the final SQL migration syntax;
- the final public `/api/v1` job representation;
- user-facing wording for retry or compensation;
- one generic compensation for every domain action;
- immediate runtime implementation of jobs, sagas or Agent queues;
- permission roles and audit retention, which are completed by ADR-0049 and Phase 62;
- Timer intent scheduling rules, which are defined by ADR-0044;
- plugin mutation enablement.

---

## Related Decisions

- [ADR-0002: SQLite as Central Metadata Database](ADR-0002-sqlite.md)
- [ADR-0026: External Orchestration Layer Above VDR](ADR-0026-external-orchestration-layer-above-vdr.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](ADR-0042-safe-mutation-revision-idempotency-contract.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Current State](../CURRENT.md)
- [Back to Documentation Index](../index.md)
- [Back to README](../../README.md)
