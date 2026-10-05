# MU.9F — Account CREATE UI

Status: **IMPLEMENTATION CANDIDATE**

MU.9F is the final currently justified bounded implementation slice of the
MU.9 Account/access browser administration surface. It exposes only the already
accepted MU.6D Public-v1 Human Account CREATE contract.

## Accepted server contract

The canonical reference client already owns `POST /api/v1/accounts` with
exactly `loginName`, `displayName`, initial request-only `password` and a
caller-owned `Idempotency-Key`.

The browser must reuse `createAccount(options)`; it must not create a second
HTTP implementation. Account CREATE requires `accounts.create@*`, browser CSRF
and the existing same-origin browser-session boundary. It is synchronous and
does not use `If-Match`.

## Browser ownership

One explicit Add-user action owns one CREATE intent and therefore one
Idempotency-Key. The browser validates only locally obvious required-field
emptiness, creates one bounded caller-owned key for that user action, forwards
active browser CSRF, calls `createAccount(...)` exactly once, never
automatically retries a failed CREATE and refreshes the Account list after
success.

The password remains request-only material. It is not stored in Settings state,
browser persistence, logs or error text. The password input is cleared when the
CREATE request is dispatched.

A successful CREATE does not automatically assign `role.admin`,
`role.read-only` or any backend grant. Access remains separate MU.7/MU.9E
administration.

## Failure semantics

- `401`: browser authentication required;
- `403`: global CREATE authority or CSRF denied;
- `400`: malformed request / Idempotency-Key;
- `415`: unsupported media type;
- `422`: invalid closed request fields;
- `409 operation_conflict`: login or other authoritative operation conflict;
- `409 idempotency_conflict`: same key reused with different non-secret intent;
- `503`: security/account persistence unavailable.

No `409` path is retried automatically.

## Deliberate exclusions

MU.9F does **not** add password reset/rotation, self-service password change,
general Credential creation, automatic role/backend-grant assignment, pairing,
Profiles or Phase 70 work. ADR-0067 remains authoritative: there is no accepted
public administrator password-reset/rotation contract to expose here.

## Acceptance

Focused acceptance must prove canonical `createAccount(options)` reuse, one
mutation request per user action, browser CSRF, same-origin credentials,
caller-owned Idempotency-Key, request-only password handling, Account-list
refresh after success, no automatic grants, explicit failure handling, no
automatic mutation retry, blank required fields failing before network mutation,
no `.innerHTML` introduction, MU.9A-E regression safety and the existing
Public-v1 Account CREATE reference-client contract.

Phase 70 remains **NOT STARTED**.

After MU.9F acceptance, perform an explicit MU.9 closeout audit rather than
inventing an MU.9G slice without demonstrated product-contract debt.
