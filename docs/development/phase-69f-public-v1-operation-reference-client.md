# Phase 69.F — Public-v1 Operation Reference Client

## Status

**ACCEPTED — thirteenth bounded 69.F slice via PR #381.**

Accepted evidence:

```text
PR #381
accepted head=f8034648907a02965c41cbf49be24b14f3e38356
merge/main=4171574f76eeb2d1a7aad9861a81de90451b820a
CI #9357 / run 36369528246 = SUCCESS (6/6)
```

Baseline:

```text
main=afd0e04708622d99a3ee82532e7d8608495d3f6b
phase69=ACTIVE
slice=69.F
```

Accepted predecessor:

```text
PR #380
accepted head=1b033576fab3c5322c14edc6405d5c6948dc50f3
merge/main=afd0e04708622d99a3ee82532e7d8608495d3f6b
CI #9352 / run 36367430072 = SUCCESS (6/6)
```

Binding architecture:

- [ADR-0048 Public API Versioning, Error and Compatibility Contract](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [Phase 69.C Public Operation Resource](phase-69c-public-operation-resource.md)
- [Phase 69.F Timer CREATE Admission Reference Client](phase-69f-public-v1-timer-create-reference-client.md)
- [Phase 69.F Client Contract Matrix](phase-69f-client-contract-matrix.md)

## Fresh post-#380 selection audit

The stable public-v1 inventory still contains exactly eight method/resource
contracts. After accepted PR #380 the reference client consumes seven. The only
remaining gap is:

```http
GET /api/v1/operations/{operationId}
```

This ordering is now coherent because Timer CREATE admission returns the
`operationId`, `Location` and current Operation ETag.

No other missing browser domain is promoted merely to extend Phase 69.F.
EPG/ProgramEvent, Recordings, SearchTimer, metadata, genre and other matrix
entries remain pre-v1/private until their own identity and representation
contracts are intentionally stabilized.

## Bounded client extension

The client gains:

```text
getOperation({
  operationId,
  ifNoneMatch,  // optional opaque Operation ETag
  ...request options
})
```

It performs one explicit request to:

```http
GET /api/v1/operations/{operationId}
```

No Operation collection is implied.

## Revision and conditional read

The accepted Operation resource maps its durable repository revision to a
strong opaque ETag. The client reuses the same revisioned-read helper as the
TimerAssignment item:

- normal visible item -> `{status: 200, etag, data}`;
- matching `If-None-Match` -> `{status: 304, etag, data: null}`;
- malformed server-side condition -> structured `400 invalid_request`.

The client does not decode or manufacture Operation ETags.

## Lifecycle semantics

HTTP `200` means only that the durable Operation representation was read. It
does not mean the mutation succeeded.

The client returns the reviewed public `state` unchanged, including terminal
or uncertain states such as:

```text
failed_verified
outcome_unknown
cancelled
```

The caller decides when to issue another explicit read. There is deliberately
no polling timer, retry loop or automatic state interpretation in this slice.

## Security/error boundary

The accepted actor-ownership boundary is preserved:

```text
owned operation -> visible
missing operation OR other actor's operation -> 404 not_found
```

The client does not distinguish or probe those cases. `503 service_unavailable`
and other public Problem Details remain structured client errors and cause no
alternate request.

## Hard boundaries

This slice does not:

- add `/api/v1/operations` collection;
- add cancellation or any Operation mutation;
- add automatic polling, interval or retry behavior;
- expose operation revision, idempotency key, fingerprints, mutation payload or
  internal result reference;
- alter Timer CREATE submission;
- change bundled browser routes;
- change server, VDR, SuiteBridge, Agent, Home or LiveTV behavior.

## Stable public-v1 coverage after this candidate

The matrix guard proves that the union of all public reference-client resources
equals the stable public-v1 method/resource inventory exactly:

```text
8 stable contracts
8 reference-covered contracts
0 uncovered stable contracts
0 extra inferred public contracts
```

Therefore the next justified work is a fresh 69.F/Phase-69 closeout audit. It
is not another automatically selected public route.

## Regression contract

The focused Node regression proves:

- exact Operation item URL;
- normal `200` returns reviewed representation + opaque ETag;
- uncertain `state=outcome_unknown` is returned unchanged;
- caller-provided ETag maps to `If-None-Match`;
- bodyless `304` is successful conditional-read state;
- invalid local Operation IDs/conditions dispatch zero requests;
- `400 invalid_request`, `404 not_found` and `503 service_unavailable`
  remain structured errors;
- no retry, polling or alternate route occurs.

No real yaVDR acceptance is required because this is a client-only consumer of
an already accepted durable read resource. Exact-head Hosted CI is the
Ready-for-Review gate.
