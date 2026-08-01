# VDR-Suite Current Project Status

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Base: origin/main @ cb77ff66e11dca7db2eafa36525762dcde35102d
Active pull request: #117
PR state: open, Draft, unmerged, mergeable
Remote branch: phase-62-security-identity-foundation
Local yaVDR branch: phase62-pr117

Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed operational hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Current active runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Repository, CI and real-runtime accepted through:
Slice 2R - Configurable Absolute Browser-Session Lifetime

Accepted code/runtime head:
d65af5a24688fe4dbf090030226fd45825260060

Accepted source/runtime GitHub Actions:
VDR-Suite CI #6661
Run ID: 30715365583
All five jobs successful

Active repository implementation:
None selected after Slice 2R closeout

Installed daemon SHA-256:
12953babb3a2ce3aebeb99a377f66a94375bf55cf1e839cf8163bf574f4d7660

Installed deferred-runtime-loader.js SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
```

Phase 61 remains completed. Phase 62 remains active and incomplete. Phase
63-67 runtime has not been advanced.

## Cumulative accepted Phase 62 scope

The accepted branch and installed runtime include:

- canonical actor, device, session, credential, request and correlation context;
- persistent identity, lifecycle, managed Basic and browser-session verifiers;
- atomic browser-session issue/logout with independent cookie and CSRF secrets;
- ordinary-route browser authentication with strict cookie precedence;
- persisted exact actor grants and fail-closed unavailable-store handling;
- fixed exact-scope `role.admin` and `role.read-only` semantics;
- memory-only Webfrontend CSRF state and exact request-owner injection;
- protected Remote, Timer, Channel Move, Recording execution and SearchTimer
  create/maintenance/execution mutations;
- explicit Safe POST classification for the accepted validation/preview family;
- protected Native Fuzzy operator refresh;
- protected query-scoped SearchTimer preview and EPG cache refresh;
- protected global Native Fuzzy stale-probe deletion;
- configurable bounded absolute browser-session lifetime shared by persistence
  and cookie construction;
- append-only pre-dispatch accountability and secret-free denial evidence;
- mutation-safe real-runtime acceptance profiles and guarded rollback.

## Completed post-Slice-2Q POST inventory

The fresh HTTP inventory found no remaining unmigrated product POST family:

- browser-session issue/logout are handled by the dedicated lifecycle gate;
- every POST registered by the central API router is either a protected mutation
  or an explicitly classified Safe POST;
- unknown browser and enforced-mode POST paths remain fail-closed.

A further route-migration slice would therefore be artificial.

## Accepted Slice 2R contract

Slice 2R adds one optional server-side setting:

```text
VDR_SUITE_BROWSER_SESSION_LIFETIME_SECONDS
```

Contract:

```text
default=28800
minimum=300
maximum=86400
format=strict unsigned decimal
```

The same immutable value controls the persisted absolute browser-session expiry
and cookie `Max-Age`. Missing configuration preserves the existing eight-hour
behaviour.

Invalid configuration does not fall back silently. Browser-session issuance
returns HTTP 503 with
`browser_session_lifetime_configuration_invalid`, emits no `Set-Cookie`, and
creates no session or credential. Other API routes and already-issued sessions
remain outside this bounded change.

The parser reuses the issuance service's existing minimum/default/maximum
constants and rejects long decimal input before multiplication can overflow.

Explicitly excluded from Slice 2R:

- idle timeout and `last_seen` persistence;
- sliding expiry or refresh;
- expired-session cleanup;
- concurrent-session limits;
- user-selectable request values;
- generic security administration.

## Latest accepted Slice 2R evidence

```text
service_pid_custom_lifetime=68813
service_pid_after_restore=68893
custom_lifetime_seconds=900
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

Evidence fingerprints:

```text
runtime_report_sha256=5fc0540f68d377c2dbce8351758fdf187527c3cb8e8538820041b224e3d9b478
database_before_sha256=35e84aa1e0b181dd425262ceeea6a65b297bfe68fd5ffe717a63d39a911de861
database_after_sha256=f6d5a57271658bca45aa0a9b30a39ee904dfa12f31c26d651206216ecdbab52f
```

The database snapshots differ because the acceptance lifecycle rows remain as
revoked evidence. All relevant browser-session, session and credential rows are
inactive with revocation timestamps, and revoked-cookie replay is denied.

The earlier `20260801T201619Z` attempt failed only in a wrapper-side comparison
between the accountability action and permission columns. Automatic rollback
passed and restored the prior runtime. That directory is rollback evidence, not
the accepted Slice-2R pass.

## Remaining Phase 62 work

Phase 62 still lacks:

- browser-session idle expiry, cleanup and concurrency policy;
- completion/outcome accountability and stronger transactional coupling;
- protected identity, credential, role and grant administration;
- native/service credential enrollment, rotation and revocation;
- generic persisted role definitions beyond the fixed catalogue;
- common revision, idempotency and durable operation contracts;
- protected audit query/export/retention;
- compatibility-retirement readiness and final Phase 62 closeout.

No next implementation slice is selected by this closeout.

## Pull request truth

PR #117 must remain open, Draft and unmerged. Do not mark it Ready for review,
merge it, enable auto-merge, force-push, rewrite branch history or change PR
metadata without explicit approval.

The PR description is materially stale. Current repository truth is this file,
[Current State](../CURRENT.md), the
[Slice 2R closeout](phase-62-slice-2r-browser-session-lifetime-configuration.md),
the [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
and the
[Security and Identity Architecture](../architecture/security-identity-foundation.md).

### Preferred edit path for new chats

Prefer direct GitHub repository updates for existing files when the connector
can perform the requested edit safely and the complete current file content is
available.

Use local edits first only when the change requires:

- compilation, generated artifacts or focused local runtime tests;
- coordinated tooling that is not available through the connector;
- a workaround because the GitHub connector blocks a file operation.

Never replace a complete file from a truncated fetch. Recheck the branch head
before every write, keep updates fast-forward-only and inspect the resulting
diff before treating a GitHub change as complete.

## Exact next action

Let the Slice-2R documentation closeout complete all five CI jobs. Then perform
a fresh bounded Phase-62 gap review and select exactly one next slice only after
its security, persistence and real-runtime-safety boundary is explicit.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Slice 2R Closeout](phase-62-slice-2r-browser-session-lifetime-configuration.md)
- [Slice 2Q Closeout](phase-62-slice-2q-native-fuzzy-stale-probe-delete-security-migration.md)
- [Slice 2P Closeout](phase-62-slice-2p-query-cache-refresh-security-migration.md)
- [Phase 62 Runtime Evidence](phase-62-runtime-evidence.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase 61 and Performance Closeout](phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](post-phase-61-platform-runtime-closeout.md)
