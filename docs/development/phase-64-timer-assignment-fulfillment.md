# Phase 64 — TimerAssignment Fulfillment State

This service owns the narrow Control-Plane transition from a deterministic durable assignment into native fulfillment.

`selected -> provisioning` is revision-fenced against the exact assignment revision, intent revision and backend generation selected by the scheduler. Entering `provisioning` happens before native CREATE dispatch work is allowed to begin.

`provisioning -> bound` (and reconciliation convergence from `reconciling -> bound`) requires an already persisted, verified managed NativeTimerBinding. The binding must belong to the exact assignment and backend generation, have a matching binding revision, contain a verified operation reference, and have neither missing evidence nor drift.

The bound transition copies only the stable `nativeTimerBindingId` onto the assignment. It does not manufacture a native identity, verify backend state itself or treat transport reachability as authority.

Both transitions are repository revision-fenced. A stale assignment/binding revision, stale intent revision or generation mismatch fails closed. Replaying an already bound assignment with the same verified binding is idempotent.

An `outcome_unknown` native CREATE is not bound merely because dispatch may have happened. Its operation must first converge through authoritative CREATE readback; only the verified managed binding produced by that readback can complete the assignment.

This service performs no provider selection, Agent dispatch, SuiteBridge call, VDR write, reassignment or failover.
