# Phase 62 Slice 2F — Minimal backend-scoped role model

Status: repository implementation; installed-runtime acceptance pending

## Scope

Slice 2F adds exactly two fixed roles without introducing a general role administration product:

- `admin`
- `read-only`

Assignments are represented by the existing persisted actor-grant rows:

```text
role.admin@<backend-id>
role.read-only@<backend-id>
```

This keeps the accepted `actor_id + permission + backend_id` persistence and lifecycle semantics. No role-definition table, actor-role table, wildcard role inheritance or protected role-management API is introduced.

## Authorization contract

Role scopes are exact. A role row with `backend_id = *` does not grant or deny rights for a concrete backend.

The fixed role catalogue for this slice is intentionally small:

```text
admin
  remote.control@<assigned-backend>

read-only
  denies remote.control@<assigned-backend>
```

`read-only` is evaluated before direct permissions and before `admin`; therefore it wins for the same backend. A `read-only` assignment for one backend does not affect another backend.

Direct permission grants remain supported. `admin` does not synthesize wildcard permission and does not grant permissions outside the explicit catalogue. Adding another protected mutation later requires an explicit catalogue change and its own route-migration slice.

## Explicit non-goals

- no generic roles or custom role definitions;
- no role or grant administration HTTP API;
- no frontend role editor;
- no new mutation route family;
- no change to backend read-only or capability enforcement;
- no replacement of existing exact permission grants;
- no Phase 63–67 runtime.

## Repository validation

The focused authorization tests prove:

- exact backend-scoped `admin` grants `remote.control`;
- wrong-scope and wildcard `admin` assignments do not grant access;
- exact backend-scoped `read-only` denies Remote even when a direct or admin grant exists;
- `read-only` on another backend does not deny the requested backend;
- `admin` cannot invent permissions outside the fixed catalogue;
- role assignment rows round-trip through `SecurityPermissionGrantRepository`.
