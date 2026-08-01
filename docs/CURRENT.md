# VDR-Suite Current State

## Navigation

- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Current Project Status](development/current-status.md)
- [Phase 62 Slice 2P Closeout](development/phase-62-slice-2p-query-cache-refresh-security-migration.md)
- [Phase 62 Runtime Evidence](development/phase-62-runtime-evidence.md)
- [Phase 62 Gap Matrix](planning/phase-62-security-identity-gap-matrix.md)
- [Strict Roadmap](planning/roadmap.md)
- [Security and Identity Architecture](architecture/security-identity-foundation.md)

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Base: main @ cb77ff66e11dca7db2eafa36525762dcde35102d
Active PR: #117
PR state: open, Draft, unmerged, mergeable

Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Current active phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Repository, CI and real-runtime accepted through:
Slice 2P - Query-Scoped Cache Refresh Security Migration

Accepted code/runtime head:
173c929964dbb7aabd30c5e482c2e250b5785d92

CI:
VDR-Suite CI #6649
Run ID 30711237050
All five jobs successful

Installed daemon SHA-256:
c0e74602334e2b9d21f53329182bc5e35c99676f3dcdf2ae0639f996151a432a

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
  -> backend-scope extraction from the route contract
  -> cookie-bound CSRF for migrated browser mutations
  -> exact permission and backend-scope authorization
  -> fixed exact-backend Admin/Read-only evaluation
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
- accepted explicit safe POST routes;
- Native Fuzzy operator refresh aliases;
- SearchTimer preview cache refresh aliases;
- EPG cache refresh.

The current Slice 2P contracts are:

```text
POST /api/searchtimers/preview/cache/refresh
POST /api/vdr/searchtimers/preview/cache/refresh
  permission: searchtimers.preview-cache.refresh@<backend-id>

POST /api/epg/cache/refresh
  permission: epg.cache.refresh@<backend-id>
```

For these routes `<backend-id>` comes only from query parameter `backend`.
Missing or empty values normalize to `default`; URL decoding and duplicate
last-value behavior match the router. Request-body backend fields cannot change
the authorization scope.

## Latest real-runtime acceptance

The yaVDR pass used the non-registered backend
`phase62-slice2p-missing-backend`, reached the existing service boundary and
stopped with `backend-not-found` before any cache refresh.

```text
SearchTimer preview cache refresh: 29 tests, 27 HTTP requests
EPG cache refresh: 18 tests, 16 HTTP requests
Total: 47 tests, 43 HTTP requests
Daemon PID remained 66229
Cache mutation: none
Backend snapshot: unchanged
Target grants: restored
Browser session: revoked
Revoked-cookie replay: denied
Accountability: secret-free
SQLite quick/foreign-key checks: passed
Service: active
```

Durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2p-20260801T180617Z-173c929964db/runtime-acceptance-slice2p
```

## Compatibility and fail-closed boundary

Legacy Basic remains a transitional compatibility path. Managed Basic and
browser actors do not inherit a legacy bypass. Browser mutations not explicitly
classified remain fail-closed with `security_policy_not_migrated`.

Query strings are removed only for exact route matching. Trailing-slash and
unrelated path variants remain fail-closed. The Native Fuzzy stale-probe delete
aliases were intentionally excluded from Slice 2P.

## Remaining Phase 62 work

- classify or migrate the remaining POST route families one bounded family at a
  time;
- add completion/outcome accountability and stronger transactional coupling;
- define refresh, idle expiry, cleanup and concurrent-session policy;
- add protected credential, identity, role and grant administration;
- add native/service credential lifecycle;
- add generic roles only after route migration is sufficiently complete;
- standardize revisions, idempotency and operation lifecycle;
- add protected audit query/export/retention;
- complete compatibility-retirement and final Phase 62 acceptance.

## Operating rules

- PR #117 remains open, Draft and unmerged.
- Do not mark it Ready, merge, auto-merge, force-push or rewrite branch history.
- Recheck volatile GitHub and local state immediately before mutation.
- Do not repeat completed runtime acceptance solely because the chat changed.
- The next implementation must be exactly one bounded Phase 62 route family.
- Do not pull Android or Phase 63-67 runtime into Phase 62 route migration.

## Exact next action

Let the documentation-closeout commit pass the complete five-job CI. Then plan
one next bounded route family with explicit routes, permission, scope source,
frontend owner, runtime-safe test boundary and rollback design before coding.
