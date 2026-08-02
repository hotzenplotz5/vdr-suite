# Phase 62 Slice 2W — Runtime Closeout

## Status

**Fully accepted on the real yaVDR runtime on 2026-08-02.**

Slice 2W closes the bounded browser-session terminal-retention gap selected
after Slice 2V. It does not close Phase 62 as a whole and does not advance
Phase 63-67 runtime.

PR #117 remains open, Draft and unmerged.

## Accepted scope

Slice 2W adds exactly one bounded physical-retention pass during Security
Runtime initialization.

```text
VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS
0                 disabled compatibility default
86400..31536000   enabled retention delay in seconds
fixed batch size  256
```

The pass runs after all security schemas and the complete security
configuration have been validated, and before `securityReady` becomes true.
There is no scheduler, background thread or request-path cleanup.

Terminal eligibility is limited to the browser verifier's own state beyond the
configured retention period:

- explicit revocation;
- absolute expiry;
- idle expiry when the accepted idle policy is enabled.

Candidates are ordered by oldest terminal time and then `token_id`. At most 256
browser lifecycles are processed in one startup pass.

## Atomic ownership contract

One `BEGIN IMMEDIATE` transaction:

1. selects and rechecks terminal eligibility;
2. appends one secret-free cleanup accountability event per deleted verifier;
3. deletes the browser verifier;
4. deletes its canonical session only when it still belongs to the selected
   lifecycle and is unreferenced;
5. deletes its canonical credential only when it is exactly type
   `browser-session`, still belongs to the selected actor and is unreferenced;
6. preserves actors, devices, issuing credentials, grants, roles and all
   accountability history;
7. rolls back the whole batch after any SQL, foreign-key or accountability
   failure.

Cleanup is not triggered solely by issuing-credential revocation and is not used
as automatic eviction for the concurrent-session limit.

## Accountability contract

Each deleted browser verifier receives exactly one event:

```text
event_type=operation.succeeded
classes=security,lifecycle,maintenance
actor_type=system
authentication_state=system-maintenance
action=browser.session.cleanup
decision=completed
reason_code=browser_session_retention_elapsed
outcome=deleted
```

Evidence and accountability exclude session secrets, CSRF secrets, verifier
hashes, cookies, Authorization headers, raw configuration and process
environments.

## Accepted repository and CI baseline

```text
Accepted source/runtime head:
bb8609151313c613d403b88b1b4c3f55453a93e2

VDR-Suite CI #6834
Run ID: 30745952119
Result: completed successfully
Direct URL:
https://github.com/hotzenplotz5/vdr-suite/actions/runs/30745952119
```

All five jobs were successful:

- `docs-check`;
- `make-test-audit`;
- `frontend-regression-test`;
- `fast-regression-test`, including the runtime-harness self-tests and daemon
  build;
- `packaging-regression-test`.

The accepted daemon build fingerprint did not change during the two
runtime-harness-only stabilization fixes.

## Runtime acceptance

The guarded real-runtime runner completed with:

```text
PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE=PASS
HEAD=bb8609151313c613d403b88b1b4c3f55453a93e2
DAEMON_SHA256=7775804306bf70eca6ef23474605467381162cfc9d5b874cdb187840ca8bc571
LOADER_SHA256=3758aba3c9f87c99751bb59408f69f852579581e2f8251c720b3b7845f75399a
CONFIGURATION_SHA256=8faffe1a18f996681d6ca5f438df9e47626f8992e8cd8d1b67e0c25b1895ed6b
RUNTIME_REPORT_SHA256=e0fbe1689b2f48e75bb4ae6836b227d7da92e08d53b009ac1c2cb371a36c74ea
EVIDENCE=/var/backups/vdr-suite-phase62-slice2w-20260802T114239Z-bb8609151313
FINAL_SERVICE_PID=89965
```

The process ID is volatile. The binary, loader, configuration and report
fingerprints are durable acceptance gates.

## Proved runtime scenarios

The acceptance used only short-lived isolated SQLite databases. Both
`VDR_SUITE_DATABASE_PATH` and `VDR_SUITE_SECURITY_DATABASE_PATH` pointed at the
scenario database while each scenario ran. The production database was backed
up and integrity-checked, but was never used as a cleanup scenario.

The successful pass proved:

- fresh security-schema initialization;
- disabled retention performs no deletion and emits no cleanup accountability;
- enabled cleanup remains unavailable and fail closed when accountability append
  is forced to fail;
- the forced failure rolls back all verifier, session, credential and audit
  changes;
- active browser lifecycles are preserved;
- revoked, absolute-expired and idle-expired lifecycles inside retention are
  preserved;
- revoked, absolute-expired and idle-expired lifecycles beyond retention are
  deleted;
- issuer revocation alone does not trigger descendant cleanup;
- a non-`browser-session` credential is preserved even when its verifier is
  deleted;
- a canonical session or browser credential that becomes referenced again
  inside the cleanup transaction is preserved;
- exactly one secret-free cleanup event is stored for every deleted verifier;
- 258 eligible lifecycles produce exactly 256 deletions in deterministic order,
  leaving the final two candidates untouched;
- every scenario database passes SQLite quick and foreign-key checks;
- the production database fingerprint is unchanged;
- the daemon configuration and deferred loader are unchanged;
- the runtime-only systemd override is removed;
- the final service runs the accepted new daemon from
  `/usr/sbin/vdr-suite-daemon`;
- zero VDR domain mutations occur.

## Harness stabilization evidence

Two failed attempts did not invalidate or partially complete the runtime
acceptance:

1. The first failure was a fixture-only `sqlite3.IntegrityError`: the fixture
   attempted to violate the real verifier table's unique session and credential
   constraints. The fixture was corrected to reproduce the intended
   re-reference race through an `AFTER DELETE` trigger.
2. The second failure was a process-observation race for the `Type=simple`
   systemd unit. The watcher was hardened to wait for `MainPID=0` after stop and
   for a stable PID whose `/proc/<pid>/exe` resolves to the real ExecStart path
   after `execve()`.

Both failed runs automatically removed the runtime override, restored the old
installed daemon and configuration, and restarted the service. No cleanup
scenario was accepted from either failed run. The final pass used the corrected
harness fingerprint and green CI #6834.

## Installed-runtime baseline after acceptance

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
```

Do not repeat Slice-2W runtime acceptance merely because a chat changes. Repeat
only after a directly relevant daemon, cleanup implementation, schema,
configuration, service-execution or acceptance-harness fingerprint changes.

## Explicitly still open

Slice 2W does not add or accept:

- periodic cleanup scheduling;
- session listing, logout-all or security administration API/UI;
- cleanup based solely on issuer revocation;
- automatic concurrency-limit eviction;
- protected actor, identity, credential, grant or role administration;
- native/service credential enrollment, rotation or revocation;
- generic operation outcomes, stronger coupling or Outbox;
- common revisions, idempotency or durable operation lifecycle;
- protected audit reads, export, redaction or audit-retention products;
- compatibility retirement;
- Android, Android TV or Phase 63-67 runtime.

## Exact next action

Perform one fresh post-Slice-2W Phase-62 gap analysis against the accepted
repository and runtime baseline. Select and document exactly one bounded next
slice before implementing more Phase-62 runtime work.

Do not reopen terminal browser-session retention unless a relevant accepted
fingerprint changes. Keep PR #117 open, Draft and unmerged.