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
Slice 2Q - Global Native Fuzzy Stale-Probe Deletion Security Migration

Accepted code/runtime head:
88ec36076d7e5114df0a3a186cc6fbd52bb2baac

Accepted closeout GitHub Actions:
VDR-Suite CI #6658
Run ID: 30714506053
All five jobs successful

Active repository implementation:
Slice 2R - Configurable Absolute Browser-Session Lifetime
CI and real-runtime acceptance pending

Installed daemon SHA-256:
9f60daaf7d772abe7c6ad55388cb9bb7e8afe8f6679fbf749aa9103143a41d07

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
- append-only pre-dispatch accountability and secret-free denial evidence;
- mutation-safe real-runtime acceptance profiles and guarded rollback.

## Completed post-Slice-2Q POST inventory

The fresh HTTP inventory found no remaining unmigrated product POST family:

- browser-session issue/logout are handled by the dedicated lifecycle gate;
- every POST registered by the central API router is either a protected mutation
  or an explicitly classified Safe POST;
- unknown browser and enforced-mode POST paths remain fail-closed.

A further route-migration slice would therefore be artificial.

## Active Slice 2R repository contract

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

This repository implementation is not accepted runtime until all five CI jobs
and the guarded custom-lifetime yaVDR pass succeed.

## Latest accepted Slice 2Q evidence

```text
service_pid_after_acceptance=67393
tests_passed=32/32
runtime_http_requests=25
snapshot_source=direct-sqlite
stale_probe_snapshot_unchanged=yes
real_stale_probe_deletes=0
delete_guard_removed=yes
target_grants_restored=yes
browser_session_revoked=yes
revoked_cookie_replay_denied=yes
accountability_secret_free=yes
database_integrity=yes
service_pid_unchanged=yes
service_state=active
```

Durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2q-20260801T191156Z-88ec36076d7e/runtime-acceptance-slice2q
```

## Remaining Phase 62 work

Phase 62 still lacks:

- Slice 2R CI and guarded real-runtime acceptance;
- browser-session idle expiry, cleanup and concurrency policy;
- completion/outcome accountability and stronger transactional coupling;
- protected identity, credential, role and grant administration;
- native/service credential enrollment, rotation and revocation;
- generic persisted role definitions beyond the fixed catalogue;
- common revision, idempotency and durable operation contracts;
- protected audit query/export/retention;
- compatibility-retirement readiness and final Phase 62 closeout.

## Pull request truth

PR #117 must remain open, Draft and unmerged. Do not mark it Ready for review,
merge it, enable auto-merge, force-push, rewrite branch history or change PR
metadata without explicit approval.

The PR description is materially stale. Current repository truth is this file,
[Current State](../CURRENT.md), the
[Slice 2Q closeout](phase-62-slice-2q-native-fuzzy-stale-probe-delete-security-migration.md),
the active
[Slice 2R contract](phase-62-slice-2r-browser-session-lifetime-configuration.md),
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

Publish Slice 2R as one bounded fast-forward commit and require all five CI jobs
to pass. Only then run a guarded yaVDR acceptance with a temporary non-default
lifetime, revoke the test session and restore the original environment file.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Slice 2R Active Contract](phase-62-slice-2r-browser-session-lifetime-configuration.md)
- [Slice 2Q Closeout](phase-62-slice-2q-native-fuzzy-stale-probe-delete-security-migration.md)
- [Slice 2P Closeout](phase-62-slice-2p-query-cache-refresh-security-migration.md)
- [Phase 62 Runtime Evidence](phase-62-runtime-evidence.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase 61 and Performance Closeout](phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](post-phase-61-platform-runtime-closeout.md)
