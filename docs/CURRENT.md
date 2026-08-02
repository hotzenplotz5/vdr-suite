# VDR-Suite Current State

## Navigation

- [Documentation Index](index.md)
- [New Chat Handoff](NEW-CHAT-HANDOFF.md)
- [Current Project Status](development/current-status.md)
- [Slice 2X Contract](development/phase-62-slice-2x-protected-mutation-response-outcomes.md)
- [Slice 2X yaVDR Runbook](development/phase-62-slice-2x-runtime-acceptance-runbook.md)
- [Slice 2W Runtime Closeout](development/phase-62-slice-2w-runtime-closeout.md)
- [Phase 62 Gap Matrix](planning/phase-62-security-identity-gap-matrix.md)
- [Strict Roadmap](planning/roadmap.md)
- [Phase Map](planning/phase-map.md)
- [VDR Ecosystem Parity](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture Gap Matrix](planning/architecture-audit-gap-matrix.md)
- [Security and Identity Architecture](architecture/security-identity-foundation.md)
- [ADR Index](adr/index.md)
- [Phase 61 and Performance Closeout](development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](development/post-phase-61-platform-runtime-closeout.md)
- [Completed Phases](development/completed-phases.md)

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Base: main @ cb77ff66e11dca7db2eafa36525762dcde35102d
Active PR: #117
PR state: open, Draft, unmerged, mergeable
Remote branch: phase-62-security-identity-foundation
Local yaVDR branch: phase62-pr117
Local checkout: /home/yavdr/vdr-suite-phase62

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

Repository, source CI and real-runtime acceptance complete through:
Slice 2W - Browser-Session Terminal Retention Cleanup

Accepted Slice-2W source/runtime head:
bb8609151313c613d403b88b1b4c3f55453a93e2

Accepted Slice-2W runtime marker:
PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE=PASS

Accepted Slice-2W durable evidence:
/var/backups/vdr-suite-phase62-slice2w-20260802T114239Z-bb8609151313

Current bounded slice:
Slice 2X - Protected Mutation Response Outcomes

Slice 2X state:
production implementation complete;
focused tests and architecture guard complete;
isolated install/runtime harness complete;
real yaVDR acceptance pending.

Phase 63-67 runtime:
not advanced
```

Phase 62 remains active and incomplete. No new implementation slice is selected
beyond Slice 2X.

## Accepted security request and lifecycle path through Slice 2W

```text
HTTP request
  -> strict browser-cookie precedence when present
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

## Fully accepted Slice 2W

Slice 2W added one bounded startup retention pass for terminal browser-session
verifiers:

```text
VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS
0                 disabled compatibility default
86400..31536000   enabled retention delay
fixed batch size  256
```

Its guarded real-yaVDR pass proved exact eligibility, deterministic bounded
processing, transaction rollback, canonical-row preservation, secret-free
accountability, unchanged production runtime inputs and zero VDR domain
mutations.

Do not repeat Slice-2W acceptance without a directly relevant daemon, cleanup,
schema, configuration, systemd execution or harness fingerprint change.

## Slice 2X implemented source result

The binding Phase-62 requirement is:

```text
every privileged mutation has actor, decision and outcome evidence
```

Slice 2X now appends one post-router event for every already-protected,
authorized mutation:

```text
HTTP 200..299  -> operation.succeeded / succeeded
all other HTTP -> operation.failed    / failed
reason_code    -> http_status_<decimal status>
```

The event reuses the already-authorized actor, device, session, authentication,
permission, backend, action, operation, request and correlation context.

`TestHttpServer` invokes the append only after `ApiRouter::handleClientPost()`
returns and before the original response is delivered. If the outcome append
fails, the client receives HTTP 503 `accountability_unavailable`; no rollback or
safe-retry claim is made for a domain effect that already occurred.

No route, permission, role, schema, repository, configuration, frontend or
packaging owner was added.

## Slice 2X validation state

The source implementation, focused gate tests and architecture guard passed all
five jobs on the earlier implementation/harness head
`4b61583b604626cd49e213356241759c81e60d04`:

```text
VDR-Suite CI #6871
Run ID 30750871845
all five jobs successful
```

After that run, the runtime path was strengthened with an isolated installation
entrypoint and a rollback-safe runbook. Therefore the current final
stabilization head must have a fresh all-green five-job CI run before yaVDR
installation.

The entrypoint:

- backs up the installed daemon, loader, configuration and production database;
- installs the candidate atomically;
- points both Suite and Security database paths to an isolated copy;
- exercises one real protected HTTP 200 result and one deterministic HTTP 500
  result;
- verifies exact authorization/outcome pairs and secret-free continuity;
- removes all scenario-owned test state and the systemd drop-in;
- restores the old daemon after a failed acceptance or failed candidate restart;
- leaves the normal production service active.

## Remaining Phase 62 decision boundary

Mandatory next gate:

1. all five CI jobs green on the final current head;
2. one bounded real yaVDR Slice-2X acceptance using the linked runbook;
3. Slice-2X runtime closeout documentation;
4. compatibility-retirement readiness and final Phase-62 closeout analysis.

Not currently proven necessary:

- protected audit reads, export, filters, redaction or retention;
- generic security administration;
- native/service credential lifecycle before a real client requires it;
- universal revisions, idempotency or durable operation framework;
- transactional Outbox or generic cross-system coupling.

Do not invent another implementation slice after Slice 2X without a new binding
requirement, concrete accepted-code gap and real failure case.

## Operating rules

- Root-level `AGENTS.md` is binding.
- Prefer GitHub-first edits when the connector can safely complete them.
- Continue through approved bounded work without artificial pauses.
- Evaluate CI on the final stabilization head.
- Every CI report includes direct link, run number, run ID, exact head, overall
  status and all five job statuses.
- PR #117 remains open, Draft and unmerged.
- Do not mark it Ready, merge, auto-merge, rebase, force-push or rewrite history.
- Do not mutate PR Base, title, body, reviewers or review state without explicit
  approval.
- Do not repeat accepted runtime work merely because the chat changed.
- Do not pull Android, Android TV or Phase 63-67 runtime into Phase 62.

## Exact next action

Require the final current head to pass all five GitHub Actions jobs. After that,
run only [the Slice-2X yaVDR acceptance](development/phase-62-slice-2x-runtime-acceptance-runbook.md).

Do not add an audit reader, administration API, Outbox, generic operation
framework or native/service lifecycle while the runtime gate is pending.
