# REST API implementation boundary — developer guide

## Navigation

- [API developer entry point](../README.md)
- [Source-audited partial Public-v1 OpenAPI contract](../openapi.json)
- [Backend discovery source contract](../backend-discovery.md)
- [Federated Channel collection source contract](../channel-collection.md)
- [TimerAssignment and protected Timer CREATE source contract](../timer-assignments.md)
- [Actor-owned durable Operations source contract](../operation-status.md)
- [Human Accounts and grants read contract](../accounts-and-grants.md)
- [TV/Device pairing and one-time credential issuance](../device-pairing.md)
- [Administrator pairing approval and Device grants](../device-administration.md)
- [Account mutation contract](../account-mutations.md)
- [Credential/Browser Session administration](../account-security-administration.md)
- [Device revoke and rotation contract](../device-lifecycle.md)
- [Public API compatibility contract (ADR-0048)](../../docs/adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [REST API developer boundary guide](../../docs/development/phase-56.26-rest-api-developer-boundary-guide.md)

## Responsibility

`api/rest` implements HTTP routing, parsing, public and internal controllers, response serialization and REST-specific validation. It does **not** make every reachable HTTP endpoint a versioned public client contract, and it does not make internal C++ types public SDK types.

## Source-of-truth map

- [ApiRouter](src/ApiRouter.cpp): main HTTP request routing and existing unversioned entry points.
- [PublicApiRuntime](src/PublicApiRuntime.cpp): many versioned `/api/v1` handlers, including the verified discovery root and capabilities routes.
- [PublicApiRuntime header](include/PublicApiRuntime.h): runtime interfaces; these are not the public wire schema.
- [Security HTTP gate](../../core/security/include/SecurityHttpGate.h): authentication and authorization classification, including Device-sensitive paths.
- [API developer entry point](../README.md): public-v1 documentation status and contract links.
- [Historical Phase-69.F matrix](../../docs/development/phase-69f-client-contract-matrix.json): frozen accepted coverage and separately classified successors.

## Required review for an API change

1. Confirm the exact HTTP method, request path, query/body, response and error serialization in current code and focused tests.
2. Check the security gate, required actor/Device grants and backend ownership; do not publish native VDR filesystem paths or provider credentials.
3. Classify the endpoint as public-v1, pre-v1, first-party private or internal; route existence alone does not establish stability.
4. If its public-v1 contract is verified and accepted, update [the OpenAPI specification](../openapi.json), examples, reference-client notes and relevant tests in the same implementation change.
5. Validate on the exact candidate revision; separately record deployment/runtime/TV acceptance where required. A successful source merge is not evidence of deployment.

**Current limitation:** the OpenAPI document covers 32 audited HTTP method/resource contracts across 21 URL templates, including discovery, Backend/Channel/TimerAssignment/Operation, Account mutations and grants, Device pairing, Device grants, revocation and credential rotation. It is not a complete public-v1 inventory; the operation-specific authentication alternatives still need a formal OpenAPI `securitySchemes` model and a bounded audited-handler-to-spec check now runs in `make test-docs`, but complete discovery and OpenAPI Schema/security equivalence checking remain open.

## Back

- [API developer entry point](../README.md)
- [Project README](../../README.md)
