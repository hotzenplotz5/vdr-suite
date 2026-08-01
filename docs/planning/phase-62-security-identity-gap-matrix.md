# Phase 62 Security and Identity Gap Matrix

Status: active Phase 62 planning and implementation matrix  
Repository baseline: `cb77ff66e11dca7db2eafa36525762dcde35102d` (`main`, merge of PR #115)  
Accepted branch slices: Slice 1 through Slice 2Q  
Accepted code/runtime head: `88ec36076d7e5114df0a3a186cc6fbd52bb2baac`  
Authoritative accepted CI: #6655 / run `30713953331` / all five jobs successful  
Slice 2Q real yaVDR zero-delete acceptance: complete

A component is not accepted installed runtime until it is connected, covered by
the complete CI graph and validated on the real yaVDR system. Code-head evidence
alone is insufficient.

## Gap matrix

| Security area | Current accepted state | Remaining gap | Next bounded work |
|---|---|---|---|
| Actor/device model | Canonical persistent actor, device, session and credential context | Protected enrollment and administration | Later lifecycle administration slice |
| Authentication | Legacy Basic, optional Managed Basic and browser sessions authenticate ordinary routes; browser cookie has strict precedence | Native/service credential mechanisms and compatibility retirement | Select only after the post-2Q audit |
| Browser sessions | Atomic issue/logout, independent cookie and CSRF secrets, persistence, expiry/revocation checks and replay denial | Refresh, idle timeout, cleanup, concurrency and recovery policy | Candidate dedicated lifecycle-policy slice |
| Grants and scopes | Active exact actor grants load from persistence; unavailable store fails closed; concrete and global exact scopes are runtime accepted | Protected grant administration and broader resource scopes | After bounded security-management design |
| Fixed roles | Exact-scope Admin and Read-only semantics are accepted for concrete backends and global `*`; no inherited wildcard semantics | Generic persisted roles remain open | Defer until the fixed catalogue and route inventory are stable |
| CSRF | Enforced for all accepted browser mutation families; frontend tokens remain memory-only and owner-injected | Future frontend mutation owners require explicit contracts | Preserve per-owner injection |
| Central authorization | Accepted through Remote, Timer, Channel Move, Recording execution, SearchTimer create/maintenance/execution, Native Fuzzy refresh, query-scoped cache refresh and global stale-probe deletion | Fresh inventory confirmation and future administration surfaces | Audit the complete POST inventory before selecting a new slice |
| Query-scoped cache refresh | SearchTimer preview and EPG cache refresh use distinct permissions and query-derived backend scope | Completion/outcome evidence only | No further route work in Slice 2P |
| Global stale-probe administration | Both Delete aliases use `epgsearch.native-fuzzy.stale-probes.delete@*`; concrete scopes deny; global Admin allows; global Read-only wins; zero-delete runtime accepted | No protected read/list API or frontend owner | Any future administration UI requires a separate slice |
| Safe POST classification | Accepted bounded validation/preview family explicitly classified | Confirm no remaining non-mutating stateful POST gaps | Fresh inventory audit |
| Backend policy | Backend read-only/capability/domain checks remain independent from actor authorization | Preserve this separation for every future migration | Every route slice |
| Accountability | Append-only pre-dispatch allow/deny evidence accepted and secret-free | Completion/outcome events, transactional coupling/outbox, protected query/export/retention | Candidate bounded accountability slice |
| Revisions/idempotency | Domain-specific partial mechanisms only | Common preconditions, idempotency and durable operation lifecycle | Later Phase 62 slice |
| Administration | No general security-management API | Protected identity, credential, grant and role operations | Separate design and implementation slices |
| Native/service clients | Core model is transport-neutral | Enrollment, rotation, refresh and revocation contracts | Later Phase 62 slice |

## Mutating and stateful POST inventory

| Route family | Current classification |
|---|---|
| Browser-session issue/logout | Real-runtime accepted self-service lifecycle |
| Remote actions | Protected mutation with exact permission/scope and browser CSRF |
| Timer create/update/delete | Protected mutations with exact permissions/scopes and browser CSRF |
| Channel Move aliases | Protected mutation with `channels.move` and browser CSRF |
| Recording execution aliases | Protected mutation with exact permission/scope and browser CSRF |
| SearchTimer create/update/delete | Protected mutations with exact permissions/scopes and browser CSRF |
| SearchTimer execution aliases | Protected mutation with exact permission/scope and browser CSRF |
| Accepted validate/plan/preview routes | Explicit Safe POST classification |
| Native Fuzzy operator refresh aliases | Protected backend-scoped administrative mutation |
| SearchTimer preview cache refresh aliases | Protected query-scoped mutation using `searchtimers.preview-cache.refresh` |
| EPG cache refresh | Protected query-scoped mutation using `epg.cache.refresh` |
| Native Fuzzy stale-probe delete aliases | Protected global mutation using `epgsearch.native-fuzzy.stale-probes.delete@*`; real-runtime accepted with zero deletions |

The prior repository audit found no other unclassified POST family in the central
router after Slice 2Q. This closeout does not treat that earlier observation as
a permanent inventory result: a fresh bounded audit is required before choosing
the next slice.

## Slice 2Q accepted contract

```text
POST /api/epgsearch/native-fuzzy/stale-probes/delete
POST /api/vdr/epgsearch/native-fuzzy/stale-probes/delete
  -> epgsearch.native-fuzzy.stale-probes.delete@*
```

Accepted rules:

- canonical scope is global `*` and cannot be changed by body or query;
- direct permission requires the global scope;
- concrete permission scopes are denied;
- `role.admin@*` is an exact global assignment and is allowed;
- concrete Admin scopes are denied;
- `role.read-only@*` denies before direct permission or Admin;
- concrete Read-only assignments do not affect the global route;
- exact paths with query strings are migrated;
- trailing slashes remain fail-closed;
- the routes remain excluded from Safe POST;
- no Webfrontend owner exists;
- runtime dispatch required an empty direct-SQLite stale/future snapshot;
- the snapshot used the production `604800`-second freshness boundary;
- a temporary cross-connection `BEFORE DELETE` trigger blocked deletion races;
- every authorized acceptance POST reported zero deleted rows;
- the guard was removed and the snapshot remained unchanged.

## Slice 2Q accepted runtime evidence

```text
Head: 88ec36076d7e5114df0a3a186cc6fbd52bb2baac
CI: #6655 / run 30713953331 / all five jobs successful
Tests: 32/32
HTTP requests: 25
Daemon PID after acceptance: 67393
Installed daemon SHA-256:
9f60daaf7d772abe7c6ad55388cb9bb7e8afe8f6679fbf749aa9103143a41d07
Installed loader SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
Snapshot SHA-256 before/after:
2b1d1af321fd9497f99ac742c694c70ecfde94b41bcb6d74ee3a70187e5d1e7a
Real stale-probe deletions: 0
Delete guard removed: yes
Database integrity: yes
Service active: yes
```

Evidence directory:

```text
/var/backups/vdr-suite-phase62-slice2q-20260801T191156Z-88ec36076d7e/runtime-acceptance-slice2q
```

The earlier GET-based attempt returned HTTP 404 before every Delete POST and
rolled back successfully. It remains safe rejection evidence only.

## Phase 62 dependency order

1. **Identity and authorization foundation — accepted.**
2. **Persistent lifecycle, browser sessions, exact grants and fixed roles —
   accepted for the current catalogue.**
3. **Bounded business and administrative POST migration — accepted through
   Slice 2Q.**
4. **Fresh POST inventory audit and next-slice selection — next.**
5. **Common revisions, idempotency and durable operation lifecycle — open.**
6. **Completion accountability, coupling/outbox and protected audit reads — open.**
7. **Protected identity, credential, grant and generic-role administration — open.**
8. **Compatibility retirement readiness and final Phase 62 closeout — open.**

## Exact next action

Let the Slice-2Q documentation closeout complete all five CI jobs. Then perform
a fresh bounded POST inventory audit and select exactly one next Phase 62 slice.
Do not combine that work with generic administration, Android, native/service
credential lifecycle or Phase 63-67 runtime unless the selected bounded slice
explicitly owns that scope.
