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
Slice 2U - Concurrent Browser-Session Limit

Accepted implementation/runtime head:
16ff04a4ba371aad32fc4a38bf82f9c0529c532d

Accepted Slice-2U source GitHub Actions:
VDR-Suite CI #6690
Run ID: 30723297375
All five jobs successful

Slice-2U documentation closeout commit:
4747d725664d4c382d17d3b19fa2776f48ba437b

Final shared closeout and workflow head:
d00fc5045a136d87323fbc13fb1bfc1030f7d3b5

Final closeout GitHub Actions:
VDR-Suite CI #6693
Run ID: 30733265772
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30733265772

Active repository implementation:
Slice 2V - Browser-Session Idle Expiry and throttled last_seen

Installed/running daemon SHA-256:
0e3ec0d57f4471804824247f712c2457015cc22ac9576df60d8d77ed8ddb3134

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
- strict optional per-actor effective browser-session limits with deny-new semantics;
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
the same actor, remain active, unrevoked and unexpired.

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

## Accepted Slice 2U runtime evidence

Slice 2U enforces a strict optional effective-session limit per actor:

```text
VDR_SUITE_BROWSER_SESSION_MAX_ACTIVE_PER_ACTOR
0     unlimited compatibility default
1..64 maximum effective active browser sessions per actor
```

The effective count joins actor, device, session, browser credential and issuing
credential lifecycle state. Count and insert remain in the serialized
`BEGIN IMMEDIATE` issuance transaction. Reaching the limit creates nothing and
does not evict or revoke an existing session.

HTTP mapping:

```text
invalid configuration -> 503 browser_session_limit_configuration_invalid
limit reached         -> 409 browser_session_limit_reached
other issue failure   -> 503 browser_session_issuance_failed
```

The guarded yaVDR pass proved:

```text
Implementation/runtime head:
16ff04a4ba371aad32fc4a38bf82f9c0529c532d

Source CI:
#6690 / run 30723297375 / all five jobs successful

Installed/running daemon SHA-256:
0e3ec0d57f4471804824247f712c2457015cc22ac9576df60d8d77ed8ddb3134

Loader SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Final service PID:
79316

Runtime report SHA-256:
7c33b06d07beff6b17bad82153ff4a0f1e7c1f5c8d8f972406f8b0b9160f4c89

Configured limit:
1

First session issue:
HTTP 200

Second same-actor issue:
HTTP 409 browser_session_limit_reached
No second lifecycle row created

Existing first session ordinary GET:
HTTP 200

First logout:
HTTP 204
Slot released

Replacement session issue:
HTTP 200

Replacement logout:
HTTP 204

Revoked replacement-cookie replay:
HTTP 401 credential_revoked

Successful issue outcomes:
2

Limit-reached failed outcomes:
1

Successful revoke outcomes:
2

Acceptance lifecycle and source identity:
revoked

Original daemon configuration:
restored

SQLite quick check:
ok

SQLite foreign-key check:
empty

VDR domain mutations:
0

Service state:
active

Automatic rollback:
not required
```

Durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2u-20260802T041910Z-16ff04a4ba37/runtime-acceptance-slice2u
```

The default remains unlimited. Idle timeout, `last_seen`, sliding refresh,
cleanup, retention, automatic eviction and session-administration APIs remain
outside Slice 2U.

## Fresh post-2U gap analysis

The accepted POST inventory is complete; no new route-migration family remains.
The open Phase 62 gaps are:

- browser-session idle expiry with throttled activity persistence;
- physical cleanup and retention as a later separate policy;
- outcome accountability for other operation families;
- stronger transactional coupling or outbox semantics;
- common revision, idempotency and durable operation lifecycle;
- protected actor, identity, credential, grant and role administration;
- native/service credential enrollment, rotation and revocation;
- protected audit reads, export, redaction and retention;
- compatibility retirement and final Phase 62 closeout.

The repository already has absolute `expires_at`, request-time browser-cookie and
CSRF resolution and an additive browser-session table. It has no dedicated
activity timestamp. Existing `updated_at` cannot be reused because revocation,
expiry and other lifecycle writes also modify it.

Therefore the next bounded slice is:

```text
Phase 62 Slice 2V - Browser-Session Idle Expiry and throttled last_seen
```

The required additive `last_seen_at` column is part of this browser-session-only
slice; no separate general timestamp or schema precursor is required.

## Pull request truth

PR #117 must remain open, Draft and unmerged. Do not mark it Ready for review,
merge it, enable auto-merge, force-push, rewrite branch history or change PR
metadata without explicit approval.

The PR description is materially stale. Current repository truth is this file,
[Current State](../CURRENT.md), the accepted
[Slice 2U closeout](phase-62-slice-2u-browser-session-concurrency-limit.md), the
[Phase 62 Runtime Evidence](phase-62-runtime-evidence.md), the
[Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md) and
the [Security and Identity Architecture](../architecture/security-identity-foundation.md).

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

Document and implement only Slice 2V. Validate its focused configuration,
repository, authenticator, HTTP and architecture contracts, then evaluate all
five GitHub Actions jobs on the final stabilization head. Because Slice 2V
changes the daemon and browser-session schema, a new guarded real-yaVDR runtime
acceptance is required only after source CI is green.

Do not begin cleanup, retention, eviction, session administration, broader
security administration, Outbox, Android or Phase 63-67 work.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Agent Workflow Rules](../../AGENTS.md)
- [Slice 2U Closeout](phase-62-slice-2u-browser-session-concurrency-limit.md)
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
