# Phase 69.C — Timer CREATE Suite-Owned Identity Authority

## Status

**ACTIVE — bounded prerequisite after accepted PR #330.**

Baseline:

```text
main=27d040ec362264bdd3de01b1b8b65c8301269748
PR #330 / CI #9124=ACCEPTED
69.C=ACTIVE
```

Binding architecture:

- [ADR-0042](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0044](../adr/ADR-0044-timer-intent-assignment-native-timer-model.md)
- [ADR-0048](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [Shared Mutation Operation Repository](phase-64-mutation-operation-repository.md)
- [NativeTimerBinding Persistence Repository](phase-64-native-timer-binding-repository.md)
- [Native Timer CREATE Operation Preparation](phase-64-native-timer-create-operation-preparation.md)

## Proven live gap

After PR #330, the production daemon owns the complete dormant pre-dispatch
chain required for native Timer CREATE, including the accepted
`TimerAssignmentFulfillmentService`.

The remaining admission prerequisite was stable Suite-owned allocation for two
identities required before `NativeTimerCreateOperationPreparationService` can
reserve the immutable CREATE handoff:

- `operationId`;
- `nativeTimerBindingId`.

The live repository had no generic Control-Plane issuer for either identity.

Existing generators are not valid substitutes:

- `backendAgentGenerateOpaqueId()` belongs to the Agent/local-command layer;
- metadata identity generation belongs to the metadata domain;
- browser-session retention's local event helper belongs to security
  maintenance.

Using any of those as the public Timer mutation owner would invert dependency
direction or create accidental cross-domain authority.

## Domain ownership

This slice keeps the two identities distinct.

### Mutation operation identity

`core/operations` issues newly created ADR-0042 operation identities:

```text
op_<32 lowercase hex characters>
```

The suffix contains 16 random bytes.

### Native Timer binding identity

`core/timers` issues newly pre-reserved managed binding correlations:

```text
ntb_<32 lowercase hex characters>
```

The suffix likewise contains 16 random bytes.

The Timer domain does not ask the Agent to invent the binding identity after
dispatch. The identity exists before preparation and is persisted in the
immutable native Timer CREATE operation payload.

## Failure behavior

Generation fails closed by returning an empty value if the platform random
source cannot produce an ID.

There is no fallback to:

- wall-clock or steady-clock time;
- process ID;
- local sequence counter;
- frontend-supplied operation identity;
- Agent-generated operation/binding identity.

The later admission owner must reject an empty generated identity before
`beginProvisioning()` or operation reservation.

## Compatibility boundary

Existing Phase-64 durable contracts intentionally accept a broader bounded
identity shape. Historical rows and accepted internal callers already contain
values such as:

```text
op_create_dispatch_1
native-timer-binding:1
```

This slice does **not** make those values invalid.

`mutationOperationIdCanonical()` and
`nativeTimerBindingIdCanonical()` describe only canonical **new issuance**.
They are not retrofitted into generic durable-domain validation.

This preserves restart/read compatibility while giving the future public-v1
admission one stable Suite-owned issuance format.

## Runtime composition

Both issuer implementations are linked exactly once into the daemon source
graph so the later admission owner can consume them without adding Agent,
metadata or security dependencies.

They remain stateless and own no lifecycle. Durable operation lifecycle remains
solely in `MutationOperationRepository`; durable binding revision/state remains
solely in `NativeTimerBindingRepository`.

## Dormant boundary

This slice does not invoke either issuer from PublicApiRuntime or
SecurityHttpGate.

It does not add:

- public Timer POST routing;
- `timers.create` public-v1 admission;
- `If-Match` mutation handling;
- `Idempotency-Key` mutation handling;
- `beginProvisioning()`;
- CREATE preparation/reservation/claim/activation calls;
- Agent, SuiteBridge or VDR mutation.

The accepted `NativeTimerCreateOperationPreparationService` continues to
**consume** the IDs supplied in its request. It must never generate replacement
IDs on replay or conflict.

## Acceptance

CI/build/architecture validation is sufficient because the issuers are dormant.

The focused regression/guard proves:

- generated operation IDs use exactly `op_` plus 128 bits of lowercase hex;
- generated binding IDs use exactly `ntb_` plus 128 bits of lowercase hex;
- malformed, wrong-prefix, wrong-length and uppercase forms are rejected by the
  canonical-new-issuance checks;
- the two identity classes are not interchangeable;
- legacy-compatible durable IDs are not mislabeled canonical;
- neither issuer depends on Agent, Metadata, browser-session retention, clocks
  or counters;
- both sources are linked exactly once;
- preparation still consumes rather than generates identity;
- public-v1 mutation routing/security remains closed.

Real yaVDR acceptance is not required until native CREATE can actually be
activated.

## Next bounded slice

After this identity prerequisite is accepted, the repository has the required
authorities to implement the first public Timer CREATE admission/orchestration
owner.

That later slice must still prove, in order:

```text
authenticate / authorize timers.create for backend
-> lookup TimerAssignment only after authorization
-> require strong If-Match
-> require and validate Idempotency-Key
-> validate closed public request JSON
-> generate op_... + ntb_... exactly once for a new logical request
-> beginProvisioning()
-> prepare()
-> reserve Agent command
-> claimAfterReservation()
-> activateDispatching()
-> return 202 + durable operation resource
   Location: /api/v1/operations/{operationId}
```

Idempotent replay must recover the existing durable operation and its immutable
payload rather than minting replacement identities or blindly dispatching
again.
