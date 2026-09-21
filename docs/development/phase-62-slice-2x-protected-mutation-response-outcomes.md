# Phase 62 Slice 2X — Protected Mutation Response Outcomes

## Status

**Production implementation, focused tests, architecture guard and isolated
runtime harness are complete. Real yaVDR acceptance is still pending.**

PR #117 remains open, Draft and unmerged.

The source implementation is not yet an accepted installed runtime. Slice 2X may
be marked fully accepted only after the bounded yaVDR runbook succeeds and the
resulting fingerprints are recorded.

Runtime procedure:

- [Slice 2X yaVDR Runtime Acceptance Runbook](phase-62-slice-2x-runtime-acceptance-runbook.md)

## Why this work is necessary

The binding Phase-62 exit criterion requires:

```text
every privileged mutation has actor, decision and outcome evidence
```

Before Slice 2X, `SecurityHttpGate::appendDecisionEvent()` recorded only the
pre-dispatch authorization result. After an allowed protected POST,
`TestHttpServer::handleRequest()` called `ApiRouter::handleClientPost()` and
returned its response without persisting the observed business-mutation result.

An authorized success and an authorized returned router/backend/domain error
were therefore indistinguishable in accountability persistence.

Browser-session issue and logout outcomes from Slice 2S are separate and do not
cover ordinary protected business mutations.

## Implemented result

For every already-classified protected mutation that is authorized and reaches
`ApiRouter::handleClientPost()`, the HTTP owner appends exactly one outcome event
after the router returns:

```text
HTTP 200..299  -> event_type=operation.succeeded, outcome=succeeded
all other HTTP -> event_type=operation.failed,    outcome=failed
reason_code    -> http_status_<decimal status>
```

The outcome reuses the authorization context retained in
`SecurityGateDecision`:

- actor and actor type;
- device and session;
- authentication state;
- permission, backend scope and action;
- operation ID when present;
- request ID and correlation ID;
- `decision=allowed`.

No request body, response body, header, cookie, credential, CSRF token, verifier
hash, configuration value or process environment is copied into the event.

## Exact owner set

The implementation changes only the smallest coherent owner set:

- `SecurityGateDecision` retains the already-calculated authorization decision
  and operation ID after successful authorization;
- `SecurityHttpGate::appendProtectedMutationOutcome()` builds and appends the
  exact result event;
- `TestHttpServer::handleRequest()` invokes the append after
  `ApiRouter::handleClientPost()` returns and before delivering the original
  response;
- `AccountabilityEventRepository` remains the unchanged append-only persistence
  owner.

No new production route, permission, role, schema, index, repository, service,
configuration variable, frontend module or packaging component was added.

## Failure semantics

The accepted pre-dispatch behavior remains unchanged:

- required authorization-accountability append failure prevents dispatch.

For the new post-dispatch outcome append:

- the router has already completed;
- if persistence fails, the original router response is not delivered as a
  successfully accountable result;
- the server returns HTTP 503 `accountability_unavailable` with the existing
  request/correlation headers;
- the implementation does not claim rollback of an already executed external or
  domain side effect;
- the implementation does not claim that automatic replay is safe.

This closes the HTTP-observed response-result evidence gap. It does not add
cross-system crash atomicity, compensation, a transactional Outbox or a generic
idempotent operation framework.

## Source validation

Focused tests prove:

- one `operation.succeeded` event for a protected 2xx result;
- one `operation.failed` event for a protected non-2xx result;
- exact permission, backend, action, operation, request and correlation
  continuity;
- exact `http_status_204` and `http_status_503` reason examples;
- no outcome append for a non-protected allowed request;
- post-dispatch persistence failure returns 503 with request/correlation headers;
- no Authorization or session/CSRF secret enters accountability fields.

The architecture guard proves:

- authorization context is retained explicitly;
- the outcome call appears after POST dispatch and before final response;
- the path is guarded by `gate.protectedMutation`;
- success/failure event and reason markers remain exact;
- request headers, request bodies and response bodies are not event inputs;
- no protected audit-read route or `security.audit.read` permission is introduced.

## Runtime harness

The source tree contains two layers:

- `protected-mutation-outcome-runner.py` proves the event pairs and restores all
  scenario-owned database changes;
- `protected-mutation-outcome-runtime-entry.py` performs guarded candidate
  installation, evidence backup, temporary systemd isolation, rollback and final
  production-service restoration.

The entrypoint points both Suite and Security database variables to one isolated
SQLite copy. The selected real protected owner is the existing Native Fuzzy
stale-probe deletion route:

```text
success: no stale scenario row -> HTTP 200
failure: one test-owned stale row plus DELETE guard -> HTTP 500
```

No production VDR domain mutation is used. The test row, guard, temporary grant
and browser session are cleaned or revoked in the scenario database, and the
production database must remain unchanged during the scenario.

## Current source gate

The earlier implementation/harness head `4b61583b604626cd49e213356241759c81e60d04`
passed VDR-Suite CI #6871, Run ID `30750871845`, with all five jobs green.

The isolated runtime-entrypoint fingerprint was added after that run. Therefore
the **final current head must obtain a fresh all-green five-job CI run before any
real-runtime installation or acceptance**.

Required jobs:

- `docs-check`;
- `make-test-audit`;
- `frontend-regression-test`;
- `fast-regression-test`, including both Slice-2X harness self-tests and daemon
  build;
- `packaging-regression-test`.

## Runtime acceptance gate

The bounded yaVDR pass must prove:

1. the exact expected branch/head and a clean worktree;
2. candidate, installed and running daemon fingerprints;
3. unchanged loader and daemon configuration;
4. both database paths point to the isolated scenario copy;
5. one protected HTTP 200 result with an exact succeeded outcome pair;
6. one protected HTTP 500 result with an exact failed outcome pair;
7. exact context continuity and secret-free persistence;
8. removal of the test-owned stale row and DELETE guard;
9. restored target grants and revoked test browser session;
10. SQLite quick and foreign-key integrity;
11. production database unchanged during the scenario;
12. temporary systemd override removed;
13. normal production service active at the end;
14. automatic restoration of the previously installed daemon after any failed
    acceptance or failed candidate production restart.

## Explicit exclusions

Slice 2X does not implement or authorize:

- a protected audit read API or audit frontend;
- audit export, filtering, pagination, redaction, deletion or retention;
- generic actor, identity, credential, grant or role administration;
- native/service credential enrollment, rotation or revocation;
- transactional Outbox or generic cross-system commit atomicity;
- revisions, `If-Match`, idempotency keys or durable replay;
- compatibility retirement itself;
- Android, Android TV or Phase 63-67 runtime;
- PR Ready transition, merge, auto-merge or review-metadata changes.

## Exact next action

1. Make the canonical Current State, Current Status, Handoff, Gap Matrix, Roadmap
   and continuation prompt reflect the implemented/source-CI-complete but
   runtime-pending state.
2. Require all five jobs green on the final stabilization head containing the
   isolated runtime entrypoint.
3. Run the bounded yaVDR procedure from the linked runbook.
4. On a pass, create the Slice-2X runtime closeout and then evaluate only
   compatibility-retirement readiness and final Phase-62 closeout.

Do not select another implementation slice before this runtime gate is resolved.
