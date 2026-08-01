# Phase 62 Security and Identity Gap Matrix

Status: active Phase 62 planning and implementation matrix  
Repository baseline: `cb77ff66e11dca7db2eafa36525762dcde35102d` (`main`, merge of PR #115)  
Accepted branch slices: Slice 1 through Slice 2S  
Active repository implementation: none selected after Slice 2S closeout  
Accepted code/runtime head: `c128867bfbf4ce10bcf7dc23d14652e5f5324c83`  
Authoritative accepted source/runtime CI: #6663 / run `30717721595` / all five jobs successful

A component is not accepted installed runtime until it is connected, covered by
the complete CI graph and validated on the real yaVDR system. Code-head evidence
alone is insufficient.

## Gap matrix

| Security area | Current accepted state | Remaining gap | Next bounded work |
|---|---|---|---|
| Actor/device model | Canonical persistent actor, device, session and credential context | Protected enrollment and administration | Later lifecycle administration slice |
| Authentication | Legacy Basic, optional Managed Basic and browser sessions authenticate ordinary routes; browser cookie has strict precedence | Native/service credential mechanisms and compatibility retirement | Select only after fresh bounded review |
| Browser sessions | Atomic issue/logout, independent cookie and CSRF secrets, persistence, configurable bounded absolute expiry, revocation checks, replay denial and lifecycle outcome evidence | Idle timeout, cleanup and concurrency remain open | Separate lifecycle-policy slices only |
| Grants and scopes | Active exact actor grants load from persistence; unavailable store fails closed; concrete and global exact scopes are runtime accepted | Protected grant administration and broader resource scopes | After bounded security-management design |
| Fixed roles | Exact-scope Admin and Read-only semantics are accepted for concrete backends and global `*`; no inherited wildcard semantics | Generic persisted roles remain open | Defer until the fixed catalogue is stable |
| CSRF | Enforced for all accepted browser mutation families; frontend tokens remain memory-only and owner-injected | Future frontend mutation owners require explicit contracts | Preserve per-owner injection |
| Central authorization | Accepted through all registered business and administrative POST families | No unmigrated product POST remains in the fresh post-2Q inventory | Do not invent another route-migration slice |
| Query-scoped cache refresh | SearchTimer preview and EPG cache refresh use distinct permissions and query-derived backend scope | Completion/outcome evidence only | No further route work in Slice 2P |
| Global stale-probe administration | Both Delete aliases use `epgsearch.native-fuzzy.stale-probes.delete@*`; zero-delete runtime accepted | No protected read/list API or frontend owner | Any future administration UI requires a separate slice |
| Browser-session lifetime | Strict configurable absolute `300..86400` alignment for persistence and cookie is runtime accepted; default remains 28800 | Idle expiry, cleanup, concurrency and refresh policy are separate gaps | Slice 2R is closed |
| Browser lifecycle outcomes | Gate-owned pre-dispatch evidence plus actual issue/revoke success/failure outcomes are accepted in repository, CI and real runtime; failed issue-outcome persistence compensates and failed revoke-outcome persistence expires the cookie | Other operation families and stronger coupling remain open | Slice 2S is closed; do not generalize it implicitly |
| Safe POST classification | All registered non-mutating stateful POSTs are explicitly classified | Re-audit only when new routes are added | No open route gap now |
| Backend policy | Backend read-only/capability/domain checks remain independent from actor authorization | Preserve this separation for every future migration | Every future operation slice |
| Accountability | Append-only pre-dispatch allow/deny evidence and browser lifecycle outcomes are accepted and secret-free | Other outcomes, stronger coupling/outbox, protected query/export/retention remain open | Select one bounded operation or infrastructure contract only |
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

## Accepted Slice 2R runtime evidence

```text
Head: d65af5a24688fe4dbf090030226fd45825260060
CI: #6662 / run 30717164017 / all five jobs successful
Custom lifetime: 900 seconds
HTTP requests: 5
Installed daemon SHA-256:
12953babb3a2ce3aebeb99a377f66a94375bf55cf1e839cf8163bf574f4d7660
Installed loader SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
Cookie Max-Age: 900
Persisted remaining lifetime: 900
Logout and revocation: passed
Revoked-cookie replay: denied
Original runtime configuration: restored
Database integrity: yes
Service active: yes
```

Evidence directory:

```text
/var/backups/vdr-suite-phase62-slice2r-20260801T202314Z-d65af5a24688/runtime-acceptance-slice2r
```

## Accepted Slice 2S contract and runtime evidence

The accepted implementation adds post-operation accountability only to the
existing browser-session issue/revoke pair.

```text
issue permission/action:  session.issue.self / browser.session.issue
revoke permission/action: session.revoke.self / browser.session.revoke
scope:                    *
event type:               operation.succeeded or operation.failed
outcome:                  succeeded or failed
```

Boundaries:

- the gate retains all authentication, permission and CSRF pre-dispatch events;
- actual service issue/revoke attempts receive exactly one outcome event;
- successful login is delivered only after outcome persistence;
- outcome append failure after issue triggers compensating revocation and no
  session cookie;
- outcome append failure after revoke leaves the session revoked and expires the
  client cookie;
- all fields remain secret-free;
- no route, schema, permission, frontend or packaging change is included.

Accepted real-runtime summary:

```text
Head: c128867bfbf4ce10bcf7dc23d14652e5f5324c83
CI: #6663 / run 30717721595 / all five jobs successful
Service PID after install/acceptance: 69610 / 69610
HTTP requests: 5
Login accountability events: 2
Missing-CSRF accountability events: 1
Logout accountability events: 2
Lifecycle accountability events: 5
Operation-succeeded events: 2
Missing-CSRF operation events: 0
Ordinary browser GET: passed
Missing-CSRF logout: denied before operation
Logout and revocation: passed
Revoked-cookie replay: denied
Accountability: secret-free
Installed daemon SHA-256:
682cfc76738454f57daff0831fe7a01786f57abf42cf16c2fa9c2ac16309a07a
Installed loader SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
Database integrity: yes
Service active: yes
Rollback: not required
```

Evidence directory:

```text
/var/backups/vdr-suite-phase62-slice2s-20260801T210333Z-c128867bfbf4/runtime-acceptance-slice2s
```

Evidence fingerprints:

```text
runtime_report_sha256=9ca22c30db9e22decb8e4f74d0204b82d53bb58c344cebdd95d4bae0893a5421
database_before_sha256=12356c390c4c852bf59b1a9636e27738332ab71f836dcb01ef46984a39dc7e0f
database_after_sha256=2153b347d97ce1148a1efdbc3628c4f9652346e82b27d0baeae50c38172e5378
```

The database snapshots differ because revoked test lifecycle rows and acceptance
accountability events remain as durable evidence.

Explicitly deferred:

- outcome events for other business operations;
- transactional outbox, retry or delivery infrastructure;
- idle timeout and `last_seen`;
- sliding expiry and refresh;
- expired-session cleanup;
- concurrent-session limits;
- protected audit query/export/retention;
- generic security administration.

## Phase 62 dependency order

1. **Identity and authorization foundation — accepted.**
2. **Persistent lifecycle, browser sessions, exact grants and fixed roles —
   accepted for the current catalogue.**
3. **Business and administrative POST migration — accepted through Slice 2Q;
   fresh inventory shows no remaining product POST gap.**
4. **Absolute browser-session lifetime configuration — Slice 2R accepted in
   repository, CI and real yaVDR runtime.**
5. **Browser-session issue/revoke outcome accountability — Slice 2S accepted in
   repository, CI and real yaVDR runtime.**
6. **Idle expiry, cleanup and concurrent-session policy — open and explicitly
   separate from Slice 2S.**
7. **Common revisions, idempotency and durable operation lifecycle — open.**
8. **Broader outcome accountability, coupling/outbox and protected audit reads — open.**
9. **Protected identity, credential, grant and generic-role administration — open.**
10. **Compatibility retirement readiness and final Phase 62 closeout — open.**

## Exact next action

Let the Slice-2S documentation closeout complete all five CI jobs. Then perform
a fresh bounded Phase-62 gap review and select exactly one next slice. Do not
combine future work with business outcomes, outbox infrastructure, idle timeout,
cleanup, concurrency, generic administration, Android or Phase 63-67 without an
explicitly bounded contract.
