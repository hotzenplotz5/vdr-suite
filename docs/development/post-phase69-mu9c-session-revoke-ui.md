# MU.9C — Session Revoke UI

Status: **IMPLEMENTATION MERGED — PR #426 / FOCUSED YAVDR PASS / MAIN CI PENDING**

Parent workstream: [Post-Phase-69 Multiuser Productization Workstream](post-phase69-multiuser-workstream.md)

Architecture authority: [ADR-0067 Human Account and Backend Access Administration](../adr/ADR-0067-human-account-backend-access-administration.md)

Backend contract authority: [MU.8B Session Revoke](post-phase69-mu8b-session-revoke.md)

Predecessor: [MU.9B Account Lifecycle Mutation UI](post-phase69-mu9b-account-lifecycle-ui.md)

## Purpose

MU.9C is the smallest bounded successor in the existing browser Account
administration surface. It exposes only the already accepted MU.8B Session
revoke operation for Sessions belonging to the selected Human Account.

The slice adds exactly one new user-facing mutation:

- explicitly revoke one active Session after confirmation.

Account CREATE, backend grant mutation, Credential revoke, password
reset/rotation, pairing and Profiles remain outside MU.9C.

## Browser ownership

The existing Settings owner remains the only browser owner.

`web/frontend/api/account-admin-client-api.js` continues to wrap the canonical
`clients/reference-js/public-v1-client.js` asset. MU.9C may call only:

- `getAccountSession(...)`;
- `revokeAccountSession(...)`.

No direct database access, unversioned mutation route or parallel client is
introduced.

## Mutation safety

The Session collection does not supply the Session-owned strong lifecycle ETag
required by MU.8B. Immediately before revoke, the browser adapter therefore
reads the selected Session item and treats its returned strong ETag as
caller-owned `If-Match` state.

The revoke path is exactly:

1. restore the active browser session when that runtime is available;
2. GET the selected revisioned Session item;
3. fail locally if no strong ETag is returned;
4. POST exactly one Session revoke with that ETag and active browser CSRF;
5. refresh the complete selected Account overview.

There is no automatic mutation retry. A stale `412` is surfaced after a read
refresh. Already-terminal Session state remains server-owned desired-state
idempotency under MU.8B. Revoke requires explicit confirmation.

If the administrator revokes the browser Session currently authorizing the
Settings page, subsequent reads may become `401`; the frontend must fail
closed and return control to the established browser-authentication lifecycle
rather than inventing replacement credentials.

## Security boundary

MU.9C does not weaken ADR-0067 or MU.8B:

- Session reads require `accounts.sessions.view@*`;
- Session revoke requires `accounts.sessions.revoke@*`;
- backend-scoped administrator authority remains insufficient;
- browser CSRF remains mandatory;
- the Session-owned ETag remains the concurrency authority;
- passive `lastSeenAt` remains excluded from that lifecycle ETag;
- no browser/session credential, cookie, CSRF value or reusable secret is
  rendered;
- the frontend never decides whether Session revoke is permitted.

## Acceptance

Focused acceptance must prove:

- revoke controls render only for non-terminal Sessions;
- explicit confirmation gates the mutation;
- one revisioned Session-item GET precedes one revoke POST;
- the POST carries that Session ETag as `If-Match` plus browser CSRF;
- success refreshes the selected Account overview;
- stale `412` is visible and never causes an automatic revoke retry;
- `401`/403 remain fail-closed authorization outcomes;
- MU.9C does not call Account CREATE, grant mutation or Credential revoke.

Expected focused checks:

```bash
make test-mu9a-account-admin-read-ui
make test-mu9b-account-lifecycle-ui
make test-mu9c-session-revoke-ui
python3 tools/check_frontend_ownership_contracts.py
python3 tools/check_phase_consistency.py
```

No daemon build, installation, restart or runtime mutation was required for
the focused real yaVDR acceptance.

## Acceptance state

Focused yaVDR acceptance passed on exact implementation head
`e2c63aec5e5a1d96016a9efb70115a0772210e97`: MU.8B predecessor, MU.9A,
MU.9B and MU.9C focused checks, frontend ownership, JavaScript syntax and phase
consistency all passed. The branch was then synchronized with the separately
accepted PR #427 warm-cache regression fix without changing MU.9C product
semantics and merged as PR #426. The resulting main merge commit is
`c8507b69aa0fb8a19e750d936a777fbce3dc3daf`.

Post-merge main CI had not yet started when this closeout status was written,
so hosted main-CI green must not be inferred from this document until a later
live check updates the evidence.

The selected successor is MU.9D Human-password Credential Revoke UI.

Phase 70 remains separate and not started.
