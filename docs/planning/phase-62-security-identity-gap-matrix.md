# Phase 62 Security and Identity Gap Matrix

Status: active Phase 62 planning and implementation matrix  
Repository baseline: `cb77ff66e11dca7db2eafa36525762dcde35102d` (`main`, merge of PR #115)  
Accepted branch slices: Slice 1 through Slice 2S  
Active repository implementation: Slice 2T browser-session issuing-credential lifecycle binding  
Accepted code/runtime head: `c128867bfbf4ce10bcf7dc23d14652e5f5324c83`  
Accepted closeout head: `064744f73905b6fcc53d737ab9088554ae2af4b6`  
Authoritative accepted closeout CI: #6664 / run `30718491649` / all five jobs successful  
Slice 2T CI and real yaVDR acceptance: pending

A component is not accepted installed runtime until it is connected, covered by
the complete CI graph and validated on the real yaVDR system. Code-head evidence
alone is insufficient.

## Gap matrix

| Security area | Current accepted state | Remaining gap | Next bounded work |
|---|---|---|---|
| Actor/device model | Canonical persistent actor, device, session and credential context | Protected enrollment and administration | Later lifecycle administration slice |
| Authentication | Legacy Basic, optional Managed Basic and browser sessions authenticate ordinary routes; browser cookie has strict precedence | Issuing-credential request-time binding is active in Slice 2T; native/service mechanisms and compatibility retirement remain open | Complete Slice 2T only |
| Browser sessions | Atomic issue/logout, independent cookie and CSRF secrets, persistence, configurable absolute expiry, revocation checks, replay denial and lifecycle outcomes | Issuer lineage is repository-active in Slice 2T; idle timeout, cleanup and concurrency remain open | Complete Slice 2T only |
| Issuing credential lineage | Issuance records `issued_from_credential_id` and validates the issuer at creation | Later cookie and CSRF requests must fail when that issuer is missing, mismatched, inactive, revoked or expired | Slice 2T |
| Grants and scopes | Active exact actor grants load from persistence; unavailable store fails closed; concrete and global exact scopes are runtime accepted | Protected grant administration and broader resource scopes | After bounded security-management design |
| Fixed roles | Exact-scope Admin and Read-only semantics are accepted for concrete backends and global `*`; no inherited wildcard semantics | Generic persisted roles remain open | Defer until the fixed catalogue is stable |
| CSRF | Enforced for all accepted browser mutation families; frontend tokens remain memory-only and owner-injected | Slice 2T binds CSRF verification to issuer lifecycle; future frontend owners still require explicit contracts | Complete Slice 2T only |
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

## Active Slice 2T contract

The fresh post-2S lifecycle audit separated already-covered canonical identity
checks from the remaining issuer-lineage gap.

`PersistentIdentityResolver` already validates:

- actor lifecycle;
- device ownership and lifecycle;
- canonical browser session lifecycle;
- canonical browser credential lifecycle.

Slice 2T adds request-time resolution of the stored
`issued_from_credential_id`:

```text
issuer exists
issuer.actor_id == browser.actor_id
issuer.active == true
issuer.revoked_at == ''
issuer.expires_at is empty or in the future
```

Both cookie authentication and CSRF verification use the resolved record.
Issuer expiry maps to `credential_expired`. Missing, mismatched, inactive or
revoked issuers map to `credential_revoked`.

Boundaries:

- raw `findByTokenId` and `findBySessionId` semantics remain unchanged;
- no descendant browser row is automatically mutated;
- no schema migration is introduced;
- no route, permission, frontend, configuration or packaging change is included;
- no idle timeout, cleanup, refresh, concurrency or administration is included.

## Phase 62 dependency order

1. **Identity and authorization foundation — accepted.**
2. **Persistent lifecycle, browser sessions, exact grants and fixed roles —
   accepted for the current catalogue.**
3. **Business and administrative POST migration — accepted through Slice 2Q;
   no remaining product POST gap.**
4. **Absolute browser-session lifetime configuration — Slice 2R accepted.**
5. **Browser-session issue/revoke outcome accountability — Slice 2S accepted.**
6. **Browser-session issuing-credential lifecycle binding — Slice 2T repository
   implementation active; CI/runtime pending.**
7. **Idle expiry, cleanup and concurrent-session policy — open and separate.**
8. **Common revisions, idempotency and durable operation lifecycle — open.**
9. **Broader outcomes, coupling/outbox and protected audit reads — open.**
10. **Protected identity, credential, grant and generic-role administration — open.**
11. **Compatibility retirement readiness and final Phase 62 closeout — open.**

## Exact next action

Publish the bounded Slice-2T repository implementation as one fast-forward
commit and require all five CI jobs. Only after full green CI may the guarded
issuer-revocation yaVDR acceptance run. Do not combine Slice 2T with idle
timeout, cleanup, concurrency, security administration, Android or Phase 63-67.
