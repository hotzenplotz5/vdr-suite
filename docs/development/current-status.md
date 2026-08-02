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

Repository, source CI and real-runtime acceptance complete through:
Slice 2W - Browser-Session Terminal Retention Cleanup

Accepted Slice-2W source/runtime head:
bb8609151313c613d403b88b1b4c3f55453a93e2

Accepted Slice-2W source GitHub Actions:
VDR-Suite CI #6834
Run ID: 30745952119
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30745952119

Runtime acceptance:
PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE=PASS

Installed/running daemon SHA-256:
7775804306bf70eca6ef23474605467381162cfc9d5b874cdb187840ca8bc571

Installed deferred-runtime-loader.js SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Daemon configuration SHA-256:
8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b

Runtime report SHA-256:
e0fbe1689b2f48e75bb4ae6836b227d7da92e08d53b009ac1c2cb371a36c74ea

Durable evidence:
/var/backups/vdr-suite-phase62-slice2w-20260802T114239Z-bb8609151313

Next bounded implementation slice:
not yet selected
```

Phase 61 remains completed. Phase 62 remains active and incomplete. Phase
63-67 runtime has not been advanced.

## Completed-phase references

- [Phase 61 and Performance Closeout](phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](post-phase-61-platform-runtime-closeout.md)

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
- bounded terminal browser-session retention cleanup with exact accountability,
  atomic deletion and a fixed 256-lifecycle startup limit;
- guarded real-runtime acceptance and rollback tooling.

## Fully accepted Slice 2W

Configuration:

```text
VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS
0                 disabled compatibility default
86400..31536000   enabled retention delay in seconds
fixed batch size  256
```

One cleanup pass runs during Security Runtime initialization after schema and
configuration validation and before `securityReady`.

Eligibility is limited to explicit revocation, absolute expiry and idle expiry
beyond the retention delay. The deterministic order is oldest terminal time
first, then token ID.

Inside one `BEGIN IMMEDIATE` transaction, eligibility is rechecked, one exact
secret-free `browser.session.cleanup` event is appended for each deleted
verifier, the verifier is removed and only its unreferenced canonical session
and exact-type `browser-session` credential are removed. Actor, device, issuer,
grants, roles and accountability history are preserved. Any enabled cleanup
failure rolls back the whole batch and leaves the Security Runtime fail closed.

The guarded real-yaVDR acceptance proved:

```text
PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE=PASS
source_runtime_head=bb8609151313c613d403b88b1b4c3f55453a93e2
source_ci_run=6834
source_ci_run_id=30745952119
daemon_sha256=7775804306bf70eca6ef23474605467381162cfc9d5b874cdb187840ca8bc571
loader_sha256=3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
configuration_sha256=8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b
runtime_report_sha256=e0fbe1689b2f48e75bb4ae6836b227d7da92e08d53b009ac1c2cb371a36c74ea
```

The pass used isolated SQLite scenario databases and proved fresh schema
initialization, disabled no-op, fail-closed rollback after forced accountability
failure, all enabled preservation/deletion boundaries, exact audit events, 258
eligible lifecycles with exactly 256 deterministic deletions, SQLite integrity,
unchanged production database/configuration/loader, removed systemd override,
final active accepted daemon and zero VDR domain mutations.

Durable evidence:

```text
/var/backups/vdr-suite-phase62-slice2w-20260802T114239Z-bb8609151313
```

Do not repeat this acceptance solely because a chat changes. Repeat only when a
directly relevant daemon, cleanup, schema, configuration, systemd execution or
acceptance-harness fingerprint changes.

## Remaining Phase 62 gaps

Still open after Slice 2W:

- operation outcomes beyond browser lifecycle and cleanup operations;
- stronger transaction coupling or Outbox semantics;
- common revisions, idempotency and durable operation lifecycle;
- protected actor, identity, credential, grant and role administration;
- native/service credential enrollment, rotation and revocation;
- protected audit reads, export, redaction and retention;
- compatibility retirement and final Phase 62 closeout.

No next implementation slice is selected yet.

## Pull request truth

PR #117 must remain open, Draft and unmerged. Do not mark it Ready, merge it,
enable auto-merge, rebase, force-push or rewrite branch history without explicit
approval. Do not mutate Base, title, body, reviewers or other review/merge
metadata without explicit approval.

PR #118 remains the separate paused TVScraper workstream and must not be mixed
with Phase 62.

## Preferred edit path

Prefer direct GitHub repository updates when the connector can perform the edit
safely and the complete current file content is available. Use local edits only
for compilation, generated artifacts, focused local runtime tests or capabilities
not exposed by the connector.

Create small coherent commits with fast-forward-only semantics. Evaluate CI on
the final stabilization head rather than stopping after every intermediate
commit.

## Exact next action

Perform one fresh post-Slice-2W gap analysis. Compare the remaining bounded gaps,
select exactly one smallest coherent next Phase-62 slice, document its contract,
tests, architecture guard, runtime boundary and exclusions, update the handoff
and require all five CI jobs before implementation.

Do not combine multiple remaining security themes or advance Android, Android TV
or Phase 63-67 runtime.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Post-Slice-2W New Chat Prompt](phase-62-post-slice-2w-new-chat-prompt.md)
- [Slice 2W Runtime Closeout](phase-62-slice-2w-runtime-closeout.md)
- [Slice 2W Contract](phase-62-slice-2w-browser-session-retention-cleanup.md)
- [Phase 62 Runtime Evidence through Slice 2V](phase-62-runtime-evidence.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Agent Workflow Rules](../../AGENTS.md)
