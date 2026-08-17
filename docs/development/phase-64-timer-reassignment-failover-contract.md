# Phase 64 — Controlled Timer Reassignment and Failover

## Scope

This slice closes the remaining ADR-0044 reassignment/failover boundary after
the managed Timer fulfillment vertical merged through PR #194. It adds one
Control-Plane-owned replacement handover. It does not start Phase 65, expose a
broad Timer UI or publish SuiteBridge Timer-write SVDRP help.

The handover is deliberately narrower than a general reconciler. It may replace
one current active owner only when the old native outcome is already safe:

1. **before dispatch** — the old assignment is still `selected`, has no native
   binding and no native mutation can have reached the backend; or
2. **verified absent** — the exact managed binding is authoritatively absent and
   the exact Timer-delete operation completed as `succeeded` after readback.

`dispatching`, `executed_unverified`, `outcome_unknown`, incomplete inventory,
external/ambiguous drift and active recording are not replacement authority.
They remain reconciliation-only and fail closed.

## Required durable evidence

Every successful reassignment records:

- old assignment ID, exact revision and repository-issued epoch;
- old backend ID and generation;
- old native outcome (`before_dispatch` or `verified_absent`);
- exact operation/binding evidence when absence is the authority;
- bounded reason;
- replacement assignment ID, new backend ID/generation and new assignment
  epoch;
- creation time.

The replacement keeps the original TimerIntent and exact intent revision but
uses a new assignment identity, role `replacement` and a newly repository-issued
assignment epoch.

## Fences and ordering

The service uses this order:

```text
read assignmentSetRevision
  -> reload exact TimerIntent and assignment set
  -> verify old assignment revision/epoch/backend/generation
  -> verify safe old native outcome
  -> plan role=replacement from current candidates
  -> atomically supersede old owner, persist replacement and evidence
```

The final repository transaction rechecks the assignment-set revision and all
old-owner fences. It commits the old terminal state, the new replacement and
the reassignment evidence together. Any stale intent, assignment, set,
generation, binding or operation evidence aborts the whole handover.

The replacement planner remains provider-neutral. Current candidate evidence
must still prove write permission, execution authority, capability, health,
channel mapping, backend generation and conflict state. It never selects the old
backend and never falls back between providers.

## Ownership invariant

Primary and replacement roles are both exclusive active-owner roles. A
deliberate replica remains the only permitted simultaneous additional owner.
The durable active-owner index therefore covers `primary` and `replacement` in
the active ownership states.

The old owner is never durably closed in one transaction and the replacement
created in a later transaction. Conversely, the replacement is never made
active while the old owner remains active. Response-loss replay returns the
same committed handover; it does not create another assignment or epoch.

## Regression contract

Focused regressions must prove:

- selected/no-binding reassignment succeeds to a different current backend;
- verified-absence reassignment requires an exact succeeded delete operation,
  managed binding, binding revision and generation;
- a new replacement ID and epoch are issued while the TimerIntent is preserved;
- old closure, replacement creation and evidence are atomic;
- exact replay returns the same durable handover;
- incompatible replacement-ID reuse fails closed;
- stale intent, assignment revision, assignment epoch, assignment-set revision,
  backend generation, binding revision and operation identity fail closed;
- `dispatching`, `executed_unverified` and `outcome_unknown` block failover;
- present, incomplete, external or ambiguous native state blocks failover;
- candidate/provider evidence is re-evaluated and the old backend is excluded;
- concurrent handovers cannot create overlapping exclusive owners;
- public SuiteBridge SVDRP help remains closed for `NTCREATE`, `NTMOD` and
  `NTDELETE`.

## Acceptance boundary

Hosted CI must pass on one exact final head. The final real yaVDR acceptance is
one bundled run on that exact commit and must retain the PR-#194 configuration,
identity, service-restoration and private-SVDRP-help checks while adding the
focused reassignment regression/build evidence.

Phase 64 is not complete until that final real-system acceptance passes. The PR
must remain Draft and must not be merged without explicit user approval.

The exact final candidate is run with:

```bash
PHASE64_EXPECTED_HEAD=<exact-40-character-commit> \
  make phase64-reassignment-failover-acceptance
```
