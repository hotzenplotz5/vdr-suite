# VDR-Suite Current State

## Navigation

- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Current Project Status](development/current-status.md)
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

Accepted CI:
VDR-Suite CI #6655
Run ID 30713953331
All five jobs successful

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

## Accepted protected route families

The cumulative accepted catalogue includes:

- Remote actions;
- Timer create, update and delete;
- Channel Move aliases;
- Recording execution aliases;
- SearchTimer create, update, delete and execution aliases;
- accepted explicit Safe POST routes;
- Native Fuzzy operator refresh aliases;
- SearchTimer preview cache refresh aliases;
- EPG cache refresh;
- global Native Fuzzy stale-probe deletion aliases.

## Latest accepted Slice 2Q contract

```text
POST /api/epgsearch/native-fuzzy/stale-probes/delete
POST /api/vdr/epgsearch/native-fuzzy/stale-probes/delete
  permission: epgsearch.native-fuzzy.stale-probes.delete@*
```

The canonical scope is global `*` and is independent of body or query values.
Direct concrete-scope grants and concrete Admin assignments do not authorize
the route. `role.admin@*` is the exact global assignment, while
`role.read-only@*` denies before direct permission or Admin.

The aliases have no Webfrontend owner and remain excluded from Safe POST.
Query-string variants use the exact base route; trailing-slash variants remain
fail-closed.

## Latest real-runtime acceptance

The guarded yaVDR pass used a direct read-only SQLite stale/future snapshot with
the production seven-day freshness boundary. The snapshot was empty before
installation, before the POST matrix and after the matrix.

A temporary cross-connection `BEFORE DELETE` trigger blocked every possible
real deletion during the acceptance pass. Cleanup removed the trigger and final
verification proved it absent.

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

All three normalized snapshot files share this SHA-256:

```text
2b1d1af321fd9497f99ac742c694c70ecfde94b41bcb6d74ee3a70187e5d1e7a
```

Durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2q-20260801T191156Z-88ec36076d7e/runtime-acceptance-slice2q
```

The earlier guarded attempt stopped safely on an unregistered historical GET
route before every Delete POST and automatically restored Slice 2P. It remains
rollback evidence, not the accepted Slice-2Q pass.

## Compatibility and fail-closed boundary

Legacy Basic remains a transitional compatibility path. Managed Basic and
browser actors do not inherit a legacy bypass. Browser mutations not explicitly
classified remain fail-closed with `security_policy_not_migrated`.

Query strings are removed only for exact route matching. Trailing-slash and
unrelated path variants remain fail-closed. Slice 2Q does not classify the
global deletion route as Safe POST.

## Remaining Phase 62 work

- perform a fresh POST inventory audit after Slice 2Q;
- add completion/outcome accountability and stronger transactional coupling;
- define refresh, idle expiry, cleanup and concurrent-session policy;
- add protected credential, identity, role and grant administration;
- add native/service credential lifecycle;
- add generic roles only after route migration is sufficiently complete;
- standardize revisions, idempotency and operation lifecycle;
- add protected audit query/export/retention;
- complete compatibility-retirement and final Phase 62 acceptance.

No next implementation slice has been selected by this closeout.

## Operating rules

- PR #117 remains open, Draft and unmerged.
- Do not mark it Ready, merge, auto-merge, force-push or rewrite branch history.
- Recheck volatile GitHub and local state immediately before mutation.
- Do not repeat completed runtime acceptance solely because the chat changed.
- Select exactly one bounded Phase 62 slice at a time.
- Do not pull Android or Phase 63-67 runtime into Phase 62 work.

## Exact next action

Let the Slice-2Q documentation closeout complete its full five-job CI. Then
perform a fresh bounded POST inventory audit and select exactly one next Phase
62 slice only after its security, persistence and runtime-safety contract is
explicit.
