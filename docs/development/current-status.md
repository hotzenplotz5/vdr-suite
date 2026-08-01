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

Current active runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Repository, CI and real-runtime accepted through:
Slice 2P - Query-Scoped Cache Refresh Security Migration

Accepted code/runtime head:
173c929964dbb7aabd30c5e482c2e250b5785d92

GitHub Actions:
VDR-Suite CI #6649
Run ID: 30711237050
All five jobs successful

Installed daemon SHA-256:
c0e74602334e2b9d21f53329182bc5e35c99676f3dcdf2ae0639f996151a432a

Installed deferred-runtime-loader.js SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
```

Phase 61 remains completed. Phase 62 remains active and incomplete. Phase
63-67 runtime has not been advanced.

## Cumulative accepted Phase 62 scope

The branch and installed runtime now include:

- canonical actor, device, session, credential, request and correlation context;
- persistent identity, lifecycle, managed Basic and browser-session verifiers;
- atomic browser-session issue/logout with independent cookie and CSRF secrets;
- ordinary-route browser authentication with strict cookie precedence;
- persisted exact actor grants and fail-closed unavailable-store handling;
- fixed exact-backend `role.admin` and `role.read-only` semantics;
- memory-only Webfrontend CSRF state and exact request-owner injection;
- protected Remote, Timer, Channel Move, Recording execution and SearchTimer
  create/maintenance/execution mutations;
- explicit safe-POST classification for the accepted validation/preview family;
- protected Native Fuzzy operator refresh;
- protected query-scoped SearchTimer preview and EPG cache refresh;
- append-only pre-dispatch accountability and secret-free denial evidence;
- mutation-safe real-runtime acceptance profiles and guarded rollback.

## Slice 2P accepted routes

```text
POST /api/searchtimers/preview/cache/refresh
POST /api/vdr/searchtimers/preview/cache/refresh
  -> searchtimers.preview-cache.refresh@<backend-id>

POST /api/epg/cache/refresh
  -> epg.cache.refresh@<backend-id>
```

The backend scope comes only from query parameter `backend`. Missing or empty
values resolve to `default`; URL decoding and duplicate last-value semantics
match the router. JSON body fields cannot override the authorization scope.

Exact route paths accept query strings. Trailing-slash variants remain
fail-closed. The two permissions are distinct and do not authorize each other.

## Slice 2P real yaVDR evidence

```text
service_pid_after_install=66229
service_pid_after_acceptance=66229
searchtimer_preview_tests=29/29
searchtimer_preview_http_requests=27
epg_cache_tests=18/18
epg_cache_http_requests=16
total_tests=47/47
total_http_requests=43
resource_state_unchanged=yes
cache_mutation=none
target_grants_restored=yes
browser_session_revoked=yes
database_integrity=yes
service_state=active
```

Durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2p-20260801T180617Z-173c929964db/runtime-acceptance-slice2p
```

The first guarded installation attempt stopped on an incorrect wrapper-side
build-path assertion and automatically restored the accepted Slice-2O runtime.
The corrected pass derived `.build` from Make and completed successfully.

## Remaining Phase 62 work

Phase 62 still lacks:

- migration or explicit classification of the remaining POST families;
- completion/outcome accountability and stronger transactional coupling;
- browser-session refresh, idle expiry, cleanup and concurrency policy;
- protected identity, credential, role and grant administration;
- native/service credential enrollment, rotation and revocation;
- generic persisted role definitions beyond the fixed catalogue;
- common revision, idempotency and durable operation contracts;
- protected audit query/export/retention;
- compatibility-retirement readiness and final Phase 62 closeout.

The Native Fuzzy stale-probe deletion aliases remain explicitly excluded from
Slice 2P and fail closed pending a separate bounded contract.

## Pull request truth

PR #117 must remain open, Draft and unmerged. Do not mark it Ready for review,
merge it, enable auto-merge, force-push, rewrite branch history or change PR
metadata without explicit approval.

The PR description is materially stale. Current repository truth is this file,
[Current State](../CURRENT.md) and the
[Slice 2P closeout](phase-62-slice-2p-query-cache-refresh-security-migration.md).

## Exact next action

First let the Slice-2P documentation closeout pass all five CI jobs. Then inspect
the remaining POST inventory and plan exactly one next Phase 62 route family.
No next implementation begins until its route set, permission, scope source,
frontend ownership, runtime-safe boundary and rollback contract are explicit.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Slice 2P Closeout](phase-62-slice-2p-query-cache-refresh-security-migration.md)
- [Slice 2O Closeout](phase-62-slice-2o-native-fuzzy-refresh-security-migration.md)
- [Phase 62 Runtime Evidence](phase-62-runtime-evidence.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)
- [Strict Roadmap](../planning/roadmap.md)
