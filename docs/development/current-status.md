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

Repository, source CI and real-runtime accepted through:
Slice 2T - Browser-Session Issuing-Credential Lifecycle Binding

Accepted implementation/runtime head:
55876356e84b3e47e52911529b3f9bfa0e17f191

Accepted Slice-2T source GitHub Actions:
VDR-Suite CI #6666
Run ID: 30719552024
All five jobs successful

Accepted Slice-2T closeout head:
e79e0eb67da75044c4a9afa162c9dab188b026fd

Accepted Slice-2T closeout GitHub Actions:
VDR-Suite CI #6667
Run ID: 30721936576
All five jobs successful

Active repository implementation:
Slice 2U - Concurrent Browser-Session Limit
Source stabilization and final-head CI pending

Installed daemon SHA-256:
34b80de4fd8f55b763c4483f0dcb50ee09e5cdc49de7f6e7c25e01ba50d84269

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
- explicit Safe POST classification for accepted validation/preview routes;
- protected Native Fuzzy refresh and global stale-probe deletion;
- protected query-scoped SearchTimer preview and EPG cache refresh;
- configurable bounded absolute browser-session lifetime;
- append-only pre-dispatch accountability;
- browser-session issue/revoke outcome accountability with fail-closed
  compensation;
- issuing-credential request-time lifecycle binding;
- mutation-safe real-runtime acceptance profiles and guarded rollback.

## Completed post-Slice-2Q POST inventory

Every central-router POST family is now either a protected mutation or an
explicitly classified Safe POST. Browser-session issue/logout remain owned by
the dedicated lifecycle gate. Unknown browser and enforced-mode POST paths fail
closed. A further route-migration slice would be artificial.

## Accepted Slice 2S evidence

```text
service_pid_after_install=69610
service_pid_after_acceptance=69610
runtime_http_requests=5
login_accountability_events=2
missing_csrf_accountability_events=1
logout_accountability_events=2
lifecycle_accountability_events=5
operation_succeeded_events=2
missing_csrf_operation_events=0
session_revoked=yes
credential_revoked=yes
revoked_cookie_replay_denied=yes
accountability_secret_free=yes
database_integrity=yes
service_state=active
automatic_rollback=not-required
```

Durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2s-20260801T210333Z-c128867bfbf4/runtime-acceptance-slice2s
```

## Accepted Slice 2T evidence

Slice 2T closes the request-time lineage gap for
`issued_from_credential_id`. Both ordinary browser-cookie authentication
and CSRF verification require the issuing credential to exist, belong to
the same actor, remain active, remain unrevoked and remain unexpired.

Fail-closed mapping:

```text
issuer expired
  -> credential_expired

issuer missing, actor-mismatched, inactive or revoked
  -> credential_revoked
```

The guarded yaVDR pass proved:

```text
service_pid_before=69610
service_pid_after_install=73034
service_pid_after_acceptance=73034
runtime_http_requests=5
login_http_status=200
active_get_http_status=200
revoked_issuer_get_http_status=401
revoked_issuer_logout_http_status=401
revoked_cookie_replay_http_status=401
raw_browser_active_before_cleanup=yes
raw_browser_unrevoked_before_cleanup=yes
canonical_session_active_before_cleanup=yes
browser_credential_active_before_cleanup=yes
issuer_revocation_effective_without_cascade=yes
logout_denied_before_csrf=yes
original_issuer_unchanged=yes
test_browser_session_revoked=yes
test_browser_credential_revoked=yes
accountability_secret_free=yes
vdr_domain_mutations=0
database_quick_check=ok
database_foreign_key_check=empty
service_state=active
automatic_rollback=not-required
```

Durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2t-20260801T223353Z-55876356e84b/runtime-acceptance-slice2t
```

Runtime report SHA-256:

```text
2ca7fcaefe21c1198e5d8ff88b3e17237b2e72a545780cc14f0200e7dd0ca983
```

The raw browser row remained unchanged by issuer invalidation. No
cascading mutation, cleanup policy or schema migration was introduced.

## Active Slice 2U repository contract

Slice 2U adds a strict optional limit for effective active browser sessions per
actor:

```text
VDR_SUITE_BROWSER_SESSION_MAX_ACTIVE_PER_ACTOR
0     unlimited compatibility default
1..64 maximum effective active browser sessions per actor
```

The effective count joins the browser verifier to the canonical actor, device,
session, browser credential and issuing credential. Every component must remain
active, unrevoked and unexpired. A raw active row whose issuer is revoked does
not consume a slot.

The count and all inserts remain inside the existing serialized
`BEGIN IMMEDIATE` issuance transaction. Reaching the limit returns the bounded
`LimitReached` result, creates no lifecycle row and never evicts or revokes an
existing session.

HTTP mapping:

```text
invalid configuration -> 503 browser_session_limit_configuration_invalid
limit reached         -> 409 browser_session_limit_reached
other issue failure   -> 503 browser_session_issuance_failed
```

The default remains unlimited. Idle timeout, `last_seen`, sliding refresh,
cleanup, retention, automatic eviction and session-administration APIs are not
part of Slice 2U.

## Remaining Phase 62 work

Phase 62 still lacks:

- Slice-2U final source stabilization, all five final-head CI jobs and guarded
  real-yaVDR acceptance;
- browser-session idle expiry and cleanup/retention policy;
- outcome accountability for other operation families;
- stronger transactional coupling or outbox semantics;
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
[Current State](../CURRENT.md), the active
[Slice 2U contract](phase-62-slice-2u-browser-session-concurrency-limit.md), the
[Slice 2T closeout](phase-62-slice-2t-browser-session-issuer-binding.md), the
[Slice 2S closeout](phase-62-slice-2s-browser-session-outcome-accountability.md),
the [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
and the
[Security and Identity Architecture](../architecture/security-identity-foundation.md).

### Preferred edit path for new chats

Root-level [Agent Workflow Rules](../../AGENTS.md) are binding.

Prefer direct GitHub repository updates for existing files when the connector
can perform the requested edit safely and the complete current file content is
available. Continue through already-approved bounded work without artificial
confirmation pauses.

Create small coherent commits and push them consecutively with fast-forward-only
semantics. Do not wait for GitHub Actions after every commit. Evaluate required
CI on the final stabilization head or immediately before a gated runtime,
Ready-for-review or merge operation.

Use local edits first only when the change requires:

- compilation, generated artifacts or focused local runtime tests;
- access to the installed real yaVDR runtime;
- coordinated tooling that is not available through the connector;
- a workaround because the GitHub connector blocks a file operation.

Never replace a complete file from a truncated fetch. Recheck the branch head
before every write, keep updates fast-forward-only and inspect the resulting
diff before treating a GitHub change as complete.

## Exact next action

Complete the bounded Slice-2U source stabilization and require all five GitHub
Actions jobs on the final current head. Only after full green final-head CI may
the guarded real-yaVDR concurrency-limit acceptance begin.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Agent Workflow Rules](../../AGENTS.md)
- [Slice 2U Active Contract](phase-62-slice-2u-browser-session-concurrency-limit.md)
- [Slice 2T Closeout](phase-62-slice-2t-browser-session-issuer-binding.md)
- [Slice 2S Closeout](phase-62-slice-2s-browser-session-outcome-accountability.md)
- [Slice 2R Closeout](phase-62-slice-2r-browser-session-lifetime-configuration.md)
- [Slice 2Q Closeout](phase-62-slice-2q-native-fuzzy-stale-probe-delete-security-migration.md)
- [Phase 62 Runtime Evidence](phase-62-runtime-evidence.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase 61 and Performance Closeout](phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](post-phase-61-platform-runtime-closeout.md)
