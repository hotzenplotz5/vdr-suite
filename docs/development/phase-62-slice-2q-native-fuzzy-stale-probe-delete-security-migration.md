# Phase 62 Slice 2Q — Global Native Fuzzy Stale-Probe Deletion Security Migration

## Status

Repository implementation is in progress. CI and real yaVDR runtime acceptance
remain pending.

PR #117 remains open, Draft and unmerged.

---

## Exact Scope

Slice 2Q protects exactly these existing aliases:

```text
POST /api/epgsearch/native-fuzzy/stale-probes/delete
POST /api/vdr/epgsearch/native-fuzzy/stale-probes/delete
  -> epgsearch.native-fuzzy.stale-probes.delete@*
```

This is a global administrative mutation. It does not accept or derive a
backend scope from the request. The canonical authorization scope is exactly
`*`.

No other POST family, identity administration, generic role administration,
Android client or Phase 63+ runtime is part of this slice.

---

## Security Contract

For browser-session requests the server performs:

1. browser credential and lifecycle validation;
2. persisted actor-grant resolution;
3. exact route classification without the query string;
4. cookie-bound CSRF validation;
5. authorization for
   `epgsearch.native-fuzzy.stale-probes.delete@*`;
6. append-only pre-dispatch accountability;
7. existing stale-probe administration controller handling.

Legacy and Managed Basic compatibility remain supported without browser CSRF.

Exact route paths remain migrated when unrelated query parameters are present.
Trailing-slash variants remain fail-closed.

A request body field cannot alter the global scope.

---

## Global Role Semantics

The existing fixed role model remains exact-scope rather than inherited
wildcard scope.

For this global route:

```text
direct permission@*                  allow
same permission@<concrete backend>   deny
role.admin@*                         allow
role.admin@<concrete backend>        deny
role.read-only@*                     deny before direct/Admin
role.read-only@<concrete backend>    no effect on the global route
```

`role.admin@*` is an exact assignment to the route's canonical global scope. It
does not make wildcard Admin rows effective for concrete backend mutations.

The permission is added explicitly to the fixed protected-mutation catalogue.
No generic role definition or wildcard inheritance mechanism is introduced.

---

## Mutation Boundary

The existing service operates only on
`epgsearch_native_fuzzy_capability_probes` rows whose persisted timestamp is
stale or in the future. It does not contact VDR and does not create, modify or
delete SearchTimer objects.

It is nevertheless a real SQLite deletion and must not be classified as a Safe
POST.

The real-runtime runner first requires the authenticated GET snapshot to be
exactly:

```json
{"staleProbes":[]}
```

If any stale or future-timestamp row is present, the runner aborts before the
first POST. It does not manufacture, age or alter a production probe row.

After the empty preflight and before the first POST, the runner creates a
bounded temporary main-schema SQLite `BEFORE DELETE` trigger on
`epgsearch_native_fuzzy_capability_probes`. The trigger raises an error for any
DELETE attempted by any database connection during the acceptance pass.

This closes the preflight-to-POST race:

- while the list remains empty, every authorized request returns the expected
  zero-delete success;
- if a row becomes stale after preflight, the database blocks the deletion, the
  endpoint cannot return the expected success and the acceptance fails safely;
- no production capability row can be deleted by the acceptance pass.

The trigger is removed in unconditional runner cleanup. Final verification
requires that it no longer exists. The outer guarded-install wrapper retains a
complete database backup and rollback path if cleanup itself fails.

Every successful authorized response must report:

```text
staleResultsFound=0
deletedResults=0
deleteFailures=0
```

The stale-probe GET snapshot is repeated after the pass and must remain byte-for-
byte equivalent after normalized JSON serialization.

---

## Runtime Harness

Profile and runner:

```text
tools/phase62-runtime-acceptance/slice-2q-native-fuzzy-stale-probe-delete.json
tools/phase62-runtime-acceptance/global-stale-probe-delete-runner.py
```

Planned target:

```text
make phase62-runtime-acceptance-global-stale-probe-delete
```

The runner reuses the accepted Phase 62 browser-session harness and adds:

- exact global-scope grants;
- an empty stale-probe precondition before any POST;
- a cross-connection SQLite delete guard for the complete POST phase;
- zero-delete response validation;
- concrete-scope denial for direct permission and Admin;
- global Admin allowance and global Read-only precedence;
- pre/post stale-probe snapshot equality;
- unconditional delete-guard removal and final absence verification;
- grant restoration, logout, revoked-cookie replay denial;
- secret-free accountability;
- daemon fingerprint/PID, backup and SQLite integrity checks.

---

## Frontend Ownership

There is currently no Webfrontend request owner for either Delete alias.
Therefore Slice 2Q adds no global fetch wrapper and no JavaScript mutation
owner. Static architecture checks must fail if either route is introduced into
the frontend without an explicit CSRF-aware owner contract.

---

## Focused Repository Verification

The focused security tests must cover:

- both exact aliases;
- Legacy Basic compatibility;
- browser CSRF enforcement;
- exact global permission scope;
- body fields being unable to change global scope;
- direct concrete-scope denial;
- exact global Admin allowance;
- concrete Admin denial;
- global Read-only precedence;
- concrete Read-only isolation;
- query-string normalization;
- trailing-slash fail-closed behavior;
- canonical global accountability fields;
- exclusion from the Safe POST allowlist;
- absence of a Webfrontend owner;
- manifest validation and runner self-test;
- delete-guard installation, cleanup and final absence contracts.

---

## Explicitly Deferred

Slice 2Q does not add:

- protected GET administration or a frontend administration page;
- creation of synthetic stale rows on the real runtime;
- deletion of any existing production capability row;
- completion/outcome accountability;
- generic identity, credential, grant or role administration;
- session refresh/idle/concurrency policy;
- Phase 63-67 runtime;
- PR Ready transition or merge.

## Acceptance Gate

The slice is not complete until:

1. focused repository checks pass;
2. all five PR CI jobs pass;
3. guarded installation succeeds on the real yaVDR system;
4. the preflight stale-probe list is empty;
5. the SQLite delete guard is installed before the first POST;
6. both aliases complete with zero deletions;
7. the delete guard is removed and verified absent;
8. the database, grants, session and service state are verified clean;
9. durable runtime evidence and final documentation are committed.
