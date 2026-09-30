# P2 Legacy Basic retirement real-runtime acceptance

## Purpose

This runbook defines the guarded real yaVDR migration/rollback acceptance that
must pass before the transitional Legacy Basic implementation can be deleted.

The sequence is:

```text
legacy-basic -> enforced -> legacy-basic -> enforced
```

A successful run deliberately ends with the existing deployment migrated to
`VDR_SUITE_SECURITY_MODE=enforced`. A failed run restores the exact pre-run
daemon binary and exact pre-run defaults-file state.

## Scope

The acceptance changes only:

- `/usr/sbin/vdr-suite-daemon`, replacing it with the exact built candidate;
- the single `VDR_SUITE_SECURITY_MODE` line in
  `/etc/default/vdr-suite-daemon`;
- normal browser-session/accountability rows produced by real authentication.

It restarts only `vdr-suite-daemon.service`. It does not restart VDR, the
Backend Agent or nginx and does not install frontend, SuiteBridge or HbbTV
artifacts.

The production SQLite database is never restored from a snapshot. That is
intentional: restoring a whole database could overwrite unrelated legitimate
runtime changes. Instead the runner verifies SQLite integrity and compares a
persistent Human Account identity fingerprint across the complete mode
transition.

## Preconditions

The runner requires:

- root;
- an exact clean repository head whose remote ref resolves to the same SHA;
- the exact candidate daemon already built from that head;
- exact current installed-daemon, defaults-file and service-PID fingerprints;
- exact hosted-CI run number and run ID for the candidate;
- an existing deployment whose effective mode is legacy compatibility
  (explicit `legacy-basic` or the historical missing-mode fallback);
- exactly one eligible selected active Human Account administrator with an
  active `human-password` credential and verifier.

If several Human Account administrators are eligible, pass
`--human-login <login>` to select one explicitly. Selection happens before any
service or file mutation.

The Human Account password is read interactively with `getpass`. The password
is read interactively and is never accepted as a command-line argument or
environment variable. It is checked against the persisted one-way verifier
before the daemon is stopped.

## Acceptance contract

The runner first installs the exact candidate daemon while retaining the
existing compatibility configuration. It then proves:

| Stage | Legacy Basic GET `/api/backends` | Human Account browser session |
|---|---:|---:|
| existing compatibility baseline | 200 | issue/read/logout succeeds |
| enforced | 401 | issue/read/logout succeeds |
| explicit rollback to legacy-basic | 200 | issue/read/logout succeeds |
| final enforced | 401 | issue/read/logout succeeds |

The defaults-file rewrite removes only existing
`VDR_SUITE_SECURITY_MODE=...` lines and appends the selected mode. All other
lines, including any stale `VDR_SUITE_BASIC_AUTH` and
`VDR_SUITE_LEGACY_BASIC_*` inputs, remain present. This is required evidence
that enforced mode disables compatibility even when old configuration remains.

The persistent identity fingerprint covers the selected Human Account, Actor,
active human-password credential, verifier and grants. It must be identical
before and after the transition.

The final daemon process must execute the exact candidate fingerprint, the
service must be active, SQLite quick-check and foreign-key-check must pass and
the final configuration must explicitly contain:

```text
VDR_SUITE_SECURITY_MODE=enforced
```

## Failure behavior

Before mutation, the runner stores a root-only evidence directory below
`/var/backups` containing the installed daemon backup, the defaults-file backup
when one existed, checksums and the runtime report.

On any failure after mutation starts, failure restores:

- the exact pre-run daemon binary;
- the exact pre-run defaults file, or removes the newly created file when the
  file was initially absent;
- the original daemon service state by starting the restored daemon and
  verifying its running binary fingerprint.

The runner never restores the production database. Browser sessions created by
the acceptance are logged out through the normal lifecycle whenever their
round-trip reaches that stage.

## Evidence

The report is secret-free. It records the exact head and hosted-CI identifiers,
daemon/configuration fingerprints, Human Account/Actor/login/credential
identifiers, the before/after persistent identity fingerprint, HTTP status codes,
SQLite integrity results, final security mode and evidence directory.

It does not record:

- the Human Account password or verifier;
- the Legacy Basic Authorization value;
- browser cookies;
- CSRF secrets.

The real deployment execution remains pending until this runner completes with:

```text
P2_LEGACY_BASIC_RETIREMENT_RUNTIME_ACCEPTANCE=PASS
FINAL_SECURITY_MODE=enforced
PERSISTENT_IDENTITY_UNCHANGED=PASS
```

Only after that evidence is retained is the next deletion slice for the
transitional Legacy Basic implementation justified.
