# Phase 69.C — Public Revision and Precondition Foundation

## Status

**ACTIVE — first bounded 69.C slice.**

Baseline:

```text
main=c4b9fc66d0f1286e72e82406ffd1f49acf4b331a
phase69=ACTIVE
69.A=COMPLETED
69.B=COMPLETED
69.C=ACTIVE
```

Binding decisions:

- [ADR-0042](../adr/ADR-0042-safe-mutation-revision-idempotency-contract.md)
- [ADR-0048](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)

## Repository truth at 69.C start

The repository already contains strong internal revision/idempotency foundations:

- durable `MutationOperationRepository`;
- `operationRevision` with revision-fenced transitions;
- durable `idempotencyKey` plus normalized request fingerprint;
- backend-generation fencing;
- Timer-domain expected revisions and binding revisions;
- Agent-side protected-write idempotency evidence.

Those are internal/domain authorities. They are not yet equivalent to a public HTTP
precondition contract.

At the 69.C start baseline:

- no mutable public-v1 resource is yet declared;
- the daemon Public API composition does not expose
  `MutationOperationRepository` directly;
- therefore adding `/api/v1/operations/{id}` immediately would require a new
  runtime read boundary rather than a trivial route alias;
- old `/api/...` mutation routes must not be retrofitted silently and called
  stable v1 resources.

## First bounded slice

The first 69.C runtime-independent foundation introduces one shared HTTP-level
resource precondition utility:

```text
resourceRevision
  -> strong opaque ETag
  -> If-Match / If-None-Match evaluation
```

The contract:

- does not expose the raw resource revision directly in the ETag;
- produces a deterministic strong ETag from an opaque revision token;
- distinguishes missing, matched, not-matched and malformed conditions;
- supports wildcard conditions;
- supports comma-separated entity-tag lists;
- requires strong comparison for `If-Match`;
- permits weak comparison semantics for `If-None-Match`;
- rejects malformed entity-tag syntax rather than silently treating it as a miss.

This code lives in `core/http`, not `api/rest`, so future public resources and
security/application boundaries can share one transport contract without making
core domain code depend on REST.

## Explicit non-goals of this slice

This slice does **not** yet:

- add a new public v1 domain resource;
- expose a Timer, Recording or MutationOperation representation;
- add `428 Precondition Required` to an existing mutation;
- add `412 Precondition Failed` to an existing mutation;
- publish an `Idempotency-Key` endpoint contract;
- change existing pre-v1 routes;
- change Phase-64 mutation lifecycle or durable idempotency semantics.

## Accepted checkpoint

This foundation was accepted in PR #321:

```text
merge=beb98f6edab39d962bd6415db7be21cf145e05cb
CI=35873370127 / #9091 / SUCCESS (6/6)
```

## Next step

The next bounded 69.C slice is documented in
[MutationOperation Read Facade](phase-69c-operation-read-facade.md). It creates
the required actor-scoped read boundary and one shared DaemonRuntime repository
composition before any public operation route is added.

