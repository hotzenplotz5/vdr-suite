# Public-v1 durable Operations — developer contract

## Navigation

- [API developer entry point](README.md)
- [Partial OpenAPI 3.1 reference](openapi.json)
- [TimerAssignment and Timer CREATE contract](timer-assignments.md)
- [Public Operation resource acceptance](../docs/development/phase-69c-public-operation-resource.md)
- [Reference Operation client acceptance](../docs/development/phase-69f-public-v1-operation-reference-client.md)

## Stable HTTP resource

```http
GET /api/v1/operations/{operationId}
```

Accepted Phase 69.C actor-owned read, cross-checked with current source at `main=8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0`. There is **no** public `/api/v1/operations` collection, cancellation or mutation route in this bounded contract. Source-verified does not mean a particular daemon version has been installed.

The authenticated caller can inspect only their **own** durable Operations. A missing Operation and one owned by another actor both produce `404 not_found`; never probe across actors.

## Read model and ETag

```json
{"operationId":"op_1","state":"queued","backendId":"backend-one","links":{"self":"/api/v1/operations/op_1"}}
```

The server returns a strong opaque `ETag` derived from the owning durable Operation revision, plus `Cache-Control: no-store`. Do not parse the ETag or invent an Operation revision from it. An optional `If-None-Match` may return `304` with **no body**, still including the current ETag. Invalid entity-tag syntax gives `400 invalid_request`.

## HTTP success is not mutation completion

An HTTP `200` only proves that the Operation record was retrieved. The `state` field expresses the real durable lifecycle:

```text
accepted · rejected · conflict · queued · dispatching
executed_unverified · succeeded · failed_before_dispatch
failed_verified · outcome_unknown · cancelled
```

Treat `outcome_unknown` as unresolved, **not** as failed or succeeded. Do not issue a replacement Timer POST on a timeout, 5xx or outcome-unknown read. Do not assume `202` admission immediately changes the native VDR state. The caller decides whether and when to make another **explicit** GET; no automatic polling/retry is provided by the reference client.

The public Operation does not expose idempotency keys, payload/fingerprints, internal result references or the raw `operationRevision`.

## Error results

| HTTP | Meaning |
| --- | --- |
| `200` | Visible durable Operation read; check `state` |
| `304` | Conditional read unchanged; bodyless |
| `400` | Malformed condition or identifier |
| `401` | Missing valid actor identity |
| `404` | Missing OR foreign Operation, deliberately indistinguishable |
| `405` | Unsupported method; `Allow: GET` |
| `503` | Required operation read service unavailable |

Public runtime errors are `application/problem+json`; earlier security-gate denials are possible. No undocumented fallback to browser-private operations is permitted.

## Verified implementation and regression anchors

- [Public Operation serializer and lookup](rest/src/PublicApiRuntime.cpp)
- [Durable Operation resource type](rest/include/PublicApiRuntime.h)
- [Operation ownership security gate](../core/security/include/SecurityHttpGate.h)
- [Operation HTTP regression](rest/tests/test_public_operation_resource.cpp)
- [JavaScript Operation reference tests](../clients/reference-js/tests/test_public_v1_operation_client.js)
- [Reference client](../clients/reference-js/public-v1-client.js)

## Back

- [API entry point](README.md)
- [Documentation index](../docs/index.md)
