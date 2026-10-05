# MU.9D — Human-password Credential Revoke UI

Status: **IMPLEMENTATION CANDIDATE**

Parent workstream: [Post-Phase-69 Multiuser Productization Workstream](post-phase69-multiuser-workstream.md)

Architecture authority: [ADR-0067 Human Account and Backend Access Administration](../adr/ADR-0067-human-account-backend-access-administration.md)

Backend contract authority: [MU.8C Human-password Credential Revoke](post-phase69-mu8c-credential-revoke.md)

Predecessor: [MU.9C Session Revoke UI](post-phase69-mu9c-session-revoke-ui.md)

## Why this slice is next

MU.9D is the smallest bounded successor after Session revoke because the
existing Settings surface already renders Credential metadata and the complete
server-side mutation contract already exists in MU.8C.

It is deliberately selected before grant mutation and Account CREATE:

- grant mutation needs permission/backend tuple selection and product-allowlist
  presentation, so it is a broader UI design slice;
- Account CREATE needs password-entry, durable idempotency and create-flow UX;
- Credential revoke reuses the same item-ETag -> one mutation -> refresh shape
  already proven by MU.9C.

## Scope

MU.9D adds exactly one new user-facing mutation:

- explicitly revoke one active Human Account `human-password` Credential after
  confirmation.

The UI must not expose a revoke control for terminal or unsupported Credential
types.

Account CREATE, grant mutation, password reset/rotation, Credential creation,
pairing and Profiles remain outside MU.9D.

## Browser ownership

The existing Settings owner remains the only browser owner.

`web/frontend/api/account-admin-client-api.js` continues to wrap the canonical
`clients/reference-js/public-v1-client.js` asset. MU.9D may use only:

- `getAccountCredential(...)`;
- `revokeAccountCredential(...)`.

No direct database access, unversioned mutation route or parallel client is
introduced.

## Mutation safety

Immediately before revoke, the browser adapter reads the selected Credential
item and treats the returned strong Credential lifecycle ETag as caller-owned
`If-Match` state.

The revoke path is exactly:

1. restore the active browser session when that runtime is available;
2. GET the selected Credential item;
3. fail locally if no strong ETag is returned;
4. POST exactly one Credential revoke with that ETag and active browser CSRF;
5. refresh the complete selected Account overview.

There is no automatic mutation retry.

A stale `412` is surfaced after a read refresh. The server-owned final usable
administrator invariant remains authoritative: `409` is displayed as a
conflict and is never reconstructed or bypassed client-side. Desired-state
terminal replay remains server-owned MU.8C behavior.

Revoking a Human-password Credential synchronously fences Browser Sessions
issued from that Credential. Confirmation text must make that consequence
explicit. If the active administrator browser Session is among those fenced,
subsequent reads may become `401`; the frontend must fail closed into the
existing browser-authentication lifecycle rather than invent replacement
credentials.

## Security boundary

MU.9D does not weaken ADR-0067 or MU.8C:

- Credential reads require `accounts.credentials.view@*`;
- Credential revoke requires `accounts.credentials.revoke@*`;
- browser CSRF remains mandatory;
- the Credential-owned strong ETag remains the concurrency authority;
- only server-accepted `human-password` Credentials are revocable;
- final-usable-administrator protection remains server-owned;
- password verifier hashes, cookies, CSRF material and reusable credentials are
  never rendered;
- the frontend never decides whether Credential revoke is authorized.

## Acceptance

Focused acceptance must prove:

- only active revocable Human-password Credentials expose a revoke control;
- explicit confirmation gates the mutation and warns that issued Sessions are
  fenced;
- one revisioned Credential-item GET precedes one revoke POST;
- the POST carries that Credential ETag as `If-Match` plus browser CSRF;
- success refreshes the selected Account overview;
- stale `412` is visible and never causes an automatic revoke retry;
- final-usable-administrator `409` is visible and never retried;
- `401`/403 remain fail-closed authorization outcomes;
- MU.9D does not call Account CREATE or grant mutation.

No daemon build, installation, restart or runtime mutation is justified for
this frontend/Public-v1 integration slice unless a focused test proves
otherwise.

Phase 70 remains separate and not started.
