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
