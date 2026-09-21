# ADR-0044: Timer Intent, Assignment and Native Timer Model

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Current State](../CURRENT.md)
- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Strict Roadmap](../planning/roadmap.md)
- [ADR-0015: Timer Operation Boundary](ADR-0015-timer-operation-boundary.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0043: Job Claim, Retry and Saga Execution Model](ADR-0043-job-claim-retry-saga-execution-model.md)

---

## Status

Accepted

Date: 2026-07-16

---

## Context

VDR-Suite already contains substantial Timer foundations:

- backend-scoped `VdrTimer` read models;
- snapshot and change-feed support for observed Timer state;
- explicit create, update, delete and toggle action types;
- `VdrTimerOperationRequest` values required by VDR and RESTfulAPI transports;
- backend adapter registries and server-side access decisions;
- real-backend Timer lifecycle smoke tests;
- Timer conflict read models;
- SearchTimer definitions and verified SearchTimer workflows;
- read-only SearchTimer automation plans, duplicate detection and candidate Timer proposals.

These components prove that VDR-Suite can read backend-native Timers and can perform bounded native Timer actions through adapter boundaries.

They do not yet provide a durable multi-backend scheduling model.

The current architecture still mixes or leaves implicit several different concepts:

```text
what the user wants recorded
which backend should own that recording
which native VDR Timer currently implements the decision
which automation rule proposed the recording
which operation and job changed the native Timer
what should happen when a backend becomes unavailable
```

A backend-native Timer is not the same thing as the user's durable recording intent.

A SearchTimer is also not the same thing as either of them. A SearchTimer is a rule or automation provider definition that can discover programme matches. It may propose recording work, but it must not become an independent global scheduler that directly owns multi-backend native Timer mutations.

Without an explicit separation, VDR-Suite cannot safely answer:

- whether one or more native Timers represent the same user intent;
- which backend currently owns that intent;
- whether a native Timer was created by VDR-Suite, by LIVE, by epgsearch or manually in VDR;
- whether a missing native Timer should be recreated, reassigned or left alone;
- whether failover would create a duplicate recording;
- whether an externally edited Timer should be accepted or restored;
- whether a SearchTimer match has already produced an equivalent TimerIntent;
- whether an old Agent generation may still complete a Timer command;
- whether a timeout means the native Timer was never created or was created but not acknowledged.

This separation is mandatory before Phase 64 multi-backend Timer orchestration and before production remote Timer writes.

---

## Decision

VDR-Suite adopts three distinct durable Timer concepts:

```text
TimerIntent
  durable desired recording outcome

TimerAssignment
  durable scheduler decision assigning the intent to a backend

NativeTimerBinding
  observed binding to one backend-native VDR Timer
```

The model also preserves automation-provider definitions such as SearchTimer as a fourth, separate concern:

```text
Automation provider or rule
  discovers or proposes TimerIntents
```

The Control Plane owns TimerIntents, TimerAssignments, scheduling policy and reconciliation.

The Backend Agent transports generation-bound native Timer commands and observations.

The VDR plugin or another backend adapter owns only bounded, safe access to backend-native Timer state and operations.

No frontend, SearchTimer provider, plugin, REST controller or Backend Agent may bypass the central intent and assignment model once the Phase 64 runtime is active.

---

## Core Identity Model

The Timer architecture distinguishes these identities:

| Identity | Meaning |
| --- | --- |
| `timerIntentId` | Stable Suite identity for one durable desired recording outcome or schedule policy. |
| `timerAssignmentId` | Stable Suite identity for one scheduler decision assigning an intent to one backend. |
| `nativeTimerBindingId` | Stable Suite identity for one observed binding to a backend-native Timer. |
| `backendNativeTimerId` | Backend-provided Timer identity, meaningful only within the backend and native identity scope. |
| `automationSourceId` | Optional identity of the SearchTimer, rule, provider or workflow that proposed the intent. |
| `programEventId` | Future canonical programme-event identity defined by ADR-0045. |
| `backendEventRef` | Backend-scoped EPG event reference used until or alongside canonical event mapping. |
| `operationId` | ADR-0042 identity for one logical mutation. |
| `jobId` | ADR-0043 identity for one durable schedulable work item. |

These identities are never interchangeable.

A backend-native Timer number or RESTfulAPI Timer ID is not a globally stable TimerIntent identity.

A mutable Timer line, title, channel name or start time is not a sufficient identity.

---

## TimerIntent

A TimerIntent represents the durable desired outcome independent of the backend-native implementation.

Examples:

- record one known programme event;
- record one manually specified channel and time window;
- maintain one explicitly requested recurring schedule;
- record one SearchTimer match after duplicate and policy evaluation;
- create two deliberate redundant recordings under an explicit redundancy policy.

A TimerIntent is owned by VDR-Suite even while no backend is currently capable of executing it.

### TimerIntent fields

A TimerIntent carries at least:

| Field | Meaning |
| --- | --- |
| `timerIntentId` | Stable Suite identity. |
| `intentRevision` | Opaque optimistic-concurrency revision. |
| `intentType` | Event, manual window or recurring schedule. |
| `state` | Durable intent lifecycle state. |
| `ownerActorId` | User, service or system actor owning the intent. |
| `createdByActorId` | Actor that created the intent. |
| `automationSourceType` | Optional provider type such as SearchTimer, manual UI or imported rule. |
| `automationSourceId` | Optional stable source definition identity. |
| `programEventId` | Optional canonical programme event. |
| `backendEventRef` | Optional backend-scoped event reference and provenance. |
| `channelRequirement` | Backend-neutral or source-qualified desired channel. |
| `schedule` | Absolute event window, manual window or recurring policy. |
| `recordingOptions` | Margins, priority, lifetime, VPS preference, directory and naming policy. |
| `assignmentPolicy` | Backend preferences, exclusions and failover behavior. |
| `replicaPolicy` | Explicit desired number and purpose of assignments. |
| `duplicatePolicy` | Duplicate-detection and operator-review rules. |
| `createdAt` | Creation timestamp. |
| `updatedAt` | Last durable update timestamp. |
| `expiresAt` | Optional point after which an unfulfilled intent is no longer schedulable. |

The exact persistence schema may normalize these fields, but their ownership and semantics are mandatory.

### TimerIntent types

The canonical intent types are:

```text
programme_event
manual_window
recurring_schedule
```

`programme_event` targets one programme occurrence and should use a canonical `programEventId` when ADR-0045 mapping is available. Until then, it retains explicit backend event evidence and schedule values.

`manual_window` targets an explicitly selected channel requirement and absolute time window without requiring EPG identity.

`recurring_schedule` represents an explicit recurring user schedule. It is not automatically equivalent to a SearchTimer rule. A SearchTimer may generate occurrence intents, while a recurring schedule directly expresses a Timer policy.

New intent types require an architecture-compatible identity, deduplication and reconciliation definition.

### TimerIntent lifecycle

The canonical states are:

| State | Meaning |
| --- | --- |
| `draft` | Intent is editable and not eligible for assignment. |
| `active` | Intent is eligible for scheduling and reconciliation. |
| `paused` | Intent remains durable but no new assignment work begins. |
| `satisfied` | Desired recording outcome is fulfilled or deliberately completed. |
| `cancel_requested` | Cancellation is durable and assignment cleanup is pending. |
| `cancelled` | Intent was safely cancelled and managed assignments were reconciled. |
| `expired` | Scheduling window elapsed without a valid completion path. |
| `failed` | Policy or operator intervention concluded the intent cannot be fulfilled. |

Intent state is distinct from:

- operation state under ADR-0042;
- job state under ADR-0043;
- assignment state;
- observed native Timer state;
- recording state.

An intent is not marked `satisfied` merely because a native Timer create request returned HTTP success. Satisfaction requires the policy-defined outcome, normally a verified native binding and eventually the intended recording result.

---

## TimerAssignment

A TimerAssignment is a durable scheduler decision binding one TimerIntent to one target backend.

It records why the backend was chosen, which capability and channel evidence was used, and which backend generation is allowed to receive the current native command.

A TimerAssignment is not itself a native VDR Timer.

### TimerAssignment fields

A TimerAssignment carries at least:

| Field | Meaning |
| --- | --- |
| `timerAssignmentId` | Stable Suite assignment identity. |
| `assignmentRevision` | Opaque concurrency revision. |
| `timerIntentId` | Owning TimerIntent. |
| `intentRevision` | Intent revision used to compute the assignment. |
| `assignmentEpoch` | Monotonic ownership epoch for the intent assignment decision. |
| `backendId` | Stable target backend. |
| `backendGeneration` | Generation against which current execution was prepared. |
| `state` | Durable assignment lifecycle state. |
| `role` | Primary, deliberate replica or replacement. |
| `channelBinding` | Selected backend-native channel and mapping evidence. |
| `capabilityRevision` | Capability evidence used by the scheduler. |
| `backendHealthRevision` | Health or snapshot evidence used by the scheduler. |
| `decisionPolicyVersion` | Scheduler policy version. |
| `decisionEvidence` | Bounded reasons, scores, exclusions, warnings and conflict facts. |
| `nativeTimerBindingId` | Optional current managed native binding. |
| `createdAt` | Assignment creation timestamp. |
| `updatedAt` | Last assignment transition timestamp. |

### TimerAssignment lifecycle

The canonical states are:

| State | Meaning |
| --- | --- |
| `proposed` | Scheduler has produced a candidate requiring confirmation or further validation. |
| `selected` | Durable backend decision exists; no native dispatch has begun. |
| `provisioning` | Safe-mutation operation and jobs are creating or updating the native Timer. |
| `bound` | Required native Timer exists and readback matches the assignment. |
| `reconciling` | Native state, generation or assignment evidence requires reconciliation. |
| `unassigned` | No current backend can safely own the intent. |
| `superseding` | A replacement assignment is being prepared under controlled handover. |
| `superseded` | Assignment no longer owns the active intent. |
| `cancel_requested` | Assignment cleanup was requested. |
| `cancelled` | Managed native state was safely removed or ownership was deliberately released. |
| `failed` | Assignment cannot continue automatically and requires rescheduling or operator action. |

A selected assignment does not imply that a native Timer exists.

A bound assignment requires authoritative native readback.

### Single-owner default

By default, one active TimerIntent has at most one owning primary assignment.

This invariant is enforced by durable persistence and assignment epoch, not by frontend convention.

The relevant active ownership states are:

```text
selected
provisioning
bound
reconciling
superseding
```

A new primary assignment cannot become active while an older assignment may still own a live native Timer unless the controlled handover and duplicate policy explicitly permit it.

---

## Explicit Redundancy

Multiple active assignments are allowed only through an explicit `replicaPolicy`.

Examples:

- record the same important event on two sites;
- create a primary and deliberate backup recording;
- retain geographically separate copies.

A replica policy defines at least:

```text
desired assignment count
allowed backend diversity
required site diversity
whether simultaneous recording is intentional
storage and retention expectations
operator-visible rationale
```

Every replica receives a distinct TimerAssignment with role and evidence.

Accidental duplicates are never reclassified as redundancy after the fact merely to hide an orchestration error.

---

## NativeTimerBinding

A NativeTimerBinding represents one observed relationship between a Suite assignment or external observation and a backend-native VDR Timer.

It contains copied values, never VDR pointers or lock-owning objects.

### NativeTimerBinding fields

A binding carries at least:

| Field | Meaning |
| --- | --- |
| `nativeTimerBindingId` | Stable Suite binding identity. |
| `bindingRevision` | Opaque revision for binding and observed-state changes. |
| `backendId` | Owning backend. |
| `backendGeneration` | Generation that produced or most recently confirmed the binding. |
| `backendNativeTimerId` | Native Timer identity in the backend scope. |
| `timerAssignmentId` | Optional managed assignment. |
| `ownership` | Managed, adopted, external, orphaned or ambiguous. |
| `observedFingerprint` | Stable normalized fingerprint of relevant native fields. |
| `observedState` | Current copied native Timer state. |
| `lastObservedAt` | Last authoritative observation. |
| `lastVerifiedOperationId` | Optional operation whose result was verified by this readback. |
| `missingSince` | Optional first observation of absence. |
| `driftState` | None, expected transition, external edit or unresolved mismatch. |

`backendNativeTimerId` is never interpreted without `backendId` and the applicable native identity scope.

A backend restart may preserve native Timer IDs while changing `backendGeneration`. The binding model must distinguish stable native data from the Agent runtime generation that reported it.

### Ownership classification

The canonical ownership classes are:

| Ownership | Meaning |
| --- | --- |
| `managed` | Native Timer was created or explicitly taken over for one active assignment. |
| `adopted` | Existing native Timer was explicitly and safely linked to an intent and assignment. |
| `external` | Native Timer is visible but owned outside VDR-Suite. |
| `orphaned_managed` | Evidence says VDR-Suite formerly managed it, but the current intent or assignment relationship is incomplete. |
| `ambiguous` | More than one possible intent or native identity match exists. |

External Timers are read-only from the scheduler's perspective until explicit adoption or an authorized direct native action.

The reconciler must not delete, update or move an external Timer merely because it resembles a TimerIntent.

---

## Import and Adoption

VDR-Suite must expect native Timers created by:

- the VDR OSD;
- LIVE;
- epgsearch;
- another VDR plugin;
- SVDRP administration;
- RESTfulAPI clients;
- older VDR-Suite versions;
- manual configuration or migration.

Initial discovery imports them as `external` unless durable managed evidence proves otherwise.

### Adoption rules

Adoption is an explicit, authorized operation.

Before adoption, VDR-Suite must verify:

- current backend and generation;
- stable native Timer identity;
- native Timer revision or fingerprint;
- selected TimerIntent;
- absence of another managed assignment or binding;
- schedule, channel and recording-option compatibility;
- current recording and pending state;
- capability and policy permission;
- operator warnings for plugin-specific `aux` or unsupported flags.

Approximate title and time similarity alone is insufficient for automatic adoption.

An ambiguous match requires operator review or remains external.

Adoption never rewrites native state unless a separate ADR-0042 mutation is explicitly requested.

---

## SearchTimer and Automation Provider Boundary

SearchTimer remains a backend/provider rule definition.

It may:

- discover matching backend EPG events;
- evaluate filters, repeat prevention and recording options;
- produce candidate Timer proposals;
- attach duplicate evidence and operator-review requirements;
- propose one or more TimerIntents.

It does not:

- choose the final multi-backend owner independently;
- bypass TimerIntent persistence;
- directly create native Timers once central orchestration is active;
- own failover or cross-site reconciliation;
- treat a backend-native SearchTimer ID as a TimerIntent ID.

The provider pipeline becomes:

```text
SearchTimer or another automation source
  -> match candidate
  -> duplicate and policy evaluation
  -> TimerIntent proposal
  -> accepted durable TimerIntent
  -> central scheduler
  -> TimerAssignment
  -> ADR-0042 operation
  -> ADR-0043 jobs
  -> Backend Agent and native executor
  -> native readback
  -> reconciled NativeTimerBinding
```

Existing read-only automation proposal types are retained as evidence and can become inputs to TimerIntent creation.

No current read-only proposal automatically becomes executable merely because ADR-0044 is accepted.

---

## Scheduler Ownership

The scheduler is a VDR-Suite Control Plane component.

It evaluates active TimerIntents against current backend evidence.

It does not run inside:

- a frontend;
- an API controller request handler as unbounded work;
- a VDR status callback;
- the VDR plugin;
- a backend adapter lock scope;
- an independent SearchTimer provider.

Slow evaluation and assignment work uses the ADR-0043 job model.

### Scheduler inputs

The scheduler considers at least:

- TimerIntent and revision;
- current assignment and assignment epoch;
- backend lifecycle, generation, lease and health;
- backend Timer capabilities and capability revision;
- backend access mode and authorization policy;
- channel mapping and availability;
- event mapping and provenance;
- Timer conflict evidence;
- storage availability and recording policy;
- site and backend preferences or exclusions;
- time remaining before the recording window;
- existing native Timers and managed bindings;
- existing recordings and duplicate evidence;
- explicit replica policy;
- operator-review requirements.

The scheduler must not infer current executability from a stale Boolean `online` value or a cached capability without freshness evidence.

### Scheduler decision evidence

Every assignment decision stores bounded, inspectable evidence:

```text
eligible backends
excluded backends and reasons
capability revision
health and generation evidence
channel mapping revision
conflict evidence
score or ordered preference
selected backend
policy version
warnings and operator-review requirement
```

The exact scoring algorithm may evolve, but its inputs and selected result are durable and auditable.

A scheduler deployment must not silently reinterpret existing assignments without a new policy version and reconciliation decision.

---

## Channel Mapping

Channel identities are backend-scoped.

A TimerIntent expresses a channel requirement through one or more of:

- canonical channel identity when available;
- source-qualified channel reference;
- programme-event identity and provenance;
- explicit allowed backend channel mappings.

A TimerAssignment selects one backend-native channel binding.

The assignment records:

```text
backendId
backend channel ID
channel mapping identity or revision
mapping evidence
selection timestamp
```

The same textual channel name on two backends is not sufficient proof that they carry the same service.

A stale or ambiguous channel mapping blocks automatic assignment or requires explicit operator review.

---

## Time and Schedule Semantics

TimerIntent schedule values use explicit absolute instants and timezone context where an occurrence is absolute.

Native VDR representations such as:

```text
day
weekdays
HHMM start
HHMM stop
flags
```

are adapter or plugin execution data below the assignment boundary.

The conversion from intent schedule to native Timer fields must define:

- timezone;
- daylight-saving transition behavior;
- cross-midnight windows;
- margins;
- recurring weekday semantics;
- VPS and event-time behavior;
- backend clock and schedule evidence.

The Control Plane must not compare or deduplicate native `HHMM` strings as if they were globally absolute timestamps.

---

## Recording Options

TimerIntent owns backend-neutral recording preferences where possible:

```text
start margin
stop margin
priority
lifetime
VPS preference
recording directory policy
file naming policy
retention policy reference
```

TimerAssignment records the normalized values selected for one backend.

NativeTimerBinding records what the backend actually accepted.

Unsupported options are handled through capability and policy decisions:

- block assignment;
- degrade with explicit warning where policy permits;
- select another backend;
- require operator review.

The scheduler must not silently discard a user preference that materially changes recording behavior.

Plugin-specific `aux` content remains below a provider or adapter mapping boundary. It is not promoted to an opaque public cross-backend field.

---

## Duplicate Detection

Duplicate detection is evidence-driven and must distinguish:

- duplicate TimerIntent;
- duplicate assignment;
- duplicate native Timer;
- existing completed Recording;
- deliberate replica;
- coincidental title or schedule similarity.

### Intent deduplication

An intent semantic key may include:

```text
owner or policy scope
intent type
canonical programme-event identity when available
channel requirement
normalized schedule
recording purpose
source rule and occurrence identity
replica policy
```

ADR-0045 supplies canonical programme-event identity and provenance.

Until canonical identity exists, duplicate decisions retain backend event references and confidence rather than pretending title and time are authoritative.

### Assignment uniqueness

Persistence enforces the single-owner default for active primary assignments.

A new assignment for the same intent is not activated merely because the scheduler runs twice.

The assignment epoch and operation idempotency key prevent repeated scheduling from creating duplicate native Timers.

### Native duplicate evidence

The reconciler compares current native Timers through stable backend identity, native IDs, normalized fields and assignment evidence.

A title/channel/time match may raise duplicate risk, but automatic destructive cleanup requires stronger ownership evidence.

---

## Timer Conflict Handling

Timer conflicts are scheduling evidence, not merely UI decoration.

Conflict evaluation may include:

- tuner capacity;
- overlapping Timers;
- encrypted channel constraints;
- channel/transponder sharing;
- backend plugin conflict information;
- priority and lifetime;
- current and imminent recording state;
- backend-specific limitations.

A backend may not expose complete future conflict information. The decision evidence must state whether conflict knowledge is:

```text
confirmed_clear
confirmed_conflict
partial
unavailable
stale
```

Policies may:

- block assignment;
- choose another backend;
- require operator review;
- accept a documented conflict risk;
- create deliberate redundancy where explicitly configured.

The scheduler must not silently lower priority, disable VPS or change channel to avoid a conflict unless the TimerIntent policy explicitly permits that transformation.

---

## Reconciler Ownership

The reconciler is a VDR-Suite Control Plane component using durable jobs.

It compares:

```text
desired TimerIntent state
current TimerAssignment state
observed NativeTimerBinding state
current backend generation, health and capabilities
operation and job outcomes
```

The reconciler is responsible for detecting:

- missing managed native Timers;
- extra external native Timers;
- duplicate managed bindings;
- native field drift;
- stale backend generation evidence;
- assignment without a binding;
- binding without a valid assignment;
- ambiguous create or delete outcomes;
- externally edited or deleted managed Timers;
- assignments whose backend can no longer execute the intent.

### Reconciliation actions

Possible outcomes include:

```text
no action
refresh evidence
mark external drift
accept external change after policy or operator decision
restore desired state through a new mutation
reassign to another backend
mark unassigned
request operator review
cancel obsolete managed native Timer
mark outcome unknown
```

Every mutating reconciliation action uses ADR-0042 and ADR-0043.

The reconciler never performs hidden direct backend writes.

---

## External Drift

A managed native Timer may be edited or deleted outside VDR-Suite.

The reconciler records drift instead of silently overwriting it.

Drift categories include:

```text
none
expected_transition
external_field_change
external_disable
external_delete
native_identity_changed
ambiguous
```

Policy options may include:

- accept the native state and revise the TimerIntent or assignment after authorization;
- restore the intended state through a new operation;
- pause reconciliation and request operator review;
- release management and classify the native Timer as external.

A running or imminent recording raises the safety level. Automatic delete, reassignment or schedule change is blocked unless a specifically accepted policy and native safety contract allow it.

---

## Failover and Reassignment

Failover is not blind duplicate creation.

### Before native dispatch

If an assignment has not crossed the native dispatch boundary and its backend becomes unavailable, it may be superseded and reassigned when policy permits.

The old assignment is durably closed or superseded before the replacement becomes the active owner.

### After verified native absence

If a managed native Timer is verified absent and sufficient time remains, the scheduler may create a replacement assignment under the same TimerIntent.

The replacement uses a new assignment identity and epoch while preserving the original intent.

### After possible dispatch or unknown outcome

If native Timer creation, update or deletion may have reached the backend, the intent enters reconciliation.

The system must first determine whether the old native Timer exists.

It must not create a Timer on another backend merely because the original response was lost.

### During active recording

VDR-Suite does not claim transparent failover of an already running recording.

A backend failure during recording is reported as a recording outcome and may trigger later recovery workflows, but it is not solved by creating a late duplicate Timer without explicit policy.

### Reassignment evidence

Every reassignment records:

```text
old assignment
old backend and generation
verified old native state or explicit unknown outcome
reason
new candidate evidence
new assignment epoch
operator decision where required
```

---

## Native Mutation Contract

Every managed native Timer create, update, delete or toggle uses ADR-0042.

The mutation envelope includes or resolves:

```text
operationId
idempotencyKey
actorId
backendId
backendGeneration
resourceType
resourceId
expectedRevision
action
normalized payload
verification policy
```

For Timer orchestration:

- `resourceType` identifies TimerAssignment or NativeTimerBinding as appropriate;
- `resourceId` is a stable Suite identity, not the native Timer number alone;
- expected revisions bind execution to the current intent, assignment and native evidence;
- the assignment is durable before native dispatch;
- a native command is fenced by current backend generation;
- duplicate delivery reuses the same operation and idempotency identity.

### Verification policy

Managed native Timer mutations default to authoritative readback:

| Action | Required verification |
| --- | --- |
| create | Exactly one expected native Timer binding exists. |
| update | The same managed binding has the expected normalized fields. |
| delete | The managed native Timer is absent. |
| toggle | The managed binding has the expected active state. |
| adopt | The selected native Timer still matches the reviewed binding evidence. |

A transport success or VDR method return is not sufficient when readback is required.

A timeout after dispatch becomes `outcome_unknown` and a reconciliation job, not an automatic second create.

---

## Job and Concurrency Model

Timer scheduling and native mutation use ADR-0043 jobs.

Typical jobs include:

```text
evaluate TimerIntent
build backend candidate set
persist TimerAssignment
validate assignment evidence
create native Timer
read back native Timer
reconcile binding
remove superseded native Timer
re-evaluate after backend lifecycle change
```

Recommended concurrency keys include:

```text
timer-intent:<timerIntentId>
timer-assignment:<timerAssignmentId>
native-timer:<backendId>:<nativeTimerBindingId>
backend-timer-mutations:<backendId>
```

The final key structure is implementation detail, but incompatible work for the same intent, assignment or native Timer must not execute concurrently.

A stale job claim, old assignment epoch or old backend generation cannot complete current assignment state.

---

## Backend Lifecycle Interaction

Backend generation, Timer assignment epoch and resource revision are distinct:

```text
backendGeneration
  Agent runtime generation

assignmentEpoch
  current ownership decision for one TimerIntent

intentRevision
  current desired intent version

assignmentRevision
  current assignment state version

bindingRevision
  current observed native binding version
```

All relevant values are checked before native dispatch.

A backend becoming `offline` does not automatically delete its assignments or native binding evidence.

Cached Timer state may remain visible with explicit staleness while live mutation becomes unavailable.

A newly connected backend generation must publish fresh Timer capabilities and a native Timer snapshot before the scheduler treats it as authoritative for reconciliation or new writes.

---

## Multi-Site Policy

TimerIntent can carry backend and site preferences without hard-coding one home VDR.

Examples:

- prefer the primary house;
- exclude a read-only remote backend;
- allow failover to a second site;
- require local storage;
- allow deliberate two-site redundancy;
- forbid metered or degraded connections;
- require a backend with VPS support.

The read-only backend policy remains a hard server-side boundary.

A scheduler score or user preference cannot override a backend mutation prohibition.

Remote internal endpoints such as RESTfulAPI, SVDRP or plugin commands remain behind the Backend Agent trust boundary.

---

## Control Plane, Agent and Plugin Boundary

### VDR-Suite Control Plane owns

- TimerIntent persistence and revision;
- TimerAssignment persistence, epoch and ownership;
- scheduler and reconciler;
- deduplication and conflict policy;
- failover and explicit replica policy;
- authorization and backend read-only enforcement;
- durable operation, job and audit state;
- final client-visible intent and assignment status.

### Backend Agent owns

- authenticated backend relationship;
- backend generation and lease participation;
- transport of native Timer commands and results;
- local capability and health publication;
- local native Timer snapshot forwarding;
- generation-bound command rejection;
- reconnect-safe command receipt and result delivery.

### VDR plugin or adapter owns

- safe VDR-native Timer access;
- VDR lock and thread-boundary correctness;
- copying native Timer values into bounded value contracts;
- bounded native create, update, delete and toggle execution when enabled;
- local validation that requires VDR-native state;
- authoritative native readback and deterministic local result facts;
- no raw VDR pointer or lock ownership crossing the boundary.

The plugin does not own:

- TimerIntent storage;
- assignment selection;
- cross-backend scheduling;
- failover;
- global duplicate policy;
- durable operation or job state;
- global retries or sagas;
- end-user authorization;
- public API contracts.

Plugin mutation capabilities remain disabled until the shared ADR-0042, ADR-0043 and ADR-0044 runtime prerequisites, generation fencing, revision checks, readback and live VDR acceptance are implemented.

---

## VDR Native Safety Rules

A native Timer implementation must preserve VDR semantics.

Required rules:

- use the VDR-recommended Timer and channel lock APIs;
- copy required native values while holding the shortest valid lock scope;
- release locks before serialization, Agent communication, database access or waiting;
- never expose `cTimer`, `cChannel`, `cEvent` or lock pointers outside the native boundary;
- validate recording, pending and imminent state before destructive changes;
- preserve VDR-specific delete behavior rather than replacing it with filesystem deletion;
- treat `flags`, VPS, recurring schedule, priority, lifetime, file and `aux` semantics deliberately;
- avoid blocking VDR callbacks or main-thread-sensitive paths;
- perform authoritative readback after mutation;
- make plugin capability state truthful and versioned;
- provide controlled live-VDR acceptance and rollback before enabling writes.

The Control Plane does not reimplement VDR lock behavior.

The plugin does not invent central scheduling policy.

---

## Capability Requirements

Timer orchestration consumes capability evidence, not backend implementation names.

Relevant capabilities may include:

```text
timer.read
timer.create
timer.update
timer.delete
timer.toggle
timer.readback
timer.conflict_read
timer.vps
timer.priority
timer.lifetime
timer.recurring
timer.event_binding
```

A capability has version, origin, freshness and current health evidence.

The existence of RESTfulAPI, SVDRP or a plugin transport does not imply write capability.

The scheduler treats a temporarily unavailable capability differently from a permanently unsupported feature.

A backend may remain eligible for read-only Timer observation while being ineligible for assignment.

---

## Authorization and Ownership

Creating or changing a TimerIntent is an authorization decision separate from executing a native Timer mutation.

Required policy questions include:

- may the actor create or edit this intent;
- may the actor target or prefer this backend or site;
- may automation create intents on the actor's behalf;
- may the scheduler execute native Timer writes on the selected backend;
- may the actor adopt or release an external native Timer;
- may the actor request redundancy or failover;
- may the actor cancel a running or imminent Timer assignment.

The owner actor and automation source are retained even when the scheduler later changes the backend assignment.

An automation provider uses a service or system identity and does not inherit unrestricted end-user permissions implicitly.

ADR-0049 defines the final audit and security event model.

---

## Public API Direction

The future public API is intent-first.

Primary client operations concern:

- creating and editing TimerIntents;
- viewing assignment and native fulfillment state;
- pausing, resuming or cancelling an intent;
- reviewing proposed assignments and conflicts;
- adopting an external native Timer;
- requesting explicit redundancy or failover where authorized.

Raw native Timer observations may remain available for diagnostics and compatibility, but they are not the durable user intent API.

Public clients do not directly choose RESTfulAPI, SVDRP or plugin commands.

Public clients do not submit a backend-native Timer line as the platform contract.

ADR-0048 defines versioning, errors, ETags and compatibility behavior.

---

## Persistence Direction

The implementation will require durable concepts equivalent to:

```text
timer_intents
timer_assignments
native_timer_bindings
timer_assignment_decisions
timer_reconciliation_runs
```

Relationships to operations, jobs, actors, backends, programme events and automation sources are explicit.

Every mutable row or aggregate has an optimistic-concurrency revision.

Assignment ownership and active-primary uniqueness are enforced transactionally.

Scheduler decisions and reconciliation facts are retained sufficiently for audit and operational recovery.

Exact SQL tables, indexes and migration layout are implementation decisions, but a schema that stores only one backend Timer row cannot satisfy this ADR.

---

## Existing Foundation Mapping

The current implementation is retained and mapped below the new model:

| Existing foundation | Role under ADR-0044 |
| --- | --- |
| `VdrTimer` | Observed backend-native Timer read-model foundation. |
| Timer snapshots and change feed | Native observation and reconciliation input. |
| `VdrTimerOperationRequest` | Native execution request below TimerAssignment and ADR-0042 boundaries. |
| Timer action types and services | Bounded native action execution foundation. |
| Timer adapter registry | Backend-specific executor resolution. |
| Backend access decision | Existing server-side read-only policy gate. |
| Timer conflict models | Scheduler and operator evidence. |
| SearchTimer | Automation-provider rule definition. |
| SearchTimer automation evaluation plan | Read-only provider evaluation foundation. |
| Candidate Timer proposal | Pre-Intent proposal and duplicate-evidence foundation. |
| SearchTimer verified execution | Existing executor-versus-readback evidence. |

These components are not discarded.

They must be placed at the correct layer rather than promoted into TimerIntent identity or global scheduling authority.

---

## Migration and Compatibility

Migration proceeds without breaking existing Timer reads.

The intended progression is:

1. continue exposing current backend-native Timer read models;
2. add Suite TimerIntent, TimerAssignment and NativeTimerBinding persistence;
3. import current native Timers as external observations;
4. identify existing VDR-Suite-managed Timers only where durable evidence exists;
5. add explicit adoption workflows;
6. route new managed Timer creation through TimerIntent and assignment;
7. migrate SearchTimer automation from proposal-only to intent production;
8. retain direct native Timer administration only as an explicitly scoped compatibility or operator path;
9. remove any remaining hidden provider-owned native write path after equivalent central orchestration exists.

Older clients may temporarily continue reading native Timers while newer clients use intent-first APIs.

Compatibility must never silently convert an external native Timer into a managed assignment.

---

## Failure and Error Semantics

Timer orchestration preserves distinct semantic categories:

```text
intent_conflict
assignment_conflict
no_eligible_backend
channel_mapping_unavailable
capability_unavailable
backend_read_only
backend_offline
generation_conflict
revision_conflict
duplicate_intent
duplicate_native_timer
native_timer_not_found
native_timer_drift
external_timer_requires_adoption
assignment_outcome_unknown
operator_review_required
conflict_information_unavailable
```

These become stable public errors only through ADR-0048.

Internally, the system must not collapse:

- no eligible backend;
- known native conflict;
- unavailable conflict information;
- external Timer ownership;
- stale assignment evidence;
- unknown native mutation outcome.

---

## Observability

Useful Timer orchestration diagnostics include:

```text
timerIntentId
intentRevision
timerAssignmentId
assignmentEpoch
backendId
backendGeneration
nativeTimerBindingId
backendNativeTimerId
automationSourceId
operationId
jobId
scheduler policy version
reconciliation reason
result category
```

Logs remain bounded and must not include credentials, raw VDR pointers or unrestricted `aux` dumps.

Operator views should explain:

- why a backend was chosen or rejected;
- whether native Timer state is verified or stale;
- whether the Timer is managed or external;
- why failover is blocked;
- whether duplicate risk or conflict information is incomplete;
- which operation or job is currently reconciling state.

---

## Test and Acceptance Standard

Runtime implementation must prove at least:

### Domain and persistence tests

- stable and non-interchangeable intent, assignment and binding IDs;
- optimistic-concurrency conflicts;
- one-active-primary enforcement;
- explicit redundancy as the only multi-owner path;
- deterministic assignment epoch changes;
- immutable scheduler decision evidence;
- external Timer protection;
- explicit adoption behavior;
- migration of existing observations without false ownership.

### Scheduler tests

- capability-aware backend selection;
- read-only and unavailable backend rejection;
- channel mapping evidence;
- deterministic policy-version behavior;
- duplicate detection;
- conflict evidence states;
- operator-review gating;
- no assignment when evidence is stale or ambiguous;
- deliberate replica selection.

### Mutation and reconciliation tests

- create with exactly-one native readback;
- update with normalized readback;
- delete with verified absence;
- timeout after dispatch producing unknown outcome;
- no blind redispatch;
- stale assignment epoch rejection;
- stale backend generation rejection;
- external edit drift;
- external deletion of a managed Timer;
- old assignment cleanup before replacement;
- crash recovery through ADR-0043 jobs.

### Multi-site tests

- same channel name with different identity does not auto-map;
- backend failure before dispatch permits safe reassignment;
- backend failure after possible dispatch requires reconciliation;
- read-only remote backend is never selected for writes;
- intentional two-site redundancy creates exactly two explicit assignments;
- reconnecting old Agent generations cannot complete current work.

### VDR plugin and live acceptance

Before native plugin writes are enabled:

- controlled disposable Timer resource;
- VDR-native lock and lifecycle review;
- create, readback, update, toggle and delete acceptance;
- recording and imminent-Timer safety cases;
- duplicate delivery and lost-response cases;
- plugin restart and Agent reconnect;
- rollback leaving no test Timer or stale plugin binary;
- proof that no real user Timer is mutated by automated CI.

---

## Implementation Sequence

Implementation follows bounded slices:

1. introduce stable TimerIntent, TimerAssignment and NativeTimerBinding IDs and state types;
2. add durable persistence, revisions and active-primary constraints;
3. import current `VdrTimer` observations as external bindings;
4. add explicit native Timer adoption and release workflows;
5. implement one manual TimerIntent through a local backend assignment and verified native create;
6. add scheduler candidate evidence, capability and channel mapping evaluation;
7. add reconciler and external drift handling;
8. route current native Timer updates and deletes through managed assignment operations;
9. convert SearchTimer candidate proposals into durable TimerIntent proposals;
10. activate accepted SearchTimer intents through the central scheduler;
11. integrate Backend Agent generation-bound command execution;
12. add safe reassignment and explicit redundancy;
13. expose final versioned API and audit contracts under ADR-0048 and ADR-0049.

No production remote Timer write may bypass ADR-0042, ADR-0043, backend trust and generation fencing.

---

## Rules

- TimerIntent, TimerAssignment and NativeTimerBinding are separate durable concepts.
- Backend-native Timer IDs are not global TimerIntent IDs.
- SearchTimer is an automation source, not the global assignment owner.
- One active primary assignment per intent is the default.
- Multiple active assignments require explicit replica policy.
- Assignment is durable before native dispatch.
- Bound assignment requires authoritative native readback.
- External native Timers are not mutated automatically.
- Adoption is explicit, authorized and revision-bound.
- Scheduler decisions use current generation, capability, health and channel evidence.
- Native writes use ADR-0042 operations and ADR-0043 jobs.
- Unknown native outcome requires reconciliation before reassignment or retry.
- Failover must not silently create duplicate Timers.
- VDR locks and native pointers remain inside the plugin or adapter boundary.
- Read-only backend policy remains a hard denial.
- Acceptance of this ADR does not implement Phase 64 runtime behavior.

---

## Consequences

### Positive

- user intent survives backend changes and outages;
- native Timers become traceable to durable Suite decisions;
- SearchTimer automation can participate without owning global writes;
- multi-site assignment and deliberate redundancy become explicit;
- external VDR and plugin-created Timers remain protected;
- failover can be reasoned about without blind duplicate creation;
- scheduler and reconciler decisions become observable and auditable;
- VDR-native safety remains below a clean Control Plane boundary.

### Trade-offs

- Timer orchestration requires more persistent entities and revisions;
- migration must classify existing native Timers conservatively;
- users may need to review ambiguous imports, conflicts or channel mappings;
- a scheduler and reconciler add operational complexity;
- exact programme-event deduplication depends on ADR-0045;
- production remote writes remain blocked until identity, RBAC, Agent and audit foundations are implemented.

---

## Non-Goals

This ADR does not:

- implement the Phase 64 scheduler or reconciler;
- define the exact SQL migration;
- choose final scheduler score weights;
- define canonical programme-event identity or provenance, which belongs to ADR-0045;
- define the final public API schema, which belongs to ADR-0048;
- define the final audit event schema, which belongs to ADR-0049;
- enable VDR plugin mutation capabilities;
- promise transparent failover after a recording has already started;
- define tuner hardware allocation algorithms for every backend;
- automatically adopt or delete existing external native Timers;
- interpret arbitrary plugin `aux` data as a backend-neutral public contract;
- replace VDR as the authority for native Timer execution semantics.

---

## Architecture Acceptance Criteria

This decision is accepted when the architecture explicitly guarantees:

- every managed native Timer is traceable to one assignment and intent;
- current native Timer reads remain available during migration;
- SearchTimer and other automation sources produce intents rather than bypassing central ownership;
- one active assignment is the default and redundancy is explicit;
- external Timers remain external until explicit adoption;
- native mutations are generation-bound, revision-bound, idempotent and verified;
- assignment failure and unknown outcome cannot silently create duplicates;
- scheduler and reconciler remain Control Plane responsibilities;
- Backend Agent and plugin responsibilities remain narrow and testable;
- runtime gaps G-13, G-14, G-18 and G-30 remain open until implementation, tests and Phase 64 exit criteria are complete.

---

## Related Decisions

- [ADR-0012: Source Capability Model](ADR-0012-source-capability-model.md)
- [ADR-0013: Permission Model](ADR-0013-permission-model.md)
- [ADR-0015: Timer Operation Boundary](ADR-0015-timer-operation-boundary.md)
- [ADR-0016: Snapshot Change Feed Architecture](ADR-0016-snapshot-change-feed-architecture.md)
- [ADR-0020: Multi-Source Federation Architecture](ADR-0020-multi-source-federation-architecture.md)
- [ADR-0029: Backend-Neutral SearchTimer Architecture](ADR-0029-backend-neutral-searchtimer-architecture.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0043: Job Claim, Retry and Saga Execution Model](ADR-0043-job-claim-retry-saga-execution-model.md)

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
- [Back to README](../../README.md)
