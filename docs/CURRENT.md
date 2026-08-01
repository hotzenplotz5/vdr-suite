# VDR-Suite Current State

## Navigation

- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Current Project Status](development/current-status.md)
- [Phase 62 Slice 2R Active Contract](development/phase-62-slice-2r-browser-session-lifetime-configuration.md)
- [Phase 62 Slice 2Q Closeout](development/phase-62-slice-2q-native-fuzzy-stale-probe-delete-security-migration.md)
- [Phase 62 Slice 2P Closeout](development/phase-62-slice-2p-query-cache-refresh-security-migration.md)
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
Slice 2Q - Global Native Fuzzy Stale-Probe Deletion Security Migration

Accepted code/runtime head:
88ec36076d7e5114df0a3a186cc6fbd52bb2baac

Accepted closeout CI:
VDR-Suite CI #6658
Run ID 30714506053
All five jobs successful

Active repository implementation:
Slice 2R - Configurable Absolute Browser-Session Lifetime
CI and real-runtime acceptance pending

Installed daemon SHA-256:
9f60daaf7d772abe7c6ad55388cb9bb7e8afe8f6679fbf749aa9103143a41d07

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

## Completed post-Slice-2Q POST inventory

The fresh inventory confirms:

- browser-session issue/logout are owned by the dedicated lifecycle gate;
- every central-router POST is a protected mutation or explicit Safe POST;
- no unmigrated product POST family remains after Slice 2Q;
- unknown browser and enforced-mode POST routes continue to fail closed.

## Active Slice 2R repository contract

```text
VDR_SUITE_BROWSER_SESSION_LIFETIME_SECONDS
Default: 28800 seconds
Inclusive range: 300..86400 seconds
Syntax: unsigned decimal digits only
```

One immutable server-side value controls both:

```text
persisted browser-session expires_at
vdr_suite_session cookie Max-Age
```

Absent configuration retains the accepted eight-hour behaviour. Invalid
configuration blocks only new browser-session issuance with HTTP 503,
`browser_session_lifetime_configuration_invalid`, no `Set-Cookie`, and no new
session or credential rows.

The parser uses the existing issuance-service bounds and rejects long input
before integer multiplication can overflow.

Slice 2R does not add idle timeout, `last_seen`, sliding expiry, refresh,
cleanup, concurrent-session limits, request-selected lifetimes or security
administration.

## Latest accepted real-runtime acceptance

The accepted Slice-2Q yaVDR pass used a direct read-only SQLite stale/future
snapshot and a temporary cross-connection `BEFORE DELETE` trigger.

```text
Slice: slice-2q-native-fuzzy-stale-probe-delete
Tests: 32 passed, 0 failed
HTTP requests: 25
Daemon PID after acceptance: 67393
Authorization scope: *
Snapshot source: direct-sqlite
Freshness maximum age: 604800 seconds
Real stale-probe deletions: 0
Snapshot unchanged: yes
Delete guard removed: yes
Target grants restored: yes
Browser session revoked: yes
Revoked-cookie replay denied: yes
Accountability secret-free: yes
SQLite integrity: yes
Service PID unchanged: yes
Service active: yes
```

Durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2q-20260801T191156Z-88ec36076d7e/runtime-acceptance-slice2q
```

## Compatibility and fail-closed boundary

Legacy Basic remains a transitional compatibility path. Managed Basic and
browser actors do not inherit a legacy bypass. Browser mutations not explicitly
classified remain fail-closed with `security_policy_not_migrated`.

Query strings are removed only for exact route matching. Trailing-slash and
unrelated path variants remain fail-closed. Slice 2R changes neither route
classification nor authorization.

## Remaining Phase 62 work

- complete all five CI jobs and guarded real-runtime acceptance for Slice 2R;
- define browser-session idle expiry, cleanup and concurrency policy;
- add completion/outcome accountability and stronger transactional coupling;
- add protected credential, identity, role and grant administration;
- add native/service credential lifecycle;
- add generic roles only after the fixed catalogue is stable;
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

Publish the bounded Slice-2R implementation and require all five CI jobs to
pass. Only after full green CI may a guarded yaVDR pass temporarily set a
non-default lifetime, issue and revoke one browser session, restore the original
environment file and verify the installed runtime.
