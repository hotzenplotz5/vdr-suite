# Phase 62 Security and Identity Gap Matrix

Status: active Phase 62 planning and implementation matrix  
Repository baseline: `cb77ff66e11dca7db2eafa36525762dcde35102d` (`main`, merge of PR #115)  
Accepted branch slices: Slice 1 through Slice 2P  
Active repository implementation: Slice 2Q global Native Fuzzy stale-probe deletion  
Accepted code/runtime head: `173c929964dbb7aabd30c5e482c2e250b5785d92`  
Authoritative accepted CI: #6649 / run `30711237050` / all five jobs successful

A component is not accepted installed runtime until it is connected, covered by
the complete CI graph and validated on the real yaVDR system. Code-head evidence
alone is insufficient.

## Gap matrix

| Security area | Current accepted state | Remaining gap | Next bounded work |
|---|---|---|---|
| Actor/device model | Canonical persistent actor, device, session and credential context | Protected enrollment and administration | Later lifecycle administration slice |
| Authentication | Legacy Basic, optional Managed Basic and browser sessions authenticate ordinary routes; browser cookie has strict precedence | Native/service credential mechanisms and compatibility retirement | Complete Slice 2Q acceptance first |
| Browser sessions | Atomic issue/logout, independent cookie and CSRF secrets, persistence, expiry/revocation checks and replay denial | Refresh, idle timeout, cleanup, concurrency and recovery policy | Dedicated lifecycle policy slice |
| Grants and scopes | Active exact actor grants load from persistence; unavailable store fails closed | Protected grant administration and broader resource scopes | After route catalogue is sufficiently complete |
| Fixed roles | Exact-scope Admin and Read-only catalogue accepted for concrete backends; wildcard roles do not become concrete assignments | Validate the exact global `*` role scope in Slice 2Q; generic roles remain open | Complete Slice 2Q CI/runtime acceptance |
| CSRF | Enforced for all accepted browser mutation families; frontend tokens remain memory-only and owner-injected | Slice 2Q repository implementation requires CI/runtime acceptance | Complete Slice 2Q acceptance |
| Central authorization | Accepted through Remote, Timer, Channel Move, Recording execution, SearchTimer create/maintenance/execution, Native Fuzzy refresh and query-scoped cache refresh | Global stale-probe deletion is implemented in Slice 2Q but not yet accepted | Complete Slice 2Q CI/runtime acceptance |
| Query-scoped cache refresh | SearchTimer preview and EPG cache refresh use distinct permissions and query-derived backend scope | Completion/outcome evidence only | No further route work in Slice 2P |
| Global stale-probe administration | GET is read-only; Slice 2Q protects the two global Delete aliases with `...delete@*` in repository code | Full CI and zero-delete yaVDR acceptance remain pending | Run the complete Slice 2Q acceptance gate |
| Safe POST classification | Accepted bounded validation/preview family explicitly classified | Remaining non-mutating stateful POST routes | Separate explicit classification slices |
| Backend policy | Backend read-only/capability/domain checks remain independent from actor authorization | Preserve this separation for every migration | Every route slice |
| Accountability | Append-only pre-dispatch allow/deny evidence accepted and secret-free | Completion/outcome events, transactional coupling/outbox, protected query/export/retention | Later accountability slices |
| Revisions/idempotency | Domain-specific partial mechanisms only | Common preconditions, idempotency and durable operation lifecycle | Later Phase 62 slice |
| Administration | No general security-management API | Protected identity, credential, grant and role operations | After business-route migration |
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
| Accepted validate/plan/preview routes | Explicit safe POST classification |
| Native Fuzzy operator refresh aliases | Protected backend-scoped administrative mutation |
| SearchTimer preview cache refresh aliases | Protected query-scoped mutation using `searchtimers.preview-cache.refresh` |
| EPG cache refresh | Protected query-scoped mutation using `epg.cache.refresh` |
| Native Fuzzy stale-probe delete aliases | Slice 2Q repository implementation: protected global mutation using `epgsearch.native-fuzzy.stale-probes.delete@*`; CI/runtime pending |

## Slice 2P accepted contract

```text
POST /api/searchtimers/preview/cache/refresh
POST /api/vdr/searchtimers/preview/cache/refresh
  -> searchtimers.preview-cache.refresh@<query backend>

POST /api/epg/cache/refresh
  -> epg.cache.refresh@<query backend>
```

Rules:

- scope source is query parameter `backend` only;
- missing or empty values normalize to `default`;
- URL decoding follows router semantics;
- the last duplicate backend value wins;
- body fields cannot override query scope;
- exact paths with query strings are migrated;
- trailing slashes remain fail-closed;
- the two permissions are non-interchangeable;
- exact-backend Admin applies; Read-only denies first.

## Slice 2Q active contract

```text
POST /api/epgsearch/native-fuzzy/stale-probes/delete
POST /api/vdr/epgsearch/native-fuzzy/stale-probes/delete
  -> epgsearch.native-fuzzy.stale-probes.delete@*
```

Repository contract pending CI/runtime acceptance:

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
- runtime dispatch is allowed only after an empty stale-probe GET preflight;
- every acceptance POST must report zero deleted rows.

## Latest accepted runtime evidence

```text
SearchTimer preview cache refresh: 29/29 tests, 27 requests
EPG cache refresh: 18/18 tests, 16 requests
Total: 47/47 tests, 43 requests
Daemon PID: 66229 before/after acceptance
Resource state unchanged: yes
Cache mutation: none
Target grants restored: yes
Browser session revoked: yes
Database integrity: yes
Service active: yes
```

Evidence directory:

```text
/var/backups/vdr-suite-phase62-slice2p-20260801T180617Z-173c929964db/runtime-acceptance-slice2p
```

## Phase 62 dependency order

1. **Identity and authorization foundation — accepted.**
2. **Persistent lifecycle, browser sessions, exact grants, fixed roles and
   bounded route migration — active and accepted through Slice 2P.**
3. **Global stale-probe deletion — Slice 2Q repository implementation active;
   CI and real-runtime acceptance pending.**
4. **Complete remaining route classification and migration — re-audit after
   Slice 2Q acceptance.**
5. **Common revisions, idempotency and durable operation lifecycle — open.**
6. **Completion accountability, coupling/outbox and protected audit reads — open.**
7. **Compatibility retirement readiness and final Phase 62 closeout — open.**

## Exact next action

Commit the bounded Slice 2Q repository implementation atomically, require all
five CI jobs to pass, and only then run the guarded real yaVDR acceptance. The
runtime pass must abort before any POST unless the stale-probe GET snapshot is
empty. Do not combine this work with generic administration, Android,
native/service credential lifecycle or Phase 63-67 runtime.
