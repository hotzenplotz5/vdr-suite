# VDR-Suite Current State

## Navigation

- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Current Project Status](development/current-status.md)
- [Phase 62 Slice 2R Closeout](development/phase-62-slice-2r-browser-session-lifetime-configuration.md)
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
Slice 2R - Configurable Absolute Browser-Session Lifetime

Accepted code/runtime head:
d65af5a24688fe4dbf090030226fd45825260060

Accepted source/runtime CI:
VDR-Suite CI #6661
Run ID 30715365583
All five jobs successful

Active repository implementation:
None selected after Slice 2R closeout

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

The guarded yaVDR pass temporarily configured `900` seconds and verified the
same canonical value in the response cookie and all persisted expiry rows.

```text
service_pid_custom_lifetime=68813
service_pid_after_restore=68893
runtime_http_requests=5
persisted_remaining_seconds=900
cookie_max_age=900
cookie_http_only=yes
cookie_secure=yes
cookie_same_site=strict
ordinary_browser_get=yes
missing_csrf_denied=yes
logout_succeeded=yes
session_revoked=yes
credential_revoked=yes
revoked_cookie_replay_denied=yes
accountability_issue_allowed=yes
accountability_csrf_denied=yes
accountability_logout_allowed=yes
accountability_secret_free=yes
original_runtime_config_restored=yes
original_runtime_environment_restored=yes
database_integrity=yes
service_state=active
```

Durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2r-20260801T202314Z-d65af5a24688/runtime-acceptance-slice2r
```

The earlier `20260801T201619Z` attempt failed only in an outer accountability
field assertion and automatically restored the pre-test runtime. It is rollback
evidence, not accepted Slice-2R evidence.

## Compatibility and fail-closed boundary

Legacy Basic remains a transitional compatibility path. Managed Basic and
browser actors do not inherit a legacy bypass. Browser mutations not explicitly
classified remain fail-closed with `security_policy_not_migrated`.

Query strings are removed only for exact route matching. Trailing-slash and
unrelated path variants remain fail-closed. Slice 2R changes neither route
classification nor authorization.

## Remaining Phase 62 work

- define browser-session idle expiry, cleanup and concurrency policy;
- add completion/outcome accountability and stronger transactional coupling;
- add protected credential, identity, role and grant administration;
- add native/service credential lifecycle;
- add generic roles only after the fixed catalogue is stable;
- standardize revisions, idempotency and operation lifecycle;
- add protected audit query/export/retention;
- complete compatibility-retirement and final Phase 62 acceptance.

No next implementation slice is selected by this closeout.

## Operating rules

- PR #117 remains open, Draft and unmerged.
- Do not mark it Ready, merge, auto-merge, force-push or rewrite branch history.
- Recheck volatile GitHub and local state immediately before mutation.
- Do not repeat completed runtime acceptance solely because the chat changed.
- Select exactly one bounded Phase 62 slice at a time.
- Do not pull Android or Phase 63-67 runtime into Phase 62 work.

## Exact next action

Let the Slice-2R documentation closeout complete its full five-job CI. Then
perform a fresh bounded Phase-62 gap review and select exactly one next slice
only after its security, persistence and real-runtime-safety contract is
explicit.
