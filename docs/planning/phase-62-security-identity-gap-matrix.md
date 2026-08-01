# Phase 62 Security and Identity Gap Matrix

Status: active Phase 62 planning and implementation matrix  
Repository baseline: `cb77ff66e11dca7db2eafa36525762dcde35102d` (`main`, merge of PR #115)  
Accepted branch slices: Slice 1 through Slice 2P  
Accepted code/runtime head: `173c929964dbb7aabd30c5e482c2e250b5785d92`  
Authoritative CI: #6649 / run `30711237050` / all five jobs successful

A component is not accepted installed runtime until it is connected, covered by
the complete CI graph and validated on the real yaVDR system. Code-head evidence
alone is insufficient.

## Gap matrix

| Security area | Current accepted state | Remaining gap | Next bounded work |
|---|---|---|---|
| Actor/device model | Canonical persistent actor, device, session and credential context | Protected enrollment and administration | Later lifecycle administration slice |
| Authentication | Legacy Basic, optional Managed Basic and browser sessions authenticate ordinary routes; browser cookie has strict precedence | Native/service credential mechanisms and compatibility retirement | Continue exact route migration first |
| Browser sessions | Atomic issue/logout, independent cookie and CSRF secrets, persistence, expiry/revocation checks and replay denial | Refresh, idle timeout, cleanup, concurrency and recovery policy | Dedicated lifecycle policy slice |
| Grants and scopes | Active exact actor grants load from persistence; unavailable store fails closed | Protected grant administration and broader resource scopes | After route catalogue is sufficiently complete |
| Fixed roles | Exact-backend Admin and Read-only catalogue accepted; Read-only wins; wildcard roles do not become concrete assignments | Generic persisted role definitions and assignments | Later protected administration slice |
| CSRF | Enforced for all currently migrated browser mutation families; frontend tokens remain memory-only and owner-injected | Remaining business and administrative POST families | One exact route family per slice |
| Central authorization | Accepted through Remote, Timer, Channel Move, Recording execution, SearchTimer create/maintenance/execution, Native Fuzzy refresh and query-scoped cache refresh | Remaining POST inventory and safe classifications | Select exactly one bounded family |
| Query-scoped cache refresh | SearchTimer preview and EPG cache refresh use distinct permissions and query-derived backend scope | Completion/outcome evidence only | No further route work in Slice 2P |
| Safe POST classification | Accepted bounded validation/preview family explicitly classified | Remaining non-mutating stateful POST routes | Separate explicit classification slices |
| Backend policy | Backend read-only/capability/domain checks remain independent from actor authorization | Preserve this separation for every migration | Every route slice |
| Accountability | Append-only pre-dispatch allow/deny evidence accepted and secret-free | Completion/outcome events, transactional coupling/outbox, protected query/export/retention | Later accountability slices |
| Revisions/idempotency | Domain-specific partial mechanisms only | Common preconditions, idempotency and durable operation lifecycle | Later Phase 62 slice |
| Administration | No general security-management API | Protected identity, credential, grant and role operations | After business-route migration |
| Native/service clients | Core model is transport-neutral | Enrollment, rotation, refresh and revocation contracts | Later Phase 62 slice |

## Accepted mutating and stateful POST inventory

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
| Native Fuzzy stale-probe delete aliases | Not migrated; explicitly excluded from Slice 2P and fail closed |

## Slice 2P contract

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
- exact-backend Admin applies; Read-only denies first;
- stale-probe deletion aliases remain excluded.

## Latest runtime evidence

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
3. **Complete remaining route classification and migration — active.**
4. **Common revisions, idempotency and durable operation lifecycle — open.**
5. **Completion accountability, coupling/outbox and protected audit reads — open.**
6. **Compatibility retirement readiness and final Phase 62 closeout — open.**

## Exact next action

Let the Slice-2P documentation closeout pass the complete five-job CI. Then
inspect the remaining POST inventory and plan exactly one next route family.
Do not combine it with generic administration, Android, native/service
credential lifecycle or Phase 63-67 runtime.
