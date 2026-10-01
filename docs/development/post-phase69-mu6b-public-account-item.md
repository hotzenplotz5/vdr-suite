# MU.6B Public Account Item + Revision/ETag

## Status

Completed by this bounded MU.6B candidate.

Baseline live `main` at slice start:

```text
c99128935e655b882b7820d563a9a42b83d1d3b3
```

The slice started with no open pull requests.

## Purpose

MU.6A already established the authoritative Human Account lifecycle state and
persisted monotone Account revision. MU.6B exposes the smallest stable
read-only Public-v1 item contract over that existing authority so later MU.6C
mutations can use the same opaque revision as their precondition basis.

MU.6B does not add Account mutation, Account CREATE, grant administration,
credential/session administration or an administration UI.

## Public-v1 Account item

The stable item route is:

```text
GET /api/v1/accounts/{accountId}
```

The representation is secret-free and contains only the existing stable Account
read fields:

```text
accountId
actorId
displayName
active
links.self
```

The persisted numeric Account revision is not exposed as a JSON field. It is
mapped server-side to an opaque resource revision and returned as a strong ETag.

Clients must treat the ETag as opaque.

## Conditional read semantics

The Account item reuses the accepted Phase-69 revisioned-resource semantics.

A normal successful read returns:

- HTTP 200;
- the secret-free Account representation;
- one strong ETag;
- the normal Public-v1 request/correlation headers.

A caller may send `If-None-Match` with the previously observed ETag.

- matching current state returns HTTP 304 with the current ETag and no body;
- a non-matching valid condition returns the current HTTP 200 representation;
- malformed `If-None-Match` follows the existing Public-v1 invalid-request
  problem semantics.

The Account revision remains the concurrency basis for later MU.6C mutation,
but MU.6B itself is read-only and does not accept `If-Match`.

## Authorization

Both Account collection and Account item reads use the existing global
`accounts.view@*` authority.

Authorization remains server-side before Public-v1 dispatch.

A backend-scoped `accounts.view@default` or `role.admin@default` does not
become global Account administration authority. An appropriate global grant,
including the accepted `role.admin@*` expansion, may satisfy the read.

## Runtime ownership

The HTTP/runtime layer does not query SQLite directly.

The productive item lookup reuses:

```text
PublicApiRuntime
  -> HumanAccountReadService::find(accountId)
  -> HumanAccountRepository::findByAccountId(accountId)
  -> existing Suite security database
```

The existing MU.6A Human Account revision is converted to an opaque internal
resource revision before the Public-v1 strong ETag is generated.

No second Account store, ACL store or read authority is introduced.

## Failure semantics

The item route preserves the existing Public-v1 problem boundary:

- malformed/invalid Account identifier -> invalid-request response;
- unknown Account identifier -> HTTP 404;
- Account storage/read failure -> HTTP 503;
- anonymous request -> HTTP 401 at the security gate;
- authenticated caller without `accounts.view@*` -> HTTP 403.

Request/correlation semantics remain unchanged.

## Collection remains unchanged

The existing read-only collection remains:

```text
GET /api/v1/accounts
```

Collection ordering, cursor semantics and representation are unchanged.
MU.6B deliberately does not add a collection ETag and does not expose the
per-item Account revision in the collection representation.

This keeps collection caching/concurrency semantics separate from the mutable
single-resource precondition boundary required by later MU.6C.

## Reference client

The existing JavaScript Public-v1 reference client now exposes a bounded
`getAccount(...)` helper.

It:

- constructs only `/api/v1/accounts/{accountId}`;
- rejects path delimiters in the Account identifier;
- uses the existing revisioned GET helper;
- exposes the returned ETag opaquely;
- accepts caller-supplied `If-None-Match`;
- treats HTTP 304 as an explicit terminal conditional-read result;
- adds no mutation helper.

## Regression coverage

Focused regression coverage proves:

- successful Account item read;
- stable strong ETag delivery;
- matching `If-None-Match` -> HTTP 304;
- malformed conditional header -> HTTP 400;
- unknown Account -> HTTP 404;
- unavailable storage/read authority -> HTTP 503;
- anonymous and unauthorized requests are rejected;
- `accounts.view@*` and `role.admin@*` authorize the item;
- backend-scoped equivalents do not;
- password, credential, session, grant and raw revision material do not appear
  in the item body;
- the existing Account collection remains usable and has no ETag;
- the reference client preserves opaque ETag semantics.

## MU.6 continuation

MU.6 remains in progress after MU.6B.

```text
MU.6A Account lifecycle authority foundation            [DONE]
MU.6B Public Account item + revision/ETag               [DONE]
MU.6C Public display-name / activate / deactivate       [NEXT - NOT STARTED]
MU.6D Atomic Account CREATE + durable idempotency       [PLANNED]
```

MU.6C is the next bounded slice. It may map the already accepted
`accounts.modify@*`, `accounts.activate@*` and `accounts.deactivate@*`
permissions plus strong `If-Match` preconditions onto the existing MU.6A
lifecycle authority.

MU.6D Account CREATE remains separate. MU.7 grant administration, MU.8
credential/session administration and MU.9 browser administration UI remain
later slices.

Phase 70 remains not started and is not authorized by this Multiuser slice.
