# MU.9B — Account Lifecycle Mutation UI

Status: **IMPLEMENTATION CANDIDATE**

Parent workstream: [Post-Phase-69 Multiuser Productization Workstream](post-phase69-multiuser-workstream.md)

Architecture authority: [ADR-0067 Human Account and Backend Access Administration](../adr/ADR-0067-human-account-backend-access-administration.md)

Predecessor: [MU.9A Account Administration Read UI](post-phase69-mu9a-account-admin-read-ui.md)

## Purpose

MU.9B is the first bounded mutation slice in the existing browser Account
administration surface. It reuses the accepted MU.6C Public-v1 Human Account
lifecycle contracts and does not create a second mutation authority.

The slice adds exactly three user-facing lifecycle operations for the selected
Human Account:

- change `displayName`;
- activate the Account;
- deactivate the Account.

Account creation, backend grant mutation, Credential revoke, Session revoke,
password reset/rotation, pairing and Profiles remain outside MU.9B.

## Browser ownership

The existing Settings owner remains the only browser owner.

`web/frontend/api/account-admin-client-api.js` continues to wrap the canonical
`clients/reference-js/public-v1-client.js` asset. MU.9B calls only the already
accepted methods:

- `updateAccountDisplayName(...)`;
- `activateAccount(...)`;
- `deactivateAccount(...)`.

No direct database access, unversioned mutation route or parallel client is
introduced.

## Mutation safety

Every mutation uses the strong Account ETag returned by the latest selected
Account read as caller-owned `If-Match` state.

Before a browser mutation the adapter restores the active browser session when
that runtime is available and copies `VdrSuiteBrowserSession.csrfHeaders()`
into the Public-v1 request. The reference client remains responsible for the
canonical Account item path, POST body and `If-Match` header.

After every successful mutation the browser reloads the complete Account
overview. This is required especially after deactivation because the accepted
MU.6C authority may revoke active browser sessions owned by the target Account.

A stale `412` response is surfaced and followed by a read refresh rather than
a blind retry. A `409` final-usable-administrator rejection is shown as a
terminal user-visible refusal. Deactivation requires explicit browser
confirmation because it can revoke active sessions.

## Security boundary

MU.9B does not weaken ADR-0067:

- display-name changes require `accounts.modify@*`;
- activation requires `accounts.activate@*`;
- deactivation requires `accounts.deactivate@*`;
- backend-scoped admin authority remains insufficient;
- browser CSRF remains mandatory;
- final-usable-administrator protection remains server-owned;
- deactivation session revocation remains server-owned;
- no reusable credential or secret is exposed to the UI.

The frontend interprets authorization/concurrency failures but never decides
whether the mutation is permitted.

## Acceptance

Focused acceptance requires:

```bash
make test-mu9a-account-admin-read-ui
make test-mu9b-account-lifecycle-ui
python3 tools/check_frontend_ownership_contracts.py
python3 tools/check_phase_consistency.py
```

The MU.9B browser integration test must prove the user-style action chain:

1. select the Account through the existing MU.9A read surface;
2. click display-name save;
3. emit exactly one Public-v1 lifecycle mutation with the selected Account ETag
   and active browser CSRF;
4. refresh the selected Account and advance to its new ETag;
5. confirm and click deactivate;
6. emit exactly one deactivate mutation using the refreshed ETag;
7. refresh the now-inactive Account and advance to its new ETag;
8. click activate;
9. emit exactly one activate mutation using that post-deactivation ETag.

The test must also prove:

- each lifecycle action is one-shot and is not retried implicitly;
- a stale `412` performs a read refresh and surfaces the concurrency conflict;
- a final-usable-administrator `409` performs a read refresh and surfaces the
  server refusal;
- MU.9B does not call Account CREATE, grant mutation, Credential revoke or
  Session revoke.

Phase 70 remains separate and not started.
