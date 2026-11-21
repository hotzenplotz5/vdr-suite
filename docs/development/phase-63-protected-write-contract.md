# Phase 63 Protected Write Contract

## Scope

This slice defines the last generic safety contract before a concrete protected VDR write is allowed. It operationalizes the mutation rules from ADR-0042 and the durable retry/reconciliation rules from ADR-0043, while remaining contract-only.

No command delivery, native dispatch, provider runtime, SQLite persistence, public API or installed service behavior is changed by this slice. `mutations=disabled` remains the production boundary.

No TimerIntent, RecordingIntent, SearchTimer, Remote, OSD, configuration or metadata mutation is introduced.

## Core rule

A protected write is not authorized merely because a backend is online or a provider is available. Availability is not authority.

A first execution candidate is valid only when all of these independent fences agree:

- stable operation identity and idempotency key,
- immutable logical request fingerprint,
- fresh resource-scoped authorization lease,
- expected authority generation,
- immutable explicit local-provider selection for the required write capability,
- current provider ownership/facts matching that selection,
- stable resource identity and expected resource revision,
- mandatory authoritative post-write readback.

Expected authority generation and expected resource revision are separate fences. A resource may be unchanged while authority moved to a new generation, or authority may remain current while the resource revision changed. Either mismatch fails closed before a native write.

## Idempotency is the first execution fence

ADR-0042 requires durable idempotency to be checked before lease, authority or revision checks that could lead to a native write.

The contract therefore has four outcomes:

- `reserveThenExecuteOnce`: no durable idempotency record exists and all write fences are current. The caller must durably reserve the idempotency key before native execution. This decision is not permission to skip that reservation.
- `returnPersistedResult`: the same idempotency key, operation ID and request fingerprint already completed and has a durable result reference. Returning that evidence authorizes no new write and does not require a still-current write lease.
- `reconcileUnknownOutcome`: the exact operation is already reserved or its outcome is unknown. Unknown outcome is reconciliation-only. A new idempotency key or blind native retry is forbidden.
- `reject`: a structural, idempotency, lease, authority, provider or revision fence failed.

The same idempotency key with a different operation ID or logical request fingerprint is a conflict and cannot be repurposed.

## Lease and scope

The immutable request carries the exact lease ID and lease expiry it was authorized under. A first execution candidate requires the corresponding lease to be active, unexpired and scoped to:

- the selected backend,
- the selected authority domain,
- the exact required write capability,
- every stable resource identity named by the request.

The lease authority generation must equal both the request's expected authority generation and the ownership generation captured in the immutable provider selection.

## Explicit provider binding

The required write capability is part of `BackendAgentLocalProviderSelection`; it is not inferred from provider availability and cannot be replaced by another available adapter.

For a first execution candidate the existing provider-selection contract is revalidated against current explicit ownership and current provider facts. Provider ID/kind, instance epoch, provider generation, capability revision, ownership generation and capability authorization must therefore still match exactly.

This contract introduces no fallback list and no provider discovery endpoint. A different provider requires a new explicit ownership/selection decision and a new protected-write request rather than silent continuation.

## Resource revision fence

Every requested resource has:

- `resourceType`,
- stable `resourceId`,
- `expectedRevision`.

Immediately before a first native write, the caller supplies an authoritative observed snapshot containing the exact same resource identities and their current revisions. Missing state or any revision mismatch rejects the write. This check occurs after identity/idempotency, lease and authority/provider checks.

## Required post-write sequence

A future runtime implementation may cross the native mutation boundary only after `reserveThenExecuteOnce` and after it has durably reserved the idempotency key. The required sequence remains:

1. durable idempotency identity/reservation,
2. fresh lease and scope validation,
3. expected authority generation and provider-selection validation,
4. expected resource revision validation,
5. exactly one native write,
6. durable result/evidence persistence,
7. authoritative readback,
8. durable reconciliation/compensation state if readback cannot confirm the outcome.

A lost response never permits a fresh idempotency key. A reserved or unknown outcome must be reconciled before any later decision about the same logical operation.

## Relationship to jobs and sagas

ADR-0043 remains the execution model for eventual runtime integration. Retry eligibility is not granted by a job retry counter: any native retry must remain safe under ADR-0042 and the same durable idempotency identity. Unknown native outcome is not a normal retryable transport failure.

This slice deliberately does not add a job repository, claim loop, compensation handler or reconciliation worker. Those runtime pieces must consume this contract rather than weaken it.

## Deferred

The next concrete write phase must define a domain-specific intent and capability, then wire durable idempotency persistence, lease issuance/verification, resource revision lookup, provider-selected native execution and authoritative readback. Phase 64 TimerIntent remains deferred until that bounded implementation slice.

No public mutation surface, protected native operation or `mutations=enabled` switch is added here.

## Validation

The contract test proves:

- a fully current request becomes only a durable-reservation-first execution candidate,
- an exact completed duplicate returns persisted evidence without a second write,
- reserved/unknown outcomes are reconciliation-only even when the old lease/provider fence is stale,
- changed payload under the same idempotency key fails before lease checks,
- inactive/changed/expired or insufficiently scoped leases fail closed,
- authority-generation mismatch wins before resource-revision mismatch,
- stale provider selection fails before revision evaluation,
- missing or stale resource revisions reject the write,
- authoritative readback is mandatory,
- the required write capability is immutable inside the provider selection.

The architecture guard also proves the new type is not wired into Agent command delivery, native-probe delivery or VDR adapter runtime. Because this is a contract-only slice with no installed runtime behavior change, no real yaVDR acceptance is required.
