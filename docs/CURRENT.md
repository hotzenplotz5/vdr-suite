# VDR-Suite Current State

## Navigation

- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Current Project Status](development/current-status.md)
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
Slice 2T - Browser-Session Issuing-Credential Lifecycle Binding

Accepted implementation/runtime head:
55876356e84b3e47e52911529b3f9bfa0e17f191

Accepted source GitHub Actions:
VDR-Suite CI #6666
Run ID 30719552024
All five jobs successful

Documentation-only Slice-2T closeout:
This commit; closeout CI pending

Active repository implementation:
None selected after Slice 2T runtime acceptance

Installed daemon SHA-256:
34b80de4fd8f55b763c4483f0dcb50ee09e5cdc49de7f6e7c25e01ba50d84269

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

## Compatibility and fail-closed boundary

Legacy Basic remains a transitional compatibility path. Managed Basic and
browser actors do not inherit a legacy bypass. Browser mutations not explicitly
classified remain fail-closed with `security_policy_not_migrated`.

A presented browser cookie never falls back to Basic. Slice 2T strengthens only
the effective browser-cookie and CSRF lifecycle resolution.

## Remaining Phase 62 work

- complete all five CI jobs for this documentation-only Slice-2T closeout;
- define browser-session idle expiry, cleanup and concurrency policy;
- extend outcome accountability beyond the bounded lifecycle pair only
  through separately designed slices;
- define stronger transactional coupling or outbox semantics separately;
- add protected credential, identity, role and grant administration;
- add native/service credential lifecycle;
- standardize revisions, idempotency and operation lifecycle;
- add protected audit query/export/retention;
- complete compatibility-retirement and final Phase 62 acceptance.

## Operating rules

- PR #117 remains open, Draft and unmerged.
- Do not mark it Ready, merge, auto-merge, force-push or rewrite branch history.
- Recheck volatile GitHub and local state immediately before mutation.
- Do not repeat completed runtime acceptance solely because the chat changed.
- Select exactly one bounded Phase 62 slice at a time.
- Do not pull Android or Phase 63-67 runtime into Phase 62 work.

## Exact next action

Require all five CI jobs for this documentation-only Slice-2T closeout.

No next Phase-62 implementation slice is selected by this closeout. After
full closeout CI, perform a fresh post-2T gap analysis and select exactly
one bounded Phase-62 slice. Do not combine that selection with the
closeout.
