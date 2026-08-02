# VDR-Suite Current State

## Navigation

- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Current Project Status](development/current-status.md)
- [Phase 62 Slice 2W Selection](development/phase-62-slice-2w-browser-session-retention-cleanup.md)
- [Phase 62 Slice 2V Closeout](development/phase-62-slice-2v-browser-session-idle-expiry.md)
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

Current active runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Repository, source CI, real-runtime and closeout accepted through:
Slice 2V - Browser-Session Idle Expiry and throttled last_seen

Accepted Slice-2V implementation/runtime head:
e84415fadb2587ff744ff8927f1f0113920ece2f

Accepted Slice-2V source CI:
VDR-Suite CI #6779
Run ID 30741293079
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30741293079

Accepted Slice-2V closeout head:
cf31b2b67f73f12718601ced5468a59a1183adcb

Accepted Slice-2V closeout CI:
VDR-Suite CI #6799
Run ID 30742295881
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30742295881

Selected next bounded slice:
Slice 2W - Browser-Session Terminal Retention Cleanup

Slice-2W state:
selection documented; implementation not started

Installed/running daemon SHA-256:
e0b6f6de08527b6af49d526ca0118b14b6fb85ff3335fc607ca1b531cdee5f60

Installed deferred-runtime-loader.js SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Restored daemon configuration SHA-256:
8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b
```

Phase 61 is complete. Phase 62 is active and incomplete. Phase 63-67 runtime
has not been advanced.

## Accepted security request path

```text
HTTP request
  -> browser cookie has strict precedence when present
  -> otherwise Legacy Basic or optional Managed Basic
  -> persistent actor/device/session/credential resolution
  -> issuing-credential request-time lifecycle binding
  -> immutable absolute lifetime and optional idle effectiveness
  -> exact route classification and scope extraction
  -> cookie-bound CSRF for browser mutations
  -> exact permission and fixed-role authorization
  -> append-only pre-dispatch accountability
  -> browser lifecycle outcome accountability
  -> existing router, backend and domain safety policy
```

Backend read-only, capability and domain policy remain independent from actor
authorization. Frontends do not own authorization decisions.

## Fully accepted Slice 2V

Slice 2V introduced:

```text
VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS
0          disabled compatibility default
300..86400 enabled idle timeout

security_browser_session_credentials.last_seen_at
minimum activity-write interval: 60 seconds
```

The additive migration backfills `last_seen_at` from `created_at`. Cookie and
CSRF verification use one repository-owned idle calculation. Absolute
`expires_at` remains the unchanged hard upper bound.

The real yaVDR acceptance proved:

```text
PHASE_62_SLICE_2V_RUNTIME_ACCEPTANCE=PASS
ordinary_get_before_idle=HTTP 200
ordinary_get_after_idle=HTTP 401 session_expired
protected_mutation_after_idle=HTTP 401 session_expired
last_seen_writes_inside_interval=1
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

Exactly one post-2V slice is selected:

```text
Phase 62 Slice 2W
Browser-Session Terminal Retention Cleanup
```

Selected contract:

```text
VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS
0                 disabled compatibility default
86400..31536000   enabled retention delay
fixed batch size  256
```

The selected implementation is limited to one bounded startup cleanup pass after
schema/configuration validation and before `securityReady`.

Only terminal browser verifiers and their own unreferenced canonical browser
session and `browser-session` credential are eligible. Actors, devices, issuing
credentials, grants, roles and accountability history must be preserved.

Eligibility is limited to terminal browser lifecycles beyond retention:

- explicit revocation;
- absolute expiry;
- idle expiry when the accepted idle policy is enabled.

The cleanup must recheck eligibility, append secret-free accountability and
delete atomically. Any enabled-policy failure leaves the Security Runtime fail
closed.

Not included:

- periodic scheduling or request-path cleanup;
- HTTP/API/UI administration;
- logout-all or session listing;
- issuer-revocation cascade deletion;
- automatic eviction to satisfy concurrency limits;
- generic security administration or generic credential cleanup;
- Outbox, Android, Android TV or Phase 63-67 work.

No Slice-2W code or runtime mutation has occurred.

## Remaining Phase 62 gaps

Beyond the selected Slice 2W, the remaining gaps are:

- broader operation outcomes and stronger transactional coupling;
- common revisions, idempotency and durable operation lifecycle;
- protected actor, identity, credential, grant and role administration;
- native/service credential lifecycle;
- protected audit reads, export, redaction and retention;
- compatibility retirement and final Phase 62 closeout.

## Operating rules

- Root-level `AGENTS.md` is binding.
- Prefer GitHub-first edits when the connector can safely complete them.
- Continue through already-approved bounded work without artificial pauses.
- Evaluate CI on the final stabilization head.
- Every CI status report includes the direct link, run number, run ID and head.
- PR #117 remains open, Draft and unmerged.
- Do not mark it Ready, merge, auto-merge, rebase, force-push or rewrite history.
- Do not repeat accepted runtime work only because the chat changed.
- Select and implement exactly one bounded Phase 62 slice at a time.
- Do not pull Android or Phase 63-67 runtime into Phase 62.

## Exact next action

Require all five GitHub Actions jobs for the documentation-only Slice-2W
selection head.

After that CI is fully green, implement only the bounded Slice-2W configuration,
repository/service cleanup transaction, startup integration, focused tests,
architecture guard and Make-test registration.

Do not combine Slice 2W with scheduling, administration APIs, issuer cascade,
automatic eviction, broader security administration, Outbox, Android or Phase
63-67 runtime.
