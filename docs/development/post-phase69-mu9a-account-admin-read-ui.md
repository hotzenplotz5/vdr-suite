# MU.9A — Account Administration Read UI

Status: **IMPLEMENTATION CANDIDATE**

MU.9A is the first bounded browser-administration slice after completion of the
MU.8 credential/session backend boundary. It adds a read-only administration
surface to the existing Settings owner and deliberately does not add mutation
controls yet.

## Product surface

The existing Settings module gains one responsive **Users & access** card.

The card provides:

- paginated Human Account collection read;
- automatic first-account selection;
- selected Account detail;
- active supported grant tuples;
- secret-free Human-password Credential metadata;
- secret-free Browser Session metadata;
- explicit unauthenticated / unauthorized / unavailable states.

The UI renders only server-provided policy state. It does not infer or grant
authorization locally.

## Client ownership

MU.9A does not invent a second Public-v1 HTTP implementation.

The browser asset:

`web/frontend/api/public-v1-client.js`

is a Git symlink to the accepted canonical client:

`clients/reference-js/public-v1-client.js`

The DOM-free adapter:

`web/frontend/api/account-admin-client-api.js`

uses only the accepted reference-client methods:

- `getAccounts()`;
- `getAccount()`;
- `getAccountGrants()`;
- `getAccountCredentials()`;
- `getAccountSessions()`.

The existing `web/frontend/api/client-api.js` remains the owner of the legacy
browser HTTP seam and is not silently migrated to Public-v1.

## Read-only boundary

MU.9A intentionally does not call any of these existing mutation helpers:

- `createAccount()`;
- `updateAccountDisplayName()`;
- `activateAccount()`;
- `deactivateAccount()`;
- `setAccountGrant()`;
- `revokeAccountCredential()`;
- `revokeAccountSession()`.

Mutation controls remain a later MU.9 slice so read rendering, permission
failures and browser/client integration can be accepted independently.

## Authentication and authorization

The browser client uses same-origin credentials.

The server remains authoritative for:

- `accounts.view@*`;
- `accounts.grants.view@*`;
- `accounts.credentials.view@*`;
- `accounts.sessions.view@*`.

HTTP 401 is rendered as a browser-sign-in requirement. HTTP 403 is rendered as
missing global administration authority. Other failures remain unavailable
states; the UI does not weaken or emulate authorization.

## Asset and packaging ownership

Production wiring covers:

- `web/frontend/index.html` script order;
- `core/http/src/TestHttpServerPaths.inc` frontend asset registry;
- `mk/install.mk` install staging;
- responsive Settings styling;
- German and English locale catalogs.

## Acceptance

Focused acceptance for this implementation candidate is:

```text
make test-mu9a-account-admin-read-ui
python3 tools/check_frontend_ownership_contracts.py
```

The slice requires no daemon build, package installation, service restart or
runtime mutation.

## Explicit non-goals

MU.9A does not implement:

- Account CREATE;
- display-name change;
- Account activate/deactivate;
- grant mutation;
- Credential revoke UI;
- Session revoke UI;
- password reset/rotation;
- TV/app pairing;
- Profiles/personalization;
- broad Timer Product UI.
