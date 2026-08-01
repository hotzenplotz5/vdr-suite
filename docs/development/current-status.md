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

Accepted GitHub Actions:
VDR-Suite CI #6655
Run ID: 30713953331
All five jobs successful

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

## Slice 2Q accepted routes

```text
POST /api/epgsearch/native-fuzzy/stale-probes/delete
POST /api/vdr/epgsearch/native-fuzzy/stale-probes/delete
  -> epgsearch.native-fuzzy.stale-probes.delete@*
```

The canonical authorization scope is global `*`. Request body and query values
cannot alter it. Direct concrete-scope grants and concrete Admin assignments are
denied. `role.admin@*` is the exact global assignment, while
`role.read-only@*` denies before direct permission or Admin.

There is no Webfrontend owner. The aliases remain excluded from Safe POST
because the existing controller can perform a real SQLite deletion.

## Slice 2Q real yaVDR evidence

The accepted pass used a direct read-only SQLite preflight with the production
freshness policy:

```text
maxAgeSeconds=604800
future timestamp -> stale
age greater than 604800 seconds -> stale
```

A temporary cross-connection `BEFORE DELETE` trigger blocked every possible
real deletion during the POST matrix. The trigger was removed in cleanup and
verified absent.

```text
service_pid_after_acceptance=67393
tests_passed=32/32
runtime_http_requests=25
accountability_authorized=8
accountability_csrf=2
accountability_permission=2
accountability_read_only=2
accountability_scope=4
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

The preinstall, pre-POST and postflight snapshots have the same SHA-256:

```text
2b1d1af321fd9497f99ac742c694c70ecfde94b41bcb6d74ee3a70187e5d1e7a
```

Runtime report:

```text
602148a61a69dadcf7a38fb566b4e4486a7e550f0dda193fffa8f42ba3a1c197
```

Durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2q-20260801T191156Z-88ec36076d7e/runtime-acceptance-slice2q
```

## Safe rejected first attempt

The first Slice-2Q runtime attempt used a historically documented stale-probe
GET alias that is not registered in `ApiRouter::handleGet`. The runtime returned
HTTP 404 before every Delete POST. Automatic rollback restored the accepted
Slice-2P runtime, and the temporary guard was absent after rollback.

```text
/var/backups/vdr-suite-phase62-slice2q-20260801T185634Z-1119c94e5184/install-before
```

That attempt is rejection and rollback evidence only. The corrected accepted
runtime pass is the direct-SQLite pass at head `88ec36076d7e5114df0a3a186cc6fbd52bb2baac`.

## Remaining Phase 62 work

Phase 62 still lacks:

- a fresh POST inventory audit after Slice 2Q;
- completion/outcome accountability and stronger transactional coupling;
- browser-session refresh, idle expiry, cleanup and concurrency policy;
- protected identity, credential, role and grant administration;
- native/service credential enrollment, rotation and revocation;
- generic persisted role definitions beyond the fixed catalogue;
- common revision, idempotency and durable operation contracts;
- protected audit query/export/retention;
- compatibility-retirement readiness and final Phase 62 closeout.

No next implementation slice has been selected by this closeout.

## Pull request truth

PR #117 must remain open, Draft and unmerged. Do not mark it Ready for review,
merge it, enable auto-merge, force-push, rewrite branch history or change PR
metadata without explicit approval.

The PR description is materially stale. Current repository truth is this file,
[Current State](../CURRENT.md), the
[Slice 2Q closeout](phase-62-slice-2q-native-fuzzy-stale-probe-delete-security-migration.md),
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

Let the Slice-2Q documentation closeout complete its full five-job CI. Then
perform a fresh bounded POST inventory audit. Select exactly one next Phase 62
slice only after its scope, persistence effects, permission, authorization
scope, accountability and runtime-safety boundary are explicit.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Slice 2Q Closeout](phase-62-slice-2q-native-fuzzy-stale-probe-delete-security-migration.md)
- [Slice 2P Closeout](phase-62-slice-2p-query-cache-refresh-security-migration.md)
- [Slice 2O Closeout](phase-62-slice-2o-native-fuzzy-refresh-security-migration.md)
- [Phase 62 Runtime Evidence](phase-62-runtime-evidence.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase 61 and Performance Closeout](phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](post-phase-61-platform-runtime-closeout.md)
