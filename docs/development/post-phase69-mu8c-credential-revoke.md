# MU.8C — Human-password Credential Revoke

Status: **IMPLEMENTATION MERGED — PR #417 / FOCUSED YAVDR PASS / HOSTED CI GREEN**

MU.8C is the bounded Credential-administration successor to MU.8B. It adds
revisioned single-Credential reads and explicit administrative revoke for Human
Account `human-password` credentials without expanding into password reset,
password rotation, credential creation, admin UI, or client pairing.

## Public-v1 contract

MU.8C owns:

- `GET /api/v1/accounts/{accountId}/credentials/{credentialId}`
- `POST /api/v1/accounts/{accountId}/credentials/{credentialId}`

GET requires `accounts.credentials.view@*`.

POST requires `accounts.credentials.revoke@*`, is a protected browser
mutation, and therefore uses the established browser CSRF contract.

The Credential collection remains read-only:

- `GET /api/v1/accounts/{accountId}/credentials`

POST to the collection remains method-not-allowed.

## Safe representation

The public Credential item contains only bounded lifecycle metadata:

- `accountId`
- `actorId`
- `credentialId`
- `credentialType`
- `active`
- `expired`
- `revoked`
- `expiresAt`
- `createdAt`
- `links.self`

It never exposes password verifier hashes, browser-session secret hashes, CSRF
secret hashes, token IDs, cookies, internal browser Credential IDs, or the raw
internal revision string.

## Credential-owned concurrency

MU.8C does not reuse Human Account revision state.

Each public Human-password Credential item owns an opaque lifecycle revision:

`credential-lifecycle:<sha256>`

The hash is derived from normalized Credential lifecycle facts. Public clients
receive it only through the standard strong Public-v1 ETag.

GET supports `If-None-Match`.

POST requires exactly one canonical strong `If-Match` value:

- missing precondition -> HTTP 428,
- malformed or wrong resource-revision type -> HTTP 400,
- stale lifecycle revision while the Credential remains non-terminal -> HTTP 412.

## Revoke semantics

The mutation body is exactly an empty JSON object: `{}`.

Only `human-password` Credentials are revocable through this bounded public
slice. Other Credential types fail validation rather than becoming a second
lifecycle authority.

Revoke is synchronous and terminal:

1. resolve the Account-owned Credential in one `BEGIN IMMEDIATE` transaction;
2. validate its Credential-owned revision;
3. protect the final usable administrator;
4. revoke the canonical Human-password Credential;
5. find active Browser Sessions whose `issued_from_credential_id` names that
   Credential;
6. revoke each issuer-session through the canonical
   `BrowserSessionLifecycleService`;
7. read back the terminal Credential state;
8. append accountability evidence and commit.

This issuer-session fencing prevents an already-issued Browser Session from
retaining authority after its Human-password issuer is revoked.

Replay is desired-state idempotent:

- exact current revision + already-terminal Credential -> success,
- stale revision + already-terminal Credential -> success,
- stale revision + still-active Credential -> revision conflict,
- replay never reactivates or recreates authority.

## Final usable administrator invariant

MU.8C protects the last usable administrator at **Credential granularity**.

Revoking one Human-password Credential is allowed when the same administrator
still has another active, unexpired, unrevoked Human-password Credential with a
resolvable verifier, or when another usable administrator exists.

The mutation is rejected with HTTP 409 when revoking the target Credential
would leave zero usable administrators.

This deliberately does not reuse
`countUsableAdministratorsExcludingActor()`, because excluding an entire Actor
would incorrectly reject an administrator who owns two independently usable
Human-password Credentials.

## Authorization and accountability

`accounts.credentials.revoke@*` is a global administration mutation
permission.

- `role.admin@*` may satisfy it.
- backend-scoped `role.admin@backend-id` does not satisfy the global scope.
- `role.read-only@*` denies the mutation.
- browser mutations remain CSRF-protected.
- mutation outcomes are appended to the established accountability store.

The administration audit action is:

`human-account.credential.revoke`

Relevant reason codes include:

- `credential_revision_conflict`
- `credential_type_not_revocable`
- `final_usable_administrator`
- `credential_revoked`
- `credential_already_terminal`

## Capability

When productive Credential-item read and mutation callbacks are both
configured, `/api/v1/capabilities` advertises:

`public-api.accounts-credential-revoke`

## Explicit non-goals

MU.8C does not implement:

- password reset,
- password rotation,
- Human-password Credential creation,
- browser-session Credential revoke as a separate public resource,
- bulk Credential revoke,
- Account/admin UI,
- TV/app pairing,
- Profile or household policy.

Local root recovery remains governed by ADR-0066.

## Acceptance state

Focused acceptance on the real yaVDR checkout passed without a daemon build,
installation, restart or runtime mutation. PR #417 merged the accepted MU.8C
implementation into `main` as `a3854381186e89169bf4ac834b564592cc48adb4`.
The PR CI and the post-merge `main` push CI both completed successfully.

MU.8 backend credential/session administration is therefore complete. Browser
administration continues in MU.9.
