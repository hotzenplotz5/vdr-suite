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
Slice 2S - Browser-Session Lifecycle Outcome Accountability

Accepted code/runtime head:
c128867bfbf4ce10bcf7dc23d14652e5f5324c83

Accepted closeout head:
064744f73905b6fcc53d737ab9088554ae2af4b6

Accepted closeout GitHub Actions:
VDR-Suite CI #6664
Run ID: 30718491649
All five jobs successful

Active repository implementation:
Slice 2T - Browser-Session Issuing-Credential Lifecycle Binding
CI and real-runtime acceptance pending

Installed daemon SHA-256:
682cfc76738454f57daff0831fe7a01786f57abf42cf16c2fa9c2ac16309a07a

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

## Active Slice 2T repository contract

The fresh lifecycle audit confirmed that `PersistentIdentityResolver` already
rechecks the browser actor, device, canonical session and browser credential.
The remaining gap is the lineage credential stored in
`issued_from_credential_id`.

Slice 2T adds an effective token lookup that joins the browser row to its issuing
credential. Both ordinary cookie authentication and CSRF verification require
the issuer to exist, belong to the same actor, remain active, remain unrevoked
and remain unexpired.

Fail-closed mapping:

```text
issuer expired
  -> credential_expired

issuer missing, actor-mismatched, inactive or revoked
  -> credential_revoked
```

The raw browser row remains unchanged. No cascading mutation, cleanup policy or
schema migration is introduced.

Explicitly excluded from Slice 2T:

- idle timeout and `last_seen` persistence;
- sliding expiry or refresh;
- expired-session cleanup;
- concurrent-session limits;
- security administration APIs;
- issuer enrollment or rotation;
- broader operation outcomes or transactional outbox;
- route, frontend, permission, configuration or packaging changes.

This repository implementation is not accepted runtime until all five CI jobs
and the guarded yaVDR issuing-credential revocation pass succeed.

## Remaining Phase 62 work

Phase 62 still lacks:

- Slice 2T CI and guarded real-runtime acceptance;
- browser-session idle expiry, cleanup and concurrency policy;
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
[Slice 2T contract](phase-62-slice-2t-browser-session-issuer-binding.md), the
[Slice 2S closeout](phase-62-slice-2s-browser-session-outcome-accountability.md),
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

Publish the bounded Slice-2T implementation as one fast-forward commit and
require all five CI jobs. Only then run guarded yaVDR acceptance using a
disposable revoked issuer credential bound solely to the test browser session.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Slice 2T Active Contract](phase-62-slice-2t-browser-session-issuer-binding.md)
- [Slice 2S Closeout](phase-62-slice-2s-browser-session-outcome-accountability.md)
- [Slice 2R Closeout](phase-62-slice-2r-browser-session-lifetime-configuration.md)
- [Slice 2Q Closeout](phase-62-slice-2q-native-fuzzy-stale-probe-delete-security-migration.md)
- [Phase 62 Runtime Evidence](phase-62-runtime-evidence.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase 61 and Performance Closeout](phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](post-phase-61-platform-runtime-closeout.md)
