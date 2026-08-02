# VDR-Suite Current State

## Navigation

- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Current Project Status](development/current-status.md)
- [Phase 62 Slice 2V Contract](development/phase-62-slice-2v-browser-session-idle-expiry.md)
- [Phase 62 Slice 2U Closeout](development/phase-62-slice-2u-browser-session-concurrency-limit.md)
- [Phase 62 Slice 2T Closeout](development/phase-62-slice-2t-browser-session-issuer-binding.md)
- [Phase 62 Slice 2S Closeout](development/phase-62-slice-2s-browser-session-outcome-accountability.md)
- [Phase 62 Slice 2R Closeout](development/phase-62-slice-2r-browser-session-lifetime-configuration.md)
- [Phase 62 Slice 2Q Closeout](development/phase-62-slice-2q-native-fuzzy-stale-probe-delete-security-migration.md)
- [Phase 62 Runtime Evidence](development/phase-62-runtime-evidence.md)
- [Phase 62 Gap Matrix](planning/phase-62-security-identity-gap-matrix.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [Security and Identity Architecture](architecture/security-identity-foundation.md)
- [Phase 61 and Performance Closeout](development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](development/post-phase-61-platform-runtime-closeout.md)
- [Completed Phases](development/completed-phases.md)
- [ADR Index](adr/index.md)

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Base: main @ cb77ff66e11dca7db2eafa36525762dcde35102d
Active PR: #117
PR state: open, Draft, unmerged, mergeable

Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Historical umbrella implementation track:
Phase 58 - Frontend and Live Parity

Completed post-phase platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)
Configurable photorealistic VDR Remote (#115)

Next strict runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Repository, source CI and real-runtime accepted through:
Slice 2U - Concurrent Browser-Session Limit

Accepted implementation/runtime head:
16ff04a4ba371aad32fc4a38bf82f9c0529c532d

Accepted Slice-2U source GitHub Actions:
VDR-Suite CI #6690
Run ID 30723297375
All five jobs successful

Slice-2U documentation closeout commit:
4747d725664d4c382d17d3b19fa2776f48ba437b

Final shared closeout and workflow head:
d00fc5045a136d87323fbc13fb1bfc1030f7d3b5

Final closeout GitHub Actions:
VDR-Suite CI #6693
Run ID 30733265772
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30733265772

Active repository implementation:
Slice 2V - Browser-Session Idle Expiry and throttled last_seen

Installed/running daemon SHA-256:
0e3ec0d57f4471804824247f712c2457015cc22ac9576df60d8d77ed8ddb3134

Installed deferred-runtime-loader.js SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
```

Phase 61 is complete. Phase 62 is active and incomplete. Phase 63-67
runtime has not been advanced.

## Accepted security request path

```text
HTTP request
  -> browser cookie has strict precedence when present
  -> otherwise Legacy Basic or optional Managed Basic
  -> browser credential and canonical persistent lifecycle resolution
  -> exact route classification
  -> route-specific backend or global scope extraction
  -> cookie-bound CSRF for migrated browser mutations
  -> exact permission and scope authorization
  -> fixed exact-scope Admin/Read-only evaluation
  -> append-only pre-dispatch accountability
  -> browser lifecycle post-operation accountability for issue/revoke
  -> existing router, backend and domain safety policy
```

Backend read-only, capability and domain policy remain independent from actor
authorization. Frontends do not own authorization decisions.

## Completed post-Slice-2Q POST inventory

The fresh inventory confirms:

- browser-session issue/logout are owned by the dedicated lifecycle gate;
- every central-router POST is a protected mutation or explicit Safe POST;
- no unmigrated product POST family remains after Slice 2Q;
- unknown browser and enforced-mode POST routes continue to fail closed.

## Accepted Slice 2R contract

```text
VDR_SUITE_BROWSER_SESSION_LIFETIME_SECONDS
Default: 28800 seconds
Inclusive range: 300..86400 seconds
Syntax: unsigned decimal digits only
```

One immutable server-side value controls persisted browser-session expiry and
cookie `Max-Age`. The guarded yaVDR pass accepted a temporary value of `900`,
revoked the test session, restored the original environment and left the service
active.

Durable accepted evidence:

```text
/var/backups/vdr-suite-phase62-slice2r-20260801T202314Z-d65af5a24688/runtime-acceptance-slice2r
```

## Accepted Slice 2S contract

The dedicated browser lifecycle gate continues to own authentication,
permission and CSRF pre-dispatch accountability. The HTTP lifecycle service adds
bounded `operation.succeeded` and `operation.failed` evidence only for browser
session issue and revoke.

A successful login is not delivered until its outcome event is persisted. If
the append fails, the new session is compensatingly revoked and no session
cookie is delivered. A successful logout remains revoked even if its outcome
append fails; the response still expires the client cookie.

Durable accepted evidence:

```text
/var/backups/vdr-suite-phase62-slice2s-20260801T210333Z-c128867bfbf4/runtime-acceptance-slice2s
```

Evidence fingerprints:

```text
runtime_report_sha256=9ca22c30db9e22decb8e4f74d0204b82d53bb58c344cebdd95d4bae0893a5421
database_before_sha256=12356c390c4c852bf59b1a9636e27738332ab71f836dcb01ef46984a39dc7e0f
database_after_sha256=2153b347d97ce1148a1efdbc3628c4f9652346e82b27d0baeae50c38172e5378
```

## Accepted Slice 2T contract and runtime evidence

Browser-session issuance records `issued_from_credential_id`. Slice 2T
revalidates that issuing credential during every ordinary browser-cookie
authentication and CSRF verification.

The issuing credential must:

- exist;
- belong to the same actor as the browser row;
- remain active;
- remain unrevoked;
- remain unexpired.

Issuer expiry maps to `credential_expired`. A missing, actor-mismatched,
inactive or revoked issuer maps to `credential_revoked`.

The guarded real-yaVDR pass redirected only one isolated test browser row
to a disposable same-actor revoked issuer. Ordinary GET and logout were
immediately denied while the raw browser row, canonical session and browser
credential remained active until controlled cleanup. This proves that the
request-time binding is effective without a cascading persistence mutation.

The original compatibility issuer remained active and unchanged. The test
browser lifecycle was subsequently fully revoked and replay was denied.

```text
Runtime head:
55876356e84b3e47e52911529b3f9bfa0e17f191

Source CI:
#6666 / run 30719552024 / all five jobs successful

Closeout head:
e79e0eb67da75044c4a9afa162c9dab188b026fd

Closeout CI:
#6667 / run 30721936576 / all five jobs successful

Installed/running daemon SHA-256:
34b80de4fd8f55b763c4483f0dcb50ee09e5cdc49de7f6e7c25e01ba50d84269

Loader SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Runtime report SHA-256:
2ca7fcaefe21c1198e5d8ff88b3e17237b2e72a545780cc14f0200e7dd0ca983
```

Durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2t-20260801T223353Z-55876356e84b/runtime-acceptance-slice2t
```

## Accepted Slice 2U contract and runtime evidence

Slice 2U adds an optional per-actor upper bound for effective active browser
sessions:

```text
VDR_SUITE_BROWSER_SESSION_MAX_ACTIVE_PER_ACTOR
Default: 0 (unlimited compatibility behaviour)
Inclusive configured range: 0..64
Syntax: unsigned decimal digits only
```

Only effective sessions count. The repository joins the browser verifier to the
canonical actor, device, session, browser credential and issuing credential and
requires every lifecycle component to remain active, unrevoked and unexpired.
A raw active browser row whose issuer is revoked therefore does not consume a
slot.

The effective count and inserts remain inside the existing serialized
`BEGIN IMMEDIATE` issuance transaction. Reaching the configured limit creates
no row, revokes no existing session and maps to:

```text
HTTP 409
browser_session_limit_reached
```

Invalid limit configuration fails closed with HTTP 503. Idle timeout,
`last_seen`, refresh, cleanup, retention, automatic eviction and session
administration remain explicitly deferred.

The guarded real-yaVDR acceptance used an isolated Managed-Basic actor with a
temporary limit of `1`. It proved deny-new semantics without evicting the first
session, slot release after logout, successful replacement issuance and revoked
cookie replay denial.

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

Durable secret-free evidence:

```text
/var/backups/vdr-suite-phase62-slice2u-20260802T041910Z-16ff04a4ba37/runtime-acceptance-slice2u
```

Slice 2U is fully closed. Its closeout commit
`4747d725664d4c382d17d3b19fa2776f48ba437b` and final shared head
`d00fc5045a136d87323fbc13fb1bfc1030f7d3b5` are covered by VDR-Suite CI
#6693, run `30733265772`, with all five jobs successful.

## Compatibility and fail-closed boundary

Legacy Basic remains a transitional compatibility path. Managed Basic and
browser actors do not inherit a legacy bypass. Browser mutations not explicitly
classified remain fail-closed with `security_policy_not_migrated`.

A presented browser cookie never falls back to Basic. Slice 2T strengthens the
effective browser-cookie and CSRF lifecycle resolution. Slice 2U affects only
new browser-session issuance; it does not mutate existing sessions.

## Fresh post-2U gap analysis

No product POST route remains to migrate. The remaining Phase 62 work is:

- browser-session idle expiry and throttled `last_seen` persistence;
- cleanup and retention as a separate later slice;
- further operation outcomes;
- stronger transaction coupling or Outbox;
- shared revisions, idempotency and durable operation lifecycle;
- protected actor, identity, credential, grant and role administration;
- native/service credential lifecycle;
- protected audit reads, export and retention;
- compatibility retirement and final Phase 62 closeout.

The browser-session table has absolute `expires_at`, `created_at` and
`updated_at`, but no activity timestamp. `updated_at` is not a valid idle clock
because lifecycle writes such as revocation also modify it. The bounded next
slice therefore adds one explicit browser-session `last_seen_at` field rather
than introducing a generic timestamp model.

## Operating rules

- Root-level `AGENTS.md` is binding for agent-driven repository work.
- Prefer GitHub-first edits, commits and pushes when the connector can perform
  the complete operation safely.
- Continue through already-approved work without artificial pauses.
- Push coherent commits consecutively; do not wait for CI after every commit.
- Evaluate required CI on the final stabilization head before runtime,
  Ready-for-review or merge gates.
- PR #117 remains open, Draft and unmerged.
- Do not mark it Ready, merge, auto-merge, force-push or rewrite branch history.
- Recheck volatile GitHub and local state immediately before mutation.
- Do not repeat completed runtime acceptance solely because the chat changed.
- Select exactly one bounded Phase 62 slice at a time.
- Do not pull Android or Phase 63-67 runtime into Phase 62 work.

## Exact next action

Implement only Phase 62 Slice 2V — Browser-Session Idle Expiry and throttled
`last_seen`. Run focused source validation and then evaluate all five GitHub
Actions jobs on the final stabilization head. A new guarded real-yaVDR runtime
acceptance is required only because the daemon and browser-session schema will
change.

Do not combine Slice 2V with cleanup, retention, automatic eviction, session
listing or administration, general security administration, Outbox, Android or
Phase 63-67 work.
