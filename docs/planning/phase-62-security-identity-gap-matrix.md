# Phase 62 Security and Identity Gap Matrix

Status: active Phase 62 planning and implementation matrix  
Repository baseline: `cb77ff66e11dca7db2eafa36525762dcde35102d` (`main`, merge of PR #115)  
Accepted branch slices: Slice 1 through Slice 2Q  
Active repository implementation: Slice 2R configurable absolute browser-session lifetime  
Accepted code/runtime head: `88ec36076d7e5114df0a3a186cc6fbd52bb2baac`  
Authoritative accepted closeout CI: #6658 / run `30714506053` / all five jobs successful  
Slice 2R CI and real yaVDR acceptance: pending

A component is not accepted installed runtime until it is connected, covered by
the complete CI graph and validated on the real yaVDR system. Code-head evidence
alone is insufficient.

## Gap matrix

| Security area | Current accepted state | Remaining gap | Next bounded work |
|---|---|---|---|
| Actor/device model | Canonical persistent actor, device, session and credential context | Protected enrollment and administration | Later lifecycle administration slice |
| Authentication | Legacy Basic, optional Managed Basic and browser sessions authenticate ordinary routes; browser cookie has strict precedence | Native/service credential mechanisms and compatibility retirement | After Slice 2R acceptance |
| Browser sessions | Atomic issue/logout, independent cookie and CSRF secrets, persistence, absolute expiry/revocation checks and replay denial | Slice 2R lifetime configuration needs CI/runtime acceptance; idle timeout, cleanup and concurrency remain open | Complete Slice 2R only |
| Grants and scopes | Active exact actor grants load from persistence; unavailable store fails closed; concrete and global exact scopes are runtime accepted | Protected grant administration and broader resource scopes | After bounded security-management design |
| Fixed roles | Exact-scope Admin and Read-only semantics are accepted for concrete backends and global `*`; no inherited wildcard semantics | Generic persisted roles remain open | Defer until the fixed catalogue is stable |
| CSRF | Enforced for all accepted browser mutation families; frontend tokens remain memory-only and owner-injected | Future frontend mutation owners require explicit contracts | Preserve per-owner injection |
| Central authorization | Accepted through all registered business and administrative POST families | No unmigrated product POST remains in the fresh post-2Q inventory | Do not invent another route-migration slice |
| Query-scoped cache refresh | SearchTimer preview and EPG cache refresh use distinct permissions and query-derived backend scope | Completion/outcome evidence only | No further route work in Slice 2P |
| Global stale-probe administration | Both Delete aliases use `epgsearch.native-fuzzy.stale-probes.delete@*`; zero-delete runtime accepted | No protected read/list API or frontend owner | Any future administration UI requires a separate slice |
| Browser-session lifetime | Accepted runtime uses fixed absolute 28800 seconds | Repository Slice 2R adds strict configurable `300..86400` alignment for persistence and cookie; CI/runtime pending | Complete guarded Slice 2R acceptance |
| Safe POST classification | All registered non-mutating stateful POSTs are explicitly classified | Re-audit only when new routes are added | No open route gap now |
| Backend policy | Backend read-only/capability/domain checks remain independent from actor authorization | Preserve this separation for every future migration | Every route slice |
| Accountability | Append-only pre-dispatch allow/deny evidence accepted and secret-free | Completion/outcome events, transactional coupling/outbox, protected query/export/retention | Candidate later accountability slice |
| Revisions/idempotency | Domain-specific partial mechanisms only | Common preconditions, idempotency and durable operation lifecycle | Later Phase 62 slice |
| Administration | No general security-management API | Protected identity, credential, grant and role operations | Separate design and implementation slices |
| Native/service clients | Core model is transport-neutral | Enrollment, rotation, refresh and revocation contracts | Later Phase 62 slice |

## Completed post-Slice-2Q POST inventory

The HTTP path has exactly two POST ownership layers:

1. `BrowserSessionHttpGate` owns issue and logout;
2. all remaining POSTs pass through `SecurityHttpGate` and the central API
   router.

Every central-router POST family is now one of:

- a protected mutation with an exact permission and scope; or
- an explicitly classified Safe POST requiring authentication.

Unknown browser-session and enforced-mode mutations remain fail-closed. There is
no remaining unmigrated product POST family after Slice 2Q.

## Active Slice 2R contract

```text
VDR_SUITE_BROWSER_SESSION_LIFETIME_SECONDS
```

Repository contract pending CI/runtime acceptance:

- absent value retains `28800` seconds;
- accepted syntax is strict unsigned decimal only;
- inclusive minimum is `300`;
- inclusive maximum is `86400`;
- parsing rejects overflow before multiplication;
- the issuance service remains the authority for all three bounds;
- one immutable value controls persisted `expires_at` and cookie `Max-Age`;
- invalid configuration returns HTTP 503 for browser-session issuance;
- invalid configuration emits no `Set-Cookie` and creates no session;
- ordinary API routes and existing sessions are unchanged;
- request data cannot override the server value;
- Packaging exposes the compatible default.

Explicitly deferred:

- idle timeout and `last_seen`;
- sliding expiry and refresh;
- cleanup of expired rows;
- concurrent-session limits;
- user-selectable lifetimes;
- generic security administration.

## Latest accepted Slice 2Q runtime evidence

```text
Head: 88ec36076d7e5114df0a3a186cc6fbd52bb2baac
CI: #6658 / run 30714506053 / all five jobs successful
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

## Phase 62 dependency order

1. **Identity and authorization foundation — accepted.**
2. **Persistent lifecycle, browser sessions, exact grants and fixed roles —
   accepted for the current catalogue.**
3. **Business and administrative POST migration — accepted through Slice 2Q;
   fresh inventory shows no remaining product POST gap.**
4. **Absolute browser-session lifetime configuration — Slice 2R repository
   implementation active; CI/runtime pending.**
5. **Idle expiry, cleanup and concurrent-session policy — open and explicitly
   separate from Slice 2R.**
6. **Common revisions, idempotency and durable operation lifecycle — open.**
7. **Completion accountability, coupling/outbox and protected audit reads — open.**
8. **Protected identity, credential, grant and generic-role administration — open.**
9. **Compatibility retirement readiness and final Phase 62 closeout — open.**

## Exact next action

Publish the bounded Slice-2R repository implementation as one fast-forward
commit and require all five CI jobs. Only after full green CI may the guarded
custom-lifetime yaVDR acceptance run. Do not combine Slice 2R with idle timeout,
refresh, cleanup, concurrency, generic administration, Android or Phase 63-67.
