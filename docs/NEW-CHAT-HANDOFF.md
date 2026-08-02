# VDR-Suite New Chat Handoff

## Purpose

This is the canonical entry point for every new VDR-Suite chat. Read it before
repeating repository-wide analysis or real-runtime acceptance. A new chat alone
is not a reason to start over.

Trust completed items marked **VERIFIED** unless a directly relevant repository,
binary, configuration, database, systemd execution, routing or behaviour
fingerprint changed.

No implementation is selected merely because it appears useful. Every new
Phase-62 implementation must first prove a binding requirement, a concrete gap
in accepted code, a real failure/security consequence and the smallest closing
change.

## Canonical reading

- [Ready-to-copy post-Slice-2W prompt](development/phase-62-post-slice-2w-new-chat-prompt.md)
- [Current project truth](CURRENT.md)
- [Current project status](development/current-status.md)
- [Slice 2X — Protected Mutation Response Outcomes](development/phase-62-slice-2x-protected-mutation-response-outcomes.md)
- [Slice 2W runtime closeout](development/phase-62-slice-2w-runtime-closeout.md)
- [Slice 2W accepted contract](development/phase-62-slice-2w-browser-session-retention-cleanup.md)
- [Phase 62 runtime evidence through Slice 2V](development/phase-62-runtime-evidence.md)
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

Current State, Current Status, this Handoff, the Gap Matrix and the Slice-2X
contract are the active selection authorities. Slice-2W runtime evidence remains
accepted and must not be repeated without a relevant changed fingerprint.

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

Next strict runtime phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Current active phase:
Phase 62 - Identity, RBAC and Accountability Foundation

Phase 62 state:
active and incomplete

Repository, source CI and real-runtime acceptance complete through:
Slice 2W - Browser-Session Terminal Retention Cleanup

Selected next bounded implementation slice:
Slice 2X - Protected Mutation Response Outcomes

Slice 2X state:
contract/documentation only; no production implementation

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

Accepted Slice-2W source/runtime head:
bb8609151313c613d403b88b1b4c3f55453a93e2

Accepted Slice-2W source CI:
VDR-Suite CI #6834
Run ID 30745952119
All five jobs successful
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30745952119

Runtime acceptance:
PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE=PASS

Continuation prompt:
docs/development/phase-62-post-slice-2w-new-chat-prompt.md
```

PR #117 must remain open and Draft. Do not mark it Ready, merge it, enable
auto-merge, rebase, force-push, rewrite branch history or mutate Base, title,
body, reviewers or other review/merge metadata without explicit approval.

PR #118 is the separate paused TVScraper bugfix workstream. Do not mix its code,
daemon fingerprint or planning into Phase 62 commits or evidence.

## Installed real-runtime truth

**VERIFIED on 2026-08-02 after successful Slice-2W acceptance:**

```text
Daemon unit:
vdr-suite-daemon.service

Installed executable:
/usr/sbin/vdr-suite-daemon

Installed/running daemon SHA-256:
7775804306bf70eca6ef23474605467381162cfc9d5b874cdb187840ca8bc571

Installed deferred-runtime-loader.js SHA-256:
3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a

Daemon configuration SHA-256:
8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b

Runtime evidence:
/var/backups/vdr-suite-phase62-slice2w-20260802T114239Z-bb8609151313

Runtime report SHA-256:
e0fbe1689b2f48e75bb4ae6836b227d7da92e08d53b009ac1c2cb371a36c74ea

Final service PID at acceptance:
89965
```

The PID and service timestamps are volatile. The accepted head, binary, loader,
configuration, report and evidence fingerprints are durable repetition gates.

The accepted daemon remains installed and active. The production database,
daemon configuration and deferred loader were unchanged by the isolated
acceptance. The runtime-only systemd drop-in was removed from the final unit.

## Fully accepted Slice 2W

**VERIFIED:**

```text
VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS
0                 disabled compatibility default
86400..31536000   enabled retention delay
fixed batch size  256
```

Exactly one bounded cleanup pass runs during Security Runtime initialization
after all security schemas and configuration are validated and before
`securityReady`.

Terminal eligibility is limited to the verifier's own explicit revocation,
absolute expiry or idle expiry beyond retention. Issuer revocation alone is not
a cleanup trigger.

One `BEGIN IMMEDIATE` transaction rechecks eligibility, appends exact secret-free
`browser.session.cleanup` accountability, deletes the verifier and deletes only
its unreferenced canonical browser session and exact-type `browser-session`
credential. Actor, device, issuing credential, grants, roles and accountability
history are preserved. Any enabled-policy failure rolls back the complete batch
and leaves the Security Runtime fail closed.

The real-runtime pass proved fresh schema creation, disabled no-op, full rollback
on forced accountability failure, every preservation/deletion boundary, exact
secret-free events, the fixed 256-item limit, SQLite integrity, unchanged
production database/configuration/loader, removed runtime override, active final
daemon and zero VDR domain mutations.

Do not repeat Slice-2W runtime acceptance merely because a chat changes. Repeat
only after a directly relevant daemon, cleanup implementation, schema,
configuration, systemd execution or acceptance-harness fingerprint changes.

## Selected Slice 2X — proof and boundary

The selected work is not an audit product. It closes one explicit accountability
requirement:

```text
every privileged mutation has actor, decision and outcome evidence
```

Current accepted code proves the gap:

- `SecurityHttpGate::appendDecisionEvent()` persists only
  `dispatch_authorized` or `dispatch_denied` before dispatch;
- an allowed protected POST is then sent to `ApiRouter::handleClientPost()`;
- the returned success or failure has no business-mutation outcome event;
- browser-session issue/logout outcomes exist separately and explicitly exclude
  ordinary business mutations.

Therefore an authorized success and an authorized returned error are currently
indistinguishable in accountability persistence.

Slice 2X adds exactly one post-router event for existing protected mutations:

```text
HTTP 200..299  -> operation.succeeded / succeeded
all other HTTP -> operation.failed    / failed
reason_code    -> http_status_<decimal status>
```

It reuses the existing actor, device, session, permission, backend, action,
operation, request and correlation context.

It adds no route, permission, role, schema, repository, index, configuration,
frontend or packaging component.

If the post-dispatch append fails, the original response is replaced by HTTP 503
`accountability_unavailable`. The slice does not claim rollback of an already
executed external/domain side effect or safe automatic replay.

Binding details, tests, guard and exclusions are in:

- [Slice 2X — Protected Mutation Response Outcomes](development/phase-62-slice-2x-protected-mutation-response-outcomes.md)

## Rejected and unproven work

The earlier protected audit-read proposal was removed. No Phase-62 exit criterion
requires an HTTP audit reader and no concrete failure was demonstrated from its
absence.

Do not implement any of the following without a new requirement-to-code failure
proof:

- protected audit read/export/filter/redaction/retention;
- generic identity, credential, grant or role administration;
- native/service credential lifecycle before a real consumer exists;
- universal revision, idempotency or durable operation infrastructure;
- transactional Outbox or generic cross-system coupling.

After Slice 2X acceptance, evaluate only compatibility-retirement readiness and
final Phase-62 closeout. Do not assume another implementation slice is needed.

## Credential and secret restrictions

The Managed-Basic plaintext password used for earlier acceptance is unavailable.
Do not rotate or reprovision credentials implicitly.

Never print, store or commit:

- Authorization headers;
- plaintext passwords or password hashes;
- cookies or complete cookie values;
- CSRF tokens;
- raw session or verifier secrets;
- login response bodies containing secrets;
- process environments.

## Anti-loop boundary

Do not repeat completed repository analysis, public-origin work, route migration,
Slice-2R lifetime, Slice-2S browser lifecycle outcomes, Slice-2T issuer binding,
Slice-2U concurrency, Slice-2V idle expiry or Slice-2W terminal retention unless
a directly relevant accepted fingerprint or contract changes.

Do not rerun the rejected audit-read selection analysis unless a new concrete
requirement or failure case exists.

## Exact next action

1. Complete consistency of Current State, Current Status, Handoff, Gap Matrix and
   the Slice-2X contract.
2. Require all five GitHub Actions jobs on the final selection head:
   `docs-check`, `make-test-audit`, `frontend-regression-test`,
   `fast-regression-test`, `packaging-regression-test`.
3. If any required job fails, fix only the demonstrated cause.
4. After all five jobs are green, implement only Slice 2X.
5. Do not perform a real-runtime acceptance for documentation-only selection
   changes.
6. After implementation and green source CI, use a bounded rollback-safe yaVDR
   acceptance for one successful and one deterministic failed already-protected
   mutation.

Do not add an audit reader, administration API, Outbox, generic operation
framework, native/service lifecycle, Android, Android TV or Phase 63-67 runtime.

## Maintenance rule

Update this file whenever repository, PR, runtime, routing or next-action truth
changes. Preserve durable non-secret evidence and keep the next permitted action
explicit.
