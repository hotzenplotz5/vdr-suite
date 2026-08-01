# VDR-Suite Current State

## Navigation

- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Current Project Status](development/current-status.md)
- [Phase 62 Slice 2T Active Contract](development/phase-62-slice-2t-browser-session-issuer-binding.md)
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
Slice 2S - Browser-Session Lifecycle Outcome Accountability

Accepted code/runtime head:
c128867bfbf4ce10bcf7dc23d14652e5f5324c83

Accepted closeout head:
064744f73905b6fcc53d737ab9088554ae2af4b6

Accepted closeout CI:
VDR-Suite CI #6664
Run ID 30718491649
All five jobs successful

Active repository implementation:
Slice 2T - Browser-Session Issuing-Credential Lifecycle Binding
CI and real-runtime acceptance pending

Installed daemon SHA-256:
682cfc76738454f57daff0831fe7a01786f57abf42cf16c2fa9c2ac16309a07a

Installed deferred-runtime-loader.js SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
```

Phase 61 is complete. Phase 62 is active and incomplete. Phase 63-67 runtime
has not been advanced.

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

## Active Slice 2T repository contract

Browser-session issuance already records `issued_from_credential_id` and checks
the source credential at issue time. Slice 2T closes the remaining request-time
lineage gap.

The effective cookie lookup joins the browser-session row to the issuing
credential. Cookie authentication and CSRF verification fail closed when the
issuer is:

- missing;
- owned by a different actor;
- inactive;
- revoked;
- expired.

Issuer expiry maps to the established `credential_expired` path. Missing,
mismatched, inactive or revoked issuers map to `credential_revoked`.

Raw browser-row lookups remain unchanged and no descendant row is mutated. The
existing `PersistentIdentityResolver` continues to own actor, device, canonical
session and browser-credential checks.

Slice 2T adds no route, permission, frontend owner, schema migration,
configuration or packaging change.

## Compatibility and fail-closed boundary

Legacy Basic remains a transitional compatibility path. Managed Basic and
browser actors do not inherit a legacy bypass. Browser mutations not explicitly
classified remain fail-closed with `security_policy_not_migrated`.

A presented browser cookie never falls back to Basic. Slice 2T strengthens only
the effective browser-cookie and CSRF lifecycle resolution.

## Remaining Phase 62 work

- complete all five CI jobs and guarded real-runtime acceptance for Slice 2T;
- define browser-session idle expiry, cleanup and concurrency policy;
- extend outcome accountability beyond the bounded lifecycle pair only through
  separately designed slices;
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

Publish Slice 2T as one bounded fast-forward commit and require all five CI jobs
to pass. Only after full green CI may a guarded yaVDR pass verify that a revoked
disposable issuing credential invalidates its test browser cookie before grants,
CSRF and route dispatch.
