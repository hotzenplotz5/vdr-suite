# Phase 62 Slice 2G — Timer CRUD security migration

Status: repository implementation; installed-runtime acceptance pending

## Scope

Slice 2G migrates exactly the existing VDR Timer mutation routes into the Phase 62 authorization and browser-CSRF contract:

```text
POST /api/vdr/timers/actions/create
  timers.create@<backend-id>

POST /api/vdr/timers/actions/update
  timers.modify@<backend-id>

POST /api/vdr/timers/actions/delete
  timers.delete@<backend-id>
```

No additional route family is migrated by this slice.

## Request-gate contract

The HTTP security gate identifies the three routes by exact request path after removing only the query string. A trailing slash is not an alias and remains unmigrated.

For an authenticated browser session, processing order is fixed:

1. resolve the browser identity and persisted grants;
2. require a valid `X-CSRF-Token` for the protected mutation;
3. authorize the route-specific permission against the JSON `backendId`;
4. persist the accountability decision;
5. permit router and controller dispatch only after all preceding checks succeed.

A missing or invalid browser CSRF token is rejected before permission evaluation and before Timer dispatch. A missing backend scope is rejected as invalid. A grant for another backend produces a backend-scope denial.

Legacy Basic compatibility remains available under the existing compatibility identity and grant configuration. Managed Basic identities do not inherit legacy compatibility: without an explicit Timer permission or fixed role grant they receive a permission denial on the now-migrated route. Other unmigrated POST routes remain fail-closed for browser sessions, enforced mode and non-legacy credentials.

## Fixed role catalogue extension

The existing backend-specific roles remain fixed rather than user-definable:

```text
role.admin@<backend-id>
  remote.control
  timers.create
  timers.modify
  timers.delete

role.read-only@<backend-id>
  denies all four protected mutations above
```

Role scope matching is exact. Wildcard role rows do not grant or deny concrete backend access. `role.read-only` is evaluated before direct permissions and before `role.admin`, so it wins for the same backend. Direct backend-specific Timer grants remain supported.

## Frontend contract

The browser runtime keeps the CSRF token only in memory. Its fetch wrapper attaches that token only to POST requests for the three exact Timer paths. Caller-provided headers are preserved, but a caller cannot replace the session CSRF value. No token is added to GET requests, trailing-slash variants or other mutation families.

## Independent backend enforcement

Slice 2G does not alter `VdrTimerActionController`, `BackendAccessPolicy`, Timer capability resolution or adapter dispatch. Backend read-only state and capability availability therefore remain independent checks after actor authorization. Actor permissions and roles cannot override those backend constraints.

## Explicit non-goals

- no Recording, Channel, SearchTimer, Admin or other route migration;
- no generic role or grant administration API;
- no wildcard role inheritance;
- no universal idempotency system;
- no Timer domain or payload redesign;
- no Phase 63 or later runtime.

## Repository validation

Focused tests cover:

- all three exact route-to-permission mappings;
- browser CSRF rejection before authorization;
- direct exact-backend grants and wrong-scope denial;
- fixed Admin grants for all three Timer permissions;
- Read-only precedence over Admin and direct grants;
- Legacy Basic compatibility and Managed Basic denial without Timer policy;
- missing backend scope, query-string handling and trailing-slash fail-closed behavior;
- accountability events without credential or CSRF disclosure;
- frontend CSRF injection limited to the three exact Timer POST routes;
- the pre-existing router-level backend read-only denial before Timer executor dispatch.
