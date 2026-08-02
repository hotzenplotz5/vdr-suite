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

Selected next bounded implementation slice:
Slice 2X - Protected Accountability Event Read

Slice-2X state:
selection and contract only; no production implementation
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
- protected Native Fuzzy refresh, stale-probe deletion and query-scoped cache refresh;
- configurable bounded absolute browser-session lifetime;
- append-only pre-dispatch accountability;
- browser-session issue/revoke outcome accountability with compensation;
- issuing-credential request-time lifecycle binding;
- strict optional per-actor effective browser-session limits with deny-new semantics;
- strict optional browser-session idle expiry with additive `last_seen_at` and a fixed 60-second activity-write throttle;
- bounded terminal browser-session retention cleanup with exact accountability, atomic deletion and a fixed 256-lifecycle startup limit;
- guarded real-runtime acceptance and rollback tooling.

## Fully accepted Slice 2W

```text
VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS
0                 disabled compatibility default
86400..31536000   enabled retention delay in seconds
fixed batch size  256
```

One cleanup pass runs during Security Runtime initialization after schema and
configuration validation and before `securityReady`. Eligibility is limited to
explicit revocation, absolute expiry and idle expiry beyond the retention delay.
The deterministic order is oldest terminal time first, then token ID.

Inside one `BEGIN IMMEDIATE` transaction, eligibility is rechecked, one exact
secret-free `browser.session.cleanup` event is appended for each deleted
verifier, the verifier is removed and only its unreferenced canonical browser
session and exact-type `browser-session` credential are removed. Actor, device,
issuer, grants, roles and accountability history are preserved. Any enabled
cleanup failure rolls back the whole batch and leaves the Security Runtime fail
closed.

The guarded real-yaVDR acceptance proved fresh schema initialization, disabled
no-op, fail-closed rollback, all deletion/preservation boundaries, exact audit
events, 258 eligible lifecycles with exactly 256 deterministic deletions, SQLite
integrity, unchanged production database/configuration/loader, removed systemd
override, final active accepted daemon and zero VDR domain mutations.

Do not repeat this acceptance solely because a chat changes. Repeat only when a
directly relevant daemon, cleanup, schema, configuration, systemd execution or
acceptance-harness fingerprint changes.

## Fresh post-Slice-2W gap analysis

Exactly one next slice was selected after comparing security value,
dependencies, coherent owner set, source testability, runtime acceptance and
scope-expansion risk:

```text
Phase 62 Slice 2X
Protected Accountability Event Read
```

Binding contract:

- [Slice 2X — Protected Accountability Event Read](phase-62-slice-2x-protected-accountability-event-read.md)

Why it was selected:

- existing append-only evidence has no protected production read path;
- one bounded read consumes the accepted identity, authorization and
  accountability foundations without changing VDR domain state;
- query and mandatory audit-of-audit can fail closed before any rows are
  returned;
- the owner set is limited to the accountability repository, one read service,
  exact Security HTTP classification and one fixed serializer;
- export, pagination, filters, configurable redaction, retention and frontend
  work remain explicitly separate.

Why the other gaps were not selected:

- generic mutation outcomes are ambiguous after dispatch without stronger
  coupling or Outbox semantics;
- revisions, idempotency and durable operation lifecycle cross multiple mutable
  resource owners;
- security administration and native/service credentials require dangerous
  mutation and enrollment contracts;
- audit export/redaction/retention is a broader privacy and storage product;
- compatibility retirement remains a near-final dependency.

## Selected Slice 2X boundary

Exact endpoint:

```text
GET /api/security/accountability/events
GET /api/security/accountability/events?limit=<1..100>
default limit 50
newest first by recorded_at DESC, event_id DESC
```

Exact permission:

```text
security.audit.read@*
```

A direct exact grant or exact global `role.admin@*` may authorize the read.
Legacy Basic compatibility does not bypass this sensitive GET. Non-global admin
scope and `role.read-only` do not grant it.

The route returns only a fixed allowlist of already secret-free accountability
fields, uses `Cache-Control: no-store`, is handled before the general
`ApiRouter`, and has no frontend owner.

The existing pre-dispatch authorization event remains mandatory. The bounded
SELECT and one exact `operation.succeeded` audit-of-audit event execute in one
local `BEGIN IMMEDIATE` transaction. Query, append or commit failure returns no
event rows.

No configuration variable is added. The fixed default and maximum limits are
compile-time contract constants.

## Slice 2X explicit exclusions

- export, download, streaming, cursor pagination, offset, filters or arbitrary history traversal;
- configurable redaction, audit retention, deletion, compaction or archival;
- frontend audit viewer;
- actor, identity, session, credential, grant or role administration;
- generic mutation outcomes, Outbox or cross-domain transaction coupling;
- revisions, `If-Match`, idempotency or durable operation replay;
- native/service credential enrollment, rotation or revocation;
- compatibility retirement;
- Android, Android TV or Phase 63-67 runtime.

## Remaining Phase 62 gaps after Slice 2X selection

Still open beyond the selected read foundation:

- operation outcomes beyond browser lifecycle, cleanup and the local audit-read outcome;
- stronger transaction coupling or Outbox semantics;
- common revisions, idempotency and durable operation lifecycle;
- protected actor, identity, credential, grant and role administration;
- native/service credential enrollment, rotation and revocation;
- audit export, configurable redaction and retention;
- broader audit pagination/search if selected separately;
- compatibility retirement and final Phase 62 closeout.

## Selection validation gate

No Slice-2X production implementation may begin until the contract and canonical
status/gap/handoff documents are mutually consistent and all five jobs pass on
the final selection head:

- `docs-check`;
- `make-test-audit`;
- `frontend-regression-test`;
- `fast-regression-test`;
- `packaging-regression-test`.

The current selection commits contain documentation only. They do not change the
daemon, schema, route table, tests, packaging or runtime harness.

## Pull request truth

PR #117 must remain open, Draft and unmerged. Do not mark it Ready, merge it,
enable auto-merge, rebase, force-push or rewrite branch history without explicit
approval. Do not mutate Base, title, body, reviewers or other review/merge
metadata without explicit approval.

PR #118 remains the separate paused TVScraper workstream and must not be mixed
with Phase 62.

### Preferred edit path for new chats

Prefer direct GitHub repository updates for existing files when the connector can
perform the edit safely and the complete current file content is available.

Use local edits first only when the change requires:

- compilation or generated artifacts;
- focused local runtime tests;
- a capability not exposed by the GitHub connector;
- a workaround because the GitHub connector blocks a file operation.

Create small coherent commits with fast-forward-only semantics. Evaluate CI on
the final stabilization head rather than stopping after every intermediate
commit.

## Exact next action

Complete the Slice-2X selection documentation and require all five CI jobs on the
final selection head. After and only after that fully green gate, implement the
bounded Slice-2X contract. Do not start any excluded Phase-62 theme or advance
Android, Android TV or Phase 63-67 runtime.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Post-Slice-2W New Chat Prompt](phase-62-post-slice-2w-new-chat-prompt.md)
- [Slice 2X Selection Contract](phase-62-slice-2x-protected-accountability-event-read.md)
- [Slice 2W Runtime Closeout](phase-62-slice-2w-runtime-closeout.md)
- [Slice 2W Contract](phase-62-slice-2w-browser-session-retention-cleanup.md)
- [Phase 62 Runtime Evidence through Slice 2V](phase-62-runtime-evidence.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Agent Workflow Rules](../../AGENTS.md)
