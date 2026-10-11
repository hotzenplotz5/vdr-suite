# VDR-Suite API — developer entry point

## Navigation

- [Project README](../README.md)
- [Documentation index](../docs/index.md)
- [REST source boundary](rest/README.md)
- [Public API v1 OpenAPI 3.1 — source-audited partial contract](openapi.json)
- [Backend discovery developer contract](backend-discovery.md)
- [Federated Channel collection developer contract](channel-collection.md)
- [TimerAssignments and Timer CREATE developer contract](timer-assignments.md)
- [Durable Operation read and reconciliation](operation-status.md)
- [Human Accounts and permission grants](accounts-and-grants.md)
- [TV/Device pairing bootstrap and credential issuance](device-pairing.md)
- [Administrator pairing approval and Device grant management](device-administration.md)
- [Human Account create/lifecycle/grant mutations](account-mutations.md)
- [Device and Credential revocation/rotation](device-lifecycle.md)
- [Public API compatibility ADR-0048](../docs/adr/ADR-0048-public-api-versioning-error-compatibility-contract.md)
- [Public v1 reference client](../clients/reference-js/README.md)

## Status and scope

**Documentation bootstrap — 2026-10-11; source baseline main `8d8dcfbf26d0b6760ad81dc4b2af4fbbd15d55b0`.**

The OpenAPI file currently specifies exactly **26 source-verified HTTP method/resource contracts** across **17 path templates**: discovery root/capabilities, Backend and Channel collections, TimerAssignment collection and item GET, protected TimerAssignment item POST for Timer CREATE, and actor-owned Operation item GET, with selected Account/Device security administration contracts detailed below. Its coverage is explicitly **partial**. This document must not be interpreted as a complete route inventory, a new compatibility guarantee, or proof that a particular daemon build is installed.

The public-v1 contract is distinct from older or browser-private `/api/...` routes, transport/provider implementation details, and the independent acceptance state of the VIDAA app.

## Getting started

Use the origin and reverse-proxy base path of **your own** VDR-Suite deployment, then read the discovery root:

```bash
SUITE_BASE='https://YOUR-SERVER/VDR-SUITE-BASE'
curl --fail-with-body -sS "${SUITE_BASE}/api/v1"
curl --fail-with-body -sS "${SUITE_BASE}/api/v1/capabilities"
```

Here `SUITE_BASE` is the actual externally reachable URL prefix immediately before `/api/v1`; deployments may expose the API at the site root rather than under a proxy prefix. The discovery responses describe compatibility metadata and runtime capability availability; capability `available` is **not** an authorization grant.

Both discovery handlers can return their contract without an authenticated actor. A gateway, reverse proxy or other public-v1 resource may still require authentication. Never infer write permissions from discovery responses. Clients should supply their own supported credentials and headers, rather than assuming browser cookies or falling back to private routes.

## Documented Backend collection

The [Backend discovery guide](backend-discovery.md) covers `GET /api/v1/backends`, filtered identity results, `limit`/`cursor` pagination, the authorization-scope boundary, and public Problem Details errors. The machine-readable contract documents the request and response shape but **does not yet define all supported deployment authentication schemes**.

## Documented Channel federation

The [Channel federation guide](channel-collection.md) documents explicit 1–16 backend scope selection, `channels.view` authorization, composite identity, keyset pagination, per-source partial success and structured failures. A 200 response can include unavailable sources.

## Documented TimerAssignments and Operations

The [TimerAssignment developer contract](timer-assignments.md) covers Backend-scoped keyset pagination, minimal item reads with `ETag`/conditional `304`, and safe CREATE admission requiring `If-Match`, `Idempotency-Key` and an empty `{}` request body. The [Operation read contract](operation-status.md) describes the durable actor-owned status record and explains why HTTP `200`/`202` must not be confused with successful native Timer execution. This corresponds to the eight historical Phase-69.F stable method/resource pairs, while the full modern source contains additional separately gated endpoints.

## Documented Account and Device bootstrap contracts

The [Account/Grant guide](accounts-and-grants.md) documents three global-administration read routes and separate Account vs grant-set ETags. The [Device pairing guide](device-pairing.md) documents anonymous bootstrap, token-only polling and one-time Device credential issuance. The [administrative Device guide](device-administration.md) additionally specifies pairing decision and Device grant administration (including scoped grant mutation). These operations **do not** mean the full Account/Device credential/session metadata, revoke/rotation or private client surface is documented, and a Device still has **zero automatic content grants** after pairing.

## Documented Account mutations and Device lifecycle

[Account mutation guidance](account-mutations.md) now covers global account CREATE with durable Idempotency-Key, ETag-fenced Account lifecycle changes and grant-set desired-state writes. [Device lifecycle guidance](device-lifecycle.md) covers Device/Credential revocation and one-time Credential rotation, including the Browser Session requirement and unrecoverable-secret failure mode. No installed-runtime acceptance is inferred from this documentation-only branch.

## What is implemented but not yet in this OpenAPI subset?

The source contains additional public-v1 areas including Account and Device grants/lifecycle **mutations**, other Account/Device lifecycle mutations, Credential revoke/rotation, Recording browse, and Device-owned Recording playback/media-track flows. **None** of those is automatically documented or promoted merely by appearing in implementation code.

For extension work, audit the exact route, method, response, authentication/permission rules, error cases, tests, and accepted contract status **before** adding each operation to `openapi.json`. The historical [Phase 69.F client matrix](../docs/development/phase-69f-client-contract-matrix.json) is bounded evidence, **not** a complete present-day route manifest.

Relevant implementation and verification entry points:

- [Public-v1 runtime](rest/src/PublicApiRuntime.cpp) and [headers](rest/include/PublicApiRuntime.h)
- [HTTP router](rest/src/ApiRouter.cpp) and [security gate](../core/security/include/SecurityHttpGate.h)
- [Public-v1 inventory guard](../tools/check_phase69_public_api_inventory.py)
- [Phase 69.F contract matrix guard](../tools/check_phase69f_client_contract_matrix.py)
- [Public-v1 reference client](../clients/reference-js/public-v1-client.js)
- [Merged Device Recording playback contract](../docs/development/public-v1-device-recording-playback.md)

## Maintenance rule

API changes must be reviewed together with the **matching developer contract**, tests and classification (public-v1, pre-v1 transition, first-party private, or internal/provider). Updating a static OpenAPI file alone does **not** prove conformance. Automated route/spec drift enforcement has **not yet been implemented** as part of this documentation bootstrap and remains required follow-up work.

## Back

- [Project README](../README.md)
- [Documentation index](../docs/index.md)
