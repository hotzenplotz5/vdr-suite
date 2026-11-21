# MU.8B — Session Revoke

Status: **IMPLEMENTATION MERGED — PR #416 / FOCUSED YAVDR PASS / HOSTED CI GREEN**

MU.8B is the bounded Session-administration successor to MU.8A. It adds
revisioned single-Session reads and explicit administrative revoke without
expanding into credential revoke, password reset, password rotation, client UI,
or unrelated identity work.

## Public-v1 contract

MU.8B owns:

- `GET /api/v1/accounts/{accountId}/sessions/{sessionId}`
- `POST /api/v1/accounts/{accountId}/sessions/{sessionId}`

The GET uses `accounts.sessions.view@*`.

The POST uses `accounts.sessions.revoke@*`, is a protected browser mutation,
and therefore requires the established browser CSRF contract.

The Session collection remains read-only:

- `GET /api/v1/accounts/{accountId}/sessions`

POST to the collection remains method-not-allowed.

## Safe representation

The public Session item contains only bounded lifecycle metadata:

- `accountId`
- `actorId`
- `sessionId`
- `deviceId`
- `issuedFromCredentialId`
- `active`
- `expired`
- `revoked`
- `expiresAt`
- `lastSeenAt`
- `createdAt`
- `links.self`

The response never exposes:

- browser-session credential IDs used internally for canonical revoke,
- password hashes,
- browser-session secret hashes,
- CSRF secret hashes,
- token IDs,
- cookies,
- raw verifier material,
- the internal revision string.

## Session-owned concurrency

MU.8B does **not** reuse Human Account revision state.

Each Session item owns an opaque lifecycle revision derived by SHA-256 from
normalized Session lifecycle facts across:

- the browser-session row,
- the canonical Session,
- the canonical browser-session credential,
- the issuing credential.

The internal revision is prefixed with `session-lifecycle:` and is exposed only
through the standard strong Public-v1 ETag encoding.

Ordinary `lastSeenAt` activity is deliberately excluded from the lifecycle
revision. Passive Session use therefore does not create false administrative
revoke conflicts.

GET supports `If-None-Match`.

POST requires exactly one canonical strong `If-Match` value:

- missing precondition -> HTTP 428,
- malformed or wrong resource-revision type -> HTTP 400,
- stale lifecycle revision while the Session remains non-terminal -> HTTP 412.

## Revoke semantics

The mutation body is exactly an empty JSON object: `{}`.

The authoritative revoke path is the existing
`BrowserSessionLifecycleService`. MU.8B does not duplicate browser-session
revocation SQL.

Revision comparison, canonical revoke, terminal readback, accountability event,
and commit occur under one `BEGIN IMMEDIATE` transaction.

Successful revoke synchronously makes the Session terminal across the canonical
Session, browser-session credential, and browser-session credential row.

Replay semantics are desired-state idempotent:

- exact current revision + already-terminal Session -> success,
- stale revision + already-terminal Session -> success,
- stale revision + still-active Session -> revision conflict,
- replay never reactivates or recreates authority.

The successful response returns the terminal Session representation and its new
strong ETag.

## Authorization and accountability

`accounts.sessions.revoke@*` is a global administration mutation permission.

- `role.admin@*` may satisfy it.
- backend-scoped `role.admin@backend-id` does not satisfy the global scope.
- `role.read-only@*` denies the mutation.
- browser Session mutations remain CSRF-protected.
- mutation outcomes are appended to the established accountability store.

The administration audit action is
`human-account.session.revoke`.

Relevant reason codes include:

- `session_revoked`
- `session_already_terminal`
- `session_revision_conflict`

## Capability

When productive item-read and mutation callbacks are both configured,
`/api/v1/capabilities` advertises:

`public-api.accounts-session-revoke`

## Explicit non-goals

MU.8B does not implement:

- `accounts.credentials.revoke`,
- human-password credential revoke,
- final usable administrator checks for credential revoke,
- password reset or password rotation,
- bulk Session revoke,
- Session creation,
- client UI or TV authorization flows.

Human-password credential revoke and its final-usable-admin protection are
owned by the bounded MU.8C successor slice.

## Acceptance state

Focused acceptance on the real yaVDR checkout passed without a local daemon
build, installation, restart, or runtime mutation. PR #416 merged after its
Hosted CI completed successfully, and the post-merge push CI also completed
successfully. A live daemon installation/restart was not required to prove this
bounded Session-administration contract.
