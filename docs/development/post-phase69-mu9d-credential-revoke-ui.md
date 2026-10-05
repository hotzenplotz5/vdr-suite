# MU.9D — Human-password Credential Revoke UI

Status: **IMPLEMENTATION MERGED — PR #428 / FOCUSED YAVDR PASS / HOSTED CI GREEN**

Parent workstream: [Post-Phase-69 Multiuser Productization Workstream](post-phase69-multiuser-workstream.md)

Architecture authority: [ADR-0067 Human Account and Backend Access Administration](../adr/ADR-0067-human-account-backend-access-administration.md)

Backend contract authority: [MU.8C Human-password Credential Revoke](post-phase69-mu8c-credential-revoke.md)

Predecessor: [MU.9C Session Revoke UI](post-phase69-mu9c-session-revoke-ui.md)

## Purpose

MU.9D is the smallest bounded successor after the merged Session-revoke UI.
It exposes only the already accepted MU.8C Human-password Credential revoke
operation for Credentials belonging to the selected Human Account.

The slice adds exactly one new user-facing mutation:

- explicitly revoke one active, non-terminal `human-password` Credential after
  confirmation.

Account CREATE, backend grant mutation, password reset/rotation, Credential
creation, pairing and Profiles remain outside MU.9D.

## Browser ownership

The existing Settings owner remains the only browser owner.

`web/frontend/api/account-admin-client-api.js` continues to wrap the canonical
`clients/reference-js/public-v1-client.js` asset. MU.9D may call only:

- `getAccountCredential(...)`;
- `revokeAccountCredential(...)`.

No direct database access, unversioned mutation route or parallel client is
introduced.

## Mutation safety

The Credential collection does not supply the Credential-owned strong lifecycle
ETag required by MU.8C. Immediately before revoke, the browser adapter therefore
reads the selected Credential item and treats its returned strong ETag as
caller-owned `If-Match` state.

The revoke path is exactly:

1. restore the active browser session when that runtime is available;
2. GET the selected revisioned Credential item;
3. fail locally if no strong ETag is returned;
4. require explicit confirmation;
5. POST exactly one Credential revoke with that ETag and active browser CSRF;
6. refresh the complete selected Account overview.

There is no automatic mutation retry. A stale `412` is surfaced after a read
refresh. HTTP `409` final-usable-administrator protection remains server-owned.
Already-terminal Credential state remains server-owned desired-state idempotency
under MU.8C.

MU.8C synchronously revokes active Browser Sessions issued from the revoked
Credential. If the current Settings browser Session was issued from that
Credential, subsequent reads may become `401`; the frontend must fail closed
and return control to the established browser-authentication lifecycle rather
than minting or selecting replacement credentials.

## Security boundary

MU.9D does not weaken ADR-0067 or MU.8C:

- Credential reads require `accounts.credentials.view@*`;
- Credential revoke requires `accounts.credentials.revoke@*`;
- backend-scoped administrator authority remains insufficient;
- browser CSRF remains mandatory;
- the Credential-owned strong ETag remains the concurrency authority;
- only `human-password` Credentials are revocable through this bounded
  contract;
- final-usable-administrator protection remains server-owned;
- issuer-session fencing remains server-owned;
- no password verifier, browser token, cookie, CSRF value, secret hash or other
  reusable credential is rendered;
- the frontend never decides whether Credential revoke is authorized.

## Acceptance

Focused acceptance must prove:

- revoke controls render only for non-terminal `human-password` Credentials;
- explicit confirmation gates the mutation;
- one revisioned Credential-item GET precedes one revoke POST;
- the POST carries that Credential ETag as `If-Match` plus browser CSRF;
- success refreshes the selected Account overview;
- stale `412` is visible and never causes an automatic revoke retry;
- final-usable-administrator `409` is visible and never bypassed client-side;
- `401`/`403` remain fail-closed authorization outcomes;
- no revoke control is exposed for unsupported Credential types;
- MU.9D does not call Account CREATE or grant mutation.

Expected focused checks:

```bash
make test-mu9a-account-admin-read-ui
make test-mu9b-account-lifecycle-ui
make test-mu9c-session-revoke-ui
make test-mu9d-credential-revoke-ui
python3 tools/check_frontend_ownership_contracts.py
python3 tools/check_phase_consistency.py
```

No local daemon build, installation, restart or runtime mutation is justified
for the initial frontend/Public-v1 integration slice unless a later test proves
otherwise.

Phase 70 remains separate and not started.


## Acceptance / closeout

Focused yaVDR acceptance passed before merge. PR #428 merged as commit `fc93a49bf01e31afe21354dbd6283122fd8c5be0`. Hosted PR CI was green. Post-merge `main` CI run #9761 (`push`, exact merge SHA) completed successfully; all six jobs were green: `architecture-check`, `frontend-regression-test`, `make-test-audit`, `docs-check`, `packaging-regression-test`, and `fast-regression-test`.

MU.9D is therefore closed. Its historical guard owns only the MU.9D contract and must not require a particular successor status. The selected successor is [MU.9E Backend Grant Mutation UI](post-phase69-mu9e-grant-mutation-ui.md).
