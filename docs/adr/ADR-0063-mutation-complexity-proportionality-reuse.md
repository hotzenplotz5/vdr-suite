# ADR-0063: Mutation Complexity Proportionality and Reuse

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [ADR Index](index.md)
- [Current State](../CURRENT.md)
- [ADR-0042: Safe Mutation, Revision and Idempotency Contract](ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0044: Timer Intent, Assignment and Native Timer Model](ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0048: Public API Versioning, Error and Compatibility Contract](ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [ADR-0059: VDR-Native Recording Editing, Marks and Cutting Authority](ADR-0059-vdr-native-recording-editing-marks-cutting-authority.md)

---

## Status

Accepted

Date: 2026-09-24

---

## Context

Phase 69 public API hardening exercises the most demanding mutation path currently
present in VDR-Suite: managed Timer CREATE across a durable Suite control plane,
a separately connected Backend Agent and VDR as native execution authority.

That path legitimately needs several independent safety concerns at once:

- optimistic resource revision/precondition handling;
- durable idempotency and operation identity;
- TimerIntent/TimerAssignment/NativeTimerBinding ownership;
- backend generation and lease fencing;
- Agent command reservation and pollability;
- dispatch ownership and restart recovery;
- native effect verification and reconciliation;
- no blind retry after an unknown transport/native outcome.

This produces a large implementation and test surface.

The same size must not become an accidental architectural template for every
future mutation. VDR-Suite also contains materially smaller mutations, including
native Recording marks/editing governed by ADR-0059, where VDR remains canonical
but the mutation does not require Timer assignment, reassignment or native Timer
binding lifecycle.

Without an explicit rule, later work could copy Timer-specific orchestration
layers merely because they are the newest hardened example. That would increase
code, lifecycle authorities, persistence and failure states without corresponding
risk reduction.

At the same time, avoiding unnecessary orchestration must not weaken the common
mutation safety contract established by ADR-0042 or the public API contract in
ADR-0048.

---

## Decision

VDR-Suite applies **mutation complexity proportionally to the authority,
distribution and uncertainty of the concrete mutation**.

Common safety contracts are reused. Domain-specific orchestration is added only
when the mutation's actual ownership and failure model requires it.

Timer CREATE is therefore a high-complexity reference case, not a mandatory
structural template for all mutations.

This ADR clarifies how ADR-0042 is implemented; it does not supersede or weaken
ADR-0042, ADR-0048, backend authorization, revision safety, generation fencing or
idempotency requirements where those requirements apply.

---

## Common foundation

A real public mutation reuses the existing common foundation as applicable to
its exposed resource and execution boundary:

- authenticated actor identity;
- backend-scoped authorization and write policy;
- CSRF protection for protected browser mutations;
- stable Suite resource identity;
- opaque revision/ETag and optimistic preconditions;
- validated/closed request representation;
- durable idempotency when delivery can be repeated or its outcome can become
  ambiguous;
- stable operation identity/lifecycle when the operation is asynchronous,
  durable or may outlive the HTTP request;
- backend generation/authority fencing when execution crosses a backend
  generation boundary;
- explicit error semantics;
- accountability/audit evidence;
- no speculative retry after an unknown mutation outcome.

These are reusable safety properties. Their implementation should come from
existing shared services/helpers rather than domain-local copies.

---

## Domain-specific layers are not universal

The following Timer architecture is **not** automatically required for another
mutation:

- TimerIntent;
- TimerAssignment;
- NativeTimerBinding;
- assignment epoch or reassignment/failover lifecycle;
- pre-reserved native Timer binding identity;
- Timer-specific fulfillment state;
- Timer-specific Agent command payload/reservation;
- Timer-specific dispatch claim/activation;
- Timer readback/reconciliation states.

A later mutation may use one or more comparable mechanisms only when its own
authority and failure model proves the need.

No implementation may introduce a second lifecycle/repository authority merely
to resemble Timer CREATE.

---

## Complexity classes

The following classes guide design review. They are architectural categories,
not new runtime entities.

### Class A — synchronous authoritative mutation

Use when one authoritative owner can validate and commit the mutation
synchronously and return a definitive result.

Typical shape:

```text
HTTP/API
-> auth/policy/precondition
-> existing domain service
-> authoritative repository/native owner
-> updated revision/result
```

Do not add an asynchronous operation lifecycle, Agent job or binding entity only
for consistency with Timer CREATE.

### Class B — remote but definitive mutation

Use when execution crosses the Backend Agent/native boundary but the protocol
can produce a definitive fenced result without a long-lived orchestration
lifecycle.

Typical additions may include:

- backend generation fencing;
- idempotent command identity;
- bounded Agent transport;
- native readback when required.

Do not add assignment/reassignment/binding models unless the resource itself
requires those concepts.

### Class C — durable asynchronous or outcome-ambiguous mutation

Use when the HTTP request may end before the operation, delivery may be repeated,
or transport/native completion can become unknown.

Typical additions include:

- MutationOperation lifecycle;
- durable idempotency;
- immutable normalized payload;
- reservation/claim/activation or equivalent dispatch fencing;
- reconciliation after ambiguous outcome.

The exact domain still owns its own authoritative state.

### Class D — distributed managed-resource orchestration

Use only when the resource has durable placement/assignment/binding semantics,
generation-aware execution, reassignment/failover or equivalent distributed
ownership.

Managed Timer CREATE is the current example.

Class D mechanisms must not be inherited by Class A-C mutations without evidence.

---

## Required design proof before adding orchestration

A slice that adds a new lifecycle table, repository, binding identity, durable
job/command layer or dispatch state must document which concrete failure it
prevents.

At minimum the design review asks:

1. Who is authoritative for the resource before, during and after mutation?
2. Can the mutation finish synchronously with a definitive outcome?
3. Can delivery be repeated after response loss?
4. Can the native outcome become unknown?
5. Does execution cross a Backend Agent generation/lease boundary?
6. Does the resource require placement, binding, reassignment or failover?
7. Can an existing mutation/operation owner provide the required guarantee?
8. What is the smallest lifecycle that remains restart-safe and fail-closed?

If a proposed layer has no distinct failure/authority responsibility, it should
not be added.

---

## Recording marks example

ADR-0059 remains the authority for native Recording marks and cutting.

A marks update can still require authentication, authorization, a Recording
revision/precondition, backend-generation safety and idempotent transport where
appropriate. It does **not** thereby require TimerIntent, TimerAssignment,
NativeTimerBinding or Timer reassignment lifecycle.

If a future public marks mutation can safely be expressed as:

```text
route
-> security / If-Match
-> existing Recording editing owner
-> fenced native marks mutation
-> refreshed revision/readback
```

that is preferable to manufacturing a Timer-like orchestration graph.

If later evidence shows that marks execution has an asynchronous or ambiguous
failure mode, only the additional lifecycle needed for that proven mode is added.

---

## Timer CREATE interpretation

The current Phase-69.C Timer CREATE chain is intentionally large because its
actual model is distributed and managed:

```text
public precondition/idempotency
-> durable TimerAssignment admission
-> durable MutationOperation/payload
-> Agent command reservation
-> dispatch claim/activation
-> native VDR CREATE
-> readback/reconciliation
-> NativeTimerBinding
```

This remains valid for Timer orchestration.

It must not be cited as evidence that every later mutation needs the same number
of slices, entities or repositories.

---

## Complexity audit

After a high-complexity mutation path is completed, the closeout should record:

- which pieces are reusable common infrastructure;
- which pieces are domain-specific;
- which pieces are required only by distributed/ambiguous execution;
- whether any newly introduced abstraction can be removed or narrowed;
- explicit guidance for the next simpler mutation.

Phase 69 should perform this audit after the native Timer CREATE path is proven
end-to-end, before using that path as a pattern for additional public mutations.

The audit is architectural review, not a new runtime phase.

---

## Consequences

### Positive

- common safety properties remain consistent;
- simple mutations stay simple;
- distributed mutations remain rigorous;
- Timer-specific state does not leak into unrelated domains;
- future API work has an explicit guard against ceremonial lifecycle growth;
- reviewers can demand evidence for every new persistent orchestration layer.

### Trade-offs

- mutation implementations will not all have identical internal shapes;
- design review must classify the actual failure model instead of copying an
  existing path;
- some mutations may move from a simpler class to a more durable class when
  real runtime evidence exposes ambiguity.

These trade-offs are intentional. Uniform safety semantics are required; uniform
internal machinery is not.

---

## Guardrail

**Reuse guarantees, not accidental complexity.**

A new mutation must be no more complex than required to preserve its real
authority, concurrency, retry, restart and native-outcome guarantees.
