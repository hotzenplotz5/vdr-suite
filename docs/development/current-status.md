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

Next strict runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Current active runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Repository, source CI, real-runtime and closeout accepted through:
Slice 2V - Browser-Session Idle Expiry and throttled last_seen

Accepted implementation/runtime head:
e84415fadb2587ff744ff8927f1f0113920ece2f

Accepted Slice-2V source GitHub Actions:
VDR-Suite CI #6779
Run ID: 30741293079
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30741293079

Accepted Slice-2V closeout head:
cf31b2b67f73f12718601ced5468a59a1183adcb

Accepted Slice-2V closeout GitHub Actions:
VDR-Suite CI #6799
Run ID: 30742295881
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30742295881

Selected next bounded slice:
Slice 2W - Browser-Session Terminal Retention Cleanup

Slice-2W state:
selection and contract documented; implementation not started

Installed/running daemon SHA-256:
e0b6f6de08527b6af49d526ca0118b14b6fb85ff3335fc607ca1b531cdee5f60

Installed deferred-runtime-loader.js SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Restored daemon configuration SHA-256:
8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b
```

Phase 61 remains completed. Phase 62 remains active and incomplete. Phase
63-67 runtime has not been advanced.

## Cumulative accepted Phase 62 scope

The accepted branch and installed runtime include:

- canonical actor, device, session, credential, request and correlation context;
- persistent identity, lifecycle, Managed Basic and browser-session verifiers;
- atomic browser-session issue/logout with independent cookie and CSRF secrets;
- ordinary-route browser authentication with strict cookie precedence;
- persisted exact actor grants and fail-closed unavailable-store handling;
- fixed exact-scope `role.admin` and `role.read-only` semantics;
- memory-only Webfrontend CSRF state and exact request-owner injection;
- protected Remote, Timer, Channel Move, Recording and SearchTimer mutations;
- explicit Safe POST classification for accepted validation and preview routes;
- protected Native Fuzzy refresh, stale-probe deletion and query-scoped cache
  refresh;
- configurable bounded absolute browser-session lifetime;
- append-only pre-dispatch accountability;
- browser-session issue/revoke outcome accountability with compensation;
- issuing-credential request-time lifecycle binding;
- strict optional per-actor effective browser-session limits with deny-new
  semantics;
- strict optional browser-session idle expiry with additive `last_seen_at` and
  a fixed 60-second activity-write throttle;
- idle-expired sessions excluded from the effective concurrency count;
- guarded real-runtime acceptance and rollback tooling.

## Fully accepted Slice 2V

Slice 2V introduced:

```text
VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS
0          disabled compatibility default
300..86400 enabled idle timeout in seconds

security_browser_session_credentials.last_seen_at
60-second minimum activity-write interval
```

The repository owns one idle calculation for ordinary cookie authentication and
CSRF verification. Absolute `expires_at` remains the immutable hard upper bound.
Idle-expired rows do not consume a Slice-2U concurrency slot and are not
physically deleted, revoked or evicted by Slice 2V.

The guarded real-yaVDR acceptance proved:

```text
PHASE_62_SLICE_2V_RUNTIME_ACCEPTANCE=PASS
ordinary_get_before_idle=HTTP 200
ordinary_get_after_idle=HTTP 401 session_expired
mutation_after_idle=HTTP 401 session_expired
last_seen_write_interval_seconds=60
absolute_expiry_unchanged=yes
replacement_logout=HTTP 204
revoked_cookie_replay=HTTP 401 credential_revoked
test_lifecycle_active_rows=0
sqlite_quick_check=ok
sqlite_foreign_key_check=empty
accountability_secret_free=yes
vdr_domain_mutations=0
final_service_state=active
runtime_dropin=removed
idle_test_environment=not-set
```

Durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2v-20260802T092139Z-e84415fadb25
```

Runtime report SHA-256:

```text
0a961fbc8b51158fd4a16aa24fc9afde7dafa9d5272e986a46ec73880c311f86
```

## Selected Slice 2W

The fresh post-2V analysis selected exactly one bounded next slice:

```text
Phase 62 Slice 2W
Browser-Session Terminal Retention Cleanup
```

The selection document is:

- [Slice 2W contract](phase-62-slice-2w-browser-session-retention-cleanup.md)

Selected boundary:

- configuration `VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS`;
- compatibility default `0`;
- enabled range `86400..31536000` seconds;
- one bounded startup cleanup pass;
- fixed maximum batch of 256 terminal browser lifecycles;
- atomic eligibility recheck, secret-free accountability and deletion;
- delete only the terminal browser verifier and its own unreferenced canonical
  browser session and `browser-session` credential;
- preserve actors, devices, issuing credentials, grants and accountability;
- fail closed before `securityReady` when enabled cleanup fails.

Explicitly excluded:

- periodic scheduler or request-path cleanup;
- session listing, logout-all or administration API/UI;
- issuer-revocation cascade cleanup;
- automatic eviction for the concurrency limit;
- generic security administration or generic credential cleanup;
- Outbox, Android, Android TV or Phase 63-67 runtime.

No Slice-2W implementation or runtime mutation has occurred.

## Remaining Phase 62 gaps after the selected slice

Beyond the selected Slice 2W, still open are:

- operation outcomes beyond browser lifecycle operations;
- stronger transaction coupling or Outbox semantics;
- common revisions, idempotency and durable operation lifecycle;
- protected actor, identity, credential, grant and role administration;
- native/service credential enrollment, rotation and revocation;
- protected audit reads, export, redaction and retention;
- compatibility retirement and final Phase 62 closeout.

## Pull request truth

PR #117 must remain open, Draft and unmerged. Do not mark it Ready, merge it,
enable auto-merge, rebase, force-push or rewrite branch history without explicit
approval.

## Exact next action

Require all five GitHub Actions jobs for the documentation-only Slice-2W
selection head.

After that selection CI is fully green, implement only the bounded Slice-2W
configuration, repository/service cleanup transaction, startup integration,
focused tests, architecture guard and Make-test registration.

Do not combine Slice 2W with a periodic scheduler, administration API, issuer
cascade, automatic eviction, broader security administration, Outbox, Android or
Phase 63-67 work.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Agent Workflow Rules](../../AGENTS.md)
- [Slice 2W Selection](phase-62-slice-2w-browser-session-retention-cleanup.md)
- [Slice 2V Closeout](phase-62-slice-2v-browser-session-idle-expiry.md)
- [Phase 62 Runtime Evidence](phase-62-runtime-evidence.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)
- [Strict Roadmap](../planning/roadmap.md)
