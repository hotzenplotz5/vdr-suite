# ADR-0049: Audit and Security Event Model

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [ADR Index](index.md)
- [Current State](../CURRENT.md)
- [Architecture Audit Gap Matrix](../planning/architecture-audit-gap-matrix.md)
- [Strict Roadmap](../planning/roadmap.md)
- [ADR-0013: Permission Model](ADR-0013-permission-model.md)
- [ADR-0039: Backend Agent and Control Plane Boundary](ADR-0039-backend-agent-control-plane-boundary.md)
- [ADR-0040: Backend Lifecycle, Generation, Lease and Health](ADR-0040-backend-lifecycle-generation-lease-health.md)
- [ADR-0041: Authentication, Agent Trust and Multi-Site Transport](ADR-0041-authentication-agent-trust-multi-site-transport.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0043: Job Claim, Retry and Saga Execution Model](ADR-0043-job-claim-retry-saga-execution-model.md)
- [ADR-0044: Timer Intent, Assignment and Native Timer Model](ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0045: Canonical EPG Event Identity and Provenance](ADR-0045-canonical-epg-event-identity-provenance.md)
- [ADR-0046: Streaming Gateway and Media Session Boundary](ADR-0046-streaming-gateway-media-session-boundary.md)
- [ADR-0047: Legacy OSD Compatibility Bridge](ADR-0047-legacy-osd-compatibility-bridge.md)
- [ADR-0048: Public API Versioning, Error and Compatibility Contract](ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [Historical Runtime Observability Strategy](008-runtime-observability-strategy.md)

---

## Status

Accepted

Date: 2026-07-16

---

## Context

VDR-Suite is intended to become a multi-user, multi-site Control Plane for VDR backends.

The platform will authorize and coordinate actions that may:

- create, change or delete native Timers;
- move, rename, trash, restore or purge Recordings;
- create and modify SearchTimers and automation rules;
- assign TimerIntents to remote backends;
- enroll, rotate or revoke Backend Agent credentials;
- change backend access mode and user permissions;
- create authenticated media sessions;
- grant temporary Legacy OSD control;
- execute asynchronous jobs and saga compensation;
- reconcile uncertain remote outcomes.

These actions require more than human-readable log messages.

The repository already contains useful observability and execution foundations:

- `IRuntimeLogger` and `RuntimeLogEntry`;
- console and null runtime loggers;
- structured runtime measurements and diagnostics;
- backend-scoped identities and access modes;
- request and correlation identity direction in ADR-0048;
- actor, operation and idempotency direction in ADR-0042;
- jobs, attempts, dispatch boundaries and sagas in ADR-0043;
- generation, lease and trust boundaries in ADR-0040 and ADR-0041;
- bounded plugin lifecycle and status logging outside VDR callbacks;
- early documentation goals for action origin and backend context.

These foundations do not yet provide a production audit or security-event model.

The current `RuntimeLogEntry` contains only:

```text
level
component
text
```

That is appropriate for human-readable runtime progress, but it cannot reliably answer:

```text
Who requested the action?
Which authenticated identity was used?
Which actor was effective after delegation?
Which backend and site were targeted?
Which backend generation was current?
Which stable resource and revision were involved?
Which policy and permission decision was made?
Was backend dispatch started?
Was the requested result verified?
Was the outcome uncertain?
Was the action retried, reconciled or compensated?
Was an Agent credential revoked or replay detected?
```

Free-form logs also cannot safely provide:

- stable event identities;
- append-only history;
- schema evolution;
- field-level redaction;
- durable correlation with operations and jobs;
- deterministic deduplication after Agent reconnect;
- scoped audit queries;
- retention by event category;
- reliable security alert input;
- proof that a denied or destructive action was recorded.

Logging, diagnostics, debugging, audit and security events must therefore remain separate concerns.

This ADR completes the target decision for architecture gap G-24. It also supplies the audit requirements referenced by G-04, G-08, G-12, G-22, G-23 and G-29.

Acceptance of this ADR does not mark the Phase 62 audit runtime as implemented.

---

## Decision

VDR-Suite adopts a durable, structured and append-only accountability event model owned by the Control Plane.

The model uses one shared immutable envelope named:

```text
AccountabilityEvent
```

Each event declares one or both semantic classes:

```text
audit
security
```

An event may therefore be:

```text
classes = [audit]
classes = [security]
classes = [audit, security]
```

This avoids creating two unrelated records when one occurrence is both an accountability fact and a security-relevant condition.

Examples:

```text
A permitted Recording rename
  -> audit

A failed login caused by an expired password
  -> security

An administrator revoking an Agent credential
  -> audit + security
```

The Control Plane owns:

- the canonical event identity;
- event persistence and append-only rules;
- actor and authorization context;
- public-resource and backend scope;
- retention and redaction policy;
- audit query authorization;
- security classification;
- event export and integration boundaries.

A Backend Agent may produce bounded source evidence and locally durable reconnect-safe event submissions for its own backend and Agent identity.

A VDR plugin may produce bounded native facts and local diagnostic logs, but it does not own global audit policy, user identity, authorization decisions, retention or the canonical event store.

---

## Separation from Other Observability Data

The following concepts are not interchangeable:

```text
Runtime Log
  human-readable progress and troubleshooting text

Runtime Measurement
  machine-readable latency, count or size measurement

Debug State
  current or recent diagnostic state

Domain Event
  state-change notification used by application workflows

AccountabilityEvent
  durable actor, decision, action and outcome evidence

Security Alert
  derived notification requiring attention

Security Incident
  investigated collection of evidence and response activity
```

### Runtime logs

Runtime logs may explain what a component is doing.

They are not the authoritative audit history.

Rules:

- audit queries do not parse console output;
- a log line does not prove durable audit persistence;
- log retention may differ from audit retention;
- log text may change without changing the audit schema;
- secrets remain forbidden in logs and audit records.

### Runtime measurements

Measurements answer questions such as latency, count and response size.

They do not identify a user decision or prove authorization.

### Debug state

Debug state may expose bounded current state for troubleshooting.

It is mutable and must not replace append-only history.

### Domain events

A domain event may trigger a refresh or workflow.

It is not automatically retained as audit evidence.

A domain event becomes an AccountabilityEvent only when the audit policy classifies the occurrence as requiring durable accountability or security evidence.

### Alerts and incidents

Security alerts and incidents are consumers of security events.

A security event does not automatically mean that an incident exists.

This ADR does not implement an alerting or incident-management platform.

---

## Canonical Event Envelope

Every AccountabilityEvent contains a stable core envelope.

Canonical fields are:

| Field | Meaning |
| --- | --- |
| `eventId` | Stable opaque VDR-Suite identity for one immutable event. |
| `schemaVersion` | Version of the stored event representation. |
| `classes` | One or both of `audit` and `security`. |
| `eventType` | Stable dotted event name. |
| `severity` | Security or operational importance classification. |
| `occurredAt` | Time the producer observed the occurrence. |
| `recordedAt` | Time the Control Plane durably accepted the event. |
| `producer` | Component and instance that produced the evidence. |
| `producerEventId` | Optional producer-local immutable identity. |
| `producerSequence` | Optional producer-local monotonic sequence. |
| `actor` | Authenticated, effective and delegated actor context. |
| `action` | Stable domain-level action name. |
| `decision` | Authorization or policy decision where applicable. |
| `outcome` | Observed action or security outcome. |
| `reasonCode` | Stable machine-readable reason. |
| `targets` | Stable Suite resource, backend, site or credential references. |
| `context` | Request, operation, job, Agent and session correlation references. |
| `changes` | Bounded revision or state-transition summary. |
| `evidence` | Bounded non-secret evidence references or summaries. |
| `retentionClass` | Configured retention category. |
| `sensitivity` | Access and redaction classification. |

The storage schema may normalize fields into related tables, but the logical contract remains one immutable event envelope.

### Opaque event identity

`eventId` is generated by VDR-Suite and is never derived from:

- log text;
- a timestamp alone;
- a native VDR event ID;
- a filesystem path;
- a username;
- an IP address;
- an operation ID;
- an Agent sequence number.

`eventId` does not replace the identities it references.

### Schema version

The event representation is durable data and may outlive the process that wrote it.

Rules:

- stored events carry an explicit schema version;
- existing field meaning is never silently changed;
- additive optional fields require documented defaults;
- incompatible changes require a new schema version;
- deterministic readers or upcasters may expose older events through a current query model;
- original evidence and event identity remain unchanged.

---

## Event Classes

### Audit class

An audit event records accountability for a meaningful decision, action or outcome.

It answers:

```text
who or what
requested, approved, denied or executed
which action
against which target
under which policy and revision
with which result
```

Audit events are required for:

- protected mutations;
- authorization and policy decisions of lasting consequence;
- identity, role, permission and trust changes;
- credential lifecycle changes;
- backend enrollment and access-mode changes;
- sensitive session creation and control grants;
- audit-log access and export;
- retention or deletion of audit data itself.

### Security class

A security event records a security-relevant condition or control result.

Examples include:

- authentication failure;
- repeated invalid credentials;
- revoked Agent credential use;
- unsupported or downgraded protocol attempt;
- backend identity mismatch;
- stale generation command;
- replay or idempotency conflict;
- authorization denial;
- read-only policy violation attempt;
- rate-limit enforcement;
- suspicious OSD control behavior;
- integrity or sequence violation;
- audit pipeline degradation.

A security event may have no user-requested mutation.

### Combined class

An event uses both classes when the same fact is relevant to accountability and security.

Examples:

- administrator changes another user's backend permission;
- administrator rotates or revokes an Agent credential;
- a forbidden destructive mutation is attempted;
- an audit export is created;
- an administrator changes audit retention policy.

---

## Event Type Naming

`eventType` uses stable lowercase dotted names.

Examples:

```text
authentication.login.succeeded
authentication.login.failed
authentication.session.created
authentication.session.revoked

authorization.allowed
authorization.denied

identity.user.created
identity.user.disabled
identity.service-account.created
identity.role.changed
identity.permission-grant.changed

backend.agent.enrollment.requested
backend.agent.enrolled
backend.agent.connected
backend.agent.disconnected
backend.agent.credential.rotated
backend.agent.credential.revoked
backend.agent.identity-mismatch
backend.agent.replay-detected
backend.generation.conflict
backend.access-mode.changed

mutation.requested
mutation.rejected
mutation.accepted
mutation.dispatch-started
mutation.executor-accepted
mutation.verified-succeeded
mutation.verified-failed
mutation.outcome-unknown
mutation.cancelled
mutation.compensation-started
mutation.compensation-completed
mutation.compensation-failed

job.claimed
job.retry-scheduled
job.waiting-reconciliation
job.dead-lettered
saga.started
saga.compensation-required
saga.completed

recording.action.requested
recording.action.completed

timer-intent.created
timer-assignment.changed
native-timer.binding.changed

media-session.created
media-session.denied
media-session.revoked
media-access-grant.issued
media-access-grant.denied

legacy-osd.session.created
legacy-osd.controller-lease.acquired
legacy-osd.controller-lease.denied
legacy-osd.input.accepted
legacy-osd.input.rejected
legacy-osd.controller-lease.released

audit.query.executed
audit.export.created
audit.retention-policy.changed
audit.events.purged
audit.pipeline.degraded
```

The exact initial catalogue is implemented incrementally in Phase 62 and later domain phases.

Rules:

- one name is never reused for a different meaning;
- event names describe domain facts, not C++ function names;
- plugin command names and RESTfulAPI paths do not become public audit event types;
- changing human-readable descriptions does not change `eventType`;
- high-volume diagnostic observations are not promoted to audit event types without policy review.

---

## Severity

Canonical severity values are:

```text
informational
low
medium
high
critical
```

Severity is not derived only from log level.

Examples:

- successful normal mutation: `informational`;
- ordinary authorization denial: `low` or `medium` according to policy;
- repeated credential failure or replay detection: `high`;
- audit store unavailable before privileged trust change: `critical`;
- confirmed credential compromise: `critical`.

Severity policy is versioned configuration.

Changing severity policy does not rewrite historical events.

---

## Actor Model

The audit model distinguishes authentication identity, effective authority and execution identity.

Canonical actor types include:

```text
user
service_account
api_client
backend_agent
system_worker
system_scheduler
administrator
external_native_actor
unknown_local_actor
```

An event actor context may include:

| Field | Meaning |
| --- | --- |
| `authenticatedActorId` | Identity whose credential authenticated the request or connection. |
| `effectiveActorId` | Identity whose authorization was evaluated for the action. |
| `delegatingActorId` | Optional actor delegating or approving authority. |
| `executorActorId` | Worker, Agent or system component executing the action. |
| `actorType` | Stable actor category. |
| `authenticationMethod` | Bounded method classification, never the credential secret. |
| `sessionRef` | Non-secret opaque session reference where permitted. |
| `serviceRef` | Calling service or API-client identity. |

### Actor rules

- a frontend label is not an actor identity;
- a request without authenticated identity does not invent a user;
- an Agent identity is not the user who requested a mutation;
- a worker identity is not the effective user actor;
- system automation retains its originating rule, TimerIntent or operation reference;
- delegated or administrative action records both authenticating and effective context;
- actor display names are not stable identity;
- credential material is never stored in the actor context.

### Native local activity

A physical remote control, local VDR keyboard, another plugin or an external SVDRP tool may change native state outside VDR-Suite.

When such activity is observed but not authenticated by VDR-Suite:

```text
actorType = external_native_actor
```

or:

```text
actorType = unknown_local_actor
```

VDR-Suite must not falsely attribute the action to the last logged-in Suite user.

Observed native drift is recorded as evidence or a reconciliation event, not as proof that a Suite actor requested the change.

---

## Target Model

An event may reference one or more stable targets.

Target types include:

- backend;
- site;
- Recording;
- TimerIntent;
- TimerAssignment;
- NativeTimerBinding;
- SearchTimer;
- ProgramEvent;
- metadata entity or assignment;
- operation;
- job;
- saga;
- user, role or permission grant;
- Agent identity or credential record;
- MediaSession or MediaAccessGrant;
- LegacyOsdSession or OsdControllerLease;
- audit export or retention policy.

A target reference includes, where applicable:

```text
resourceType
resourceId
resourceRevision
backendId
backendGeneration
siteId
nativeBindingReference
```

Rules:

- mutable filesystem paths are evidence, not stable target identity;
- titles and channel names are display summaries, not identity;
- native Timer numbers are backend-scoped evidence;
- target references are filtered when the querying actor may not see the resource;
- an event may retain a stable resource ID even after the resource is deleted;
- deleted-resource audit history does not require recreating the resource.

---

## Request and Workflow Correlation

ADR-0048 defines request and correlation IDs. ADR-0042 and ADR-0043 define operation and job identities.

The audit context keeps these values separate:

```text
requestId
correlationId
operationId
jobId
attemptId
sagaInstanceId
dispatchId
backendId
backendGeneration
agentConnectionId
mediaSessionId
routeEpoch
legacyOsdSessionId
osdControllerLeaseId
osdEpoch
```

Rules:

- `requestId` identifies one HTTP request;
- `correlationId` groups a workflow but does not prove identity or idempotency;
- `operationId` identifies one logical mutation;
- `jobId` identifies one schedulable unit;
- `attemptId` identifies one execution attempt;
- `eventId` identifies one immutable accountability event;
- none of these values is a bearer credential;
- public clients cannot choose trusted Agent or worker identities;
- correlation fields are length-limited and normalized before storage.

A single operation may therefore produce several AccountabilityEvents without losing one logical mutation identity.

---

## Authorization Decision Evidence

Protected actions record the server-side authorization and policy decision.

Canonical decision values are:

```text
allowed
denied
not_applicable
```

A decision summary may include:

```text
permission
scope
backendId
siteId
policyId
policyVersion
backendAccessMode
capabilityDecision
resourceRevisionChecked
backendGenerationChecked
reasonCode
```

The audit event does not store the complete policy engine implementation or every intermediate rule evaluation by default.

It stores enough stable evidence to explain the effective decision.

### Denial handling

A denial event must not leak a resource that the actor was not allowed to discover.

For example:

- the internal event may retain protected target evidence;
- the public API may return existence-hiding `404`;
- the audit query may reveal the target only to an appropriately privileged reviewer.

### Allow decisions

Not every low-risk read requires an individual durable allow event.

Mandatory allow-event categories include:

- protected mutation authorization;
- role, permission and backend access-mode changes;
- Agent enrollment, credential and trust changes;
- audit access and export;
- MediaSession creation where policy requires accountability;
- Legacy OSD controller-lease acquisition;
- other high-impact actions defined by policy.

High-volume safe reads may use aggregate, sampled or no per-request audit according to explicit policy.

---

## Outcome Model

Canonical outcome values include:

```text
requested
accepted
rejected
started
succeeded
failed
outcome_unknown
cancelled
expired
revoked
compensated
partially_completed
```

Outcome does not replace the detailed domain lifecycle.

The event also references the actual operation, job, assignment, session or lease state when available.

Examples:

```text
HTTP 202
  may produce outcome = accepted
  operation state = queued

Backend response lost after dispatch
  produces outcome = outcome_unknown
  operation state = outcome_unknown

Readback verifies Recording move
  produces outcome = succeeded
  operation state = succeeded
```

A later verification event does not rewrite the earlier dispatch or unknown-outcome event.

History remains append-only.

---

## Change Summary

Audit records changes using bounded summaries rather than unrestricted before-and-after payload copies.

Preferred change evidence includes:

```text
previousRevision
newRevision
previousState
newState
changedFieldNames
normalizedActionSummary
policyDecisionSummary
```

Rules:

- secrets are never copied into before/after data;
- complete Recording descriptions are not duplicated into audit history;
- complete EPG payloads are not duplicated;
- OSD frames and media payloads are never stored;
- filesystem paths are included only when required and access-controlled;
- metadata values are summarized or referenced by stable identity;
- large request bodies are represented by a normalized fingerprint and bounded summary.

---

## Mutation Audit Contract

Every production mutation governed by ADR-0042 produces durable accountability evidence.

The minimum lifecycle is:

```text
request received
  -> authentication and authorization decision
  -> operation accepted or rejected
  -> dispatch boundary
  -> verification or reconciliation
  -> terminal or unknown outcome
```

The audit model does not require a separate event for every in-memory function call.

It requires events at durable semantic boundaries.

### Before dispatch

Before external or VDR-native side effects begin, the durable state must contain evidence of:

- actor;
- target backend and resource;
- expected revision and generation where required;
- authorization and policy decision;
- operation identity;
- normalized action;
- accepted or rejected state.

The preferred implementation is a transaction or transactional outbox shared with operation persistence.

### Dispatch

The dispatch-start event references:

- operation ID;
- job and attempt IDs;
- backend ID and generation;
- Agent or executor identity;
- idempotency scope;
- dispatch state.

It does not claim success.

### Verification

Success requires the verification policy defined by ADR-0042.

The verification event records whether the requested state was proven.

Executor acknowledgement alone is not silently reported as verified success.

### Unknown outcome

If dispatch may have reached the backend but completion cannot be proven:

```text
outcome = outcome_unknown
```

This is a first-class audit fact.

It must not be changed into generic failure, success or a replacement mutation.

### Reconciliation

Reconciliation appends new evidence and a new event.

It never deletes the unknown-outcome history.

### Compensation

Saga compensation records:

- original operation and saga;
- compensation step;
- reason;
- actor or system policy authorizing compensation;
- dispatch and verification outcome;
- whether compensation restored the intended invariant.

Compensation is not described as if the original action never happened.

---

## Job, Attempt and Saga Events

ADR-0043 keeps append-only attempt evidence.

The audit layer references that evidence without duplicating every technical field.

Mandatory accountability events include:

- job enters dead letter;
- job waits for reconciliation after possible dispatch;
- privileged operator retries, cancels or resolves a job;
- saga starts for a protected mutation;
- compensation becomes required;
- compensation fails;
- operator overrides an automatic decision.

Ordinary claim renewals and heartbeat measurements remain operational diagnostics unless security policy promotes them.

A stale worker write or invalid claim token may generate a security event.

---

## Authentication and Session Events

The identity runtime records bounded events for:

- login success;
- login failure;
- session creation;
- session expiry;
- session revocation;
- password or authentication-factor change;
- service-account credential issuance and revocation;
- suspicious repeated failures;
- token misuse or invalid audience;
- CSRF or origin-policy rejection where applicable.

Rules:

- passwords, password hashes and bearer tokens are never stored;
- session IDs are stored only as non-secret references or protected fingerprints;
- public error responses remain existence-hiding where required;
- authentication failure events do not reveal whether an unknown username exists;
- source-network evidence is minimized and sensitivity-classified.

---

## Agent and Multi-Site Security Events

The Agent trust boundary records:

- enrollment requested, approved, rejected or cancelled;
- Agent identity and backend binding established;
- credential issued, activated, rotated, expired or revoked;
- connection established or terminated;
- unsupported protocol version;
- backend or site identity mismatch;
- obsolete backend generation;
- expired command;
- replay or duplicate producer event;
- invalid signature or transport identity;
- local queue overflow or evidence loss;
- reconnect reconciliation started and completed;
- Agent trust disabled after compromise.

The Control Plane records the authoritative trust decision.

The Agent may submit source evidence only for:

- its enrolled identity;
- its own backend and site;
- its current or explicitly historical generation;
- producer event IDs and sequences it can prove locally.

An Agent cannot create an authoritative event claiming that a user was authorized unless the Control Plane supplied and bound that context to the command.

---

## Timer and Automation Events

Later Phase 64 implementation records meaningful transitions such as:

- TimerIntent created, paused, cancelled or failed;
- TimerAssignment created, changed, released or failed;
- deliberate replica assignment created;
- failover decision made;
- NativeTimerBinding verified, missing, drifted or adopted;
- external native Timer detected;
- SearchTimer proposal accepted or rejected as an intent;
- duplicate or ambiguous candidate requires review.

The audit event references stable TimerIntent and assignment identities.

It does not treat a native Timer number as global identity.

High-volume EPG matching candidates are not all mandatory audit events.

---

## Media Session Events

Later Phase 65 implementation records:

- MediaSession requested, allowed, denied, created, expired or revoked;
- MediaAccessGrant issued, denied, expired or revoked;
- route selection changed for a security-relevant reason;
- provider lease exhausted or forcibly released;
- protected Recording download or export where enabled;
- access to a backend or resource denied.

The audit store never contains:

- media packets;
- Streamdev URLs;
- bearer grant tokens;
- DRM or transport secrets;
- full playback telemetry by default.

Ordinary playback connection progress is operational telemetry unless policy requires a bounded accountability record.

---

## Legacy OSD Events

Later Phase 66 implementation records:

- LegacyOsdSession created or denied;
- viewer binding created or removed when policy requires;
- controller lease acquired, denied, expired, revoked or released;
- allowlisted input accepted or rejected;
- rate-limit or stale-epoch rejection;
- sequence loss requiring resynchronization;
- attempt to control a read-only backend.

OSD frames, deltas, screenshots, menu text and secret input are not copied into audit storage.

An input event records only bounded command classification, target session, lease and result.

---

## Audit of Audit Operations

The audit subsystem is itself protected and auditable.

The following actions create combined audit and security events:

- query of sensitive audit scopes;
- export creation or download;
- retention-policy change;
- sensitivity or redaction-policy change;
- audit integration configuration change;
- manual event annotation;
- legal hold creation or removal where implemented;
- retention purge execution;
- audit store repair or migration;
- audit pipeline degradation override.

An administrator cannot silently disable audit for privileged actions.

---

## Append-Only Persistence

AccountabilityEvents are immutable after durable acceptance.

Permitted operations are:

```text
append event
read event
append annotation event
append correction event
expire or purge under retention policy
export under authorization
```

Not permitted:

```text
edit historical actor
replace historical outcome
delete one inconvenient event outside retention policy
rewrite reason text to change meaning
reuse event identity
```

### Corrections

If evidence was incorrect or incomplete, VDR-Suite appends a correction event referencing:

```text
correctedEventId
correctionReason
correctingActorId
replacement evidence summary
```

The original event remains visible to authorized reviewers.

### Storage direction

Phase 62 should introduce a Suite-owned audit repository and migrations.

Likely persistence includes:

- accountability events;
- actor references;
- target references;
- correlation references;
- producer sequences;
- retention and sensitivity classes;
- event annotations and corrections;
- transactional outbox records;
- export manifests;
- retention purge summaries.

This ADR does not mandate one final SQL table layout.

---

## Transactional Outbox and Failure Semantics

Audit failure behavior depends on whether an external effect may already have happened.

### Before privileged dispatch

For protected mutations, permission changes, credential changes, trust changes and audit-policy changes:

```text
required accountability record cannot be persisted
  -> do not begin external dispatch
```

The operation fails closed before side effects.

The durable operation state and required pre-dispatch audit event should be committed in one transaction or through a transactional outbox whose creation is in that transaction.

### After dispatch may have begun

Once external dispatch may have happened:

- audit failure cannot roll back the VDR or remote filesystem;
- audit failure cannot justify blind redispatch;
- operation, job and dispatch evidence remain authoritative;
- an outbox or recovery queue must retry audit publication;
- the system records `audit.pipeline.degraded` when possible;
- operator-visible health becomes degraded;
- reconciliation determines the domain outcome.

### Audit store outage

The system defines event policy classes:

```text
mandatory_precondition
mandatory_recoverable
best_effort_diagnostic
```

Examples:

- credential revocation and destructive mutation: `mandatory_precondition` before dispatch;
- post-dispatch verified result: `mandatory_recoverable` through outbox;
- repetitive low-value diagnostic observation: not an AccountabilityEvent or `best_effort_diagnostic`.

Exact operational limits are configuration and implementation details, but security-critical event loss must never be silent.

---

## Agent Offline Buffering and Deduplication

A remote Agent may temporarily buffer producer evidence while disconnected.

The buffer must be:

- durable for events classified as required;
- bounded by size and retention;
- ordered per Agent producer generation;
- protected against unauthorized local modification as far as the platform design permits;
- replay-safe on reconnect;
- observable when full or degraded.

Agent event submissions carry:

```text
producerId
producerGeneration
producerEventId
producerSequence
occurredAt
backendId
backendGeneration
bounded evidence
```

The Control Plane deduplicates using the producer identity and event identity or sequence.

Duplicate delivery returns the existing accepted event reference and does not create a second semantic event.

A sequence gap is recorded and reconciled.

The Control Plane does not assume that remote source clocks provide a global total order.

---

## Ordering and Time

The model distinguishes:

```text
occurredAt
  producer-observed time

recordedAt
  Control Plane durable acceptance time

producerSequence
  order within one producer generation

storeSequence
  optional monotonic order within the canonical store

backendGeneration
  runtime ownership epoch
```

Rules:

- `occurredAt` is not trusted as a global ordering key across sites;
- clock skew does not rewrite event order;
- producer sequence is meaningful only within its producer scope;
- backend generation is not an event sequence;
- request and correlation IDs are not ordering keys;
- queries use a deterministic stable order such as `recordedAt` plus `eventId` or an opaque store cursor.

---

## Data Minimization and Secret Handling

The audit model follows strict data minimization.

Never store:

- passwords;
- password hashes;
- bearer tokens;
- session cookies;
- private keys;
- complete certificates containing unnecessary material;
- raw authorization headers;
- Streamdev credentials or URLs;
- complete HTTP request or response bodies by default;
- complete EPG descriptions;
- full Recording media or thumbnails;
- OSD frames or screenshots;
- arbitrary plugin payload dumps;
- raw VDR pointers or memory addresses.

Store bounded references or fingerprints instead.

### Network metadata

Source address, device and user-agent information may be useful for security investigation, but can be personal or sensitive data.

Rules:

- collect only what policy requires;
- classify sensitivity;
- restrict access;
- apply retention independently where needed;
- allow configured truncation, normalization or pseudonymization;
- never use an IP address as actor identity.

### Error evidence

Internal error evidence is bounded and redacted.

Native adapter details may be retained for privileged diagnosis without becoming public API error text.

---

## Privacy and Retention

Audit integrity and privacy obligations must both be supported.

Retention is policy-based rather than one universal duration.

Example retention classes include:

```text
security_authentication
security_agent_trust
privileged_configuration
mutation_accountability
media_access
legacy_osd_control
audit_administration
operational_short_term
```

Rules:

- exact durations are deployment and legal-policy configuration;
- security and destructive-mutation evidence normally outlives routine logs;
- expired events are purged only by an authorized retention process;
- purge creates a bounded audit summary before deletion where possible;
- legal hold, if implemented, overrides ordinary expiry under explicit authorization;
- changing retention policy does not silently rewrite historical event meaning.

### Identity deletion and pseudonymization

Deleting or disabling a user does not falsify historical audit records.

Where privacy policy requires reduced identifiability, the system may replace direct display data with a stable pseudonymous reference while retaining event integrity and authorization history.

The exact legal erasure policy is deployment-specific and is not certified by this ADR.

---

## Query and Access Control

Audit data is not an unrestricted public API.

Access requires explicit permissions and scope.

Possible permissions include:

```text
audit.read.self
audit.read.backend
audit.read.site
audit.read.security
audit.read.global
audit.export
audit.annotate
audit.manage-retention
```

The final RBAC model is implemented in Phase 62.

Rules:

- users do not automatically see events for every backend;
- backend-scoped administrators see only authorized scopes unless granted wider authority;
- security details may require stronger permission than ordinary mutation history;
- audit queries themselves are audited;
- existence-hiding rules apply to unauthorized resources;
- query results redact fields according to actor permission and event sensitivity;
- service accounts receive only explicitly granted audit scopes.

### Query shape

A future public or administrative v1 audit API follows ADR-0048:

- versioned path;
- structured errors;
- request and correlation IDs;
- opaque cursor pagination;
- deterministic ordering;
- bounded filters;
- no speculative fallback;
- explicit partial-result semantics where external archives are involved.

Likely filters include:

```text
eventType
class
severity
actorId
backendId
siteId
resourceType
resourceId
operationId
correlationId
recorded time range
```

The final route name and representation are Phase 62 and Phase 67 implementation decisions.

---

## Export and External Integration

VDR-Suite may later export security and audit events to external systems.

The canonical store remains Suite-owned.

An export adapter may target:

- syslog or journald;
- JSON lines files;
- security information and event management systems;
- protected webhook or message-bus integrations;
- offline archive storage.

Rules:

- export failure does not delete canonical events;
- exports have stable event IDs for deduplication;
- export configuration is audited;
- secret fields remain excluded;
- external sink acknowledgement is not the canonical event identity;
- external systems do not become the Control Plane database;
- an export manifest records scope, filter, actor and event range.

Cryptographic signing, hash chaining or write-once external storage may be added later as deployment hardening.

This ADR does not claim cryptographic tamper proofing before it is implemented and tested.

---

## Security Event Consumers

Security events may feed:

- rate limiting;
- account lockout policy;
- administrator notifications;
- Agent revocation workflow;
- health dashboards;
- external SIEM integrations;
- incident review.

Consumers must not mutate historical events.

Derived alerts have their own identity and lifecycle.

A cleared alert does not delete the source event.

---

## Control Plane Ownership

The Control Plane is authoritative for:

- public actor identity;
- authorization decisions;
- role and permission changes;
- backend access policy;
- operation and job correlation;
- audit event classification;
- canonical persistence;
- retention, query and export policy.

The Control Plane may accept producer evidence from Agents, adapters and plugins, but it records the trust source.

An untrusted or degraded producer cannot overwrite Control Plane decisions.

---

## Backend Agent Boundary

The Backend Agent may:

- attach its enrolled identity;
- attach backend and generation;
- preserve command, dispatch and local result references;
- produce monotonic local event sequence;
- durably buffer required evidence while offline;
- resubmit evidence idempotently after reconnect;
- report local security conditions;
- provide bounded native readback evidence.

The Backend Agent must not:

- invent public user authorization;
- read or write audit records for another backend;
- store user bearer credentials in event payloads;
- become the global audit query service;
- choose global retention policy;
- rewrite events after acceptance;
- report a stale generation as current;
- suppress sequence gaps silently.

---

## VDR Plugin Boundary

The VDR plugin remains a narrow native observer and executor.

It may later produce bounded facts such as:

- plugin lifecycle transition;
- local command receipt reference;
- native operation result category;
- readback revision or fingerprint;
- native error category;
- bounded status or capability transition.

It does not own:

- user identity;
- session authentication;
- authorization or RBAC;
- global event identity;
- durable audit retention;
- audit query endpoints;
- security alerting;
- Agent trust;
- public request IDs;
- operation or saga policy.

### VDR safety

Audit work must never violate VDR callback and lock safety.

Rules:

- no database, network or filesystem audit write under VDR locks;
- no audit serialization in status callbacks;
- no blocking audit queue operation in VDR callbacks;
- no raw VDR pointers retained as audit evidence;
- native values are copied under short correct locks;
- the Agent or outer adapter converts bounded facts into transport evidence;
- callback counters remain diagnostics, not audit sequence.

The SB.7 rule that lifecycle and status logging stays outside callbacks remains compatible with this ADR.

Plugin-local logs remain operational evidence and are not the authoritative global audit trail.

---

## Existing Foundation Mapping

Existing repository components are retained and mapped as follows:

| Existing foundation | Role under ADR-0049 |
| --- | --- |
| `RuntimeLogEntry` and `IRuntimeLogger` | Human-readable runtime logging only. |
| Runtime diagnostics and measurements | Operational measurements, not audit history. |
| Backend registry and access mode | Target scope and policy evidence. |
| ADR-0041 actor and Agent identity direction | Identity and trust context. |
| ADR-0042 operation envelope | Actor, target, revision, idempotency and mutation outcome correlation. |
| ADR-0043 job, attempt and saga model | Execution, retry, reconciliation and compensation evidence. |
| ADR-0044 TimerIntent and assignment | Stable timer orchestration targets. |
| ADR-0045 event provenance | Source-evidence pattern, separate from accountability evidence. |
| ADR-0046 MediaSession | Stable media-access target and lifecycle. |
| ADR-0047 OSD session and controller lease | Stable compatibility-control target. |
| ADR-0048 request/correlation IDs and `/api/v1` | Public request context and future query contract. |
| Existing backend action audit planning note | Historical goal, superseded in detail by this ADR. |
| Suite Bridge bounded logs and snapshots | Plugin-local evidence, not canonical global audit. |

No current runtime logger, job table or plugin log is renamed or treated as a complete audit store merely because this ADR is accepted.

---

## Implementation Sequence

Phase 62 should implement the model in bounded slices.

Recommended order:

1. introduce stable actor, request-context and accountability-event value types;
2. define event-type catalogue, schema version, severity, sensitivity and retention classes;
3. add append-only persistence, migrations and repository tests;
4. add transactional outbox support with operation persistence;
5. propagate request ID, correlation ID and authenticated actor context;
6. record authentication, session and authorization decisions;
7. record user, role, permission and backend access-mode changes;
8. bind ADR-0042 operation lifecycle to audit events;
9. bind ADR-0043 job, attempt, reconciliation and saga operator actions;
10. add Agent enrollment, credential, trust and reconnect security events;
11. add protected query, redaction and cursor pagination;
12. add audit-of-audit events, retention jobs and export manifests;
13. add failure-injection tests for audit-store and outbox outages;
14. add Agent offline buffering and deduplication in Phase 63;
15. add Timer, media and Legacy OSD events in their later domain phases;
16. expose the final versioned administrative API under ADR-0048 rules;
17. add external export adapters only after canonical persistence is proven.

No new production remote mutation path may bypass the required pre-dispatch audit and recovery semantics.

---

## Acceptance Criteria

ADR-0049 runtime implementation is not complete until tests prove at least:

- audit records are structured and append-only;
- logs and diagnostics are not parsed as audit data;
- every protected mutation has actor, target, authorization, operation and outcome correlation;
- required pre-dispatch audit failure prevents side effects;
- post-dispatch audit failure does not trigger blind mutation retry;
- transactional outbox delivery is idempotent;
- corrections append new events rather than modifying history;
- secrets and raw credentials are rejected or redacted;
- unauthorized audit queries cannot discover protected resources;
- query access and exports are themselves audited;
- Agent producer events are backend- and generation-scoped;
- duplicate Agent delivery does not create duplicate semantic events;
- Agent sequence gaps and buffer degradation are visible;
- unknown native actors are not falsely attributed to Suite users;
- retention purge follows policy and leaves a bounded purge summary;
- plugin callbacks perform no blocking audit work;
- event schema migration preserves original identity and meaning;
- current G-24 status is updated only after implementation, tests and documentation exist.

---

## Rules

- Audit, security events, logs, diagnostics, debug state and domain events remain separate concepts.
- The Control Plane owns the canonical accountability store.
- Events are immutable after durable acceptance.
- Corrections and annotations append new events.
- Every protected mutation is correlated with actor, target, authorization, operation and outcome.
- Required audit persistence is a precondition before privileged external dispatch.
- Audit failure after possible dispatch never authorizes blind redispatch.
- `outcome_unknown` remains a durable accountability fact until reconciliation.
- Actor, executor and Agent identities are not interchangeable.
- Native local activity is not attributed to a Suite user without evidence.
- Secrets, tokens, passwords, keys, media payloads and OSD frames are never audit fields.
- Agent evidence is scoped to its enrolled identity, backend and generation.
- Producer clocks do not create a global total order.
- Audit access, export, retention and configuration changes are themselves audited.
- Event retention is policy-based and privacy-aware.
- The plugin performs no blocking audit work under VDR locks or callbacks.
- Acceptance of this ADR does not mark Phase 62 or G-24 implemented.

---

## Non-Goals

This ADR does not implement or choose:

- one final user identity provider;
- final role and permission database tables;
- one mandatory audit SQL schema;
- final retention durations for every jurisdiction;
- legal compliance certification;
- a full security information and event management product;
- automatic security incident response;
- one mandatory external log or audit sink;
- cryptographic hash chaining or hardware-backed signing;
- write-once storage;
- full packet, media or OSD capture;
- universal auditing of every safe read;
- replacement of operation, job, attempt or saga tables;
- reconstruction of complete domain state only from audit events;
- public exposure of all audit fields;
- plugin-owned user or audit storage;
- runtime implementation in this documentation change.

---

## Consequences

Positive consequences:

- destructive and remote actions become attributable;
- authorization and read-only decisions can be reviewed;
- uncertain outcomes remain visible instead of being hidden by retries;
- Agent trust failures become structured security evidence;
- audit access itself becomes accountable;
- logs can remain readable without becoming a fragile database;
- privacy and secret-minimization rules are explicit;
- external SIEM or archive integrations remain possible without owning the canonical model;
- multi-site sequence and clock limitations are handled honestly;
- plugin callback safety remains intact.

Trade-offs:

- append-only storage and retention add database and operational complexity;
- actor and request context must propagate through many layers;
- transactional outbox handling is required for reliable mutation evidence;
- security-sensitive query authorization is more complex than ordinary logs;
- retention and privacy policies require deployment decisions;
- Agent offline buffering and deduplication require additional protocol work;
- event catalogues and schema compatibility require long-term discipline.

---

## Architecture Package Completion

ADR-0049 completes the planned ADR-0042 through ADR-0049 contract package.

The next repository work is not immediate runtime mutation implementation.

The required package closeout is:

```text
update affected architecture diagrams
create the domain dependency map
create the implementation dependency map
verify cross-references and strict roadmap alignment
```

Only after that closeout does the strict sequence proceed to:

```text
Phase 60.15 - Recording Metadata and Poster Preparation
```

Accepted architecture remains distinct from implemented runtime behavior.

---

## Back

- [Back to ADR Index](index.md)
- [Back to Documentation Index](../index.md)
- [Back to Current State](../CURRENT.md)
- [Back to Strict Roadmap](../planning/roadmap.md)
- [Back to README](../../README.md)
