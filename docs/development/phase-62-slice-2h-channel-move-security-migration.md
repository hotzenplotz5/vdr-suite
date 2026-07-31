# Phase 62 Slice 2H — Channel Move security migration

Status: repository implementation; installed-runtime acceptance pending

## Scope

Slice 2H migrates exactly the two existing aliases of the VDR Channel Move
mutation into the Phase 62 authorization and browser-CSRF boundary:

```text
POST /api/vdr/channels/move
POST /api/vdr/channels/actions/move

permission: channels.move@<backend-id>
action:     channels.move
```

Both routes already dispatch to the same Channel Move controller. This slice
does not add, remove or redesign a public API route.

No other Channel, Recording, SearchTimer, EPG, operator or administration
route is migrated.

## Exact-route contract

The security gate compares the request path after removing only its query
string. Therefore query-string variants of both aliases are protected.

Trailing-slash variants are not aliases and remain fail-closed as unmigrated
browser mutations.

For a browser-authenticated request, the enforced order is:

1. authenticate the browser-session credential;
2. resolve persistent lifecycle state and actor grants;
3. validate the independent `X-CSRF-Token`;
4. read the JSON `backendId`;
5. authorize `channels.move` against that exact backend scope;
6. persist the pre-dispatch accountability decision;
7. permit router and controller dispatch only after all checks succeed.

Missing or invalid CSRF is rejected before permission evaluation and before
Channel Move dispatch. Missing backend scope is invalid. A permission or role
for another backend does not authorize the request.

## Fixed roles

The fixed backend-specific role catalogue includes:

```text
role.admin@<backend-id>
  remote.control
  timers.create
  timers.modify
  timers.delete
  channels.move
```

`role.read-only@<backend-id>` denies Channel Move for the same exact backend
scope and takes precedence over direct permissions and `role.admin`.

Wildcard role rows do not inherit into concrete backend scopes.

## Basic compatibility

The exact legacy compatibility credential retains its transitional
unmigrated/migrated mutation compatibility.

Managed Basic does not inherit that bypass. Once Channel Move is migrated, a
Managed Basic actor without `channels.move`, a matching fixed Admin role or a
matching direct wildcard grant receives a permission denial rather than
`security_policy_not_migrated`.

## Frontend contract

The browser runtime keeps the CSRF token only in memory. Its fetch wrappers
share one exact-route header-merging implementation while retaining separate
Timer and Channel-Move installation markers.

The Channel-Move wrapper adds the active session CSRF token only to POST
requests for the two exact aliases. It supports query strings, preserves
caller headers and prevents a caller from replacing the active session token.

It does not add CSRF to GET requests, trailing-slash variants or unrelated
mutations.

## Independent backend and domain safety

Slice 2H does not alter:

- `ApiRouter`;
- `VdrChannelMoveController`;
- `BackendAccessPolicy`;
- `VdrChannelMoveExecutionService`;
- adapter registration or SVDRP execution;
- the existing successful-mutation snapshot refresh.

Backend write access remains a separate mandatory decision after actor
authorization.

A Channel Move dry-run validates the command and returns before adapter lookup
or executor dispatch. Tests also prove that read-only, unknown and
adapter-less backends cannot reach the registered executor.

No real channel ordering is changed by repository or automated runtime
acceptance.

## Accountability

Allow and deny decisions use the canonical non-secret fields:

```text
permission = channels.move
action     = channels.move
backendId  = requested JSON backend scope
outcome    = dispatch_authorized or dispatch_denied
```

Authorization headers, cookies, session secrets, CSRF values and password
material are never persisted or reflected.

## Repository validation

Focused coverage includes:

- both exact route aliases;
- query strings and fail-closed trailing slashes;
- missing and invalid browser CSRF;
- direct permission, wrong scope and missing backend scope;
- fixed Admin allowance and Read-only precedence;
- Legacy Basic compatibility;
- Managed Basic denial without Channel Move policy;
- enforced-mode direct permission and wrong-scope denial;
- secret-free Channel Move accountability;
- exact frontend CSRF injection and caller-header precedence;
- dry-run without adapter dispatch;
- backend read-only, unknown-backend and missing-adapter denial.

## Non-goals

- no real Channel Move during automated acceptance;
- no other route-family migration;
- no role or grant administration API;
- no generic frontend permission UI;
- no universal idempotency or revision contract;
- no Phase 63 or later runtime;
- no PR merge or Ready-for-review transition.
