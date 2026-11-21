# Phase 69.B Closeout — Common Request/Response Metadata and Errors

## Status

**COMPLETED**

Phase 69 remains active. The next bounded runtime slice is:

```text
69.C - Revision/precondition/idempotency exposure
```

Binding architecture: [ADR-0048](../adr/ADR-0048-public-api-versioning-error-compatibility-contract.md).

## Accepted runtime checkpoints

```text
PR #316 -> 72a637cc6f18fc1c1c1a77859abb3ab1f15e2b03
CI #9081 / 35843808669 -> SUCCESS (6/6)

PR #317 -> 5ff36d635072c63e1c46ccb377c830ec99988108
CI #9083 / 35846333459 -> SUCCESS (6/6)

PR #318 -> e9d59b87349d7e1be9356efc5cd656f41e11ea12
CI #9085 / 35861961215 -> SUCCESS (6/6)

PR #319 -> 0248db3d63626a87d391f7649a984adc97b45232
CI #9087 / 35863518112 -> SUCCESS (6/6)
```

No real-yaVDR acceptance was repeated for 69.B. The accepted work is confined
to public HTTP request/error metadata, routing and serialization. It does not
change VDR, SuiteBridge, Agent, Timer, MediaSession, Broadcast Companion or
Legacy OSD native/domain behavior.

## Delivered contract

### Request and correlation identity

Phase 69.B reuses the Phase-62 request-context authority.

- every current public-v1 response carries the accepted `X-Request-ID`;
- accepted `X-Correlation-ID` is propagated when present;
- public Problem Details repeat the same identifiers as structured fields;
- request IDs remain request identity only and do not become idempotency keys.

### Public Problem Details

One lower-level `PublicProblemDetails` model/serializer now owns the current
public error representation. It is located below the REST/security split so
both `PublicApiRuntime` and `SecurityHttpGate` can consume it without making
`core/security` depend on `api/rest`.

Current public errors use:

```text
Content-Type: application/problem+json
Cache-Control: no-store
X-Content-Type-Options: nosniff
```

and the ADR-0048 required fields:

- `type`;
- `title`;
- `status`;
- stable public `code`;
- `requestId`.

Optional `detail`, `instance` and `correlationId` are emitted only when the
owning layer has that context.

### Stable public codes

Internal Phase-62 reason codes remain accountability/authorization evidence.
They are not automatically frozen as public API codes.

Security failures are normalized onto ADR-0048 public categories such as:

- `unauthorized`;
- `forbidden`;
- `invalid_request`;
- `read_only_backend`;
- `service_unavailable`.

Public routing uses stable codes including:

- `not_found`;
- `method_not_allowed`.

Human-readable `title` and `detail` are not machine branch keys.

### HTTP routing/status semantics

For the currently declared v1 resource set:

- unknown GET resources below `/api/v1/...` return `404 not_found`;
- POST to the current GET-only v1 resources returns
  `405 method_not_allowed` plus `Allow: GET`;
- other unsupported methods on known current v1 resources also return
  `405 method_not_allowed` plus `Allow: GET`;
- unsupported methods on unknown v1 paths return `404 not_found`, avoiding a
  false assertion that the resource exists;
- unknown/future v1 POST mutations remain fail-closed at the Phase-62 security
  boundary until an explicit authorization/mutation contract exists;
- pre-v1 `/api/...` compatibility error bodies are not rewritten by 69.B.

## Retry and deprecation metadata boundary

ADR-0048 requires the response abstraction to carry safe metadata such as
`Retry-After`, `Deprecation`, `Sunset`, `Link` and `Allow`.
That structural capability already exists through arbitrary safe response
headers and is proven by current `Allow` propagation.

69.B deliberately does **not** invent retry or deprecation values where there is
no owning domain semantic yet:

- `Retry-After` is emitted only by a later resource/operation contract that can
  state a truthful retry interval;
- revision/precondition/idempotency semantics belong to 69.C;
- collection/partial-source retry semantics may be introduced with 69.D;
- `Deprecation`, `Sunset` and successor-link policy are implemented under
  69.E.

This is a bounded ownership decision, not missing transport support.

## Safety and compatibility

69.B preserves:

- Phase-62 authentication, authorization, CSRF and accountability decisions;
- fail-closed handling of unmigrated mutations;
- pre-v1 route/error compatibility;
- separate Agent, media, plugin and Legacy OSD protocol/version boundaries;
- no speculative mutation retry;
- no native runtime dispatch from error handling.

## Acceptance

Phase 69.B is accepted because the current public-v1 foundation now has:

- one request/correlation identity authority;
- one common public Problem Details serializer;
- stable public error codes distinct from internal reason strings;
- correct current routing/status behavior for 401/403/404/405 and service
  failures crossing the security boundary;
- safe response-header propagation;
- negative contract coverage;
- full hosted CI success on each accepted runtime checkpoint.

The next bounded runtime work is 69.C
**Revision/precondition/idempotency exposure**. Existing ADR-0042 resource and
operation ownership must be reused rather than replaced.
