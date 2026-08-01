# Phase 62 Security and Identity Gap Matrix

Status: active Phase 62 planning and implementation matrix
Repository baseline: `cb77ff66e11dca7db2eafa36525762dcde35102d` (`main`, merge of PR #115)
Accepted branch slices: Slice 1 through Slice 2T
Active repository implementation: none selected after Slice 2T runtime acceptance
Accepted implementation/runtime head: `55876356e84b3e47e52911529b3f9bfa0e17f191`
Authoritative accepted source CI: #6666 / run `30719552024` / all five jobs successful
Documentation-only Slice-2T closeout: this commit; closeout CI pending

A component is not accepted installed runtime until it is connected, covered by
the complete CI graph and validated on the real yaVDR system. Code-head evidence
alone is insufficient.

## Gap matrix

| Security area | Current accepted state | Remaining gap | Next bounded work |
|---|---|---|---|
| Actor/device model | Canonical persistent actor, device, session and credential context | Protected enrollment and administration | Later lifecycle administration slice |
| Authentication | Legacy Basic, optional Managed Basic and browser sessions authenticate ordinary routes; browser cookie has strict precedence; issuing-credential request-time binding is runtime accepted | Native/service mechanisms and compatibility retirement remain open | Select only after fresh post-2T analysis |
| Browser sessions | Atomic issue/logout, independent cookie and CSRF secrets, persistence, configurable absolute expiry, revocation checks, replay denial, lifecycle outcomes and issuer-lineage enforcement are runtime accepted | Idle timeout, cleanup and concurrency remain open | Separate future policy slice |
| Issuing credential lineage | Issuance records `issued_from_credential_id`; later cookie and CSRF requests fail when the issuer is missing, mismatched, inactive, revoked or expired | No cascading descendant cleanup or issuer-management surface | Slice 2T is runtime accepted |
| Grants and scopes | Active exact actor grants load from persistence; unavailable store fails closed; concrete and global exact scopes are runtime accepted | Protected grant administration and broader resource scopes | After bounded security-management design |
| Fixed roles | Exact-scope Admin and Read-only semantics are accepted for concrete backends and global `*`; no inherited wildcard semantics | Generic persisted roles remain open | Defer until the fixed catalogue is stable |
| CSRF | Enforced for all accepted browser mutation families; frontend tokens remain memory-only and owner-injected; issuer lifecycle binding is runtime accepted | Future frontend owners still require explicit contracts | Preserve explicit ownership |
| Central authorization | Accepted through all registered business and administrative POST families | No unmigrated product POST remains in the fresh post-2Q inventory | Do not invent another route-migration slice |
| Query-scoped cache refresh | SearchTimer preview and EPG cache refresh use distinct permissions and query-derived backend scope | Completion/outcome evidence only | No further route work in Slice 2P |
| Global stale-probe administration | Delete aliases use `epgsearch.native-fuzzy.stale-probes.delete@*`; zero-delete runtime accepted | No protected read/list API or frontend owner | Any future UI requires a separate slice |
| Browser-session lifetime | Strict configurable absolute `300..86400` alignment for persistence and cookie is runtime accepted | Idle expiry, cleanup, concurrency and refresh policy are separate gaps | Slice 2R is closed |
| Browser lifecycle outcomes | Gate-owned pre-dispatch evidence plus issue/revoke outcomes are accepted; failed issue-outcome persistence compensates and failed revoke-outcome persistence expires the cookie | Other operation families and stronger coupling remain open | Slice 2S is closed |
| Safe POST classification | All registered non-mutating stateful POSTs are explicitly classified | Re-audit only when new routes are added | No open route gap now |
| Backend policy | Backend read-only/capability/domain checks remain independent from actor authorization | Preserve this separation for every future migration | Every future operation slice |
| Accountability | Pre-dispatch evidence and browser lifecycle outcomes are accepted and secret-free | Other outcomes, stronger coupling/outbox, protected query/export/retention remain open | Select one bounded contract only |
| Revisions/idempotency | Domain-specific partial mechanisms only | Common preconditions, idempotency and durable operation lifecycle | Later Phase 62 slice |
| Administration | No general security-management API | Protected identity, credential, grant and role operations | Separate design and implementation slices |
| Native/service clients | Core model is transport-neutral | Enrollment, rotation, refresh and revocation contracts | Later Phase 62 slice |

## Completed post-Slice-2Q POST inventory

The HTTP path has exactly two POST ownership layers:

1. `BrowserSessionHttpGate` owns issue and logout;
2. all remaining POSTs pass through `SecurityHttpGate` and the central API
   router.

Every central-router POST family is a protected mutation or an explicitly
classified Safe POST. Unknown browser-session and enforced-mode mutations remain
fail-closed. There is no remaining unmigrated product POST family.

## Accepted Slice 2S evidence

```text
Head: c128867bfbf4ce10bcf7dc23d14652e5f5324c83
Source CI: #6663 / run 30717721595 / all five jobs successful
Closeout: 064744f73905b6fcc53d737ab9088554ae2af4b6
Closeout CI: #6664 / run 30718491649 / all five jobs successful
Service PID after install/acceptance: 69610 / 69610
HTTP requests: 5
Lifecycle accountability events: 5
Operation-succeeded events: 2
Missing-CSRF operation events: 0
Session and credential revocation: passed
Revoked-cookie replay: denied
Accountability: secret-free
Database integrity: yes
Service active: yes
```

Evidence directory:

```text
/var/backups/vdr-suite-phase62-slice2s-20260801T210333Z-c128867bfbf4/runtime-acceptance-slice2s
```

## Accepted Slice 2T evidence

Slice 2T request-time resolution of `issued_from_credential_id` is accepted
in source CI and on the real yaVDR runtime.

```text
Head: 55876356e84b3e47e52911529b3f9bfa0e17f191
Source CI: #6666 / run 30719552024 / all five jobs successful
Installed/running daemon:
34b80de4fd8f55b763c4483f0dcb50ee09e5cdc49de7f6e7c25e01ba50d84269
Runtime report SHA-256:
2ca7fcaefe21c1198e5d8ff88b3e17237b2e72a545780cc14f0200e7dd0ca983
Active ordinary GET before issuer invalidation: HTTP 200
Revoked-issuer ordinary GET: HTTP 401 credential_revoked
Revoked-issuer logout: HTTP 401 credential_revoked
Logout denied before CSRF: yes
Raw browser row unchanged before cleanup: yes
Original issuer unchanged: yes
Test browser lifecycle revoked: yes
Replay denied: yes
VDR domain mutations: 0
Database integrity: yes
Service active: yes
```

Evidence directory:

```text
/var/backups/vdr-suite-phase62-slice2t-20260801T223353Z-55876356e84b/runtime-acceptance-slice2t
```

Boundaries remain unchanged:

- raw `findByTokenId` and `findBySessionId` retain stored-row semantics;
- no descendant browser row is automatically mutated;
- no schema migration was introduced;
- no route, permission, frontend, configuration or packaging change was
  added;
- idle timeout, cleanup, refresh, concurrency and security administration
  remain separate gaps.

## Phase 62 dependency order

1. **Identity and authorization foundation — accepted.**
2. **Persistent lifecycle, browser sessions, exact grants and fixed roles —
   accepted for the current catalogue.**
3. **Business and administrative POST migration — accepted through
   Slice 2Q; no remaining product POST gap.**
4. **Absolute browser-session lifetime configuration — Slice 2R
   accepted.**
5. **Browser-session issue/revoke outcome accountability — Slice 2S
   accepted.**
6. **Browser-session issuing-credential lifecycle binding — Slice 2T
   source CI and real yaVDR runtime accepted.**
7. **Documentation-only Slice-2T closeout and closeout CI — pending.**
8. **Idle expiry, cleanup and concurrent-session policy — open and
   separate.**
9. **Common revisions, idempotency and durable operation lifecycle —
   open.**
10. **Broader outcomes, coupling/outbox and protected audit reads —
    open.**
11. **Protected identity, credential, grant and generic-role
    administration — open.**
12. **Compatibility retirement readiness and final Phase 62 closeout —
    open.**

## Exact next action

Require all five CI jobs for this documentation-only Slice-2T closeout.

No next Phase-62 implementation slice is selected by this closeout. After
full closeout CI, perform a fresh post-2T gap analysis. Do not combine the
closeout with idle timeout, cleanup, concurrency, security administration,
Android or Phase 63-67 work.
