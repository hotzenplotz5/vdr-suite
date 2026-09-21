# Phase 62 Slice 2P — Query-Scoped Cache Refresh Security Migration

## Status

Repository implementation, CI and real yaVDR runtime acceptance are complete.

Accepted code/runtime head:

```text
173c929964dbb7aabd30c5e482c2e250b5785d92
```

GitHub Actions:

```text
VDR-Suite CI #6649
Run ID: 30711237050
Result: all five jobs successful
```

PR #117 remains open, Draft, unmerged and mergeable. This documentation
closeout does not change review state, merge state or PR metadata.

---

## Scope

Slice 2P protects the existing query-scoped cache refresh routes:

```text
POST /api/searchtimers/preview/cache/refresh
POST /api/vdr/searchtimers/preview/cache/refresh
  -> searchtimers.preview-cache.refresh@<backend-id>

POST /api/epg/cache/refresh
  -> epg.cache.refresh@<backend-id>
```

The authorization scope is read from the `backend` query parameter. Missing or
empty values resolve to `default`. URL decoding follows the router-compatible
query parser, and the last duplicate `backend` value wins. A JSON body field
cannot replace or override the query-derived authorization scope.

Exact base paths remain migrated when other query parameters are present.
Trailing-slash variants remain fail-closed.

---

## Security Contract

For browser-session requests the server performs:

1. browser credential and lifecycle validation;
2. persisted actor-grant resolution;
3. exact route classification without the query string;
4. query parsing and backend-scope normalization;
5. cookie-bound CSRF validation;
6. exact permission and backend-scope authorization;
7. append-only pre-dispatch accountability;
8. existing controller and cache-refresh service handling.

Legacy and Managed Basic compatibility remain supported without browser CSRF.

The two permissions are intentionally distinct and do not authorize each
other. Exact-scope `role.admin@<backend-id>` expands to both permissions only
for the same concrete backend. Exact-scope `role.read-only@<backend-id>` denies
the refresh before direct grants or Admin expansion. Wildcard role rows remain
ineffective for a concrete backend.

---

## Webfrontend Ownership

The existing general EPG refresh owner remains in:

```text
web/frontend/api/client-api.js
```

The SearchTimer preview refresh path now also uses a dedicated Client API
function. The direct POST was removed from the workflow owner and deferred
runtime owner. Active browser-session CSRF is injected at the exact request
owners and overrides a caller-supplied `X-CSRF-Token` value. No global fetch
wrapper was introduced.

The changed owner paths include:

```text
web/frontend/api/client-api.js
web/frontend/epg-searchtimer-actions.js
web/frontend/platform/deferred-runtime-loader.js
```

Static ownership and runtime contract tests enforce that these route families
do not drift back to route-foreign direct POST calls.

---

## Runtime-Safe Acceptance Boundary

Both runtime profiles use a guaranteed non-registered backend:

```text
phase62-slice2p-missing-backend
```

The checked-in profiles are:

```text
tools/phase62-runtime-acceptance/slice-2p-searchtimer-preview-cache-refresh.json
tools/phase62-runtime-acceptance/slice-2p-epg-cache-refresh.json
```

They append the backend scope to the query string and expect the existing
service boundary to return HTTP 404 `backend-not-found`. This proves that an
authorized request reaches the controller/service boundary while stopping
before any real backend cache refresh or persisted cache mutation.

The batch target is:

```text
make phase62-runtime-acceptance-query-cache-batch
```

The acceptance harness snapshots `/api/backends` before and after each pass,
restores targeted grants exactly, revokes the temporary browser session,
verifies revoked-cookie replay denial, checks secret-free accountability and
runs SQLite quick and foreign-key checks.

---

## Focused Verification

Repository and CI coverage includes:

- all three exact routes;
- Legacy Basic compatibility;
- browser CSRF enforcement;
- distinct permissions and exact backend scopes;
- default backend resolution;
- URL-decoded query values;
- duplicate query values with last-value semantics;
- body backend fields being unable to override query scope;
- exact Admin allowance and wildcard Admin denial;
- Read-only precedence;
- canonical, secret-free accountability;
- query-string normalization and trailing-slash fail-closed behavior;
- frontend owner and installed-bundle contracts;
- exclusion of Native Fuzzy stale-probe deletion aliases;
- runtime manifest validation and self-tests.

GitHub Actions run `30711237050` completed successfully with:

```text
docs-check                    success
make-test-audit               success
frontend-regression-test      success
fast-regression-test          success
packaging-regression-test     success
```

---

## Real yaVDR Runtime Acceptance

Accepted on 2026-08-01 against the installed yaVDR runtime.

Installed fingerprints:

```text
repository_head=173c929964dbb7aabd30c5e482c2e250b5785d92
service_pid_after_install=66229
service_pid_after_acceptance=66229
build_daemon_sha256=c0e74602334e2b9d21f53329182bc5e35c99676f3dcdf2ae0639f996151a432a
installed_daemon_sha256=c0e74602334e2b9d21f53329182bc5e35c99676f3dcdf2ae0639f996151a432a
installed_loader_sha256=3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
```

SearchTimer preview cache refresh results:

```text
slice=slice-2p-searchtimer-preview-cache-refresh
tests_passed=29
tests_failed=0
runtime_http_requests=27
accountability_authorized=8
accountability_csrf=2
accountability_permission=2
accountability_read_only=2
accountability_scope=4
report_sha256=e806481147a81004753ba83678281cbcc67ae86aeaa7c171a2a5f68431086433
```

EPG cache refresh results:

```text
slice=slice-2p-epg-cache-refresh
tests_passed=18
tests_failed=0
runtime_http_requests=16
accountability_authorized=4
accountability_csrf=1
accountability_permission=1
accountability_read_only=1
accountability_scope=2
report_sha256=5bdd9f28801a91459aa58f55ed5e27dae12595900cfdf6b4bb5cde340ed588de
```

Cumulative Slice 2P acceptance:

```text
tests_passed=47
tests_failed=0
runtime_http_requests=43
resource_state_unchanged=yes
cache_mutation=none
target_grants_restored=yes
unrelated_grants_untouched=yes
browser_session_revoked=yes
revoked_cookie_replay_denied=yes
accountability_secret_free=yes
database_integrity=yes
service_pid_unchanged=yes
service_state=active
```

Runtime backup and durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2p-20260801T180617Z-173c929964db/install-before
/var/backups/vdr-suite-phase62-slice2p-20260801T180617Z-173c929964db/runtime-acceptance-slice2p
```

The acceptance confirmed that the deliberately absent backend remained unknown,
`/api/backends` was stable across both passes, no real SearchTimer preview cache
refresh occurred, no real EPG cache refresh occurred, the daemon PID did not
change during either pass and the worktree remained clean.

---

## Guarded Installation Rollback Evidence

The first installation attempt was guarded by a complete runtime backup and
automatic rollback. It stopped before runtime acceptance because the
verification wrapper used the wrong fixed build path (`build/` instead of the
Make-defined `.build/`). The product build and installation were not at fault.

Rollback evidence:

```text
backup=/var/backups/vdr-suite-phase62-slice2p-20260801T180300Z-173c929964db/install-before
automatic_rollback=passed
restored_daemon_sha256=d0d7457b82ad55e19263efe255949856f28679ef95cde4bb86b755abd95c1a41
restored_loader_sha256=47c78c871c7caefed8eee2e11e14b8fda5edd3bf85a07f3f099ff24b0a88ce51
```

The corrected pass derived `BUILD_DIR` from Make, installed the accepted
Slice-2P build and completed the mutation-free runtime acceptance.

---

## Explicitly Deferred

Slice 2P does not migrate:

```text
POST /api/epgsearch/native-fuzzy/stale-probes/delete
POST /api/vdr/epgsearch/native-fuzzy/stale-probes/delete
```

Those aliases are global stale-probe administration rather than query-scoped
cache refresh. They remain fail-closed and require a separate bounded security,
frontend-ownership and runtime-safety contract.

Slice 2P also does not add generic role administration, credential lifecycle,
completion/outcome accountability, Android clients, Phase 63+ runtime, a
PR-ready transition or merge.

## Exact Next Action

1. let the documentation-closeout commit complete its full five-job CI;
2. keep PR #117 open, Draft and unmerged;
3. inspect the remaining POST inventory and plan exactly one bounded Phase 62
   route family;
4. do not implement the next family until its permission, scope source,
   frontend owner, safe runtime boundary and rollback contract are explicit.
