# VDR-Suite New Chat Handoff

## Purpose

This is the canonical entry point for every new VDR-Suite chat. Read it before
repeating repository analysis, CI or real-runtime acceptance. A new chat alone
is not a changed fingerprint.

Trust completed items marked **VERIFIED** until a directly relevant repository,
binary, configuration, database, systemd execution, routing or behavior
fingerprint changes.

No new Phase-62 implementation is selected merely because it appears useful.
Every feature requires a binding requirement, a concrete accepted-code gap, a
real failure/security consequence and the smallest closing change.

## Canonical reading

- [Current project truth](CURRENT.md)
- [Current project status](development/current-status.md)
- [Slice 2X contract](development/phase-62-slice-2x-protected-mutation-response-outcomes.md)
- [Slice 2X yaVDR runbook](development/phase-62-slice-2x-runtime-acceptance-runbook.md)
- [Post-Slice-2W continuation prompt](development/phase-62-post-slice-2w-new-chat-prompt.md)
- [Slice 2W runtime closeout](development/phase-62-slice-2w-runtime-closeout.md)
- [Phase 62 gap matrix](planning/phase-62-security-identity-gap-matrix.md)
- [Security and identity architecture](architecture/security-identity-foundation.md)
- [Strict roadmap](planning/roadmap.md)
- [Phase map](planning/phase-map.md)
- [Parity and frontend gap roadmap](planning/parity-audit-and-frontend-gap-roadmap.md)
- [Architecture audit gap matrix](planning/architecture-audit-gap-matrix.md)
- [Completed phases](development/completed-phases.md)
- [Phase 61 closeout](development/phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 runtime closeout](development/post-phase-61-platform-runtime-closeout.md)
- [ADR index](adr/index.md)
- [Agent workflow rules](../AGENTS.md)

Current State, Current Status, this Handoff, the Gap Matrix, Roadmap and Slice-2X
contract own the active truth.

## Stable project position

```text
Latest completed numbered runtime phase:
Phase 61 - Suite Metadata and Genre Platform

Completed hardening:
Post-Phase 61 Performance Hardening (B1-B4)

Historical umbrella track:
Phase 58 - Frontend and Live Parity

Completed platform features:
VDR Remote and Live Overlay hardening (#110)
Backend-scoped Global Search (#111)
Configurable photorealistic VDR Remote (#115)

Current active phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Phase 62 state:
active and incomplete

Repository, source CI and real-runtime acceptance complete through:
Slice 2W - Browser-Session Terminal Retention Cleanup

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

## Active workstream

```text
Repository: hotzenplotz5/vdr-suite
Checkout: /home/yavdr/vdr-suite-phase62
Local branch: phase62-pr117
Remote branch: phase-62-security-identity-foundation
Pull request: #117
Base: main @ cb77ff66e11dca7db2eafa36525762dcde35102d
PR state: open, Draft, unmerged, mergeable
```

PR #117 must remain open and Draft. Do not mark it Ready, merge it, enable
auto-merge, rebase, force-push, rewrite history or mutate Base, title, body,
reviewers or other review/merge metadata without explicit approval.

PR #118 is the separate paused TVScraper workstream. Do not mix its code,
binaries, fingerprints, planning or runtime evidence into Phase 62.

## Installed runtime baseline

**VERIFIED after Slice 2W acceptance on 2026-08-02:**

```text
Installed executable:
/usr/sbin/vdr-suite-daemon

Installed/running Slice-2W daemon SHA-256:
7775804306bf70eca6ef23474605467381162cfc9d5b874cdb187840ca8bc571

Installed deferred-runtime-loader.js SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Daemon configuration SHA-256:
8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b

Slice-2W evidence:
/var/backups/vdr-suite-phase62-slice2w-20260802T114239Z-bb8609151313
```

These remain the pre-Slice-2X installation baseline. The service PID is volatile.
Do not claim that the Slice-2X candidate is installed until the runtime runbook
passes.

## Fully accepted Slice 2W

**VERIFIED:** bounded startup cleanup for terminal browser sessions, exact
eligibility and deterministic 256-item processing, transaction rollback,
canonical-row preservation, secret-free accountability, restored systemd state,
SQLite integrity and zero VDR domain mutations.

Do not repeat Slice-2W acceptance unless a directly relevant daemon, cleanup,
schema, configuration, systemd execution or harness fingerprint changes.

## Implemented Slice 2X source contract

The binding requirement is:

```text
every privileged mutation has actor, decision and outcome evidence
```

Before Slice 2X, protected business POSTs persisted only pre-dispatch
`dispatch_authorized`/`dispatch_denied`; success and returned failure were
indistinguishable.

The implementation now records exactly one post-router event:

```text
HTTP 200..299  -> operation.succeeded / succeeded
all other HTTP -> operation.failed    / failed
reason_code    -> http_status_<decimal status>
```

It reuses actor, device, session, authentication, permission, backend, action,
operation, request and correlation context. It adds no route, permission, role,
schema, repository, configuration, frontend or packaging owner.

If outcome persistence fails after dispatch, the server returns HTTP 503
`accountability_unavailable`. No domain rollback or safe automatic replay is
claimed.

## Source and CI truth

The earlier implementation/harness head
`4b61583b604626cd49e213356241759c81e60d04` passed:

```text
VDR-Suite CI #6871
Run ID 30750871845
all five jobs successful
```

After that run, the runtime path was hardened with:

- `protected-mutation-outcome-runtime-entry.py`;
- automatic backup of old daemon, loader, configuration and production SQLite;
- atomic candidate installation;
- temporary systemd override for both Suite and Security database paths;
- an isolated scenario copy;
- rollback to the old daemon after failed acceptance or candidate restart;
- final production-service restoration;
- a dedicated yaVDR runbook.

The final current head containing that fingerprint must pass all five jobs before
runtime installation. Do not rely only on CI #6871 for the newer runtime-entry
fingerprint.

## Runtime acceptance boundary

Use only:

- [Slice 2X yaVDR Runtime Acceptance Runbook](development/phase-62-slice-2x-runtime-acceptance-runbook.md)

The selected real protected owner is the existing Native Fuzzy stale-probe
delete route. The isolated scenario must produce:

```text
no stale row                        -> HTTP 200 -> operation.succeeded
one test stale row + DELETE guard  -> HTTP 500 -> operation.failed
```

Required proof includes exact event-pair continuity, secret-free persistence,
scenario cleanup, restored grants, revoked test session, production database
unchanged during the scenario, removed systemd override, unchanged loader and
configuration, and an active final production service.

A failed attempt is not partial acceptance. Fix only the demonstrated cause and
repeat only after any changed relevant fingerprint has green CI.

## Rejected and unproven work

Do not implement without a new full necessity proof:

- protected audit read/export/filter/redaction/retention;
- generic identity, credential, grant or role administration;
- native/service credential lifecycle before a real consumer exists;
- universal revisions, idempotency or durable operation infrastructure;
- transactional Outbox or generic cross-system coupling.

After Slice-2X acceptance, evaluate only compatibility-retirement readiness and
final Phase-62 closeout. Do not assume another implementation slice is required.

## Credential and secret restrictions

Never print, store or commit:

- Authorization headers;
- plaintext passwords or password hashes;
- cookies or complete cookie values;
- CSRF tokens;
- raw session or verifier secrets;
- login response bodies containing secrets;
- process environments.

## Anti-loop boundary

Do not repeat completed public-origin work, route migration, Slice-2R lifetime,
Slice-2S browser outcomes, Slice-2T issuer binding, Slice-2U concurrency,
Slice-2V idle expiry or Slice-2W retention without a changed relevant
fingerprint.

Do not rerun the rejected audit-read analysis unless a new requirement or failure
case exists.

## Exact next action

1. Finish canonical documentation consistency for the implemented,
   runtime-pending Slice 2X state.
2. Require `docs-check`, `make-test-audit`, `frontend-regression-test`,
   `fast-regression-test` and `packaging-regression-test` green on the final
   current head.
3. Run the bounded yaVDR acceptance from the Slice-2X runbook.
4. Record accepted head, CI, daemon/loader/configuration/report hashes, evidence
   directory and final service PID.
5. Create the Slice-2X runtime closeout.
6. Then evaluate compatibility-retirement readiness and final Phase-62 closeout
   only.

## Maintenance rule

Update this file whenever repository, PR, runtime, routing or next-action truth
changes. Preserve durable non-secret evidence and keep the next permitted action
explicit.
