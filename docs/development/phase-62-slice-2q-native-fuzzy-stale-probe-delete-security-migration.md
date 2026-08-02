# Phase 62 Slice 2Q — Global Native Fuzzy Stale-Probe Deletion Security Migration

## Status

Repository implementation, focused verification, both five-job CI runs and the
real yaVDR zero-delete runtime acceptance are complete.

Accepted code/runtime head:

```text
88ec36076d7e5114df0a3a186cc6fbd52bb2baac
```

Accepted GitHub Actions:

```text
VDR-Suite CI #6655
Run ID: 30713953331
Result: all five jobs successful
```

PR #117 remains open, Draft, unmerged and mergeable. This documentation
closeout does not change review state, merge state or PR metadata.

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

No other POST family, new GET API, identity administration, generic role
administration, Android client or Phase 63+ runtime is part of this slice.

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
Trailing-slash variants remain fail-closed. Request body and query values cannot
alter the global authorization scope.

The aliases remain excluded from the Safe POST allowlist because the controller
can perform a real SQLite deletion.

---

## Global Role Semantics

The fixed role model remains exact-scope rather than inherited wildcard scope.
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
The permission is part of the fixed protected-mutation catalogue; no generic
role definition or wildcard inheritance mechanism was introduced.

---

## Mutation-Safe Runtime Boundary

The existing service deletes only persisted rows from
`epgsearch_native_fuzzy_capability_probes` whose timestamp is stale or lies in
the future. It does not contact VDR and does not create, modify or delete
SearchTimer objects.

The runtime runner obtains a direct read-only SQLite snapshot before any Delete
POST. It applies the production freshness rules:

```text
maxAgeSeconds = 604800
ageSeconds < 0       -> future-timestamp and stale
ageSeconds > 604800  -> stale
otherwise            -> fresh
```

The normalized snapshot must be exactly:

```json
{"staleProbes":[]}
```

If any stale or future-timestamp row exists, the runner aborts before the first
Delete POST. It does not manufacture, age or alter a production probe row.

After the empty snapshot and before the first Delete POST, the runner creates a
bounded temporary main-schema SQLite `BEFORE DELETE` trigger on
`epgsearch_native_fuzzy_capability_probes`. The trigger blocks every deletion
attempt from every database connection during acceptance.

This closes the snapshot-to-POST race:

- an empty table state permits the expected zero-delete responses;
- a row becoming stale after preflight causes a safe test failure rather than a
  deletion;
- no production capability row can be deleted by the acceptance pass.

Cleanup removes the trigger unconditionally. Final verification requires the
trigger to be absent, the preflight and postflight snapshots to be identical,
the database to pass quick and foreign-key checks and the service PID to remain
unchanged during the acceptance run.

Every successful authorized response reports:

```text
schemaReady=true
staleResultsFound=0
deletedResults=0
deleteFailures=0
```

---

## First Runtime Attempt — Safe Rejection Evidence

The first guarded attempt at repository head
`1119c94e5184245f5d161283270c51b604153e0b` used the historically documented
route:

```text
GET /api/epgsearch/native-fuzzy/stale-probes
```

The installed router returned:

```text
HTTP 404
{"error":"not found"}
```

Repository inspection confirmed that both Delete POST aliases are registered in
`ApiRouter::handlePost`, while neither stale-probe GET alias is registered in
`ApiRouter::handleGet`.

No Delete POST had been sent. The outer guarded wrapper restored the previously
accepted Slice-2P runtime:

```text
automatic_rollback=passed
restored_daemon_sha256=c0e74602334e2b9d21f53329182bc5e35c99676f3dcdf2ae0639f996151a432a
restored_loader_sha256=3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
delete_guard_absent_after_rollback=yes
```

Rollback backup:

```text
/var/backups/vdr-suite-phase62-slice2q-20260801T185634Z-1119c94e5184/install-before
```

This remains useful safe-rejection evidence but is not the accepted Slice-2Q
runtime pass.

---

## Corrected Repository Verification

The direct-SQLite correction was committed at:

```text
88ec36076d7e5114df0a3a186cc6fbd52bb2baac
```

The correction changed only the runtime profile, runner, static contract and
status documentation. It did not change the HTTP gate, authorization catalogue,
router, controller, service, database schema or frontend.

The runner self-test covers empty, fresh, stale and future-timestamp fixtures.
The complete CI run succeeded:

```text
docs-check                    success
make-test-audit               success
frontend-regression-test      success
fast-regression-test          success
packaging-regression-test     success
```

The focused contracts include:

- both exact Delete aliases;
- Legacy Basic compatibility;
- browser CSRF enforcement;
- exact global permission scope;
- body and query values unable to change scope;
- direct concrete-scope denial;
- exact global Admin allowance;
- concrete Admin denial;
- global Read-only precedence;
- concrete Read-only isolation;
- query-string normalization;
- trailing-slash fail-closed behavior;
- canonical global accountability fields;
- exclusion from Safe POST;
- absence of a Webfrontend owner;
- direct SQLite freshness-policy self-tests;
- delete-guard installation, cleanup and final absence.

---

## Real yaVDR Runtime Acceptance

Accepted on 2026-08-01 against the installed yaVDR runtime.

Installed and runtime fingerprints:

```text
repository_head=88ec36076d7e5114df0a3a186cc6fbd52bb2baac
ci_run_id=30713953331
service_pid_after_acceptance=67393
installed_daemon_sha256=9f60daaf7d772abe7c6ad55388cb9bb7e8afe8f6679fbf749aa9103143a41d07
installed_loader_sha256=3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
```

Acceptance result:

```text
slice=slice-2q-native-fuzzy-stale-probe-delete
tests_passed=32
tests_failed=0
runtime_http_requests=25
accountability_authorized=8
accountability_csrf=2
accountability_permission=2
accountability_read_only=2
accountability_scope=4
resource_state_unchanged=yes
target_grants_restored=yes
unrelated_grants_untouched=yes
browser_session_revoked=yes
revoked_cookie_replay_denied=yes
accountability_secret_free=yes
database_integrity=yes
service_pid_unchanged=yes
phase62_runtime_acceptance=passed
```

Zero-delete and guard evidence:

```text
authorization_scope=*
snapshot_source=direct-sqlite
freshness_max_age_seconds=604800
stale_probe_postflight=empty
stale_probe_snapshot_unchanged=yes
real_stale_probe_deletes=0
delete_guard_removed=yes
service_state=active
```

Artifact hashes:

```text
runtime_report_sha256=602148a61a69dadcf7a38fb566b4e4486a7e550f0dda193fffa8f42ba3a1c197
snapshot_preinstall_sha256=2b1d1af321fd9497f99ac742c694c70ecfde94b41bcb6d74ee3a70187e5d1e7a
snapshot_before_sha256=2b1d1af321fd9497f99ac742c694c70ecfde94b41bcb6d74ee3a70187e5d1e7a
snapshot_after_sha256=2b1d1af321fd9497f99ac742c694c70ecfde94b41bcb6d74ee3a70187e5d1e7a
```

Guarded installation backup and durable runtime evidence:

```text
/var/backups/vdr-suite-phase62-slice2q-20260801T191156Z-88ec36076d7e/install-before
/var/backups/vdr-suite-phase62-slice2q-20260801T191156Z-88ec36076d7e/runtime-acceptance-slice2q
```

The three stale-probe snapshot hashes are identical. No stale or future-dated
probe row existed before installation, before the POST matrix or after the
matrix. The temporary trigger was removed, no real stale-probe deletion
occurred, all temporary grants were restored, the browser session was revoked,
revoked-cookie replay was denied, accountability remained secret-free and the
service remained active.

---

## Frontend Ownership

There is no Webfrontend request owner for either Delete alias. Slice 2Q adds no
global fetch wrapper and no JavaScript mutation owner. Static architecture
checks fail if either route appears in the frontend without a future explicit
CSRF-aware ownership contract.

---

## Explicitly Deferred

Slice 2Q does not add:

- protected stale-probe GET administration or a frontend administration page;
- creation of synthetic stale rows on the real runtime;
- deletion of any existing production capability row;
- completion/outcome accountability;
- generic identity, credential, grant or role administration;
- browser-session refresh, idle, cleanup or concurrency policy;
- Phase 63-67 runtime;
- PR Ready transition or merge.

## Exact Next Action

1. let this documentation-closeout commit complete its full five-job CI;
2. keep PR #117 open, Draft and unmerged;
3. perform a fresh bounded POST inventory audit after Slice 2Q;
4. select exactly one next Phase 62 slice only after its scope, persistence,
   authorization, accountability and runtime-safety boundary are explicit;
5. do not combine the next slice with Android, generic administration or Phase
   63-67 runtime.
