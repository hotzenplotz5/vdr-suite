# Phase 62 Slice 2X — yaVDR Runtime Acceptance Runbook

## Status

Slice 2X production implementation and source-side validation are complete.
Real yaVDR installation and runtime acceptance are still pending.

This runbook is the only approved runtime path for Slice 2X. It does not close
the slice by itself and must not be treated as runtime evidence until the
command completes successfully and the generated evidence is recorded.

PR #117 remains open, Draft and unmerged.

## Safety boundary

The runtime entrypoint:

- requires a clean checkout on the exact expected branch and head;
- requires the candidate daemon hash and installed loader hash explicitly;
- stops the daemon before creating the evidence backup;
- backs up the installed daemon, loader, daemon configuration and a consistent
  SQLite copy of the production database;
- installs the candidate daemon atomically;
- starts the real systemd unit with a temporary runtime drop-in;
- points both `VDR_SUITE_DATABASE_PATH` and
  `VDR_SUITE_SECURITY_DATABASE_PATH` to an isolated scenario database;
- runs one real protected `200` outcome and one deterministic protected `500`
  outcome against that scenario database;
- restores grants, revokes the test browser session, removes the test-owned
  stale-probe row and drops the DELETE guard;
- stops the scenario daemon, removes the runtime drop-in and reloads systemd;
- verifies that the production database was unchanged during the scenario;
- keeps the candidate daemon only after a complete pass;
- restores the previously installed daemon after any failed acceptance or
  candidate production-restart failure;
- leaves the normal production service active at the end;
- never prints Authorization headers, cookies, CSRF tokens, verifier hashes or
  process environments.

No production VDR timer, recording, channel, EPG or remote-control mutation is
used.

## Selected real protected owner

The harness reuses the existing protected route:

```text
POST /api/epgsearch/native-fuzzy/stale-probes/delete
permission=epgsearch.native-fuzzy.stale-probes.delete
backend_scope=*
action=epgsearch.native-fuzzy.stale-probes.delete
```

Success proof:

- scenario contains no stale probe;
- route returns HTTP 200;
- exactly one `authorization.allowed` and one `operation.succeeded` event are
  persisted for the request.

Failure proof:

- the harness inserts one test-owned stale probe only into the scenario copy;
- a test-owned SQLite trigger blocks DELETE;
- route returns HTTP 500 with one delete failure;
- exactly one `authorization.allowed` and one `operation.failed` event are
  persisted for the request.

Both pairs must preserve actor, session, authentication state, permission,
backend scope, operation ID, request ID, correlation ID, action and allowed
decision context. The outcome reason codes must be `http_status_200` and
`http_status_500`.

## Required source gate

Before running the command below, the exact checkout head must have all five
required GitHub Actions jobs green:

- `docs-check`;
- `make-test-audit`;
- `frontend-regression-test`;
- `fast-regression-test`, including daemon build and both Slice-2X harness
  self-tests;
- `packaging-regression-test`.

Do not use an older CI run after the runtime-entrypoint fingerprint changes.

## Execution

Run from an already opened root shell on the real yaVDR host. The repository
must already be registered as a safe Git directory for that shell. Do not echo
or inspect the daemon process environment.

```bash
cd /home/yavdr/vdr-suite-phase62
git fetch origin phase-62-security-identity-foundation
git merge --ff-only origin/phase-62-security-identity-foundation
git status --short --branch
make daemon
HEAD="$(git rev-parse HEAD)"
SHORT_HEAD="$(git rev-parse --short=12 HEAD)"
DAEMON_SHA256="$(sha256sum .build/vdr-suite-daemon | awk '{print $1}')"
LOADER_SHA256="$(sha256sum /usr/share/vdr-suite/web/frontend/platform/deferred-runtime-loader.js | awk '{print $1}')"
EVIDENCE="/var/backups/vdr-suite-phase62-slice2x-$(date -u +%Y%m%dT%H%M%SZ)-${SHORT_HEAD}"
make phase62-protected-mutation-outcome-runtime-acceptance \
  PHASE62_ACCEPTANCE_BACKUP_DIR="${EVIDENCE}" \
  PHASE62_EXPECTED_BRANCH=phase62-pr117 \
  PHASE62_EXPECTED_HEAD="${HEAD}" \
  PHASE62_EXPECTED_DAEMON_SHA256="${DAEMON_SHA256}" \
  PHASE62_EXPECTED_LOADER_SHA256="${LOADER_SHA256}"
```

The runner intentionally rejects:

- an unclean worktree;
- a branch or head mismatch;
- a candidate hash mismatch;
- an existing evidence directory;
- an existing Slice-2X runtime drop-in;
- missing production database, daemon or loader files;
- a non-root execution;
- a production service that is not active before the run.

## Required successful output

A valid pass ends with:

```text
PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS
HEAD=<exact accepted head>
DAEMON_SHA256=<installed and running candidate hash>
LOADER_SHA256=<unchanged loader hash>
CONFIGURATION_SHA256=<unchanged daemon configuration hash>
EVIDENCE=<durable evidence directory>
FINAL_SERVICE_PID=<positive PID>
```

The inner report must additionally prove:

```text
protected_mutation_succeeded_outcome=yes
protected_mutation_failed_outcome=yes
outcome_context_continuity=yes
accountability_secret_free=yes
test_owned_stale_probe_removed=yes
target_grants_restored=yes
browser_session_revoked=yes
database_integrity=yes
service_pid_unchanged=yes
```

## Evidence files

The evidence directory contains at least:

- previous installed daemon;
- candidate daemon;
- previous installed loader;
- previous daemon configuration when present;
- consistent production-database backup;
- isolated scenario database;
- inner protected-mutation outcome report;
- outer installation/isolation/rollback report;
- production database logical fingerprint from before the scenario;
- `SHA256SUMS` covering the final evidence set.

After a pass, record the following in the Slice-2X closeout documents:

```text
accepted_head
source_ci_run_number
source_ci_run_id
daemon_sha256
loader_sha256
configuration_sha256
runtime_report_sha256
evidence_directory
final_service_pid
```

The PID is volatile. The accepted head, daemon, loader, configuration, report
and evidence fingerprints are the durable repetition gates.

## Failure handling

A failed run is not partial acceptance.

The wrapper restores the previously installed daemon whenever the inner
acceptance fails or the candidate cannot be restarted on the production
configuration. It removes the temporary systemd drop-in and restarts the normal
service.

If the final output is `FAIL`, do not rerun broadly. Inspect only the non-secret
`FAILURE_REASON`, outer report, inner report and systemd status. Fix the exact
harness, build or runtime cause, obtain green CI for any changed relevant
fingerprint and then repeat this bounded acceptance.

Do not restore the production database from the evidence copy unless an actual
integrity or unintended-mutation failure has first been demonstrated and the
service has been stopped.

## After a successful pass

A successful pass permits only:

1. Slice-2X runtime closeout documentation;
2. final CI on the documentation-only closeout head;
3. compatibility-retirement readiness and final Phase-62 closeout analysis.

It does not authorize an audit reader, security administration API, generic
Outbox, universal revision/idempotency framework, native/service credential
lifecycle, Android, Android TV or Phase 63-67 runtime work.
