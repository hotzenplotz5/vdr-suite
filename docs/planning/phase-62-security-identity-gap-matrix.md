# Phase 62 Security and Identity Gap Matrix

Status: active Phase 62 planning and implementation matrix

```text
Repository baseline:
cb77ff66e11dca7db2eafa36525762dcde35102d (main, merge of PR #115)

Accepted runtime slices:
Slice 1 through Slice 2V

Accepted Slice-2V implementation/runtime head:
e84415fadb2587ff744ff8927f1f0113920ece2f

Accepted Slice-2V source CI:
VDR-Suite CI #6779
Run ID 30741293079
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30741293079

Documentation-only Slice-2V closeout:
Canonical closeout updates in progress; final closeout CI pending

Active repository implementation:
None selected after Slice 2V runtime acceptance

PR #117:
open, Draft, unmerged
```

A component is not accepted installed runtime until it is connected, covered by
the complete CI graph and validated on the real yaVDR system. Code-head evidence
alone is insufficient.

## Gap matrix

| Security area | Current accepted state | Remaining gap | Next bounded work |
|---|---|---|---|
| Actor/device model | Canonical persistent actor, device, session and credential context | Protected enrollment and administration | Later lifecycle-administration slice |
| Authentication | Legacy Basic, optional Managed Basic and browser sessions authenticate ordinary routes; browser cookie has strict precedence; issuing-credential request-time binding, absolute expiry and idle expiry are runtime accepted | Native/service mechanisms and compatibility retirement remain open | Preserve during the next bounded selection |
| Browser sessions | Atomic issue/logout, independent cookie and CSRF secrets, persistence, configurable absolute expiry, replay denial, lifecycle outcomes, issuer-lineage enforcement, optional effective per-actor concurrency limit and optional idle expiry are runtime accepted | Physical cleanup, retention and administration remain separate | Fresh post-2V analysis after closeout CI |
| Browser-session idle expiry | Additive `last_seen_at`, strict optional `0` or `300..86400` idle policy, shared cookie/CSRF effectiveness, 60-second activity-write throttle and idle-aware concurrency count are source- and runtime-accepted | Physical cleanup and retention remain separate | Slice 2V is closed after documentation CI |
| Concurrent browser sessions | Strict optional `0..64` per-actor effective-session limit with atomic deny-new semantics is fully source-, runtime- and closeout-accepted | Cleanup and administration remain separate; idle-aware counting is accepted through Slice 2V | Slice 2U is closed |
| Issuing credential lineage | Issuance records `issued_from_credential_id`; later cookie and CSRF requests fail when the issuer is missing, mismatched, inactive, revoked or expired | No cascading descendant cleanup or issuer-management surface | Slice 2T is closed |
| Grants and scopes | Active exact actor grants load from persistence; unavailable store fails closed; concrete and global exact scopes are runtime accepted | Protected grant administration and broader resource scopes | Later bounded security-management design |
| Fixed roles | Exact-scope Admin and Read-only semantics are accepted for concrete backends and global `*`; no inherited wildcard semantics | Generic persisted roles remain open | Defer until protected administration is designed |
| CSRF | Enforced for all accepted browser mutation families; frontend tokens remain memory-only and owner-injected; issuer and idle lifecycle binding are runtime accepted | Future frontend owners still require explicit contracts | Preserve explicit ownership |
| Central authorization | Accepted through all registered business and administrative POST families | No unmigrated product POST remains | Do not invent another route-migration slice |
| Query-scoped cache refresh | SearchTimer preview and EPG cache refresh use distinct permissions and query-derived backend scope | Completion/outcome evidence only | No further route work |
| Global stale-probe administration | Delete aliases use `epgsearch.native-fuzzy.stale-probes.delete@*`; zero-delete runtime accepted | No protected read/list API or frontend owner | Any future UI requires a separate slice |
| Browser-session lifetime | Strict configurable absolute `300..86400` alignment for persistence and cookie is runtime accepted | Cleanup and any future refresh policy remain separate; idle activity never extends absolute expiry | Preserve Slice-2R hard upper bound |
| Browser lifecycle outcomes | Gate-owned pre-dispatch evidence plus issue/revoke outcomes are accepted; failed issue-outcome persistence compensates and failed revoke-outcome persistence expires the cookie | Other operation families and stronger coupling remain open | Later separate outcome/coupling slice |
| Safe POST classification | All registered non-mutating stateful POSTs are explicitly classified | Re-audit only when new routes are added | No open route gap |
| Backend policy | Backend read-only/capability/domain checks remain independent from actor authorization | Preserve this separation for every future change | Every future operation slice |
| Accountability | Pre-dispatch evidence, browser lifecycle outcomes, concurrency-limit policy outcomes and idle-expiry denial evidence are accepted and secret-free | Other outcomes, stronger coupling/outbox, protected query/export/retention remain open | Separate future design |
| Revisions/idempotency | Domain-specific partial mechanisms only | Common preconditions, idempotency and durable operation lifecycle | Later Phase 62 slice |
| Administration | No general security-management API | Protected identity, credential, grant and role operations | Separate design and implementation slices |
| Native/service clients | Core model is transport-neutral | Enrollment, rotation, refresh and revocation contracts | Later Phase 62 slice |
| Audit reads and retention | Append-only writes exist for the accepted scope | Protected reads, export, redaction and retention | Later bounded audit-product slice |
| Compatibility retirement | Legacy compatibility remains explicitly transitional | Retirement criteria, migration tooling and final enforcement readiness | Near final Phase 62 closeout |

## Completed POST inventory

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

Slice 2T request-time resolution of `issued_from_credential_id` is accepted in
source CI and on the real yaVDR runtime.

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

## Fully closed Slice 2U evidence

Slice 2U introduced:

```text
VDR_SUITE_BROWSER_SESSION_MAX_ACTIVE_PER_ACTOR
0     unlimited compatibility default
1..64 maximum effective active browser sessions per actor
```

The effective count joins the browser row to the canonical actor, device,
session, browser credential and issuing credential. Every component must remain
active, unrevoked and unexpired. Count and insert remain inside the serialized
`BEGIN IMMEDIATE` transaction.

At the configured bound, issuance returns `LimitReached`, creates nothing and
maps to HTTP 409 `browser_session_limit_reached`. It never evicts, revokes or
rewrites an existing session.

```text
Implementation/runtime head:
16ff04a4ba371aad32fc4a38bf82f9c0529c532d

Source CI:
#6690 / run 30723297375 / all five jobs successful

Closeout commit:
4747d725664d4c382d17d3b19fa2776f48ba437b

Final shared closeout head:
d00fc5045a136d87323fbc13fb1bfc1030f7d3b5

Final closeout CI:
#6693 / run 30733265772 / all five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30733265772

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

## Runtime-accepted Slice 2V evidence

Slice 2V introduced:

```text
VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS
0          disabled compatibility default
300..86400 enabled idle timeout

security_browser_session_credentials.last_seen_at
60-second minimum activity-write interval
```

The additive migration backfills existing rows from `created_at`. Cookie and
CSRF verification share one repository-owned idle calculation. Absolute
`expires_at` remains unchanged. Idle-expired rows do not consume a concurrency
slot and are not physically deleted, revoked or evicted.

```text
PHASE_62_SLICE_2V_RUNTIME_ACCEPTANCE=PASS

Implementation/runtime head:
e84415fadb2587ff744ff8927f1f0113920ece2f

Source CI:
#6779 / run 30741293079 / all five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30741293079

Installed/running daemon SHA-256:
e0b6f6de08527b6af49d526ca0118b14b6fb85ff3335fc607ca1b531cdee5f60

Loader SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Restored configuration SHA-256:
8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b

Final service PID:
86549

Runtime report SHA-256:
0a961fbc8b51158fd4a16aa24fc9afde7dafa9d5272e986a46ec73880c311f86

Configured idle timeout:
300 seconds

Activity-write interval:
60 seconds

Ordinary GET before idle expiry:
HTTP 200

Ordinary GET after idle expiry:
HTTP 401 session_expired

Protected mutation after idle expiry:
HTTP 401 session_expired

last_seen writes inside accepted interval:
1

Absolute expiry unchanged:
yes

Replacement logout:
HTTP 204

Revoked replacement-cookie replay:
HTTP 401 credential_revoked

Acceptance lifecycle active rows after cleanup:
0

SQLite quick check:
ok

SQLite foreign-key check:
empty

Accountability secret-free:
yes

VDR domain mutations:
0

Final service state:
active

Runtime drop-in:
removed

Idle test environment:
not set
```

Evidence directory:

```text
/var/backups/vdr-suite-phase62-slice2v-20260802T092139Z-e84415fadb25
```

The accepted Slice-2V runtime deliberately contains no cleanup, retention,
automatic eviction, session-administration API, sliding absolute expiry, generic
security administration, Android or Phase 63-67 change.

## Phase 62 dependency order

1. **Identity and authorization foundation — accepted.**
2. **Persistent lifecycle, browser sessions, exact grants and fixed roles —
   accepted for the current catalogue.**
3. **Business and administrative POST migration — accepted through Slice 2Q;
   no remaining product POST gap.**
4. **Absolute browser-session lifetime configuration — Slice 2R accepted.**
5. **Browser-session issue/revoke outcome accountability — Slice 2S accepted.**
6. **Browser-session issuing-credential lifecycle binding — Slice 2T accepted.**
7. **Concurrent effective browser-session limit — Slice 2U fully closed.**
8. **Browser-session idle expiry and throttled `last_seen` — Slice 2V runtime
   accepted; documentation closeout CI pending.**
9. **Fresh post-2V gap analysis — required after closeout CI.**
10. **Physical cleanup and retention — open as a separate candidate.**
11. **Common revisions, idempotency and durable operation lifecycle — open.**
12. **Broader outcomes, coupling/outbox and protected audit reads — open.**
13. **Protected identity, credential, grant and generic-role administration —
    open.**
14. **Native/service credential lifecycle — open.**
15. **Compatibility retirement readiness and final Phase 62 closeout — open.**

## Exact next action

Require all five GitHub Actions jobs for the documentation-only Slice-2V
closeout.

No next implementation slice is selected by this runtime acceptance. Perform a
fresh post-2V gap analysis only after the closeout CI is fully green, then select
exactly one bounded slice.

Do not begin cleanup, retention, automatic eviction, session administration,
general security administration, Outbox, Android or Phase 63-67 work before
Slice 2V is fully closed.
