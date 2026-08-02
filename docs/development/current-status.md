# VDR-Suite Current Project Status

## Current verified position

```text
Repository: hotzenplotz5/vdr-suite
Base: origin/main @ cb77ff66e11dca7db2eafa36525762dcde35102d
Active pull request: #117
PR state: open, Draft, unmerged, mergeable
Remote branch: phase-62-security-identity-foundation
Local yaVDR branch: phase62-pr117
Local checkout: /home/yavdr/vdr-suite-phase62

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

Accepted Slice-2W runtime marker:
PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE=PASS

Durable Slice-2W evidence:
/var/backups/vdr-suite-phase62-slice2w-20260802T114239Z-bb8609151313

Current bounded slice:
Slice 2X - Protected Mutation Response Outcomes

Current Slice-2X state:
production implementation complete;
focused tests complete;
architecture guard complete;
isolated installation/runtime harness complete;
real yaVDR runtime acceptance pending.
```

Phase 61 remains completed. Phase 62 remains active and incomplete. Phase 63-67
runtime has not been advanced.

## Completed-phase references

- [Phase 61 and Performance Closeout](phase-61-metadata-genre-performance-closeout.md)
- [Post-Phase-61 Platform Runtime Closeout](post-phase-61-platform-runtime-closeout.md)

## Completed platform markers

The current project truth continues to include:

- Phase 61 - Suite Metadata and Genre Platform;
- Post-Phase 61 Performance Hardening (B1-B4);
- VDR Remote and Live Overlay hardening (#110);
- Backend-scoped Global Search (#111);
- Phase 62 - Identity, RBAC and Accountability Foundation.

## Cumulative accepted Phase 62 runtime through Slice 2W

The installed and real-runtime accepted baseline includes:

- canonical actor, device, session, credential, request and correlation context;
- persistent identity and request-time lifecycle resolution;
- Legacy Basic compatibility, optional Managed Basic and browser sessions;
- strict cookie precedence and cookie-bound CSRF;
- exact actor grants and fixed exact-scope Admin/Read-only roles;
- protected Remote, Timer, Channel Move, Recording, SearchTimer, Native Fuzzy and
  query-scoped refresh mutation families;
- explicit Safe POST classification;
- immutable browser-session absolute lifetime;
- browser issue/revoke outcome accountability;
- issuing-credential lifecycle binding;
- optional per-actor browser-session concurrency limits;
- optional idle expiry with throttled activity persistence;
- bounded terminal browser-session retention cleanup with atomic secret-free
  accountability;
- guarded real-runtime acceptance and rollback tooling.

## Fully accepted Slice 2W

Slice 2W introduced:

```text
VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS
0                 disabled compatibility default
86400..31536000   enabled retention delay in seconds
fixed batch size  256
```

One bounded pass runs after security schema/configuration validation and before
`securityReady`. Eligibility is limited to explicit revocation, absolute expiry
and idle expiry beyond retention. Processing is deterministic and transactionally
rechecked; any enabled failure rolls back and leaves the Security Runtime fail
closed.

The real yaVDR pass proved all deletion/preservation boundaries, one exact
secret-free event per deleted verifier, the fixed 256-item bound, SQLite
integrity, restored systemd state, active final daemon and zero VDR domain
mutations.

Do not repeat Slice-2W acceptance without a directly relevant changed daemon,
cleanup, schema, configuration, systemd execution or harness fingerprint.

## Implemented Slice 2X

The binding requirement is:

```text
every privileged mutation has actor, decision and outcome evidence
```

The implementation closes the prior post-dispatch evidence gap:

```text
protected result 200..299 -> operation.succeeded / succeeded
protected result otherwise -> operation.failed / failed
reason_code -> http_status_<decimal status>
```

`SecurityGateDecision` retains the successful authorization decision and
operation ID. `SecurityHttpGate` constructs and appends the outcome.
`TestHttpServer` invokes it only after `ApiRouter::handleClientPost()` returns and
before the original response is delivered.

If the post-dispatch append fails, the HTTP response becomes 503
`accountability_unavailable`. The implementation does not claim domain rollback
or replay safety.

No new route, permission, role, schema, repository, configuration, frontend or
packaging component was introduced.

Binding documents:

- [Slice 2X Contract](phase-62-slice-2x-protected-mutation-response-outcomes.md)
- [Slice 2X yaVDR Runbook](phase-62-slice-2x-runtime-acceptance-runbook.md)

## Source validation truth

The earlier implementation/harness head
`4b61583b604626cd49e213356241759c81e60d04` passed:

```text
VDR-Suite CI #6871
Run ID 30750871845
all five jobs successful
```

The runtime path was subsequently strengthened with an installation entrypoint
that owns backup, candidate deployment, isolated dual-database systemd override,
rollback and final production-service restoration. Any head containing that new
fingerprint must pass a fresh complete five-job CI before runtime installation.

Required jobs:

- `docs-check`;
- `make-test-audit`;
- `frontend-regression-test`;
- `fast-regression-test`, including both Slice-2X harness self-tests and daemon
  build;
- `packaging-regression-test`.

## Runtime gate

The bounded real yaVDR pass must:

- run on the exact expected clean branch/head;
- back up old daemon, loader, configuration and production SQLite state;
- install the candidate daemon atomically;
- point both Suite and Security database paths at one isolated scenario copy;
- produce a real protected HTTP 200 success outcome pair;
- produce a deterministic protected HTTP 500 failure outcome pair;
- preserve exact actor/scope/operation/request/correlation context;
- prove secret-free accountability;
- remove the test-owned stale row and DELETE guard;
- restore grants and revoke the test browser session in the scenario;
- leave the production database untouched during the scenario;
- remove the systemd drop-in;
- keep the candidate only after a complete pass;
- restore the old daemon after any failed acceptance or failed candidate restart;
- leave the normal production service active.

## Necessity boundary

The following remain unproven and must not be implemented while Slice 2X is
pending runtime acceptance:

- protected audit read/export/filter/redaction/retention;
- generic identity, credential, grant or role administration;
- native/service credential lifecycle before a concrete client requires it;
- universal revision/idempotency/operation infrastructure;
- transactional Outbox or generic cross-system coupling.

After Slice-2X runtime acceptance, evaluate only compatibility-retirement
readiness and final Phase-62 closeout. Do not assume another implementation slice
is necessary.

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
- access to the installed yaVDR runtime;
- a capability not exposed by the GitHub connector;
- a workaround because the GitHub connector blocks a file operation.

Create small coherent commits with fast-forward-only semantics. Evaluate CI on
the final stabilization head rather than stopping after every intermediate
commit.

## Exact next action

1. Complete canonical documentation consistency for the implemented,
   runtime-pending Slice 2X state.
2. Require all five jobs green on the final stabilization head.
3. Run only the bounded real yaVDR procedure from the Slice-2X runbook.
4. If it passes, record hashes/evidence and create the Slice-2X runtime closeout.
5. Then perform compatibility-retirement readiness and final Phase-62 closeout
   analysis without presuming additional feature work.

## Authoritative links

- [Current State](../CURRENT.md)
- [New Chat Handoff](../NEW-CHAT-HANDOFF.md)
- [Slice 2X Contract](phase-62-slice-2x-protected-mutation-response-outcomes.md)
- [Slice 2X yaVDR Runbook](phase-62-slice-2x-runtime-acceptance-runbook.md)
- [Slice 2W Runtime Closeout](phase-62-slice-2w-runtime-closeout.md)
- [Phase 62 Gap Matrix](../planning/phase-62-security-identity-gap-matrix.md)
- [Security and Identity Architecture](../architecture/security-identity-foundation.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Agent Workflow Rules](../../AGENTS.md)
