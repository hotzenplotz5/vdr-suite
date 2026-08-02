# Phase 62 Slice 2H — Channel Move security migration

Status: repository, CI and mutation-free installed-runtime acceptance complete

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

## Repository and CI acceptance

Accepted source/runtime head:

```text
2e0b31f671edf18393d7d48ea6e15697fc3a044d
```

GitHub Actions VDR-Suite CI #6559 completed successfully:

```text
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30627974107
```

The complete documentation, test-inventory, frontend, security, controller,
architecture, daemon-build and packaging graph passed before installation.

## Installed-runtime acceptance

**VERIFIED on the real yaVDR runtime on 2026-07-31.**

```text
Installed daemon SHA-256:
ff7582b6fdb6a2faa7d0e29f6795ad634ea76d95a42280a6140e005e249cbf52

Installed deferred-runtime-loader.js SHA-256:
e4860a2b7c613919f3a084fc625f398bd5f339191ae48133cfc76431c0189ca9

Guarded installation backup:
/var/backups/vdr-suite-phase62-slice2h-install-20260731-140438

Runtime-acceptance database backup:
/var/backups/vdr-suite-phase62-slice2h-runtime-20260731-142540
```

The installed-runtime pass verified:

- the installed loader through the direct daemon and public `/vdr-suite`
  origin;
- Legacy Basic dry-run compatibility on both exact aliases;
- browser-session issuance and hardened cookie handling;
- missing and wrong CSRF denial on both aliases;
- permission denial and invalid/mismatched backend-scope denial;
- direct `channels.move@default` authorization;
- fixed Admin allowance and Read-only precedence;
- Read-only isolation between backends;
- non-effective wildcard role rows;
- authorized unknown-backend rejection by independent backend policy;
- fail-closed trailing-slash variants;
- secret-free Channel Move accountability;
- logout and revoked-cookie replay denial;
- exact restoration of temporary grant rows;
- SQLite integrity and active daemon state.

All successful Channel Move requests used `dryRun:true` and returned before
adapter or executor dispatch.

```text
channel_move_requests=22
real_channel_moves=0
browser_session_active=0
grants_restored=yes
sqlite_quick_check=ok
service_state=active

PHASE 62 SLICE 2H MUTATION-FREE RUNTIME ACCEPTANCE: PASS
```

## Non-goals

- no real Channel Move during automated acceptance;
- no other route-family migration;
- no role or grant administration API;
- no generic frontend permission UI;
- no universal idempotency or revision contract;
- no Phase 63 or later runtime;
- no PR merge or Ready-for-review transition.
