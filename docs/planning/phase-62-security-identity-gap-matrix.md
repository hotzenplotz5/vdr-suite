# Phase 62 Security and Identity Gap Matrix

Status: active Phase 62 planning and implementation matrix
Repository baseline: `cb77ff66e11dca7db2eafa36525762dcde35102d` (`main`, merge of PR #115)
Accepted runtime slices: Slice 1 through Slice 2U
Accepted Slice-2U implementation/runtime head: `16ff04a4ba371aad32fc4a38bf82f9c0529c532d`
Accepted Slice-2U source CI: #6690 / run `30723297375` / all five jobs successful
Documentation-only Slice-2U closeout: this commit; closeout CI pending
Active repository implementation: none selected after Slice 2U runtime acceptance

A component is not accepted installed runtime until it is connected, covered by
the complete CI graph and validated on the real yaVDR system. Code-head evidence
alone is insufficient.

## Gap matrix

| Security area | Current accepted state | Remaining gap | Next bounded work |
|---|---|---|---|
| Actor/device model | Canonical persistent actor, device, session and credential context | Protected enrollment and administration | Later lifecycle administration slice |
| Authentication | Legacy Basic, optional Managed Basic and browser sessions authenticate ordinary routes; browser cookie has strict precedence; issuing-credential request-time binding is runtime accepted | Native/service mechanisms and compatibility retirement remain open | Preserve while Slice 2U changes issuance policy only |
| Browser sessions | Atomic issue/logout, independent cookie and CSRF secrets, persistence, configurable absolute expiry, revocation checks, replay denial, lifecycle outcomes, issuer-lineage enforcement and an optional effective per-actor concurrency limit are runtime accepted | Idle timeout and cleanup/retention remain open | Select only after post-2U gap analysis |
| Concurrent browser sessions | Strict optional `0..64` per-actor effective-session limit with atomic deny-new semantics is source- and runtime-accepted; default `0` remains unlimited | Idle expiry, cleanup and administration remain separate | Slice 2U closed pending documentation CI |
| Issuing credential lineage | Issuance records `issued_from_credential_id`; later cookie and CSRF requests fail when the issuer is missing, mismatched, inactive, revoked or expired | No cascading descendant cleanup or issuer-management surface | Slice 2T is runtime accepted |
| Grants and scopes | Active exact actor grants load from persistence; unavailable store fails closed; concrete and global exact scopes are runtime accepted | Protected grant administration and broader resource scopes | After bounded security-management design |
| Fixed roles | Exact-scope Admin and Read-only semantics are accepted for concrete backends and global `*`; no inherited wildcard semantics | Generic persisted roles remain open | Defer until the fixed catalogue is stable |
| CSRF | Enforced for all accepted browser mutation families; frontend tokens remain memory-only and owner-injected; issuer lifecycle binding is runtime accepted | Future frontend owners still require explicit contracts | Preserve explicit ownership |
| Central authorization | Accepted through all registered business and administrative POST families | No unmigrated product POST remains in the fresh post-2Q inventory | Do not invent another route-migration slice |
| Query-scoped cache refresh | SearchTimer preview and EPG cache refresh use distinct permissions and query-derived backend scope | Completion/outcome evidence only | No further route work in Slice 2P |
| Global stale-probe administration | Delete aliases use `epgsearch.native-fuzzy.stale-probes.delete@*`; zero-delete runtime accepted | No protected read/list API or frontend owner | Any future UI requires a separate slice |
| Browser-session lifetime | Strict configurable absolute `300..86400` alignment for persistence and cookie is runtime accepted | Idle expiry, cleanup and refresh policy are separate gaps | Slice 2R is closed |
| Browser lifecycle outcomes | Gate-owned pre-dispatch evidence plus issue/revoke outcomes are accepted; failed issue-outcome persistence compensates and failed revoke-outcome persistence expires the cookie | Other operation families and stronger coupling remain open | Slice 2S is closed |
| Safe POST classification | All registered non-mutating stateful POSTs are explicitly classified | Re-audit only when new routes are added | No open route gap now |
| Backend policy | Backend read-only/capability/domain checks remain independent from actor authorization | Preserve this separation for every future migration | Every future operation slice |
| Accountability | Pre-dispatch evidence, browser lifecycle outcomes and the Slice-2U limit-reached issue-policy outcome are accepted and secret-free | Other outcomes, stronger coupling/outbox, protected query/export/retention remain open | Separate future design |
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
Closeout: e79e0eb67da75044c4a9afa162c9dab188b026fd
Closeout CI: #6667 / run 30721936576 / all five jobs successful
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

## Accepted Slice 2U evidence

Slice 2U introduced:

```text
VDR_SUITE_BROWSER_SESSION_MAX_ACTIVE_PER_ACTOR
0     unlimited compatibility default
1..64 maximum effective active browser sessions per actor
```

The effective count joins the browser row to the canonical actor, device,
session, browser credential and issuing credential. Every component must remain
active, unrevoked and unexpired. Count and insert remain inside the existing
serialized `BEGIN IMMEDIATE` transaction.

At the configured bound, issuance returns `LimitReached`, creates nothing and
maps to HTTP 409 `browser_session_limit_reached`. It never evicts, revokes or
rewrites an existing session.

```text
Implementation/runtime head:
16ff04a4ba371aad32fc4a38bf82f9c0529c532d

Source CI:
#6690 / run 30723297375 / all five jobs successful

Installed/running daemon SHA-256:
0e3ec0d57f4471804824247f712c2457015cc22ac9576df60d8d77ed8ddb3134

Loader SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Final service PID:
79316

Runtime report SHA-256:
7c33b06d07beff6b17bad82153ff4a0f1e7c1f5c8d8f972406f8b0b9160f4c89

Configured limit:
1

First session issue:
HTTP 200

Second same-actor issue:
HTTP 409 browser_session_limit_reached
No second lifecycle row created

Existing first session ordinary GET:
HTTP 200

First logout:
HTTP 204
Slot released

Replacement session issue:
HTTP 200

Replacement logout:
HTTP 204

Revoked replacement-cookie replay:
HTTP 401 credential_revoked

Successful issue outcomes:
2

Limit-reached failed outcomes:
1

Successful revoke outcomes:
2

Acceptance lifecycle and source identity:
revoked

Original daemon configuration:
restored

SQLite quick check:
ok

SQLite foreign-key check:
empty

VDR domain mutations:
0

Service state:
active

Automatic rollback:
not required
```

Evidence directory:

```text
/var/backups/vdr-suite-phase62-slice2u-20260802T041910Z-16ff04a4ba37/runtime-acceptance-slice2u
```

Boundaries retained:

- no schema migration;
- no idle timeout or `last_seen`;
- no sliding refresh;
- no cleanup or retention;
- no automatic eviction or oldest-session selection;
- no session listing or administration API;
- no route, permission or frontend change;
- no Android or Phase 63-67 work.

## Phase 62 dependency order

1. **Identity and authorization foundation — accepted.**
2. **Persistent lifecycle, browser sessions, exact grants and fixed roles —
   accepted for the current catalogue.**
3. **Business and administrative POST migration — accepted through
   Slice 2Q; no remaining product POST gap.**
4. **Absolute browser-session lifetime configuration — Slice 2R accepted.**
5. **Browser-session issue/revoke outcome accountability — Slice 2S accepted.**
6. **Browser-session issuing-credential lifecycle binding — Slice 2T accepted.**
7. **Concurrent effective browser-session limit — Slice 2U source- and
   runtime-accepted.**
8. **Documentation-only Slice-2U closeout — CI pending.**
9. **Fresh post-2U gap analysis and one bounded next-slice selection — pending.**
10. **Idle expiry and cleanup/retention policy — open and separate.**
11. **Common revisions, idempotency and durable operation lifecycle — open.**
12. **Broader outcomes, coupling/outbox and protected audit reads — open.**
13. **Protected identity, credential, grant and generic-role administration —
    open.**
14. **Compatibility retirement readiness and final Phase 62 closeout — open.**

## Exact next action

Require all five CI jobs for this documentation-only Slice-2U closeout.

Do not select or implement another Phase-62 slice until closeout CI is fully
green. Then perform a fresh post-2U gap analysis and choose exactly one bounded
slice. Do not combine idle timeout, cleanup, security administration, Android or
Phase 63-67 work with this closeout.
