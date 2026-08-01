# VDR-Suite Current State

## Navigation

- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Current Project Status](development/current-status.md)
- [Phase 62 Slice 2S Active Contract](development/phase-62-slice-2s-browser-session-outcome-accountability.md)
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

Repository, CI and real-runtime accepted through:
Slice 2R - Configurable Absolute Browser-Session Lifetime

Accepted code/runtime head:
d65af5a24688fe4dbf090030226fd45825260060

Accepted closeout CI:
VDR-Suite CI #6662
Run ID 30717164017
All five jobs successful

Active repository implementation:
Slice 2S - Browser-Session Lifecycle Outcome Accountability
CI and real-runtime acceptance pending

Installed daemon SHA-256:
12953babb3a2ce3aebeb99a377f66a94375bf55cf1e839cf8163bf574f4d7660

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
  -> persistent lifecycle and actor-grant resolution
  -> exact route classification
  -> route-specific backend or global scope extraction
  -> cookie-bound CSRF for migrated browser mutations
  -> exact permission and scope authorization
  -> fixed exact-scope Admin/Read-only evaluation
  -> append-only pre-dispatch accountability
  -> existing router, backend and domain safety policy
```

Backend read-only, capability and domain policy remain independent from actor
authorization. Frontends do not own authorization decisions.

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

## Active Slice 2S repository contract

The dedicated browser lifecycle gate continues to own authentication,
permission and CSRF pre-dispatch accountability.

The HTTP lifecycle service adds bounded post-operation evidence only for:

```text
session issue  -> operation.succeeded / operation.failed
session revoke -> operation.succeeded / operation.failed
```

Canonical issue fields remain:

```text
permission=session.issue.self
action=browser.session.issue
backend=*
```

Canonical revoke fields remain:

```text
permission=session.revoke.self
action=browser.session.revoke
backend=*
```

A successful login is not delivered until its outcome event is persisted. If
the append fails, the newly created session is compensatingly revoked, secrets
are wiped and HTTP 503 is returned without `Set-Cookie`.

A successful logout remains revoked even when its outcome append fails. The
503 response still expires the client cookie. Authentication and CSRF denials
remain gate-owned and do not receive duplicate operation outcomes.

Slice 2S does not add business-mutation outcomes, a transactional outbox, idle
timeout, cleanup, session limits, audit-query APIs or security administration.

## Latest accepted real-runtime acceptance

```text
Slice: Slice 2R configurable absolute browser-session lifetime
Custom lifetime: 900 seconds
HTTP requests: 5
Persisted remaining lifetime: 900 seconds
Cookie Max-Age: 900
Ordinary browser GET: passed
Missing-CSRF logout: denied
Logout and lifecycle revocation: passed
Revoked-cookie replay: denied
Lifecycle accountability: secret-free
Original runtime configuration and environment: restored
Database integrity: yes
Service active: yes
```

## Compatibility and fail-closed boundary

Legacy Basic remains a transitional compatibility path. Managed Basic and
browser actors do not inherit a legacy bypass. Browser mutations not explicitly
classified remain fail-closed with `security_policy_not_migrated`.

Query strings are removed only for exact route matching. Trailing-slash and
unrelated path variants remain fail-closed. Slice 2S changes no route,
permission, frontend owner, schema or packaging contract.

## Remaining Phase 62 work

- complete all five CI jobs and guarded real-runtime acceptance for Slice 2S;
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

Publish Slice 2S as one bounded fast-forward commit and require all five CI jobs
to pass. Only after full green CI may a guarded yaVDR pass verify issue/revoke
outcome pairs, CSRF non-duplication, revocation, replay denial and secret-free
evidence.
