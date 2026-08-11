# Phase 64 Slice 24 — Native Timer Delete Agent Contract

## Status

Bounded contract-only slice stacked on Phase 64 Slice 23 / Draft PR #176.

Slice 23 defines the Control-Plane dispatch claim and its three typed executor
outcomes. Slice 24 defines the exact Agent-side command/evidence envelope that a
later runtime slice may use to carry one claimed Timer delete to the explicitly
selected local provider.

This slice adds **no runtime wiring** and does not modify the existing
`vdr.native.probe` command path.

## Domain-specific capability

The new contract is deliberately narrow:

```text
commandType = vdr.timer.delete
authorityDomain = vdr.timer
requiredCapability = vdr.timer.delete
providerId = suitebridge:local
providerKind = suitebridge
payloadVersion = 1
```

There is no arbitrary native operation string, SVDRP text, shell fragment,
provider URL or free-form JSON extension. `vdr.timer.delete` cannot be inferred
from ownership of `vdr.native.probe`.

## Availability is not authority

The Phase-63 provider model remains authoritative: **Availability is not authority**.
A future assignment/runtime must persist an explicit
`BackendAgentLocalProviderSelection` for `vdr.timer.delete` and must have
separately configured ownership allowing that exact capability.

The contract validates that the persisted selection is structurally valid and
matches all fixed Timer-delete constants, including backend, authority domain,
provider identity/kind and required capability.

RESTfulAPI presence, another SuiteBridge capability or generic VDR availability
cannot satisfy this contract.

## Immutable command identity

`BackendAgentNativeTimerDeleteCommand` binds one future Agent command to the
Slice-23 claim through:

- command ID and normalized request fingerprint;
- exact mutation operation ID and operation revision;
- exact NativeTimerBinding ID and expected binding revision;
- exact TimerAssignment ID;
- exact backend-native Timer identity;
- job ID, attempt ID and claim epoch;
- exact backend ID/generation;
- exact Agent ID and Agent-instance ID;
- the Control-Plane dispatch-claim timestamp;
- the complete local provider selection fence.

The Agent contract does not reload Timer state and does not decide authorization.
Those remain Control-Plane responsibilities before command assignment and again
at future dispatch continuation fences.

## Durable starting-before-execution evidence

Phase 63 requires local `starting` state to be durable before a native executor
call. The Slice-24 evidence contract preserves that invariant explicitly through
`localStartingPersistedAt`.

Evidence is accepted only when:

```text
controlPlaneClaimedAt <= localStartingPersistedAt <= completedAt
```

For an executor outcome that may have dispatched:

```text
localStartingPersistedAt <= dispatchStartedAt <= completedAt
```

For `rejectedWithoutEffect`, `dispatchStartedAt` must be zero.

The future runtime must persist `starting` before calling SuiteBridge. Merely
constructing this evidence object does not grant execution authority.

## Executor outcome vocabulary

The Agent-side evidence uses the same bounded meanings as Slice 23:

```text
rejectedWithoutEffect
acceptedUnverified
outcomeUnknown
```

It carries no `succeeded` outcome because authoritative Timer success still
requires the Slice-19 readback and Slice-21 shared-operation completion.

Evidence must reproduce the immutable command/job/attempt/claim/Agent/backend,
operation revision, request fingerprint and provider-instance epoch. A result
from another Agent instance, plugin epoch, operation revision or command attempt
cannot be correlated to the claim.

## Separation from the existing native probe

The current Phase-63 `vdr.native.probe` path remains side-effect-free and keeps
`mutations=disabled`. Slice 24 does not broaden its validator, provider
capability or transport API.

In particular, this slice does not make:

```text
vdr.native.probe ownership
```

equivalent to:

```text
vdr.timer.delete ownership
```

and it does not advertise `vdr.timer.delete` from any Agent.

## Scope boundary

This slice adds only:

- the typed Agent Timer-delete contract;
- structural/provider-fence validation;
- typed durable outcome-evidence validation;
- focused regression;
- architecture guard;
- isolated Make test fragment;
- documentation.

It adds:

- no BackendAgentCommand assignment/delivery integration;
- no Agent poll capability advertisement;
- no provider-ownership administration for Timer deletes;
- no SuiteBridge transport method;
- no local VDR Timer delete;
- no daemon/Agent runtime source wiring;
- no mutation API;
- no TimerAssignment transition;
- no broad Timer UI.

Therefore **mutations remain disabled** in the installed runtime and real yaVDR
acceptance is not required for Slice 24.

## Next bounded work

**Slice 25** should add the Control-Plane assignment/provider-selection contract
for `vdr.timer.delete`: require explicit `vdr.timer` ownership, observed
`suitebridge:local` capability, active Agent lease and exact Agent/backend/provider
generations, then persist the command plus provider-selection sidecar atomically.

That slice should still avoid local execution if possible. The first slice that
advertises the mutating capability or invokes SuiteBridge/VDR must be treated as
an installed runtime/security boundary and require exact-head real yaVDR
acceptance.
