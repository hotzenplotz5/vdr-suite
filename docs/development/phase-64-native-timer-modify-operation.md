# Phase 64 native Timer UPDATE/TOGGLE operation

Managed UPDATE and TOGGLE share one durable modify payload while retaining
distinct action families. Preparation fences the current assignment, intent,
binding revision, backend generation, native identity and exact pre-mutation
fingerprint. UPDATE carries the complete desired backend-neutral specification.
TOGGLE is narrower: every specification field except `enabled` must remain
unchanged.

A recording, pending, missing or drifted Timer is rejected before dispatch.
The operation is stored with readback-required verification. No blind retry is
authorized after dispatch; accepted-unverified and outcome-unknown execution
must both proceed to reconciliation.

Completion requires an authoritative PRESENT observation at or after the
dispatch boundary. The observation must match the stable native identity and
the complete desired specification. Only then is the copied binding state and
last verified operation advanced under optimistic concurrency.

## Dispatch, recovery and terminal verification

The durable modify payload is now the only source for the dispatch handoff after
restart. Claiming reloads and validates that payload, rechecks the current
binding revision, generation, native identity, predecessor fingerprint and safe
non-recording state, then revision-fences the operation from `accepted` to
`dispatching`. Reclaiming the exact dispatch is idempotent; it never authorizes
a second native write.

Executor outcomes map into the shared mutation lifecycle:

- `rejected_without_effect` becomes `failed_verified` without readback;
- `accepted_unverified` becomes `executed_unverified`;
- `outcome_unknown` stays reconciliation-only as `outcome_unknown`.

The latter two produce a PRESENT-readback expectation whose lower time boundary
is the executor's durable dispatch-start timestamp. Verification accepts both
states, updates the binding only from an authoritative matching observation,
and completion advances the operation to `succeeded` only after that persisted
binding names the operation as its last verified mutation. Exact claim, outcome
and completion replays are idempotent.
