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

The runner supports both real upgrade shapes: an already claimed P2 deployment
and a pre-P2 legacy deployment whose production database does not yet contain
`security_human_accounts`. The latter is the state observed on the first real
yaVDR attempt; schema absence is therefore an expected upgrade input, not a
SQLite corruption condition.

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
- either exactly one eligible selected active Human Account administrator with
  an active `human-password` credential and verifier, or explicit
  `--bootstrap-first-admin` authorization for an unclaimed/pre-P2 deployment;
- when first-admin bootstrap is needed, the exact candidate
  `.build/vdr-suite-first-admin-bootstrap` fingerprint.

If several Human Account administrators are eligible, pass
`--human-login <login>` to select one explicitly. Selection happens before any
service or file mutation.

For an already claimed deployment, the Human Account password is read
interactively with `getpass` and checked against the persisted one-way verifier
before the daemon is stopped.

For a pre-P2 or unclaimed deployment, `--bootstrap-first-admin` is required.
The runner prompts interactively for the First Human Account login, display name
and password before mutation. The password is entered twice and is never
accepted as a command-line argument or environment variable. After the exact
candidate daemon starts in compatibility mode and initializes the P2 schema, the
runner invokes the exact built root-only bootstrap issuer against the production
Suite database and submits the returned one-time proof to the real
`/api/security/first-admin/claim` endpoint. The raw setup secret is held only
in process memory and is never written to the evidence report.

## Acceptance contract

The runner first installs the exact candidate daemon while retaining the
existing compatibility configuration. On a pre-P2 database this first candidate
startup is also the controlled schema initialization step. If the server is
unclaimed and `--bootstrap-first-admin` was authorized, the First Admin is
claimed before the Human Account baseline check. It then proves:

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
active human-password credential, verifier and grants. For an existing account
the baseline fingerprint is taken before daemon replacement. For a newly claimed
First Admin it is taken immediately after the atomic claim and before any
security-mode transition. It must be identical after the complete retirement
sequence.

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

If an unclaimed deployment successfully completes the atomic First Admin claim
but a later retirement step fails, that successful First Admin claim persists.
This is intentional: compatibility rollback changes authentication mode but does
not delete Human Accounts, credentials, grants or accountability history. The
prior daemon/configuration are still restored, and a rerun reuses the now
existing Human Account instead of creating another.

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
