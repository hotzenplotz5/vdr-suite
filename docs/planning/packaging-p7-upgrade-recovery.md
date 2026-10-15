# P7 — Upgrade/recovery policy and preflight

Status: policy, **read-only inspection** and a private-workspace **offline backup/restore rehearsal** implemented. No production migration, automated live backup, or supported downgrade path.

## Safety contract

1. Stop write producers only during a separately approved controlled maintenance window. Package installation must not assume it can safely stop a running VDR on first install.
2. Inspect SQLite database with `python3 tools/packaging_upgrade_preflight.py --database /path/to/offline-copy.db`. The preflight is strictly read-only and uses SQLite immutable mode. **It now rejects any `-wal`, `-shm` or `-journal` sidecar, including symlinks**, because immutable inspection can silently ignore committed WAL transactions and report a stale but superficially valid schema. Use an already quiesced, checkpointed offline snapshot; do not simply delete sidecars or inspect a live file. The repeated sidecar check narrows but cannot eliminate races with active writers. This is not a consistent backup.
3. For an actual backup use SQLite's supported online backup API or safely shut down writers, and verify integrity of the resulting backup. Include SQLite WAL changes; never simply copy a live main `.db` file while a WAL is active.
4. Record source version, target package version, schema version, backup location and file hashes; protect secrets and backups using least privilege.
5. Install/upgrade only in an isolated test environment until schema migration is proven idempotent, transactional and safely resumable.
6. Recover by stopping application writes and restoring a verified, compatible backup before reverting binaries. A newer DB schema must not be opened by an older unsupported binary.
7. Do not delete persistent databases during package `remove`; `purge` must never silently destroy recorded content.

## Offline scratch recovery rehearsal (experimental)

The source file `tools/packaging_recovery_rehearsal.py` accepts an **already quiesced database copy** in a private directory owned by the calling non-root user (mode `0700`). It refuses a symlink or real-user state outside that scratch workspace. It creates a fresh backup with SQLite's backup API, verifies integrity/schema/user_version and SHA-256, restores to a second *new* filename in the same workspace, and verifies integrity and byte identity. Existing files are never overwritten. Resource checks reserve at least 4 GiB or 15% of the workspace filesystem, with a budget for concurrent artifacts.

For a pre-existing **private, offline test workspace only**, with `test-copy.db` already located in it:

```sh
python3 tools/packaging_recovery_rehearsal.py \\
  --workspace /path/to/private-0700-test-workspace \\
  --source test-copy.db --backup test-backup.db --restored test-restored.db
```

This is a rehearsal, not a procedure for copying the active VDR-Suite database. Do **not** point it at `/var/lib/vdr-suite`, live WAL files or existing backups. There is no service shutdown, package installation, migration or live restore. Tests in `tools/test_packaging_recovery_rehearsal.py` cover committed WAL data, integrity, overwrite refusal, private workspace, symlinks, and reserved space.

## Fail-closed offline inspection (P7 hardening)

A private WAL-mode regression demonstrates the defect: `mode=ro&immutable=1` can fail to see a table and rows committed in an accompanying WAL file, even though a normal `mode=ro` reader sees them. The preflight now refuses sidecars rather than claiming an upgrade-ready schema. Tests cover committed WAL data, orphaned rollback journals and dangling shared-memory symlinks without altering any original bytes. This validation checks only a **pre-existing offline copy**, not a live source or a production quiesce protocol.

Schema versions are not globally unified: `PRAGMA user_version` alone is insufficient to assess Suite metadata, search, security, timer, and other repository-owned migration state. The offline inspector now emits the **observed positive integer markers** in `suite_metadata_schema_versions` and `vdr_global_search_schema_versions` (when those tables exist), keeping absent tables absent. Unexpected non-integer or nonpositive markers fail closed. The output schema revision is now `2`; these are **partial observations, not upgrade compatibility decisions**. In particular, `database/migrations/README.md` explicitly reserves that directory; no complete migration runner is present there. Never infer backward compatibility or authorize a package upgrade solely from the read-only preflight.

## Historical tagged schema rehearsal — isolated only

A new offline regression uses the **byte-exact** `database/schema/vdr-suite.sql` from Git tag
`v1.60.15m-recording-actions-cache-consistency`, annotated tag object
`50e699956dbd4407a57680b6359dca54f000ee74`, peeled commit `646020746bc449b220ff4f24b80992e5f38f0162` (Git blob
`af0dd4265ca37ce8ef34f9ff1fbac18e079cf0ac`), vendored as
`tools/fixtures/packaging/v1.60.15m-vdr-suite.sql`.
The test verifies source identity before testing, then creates a **temporary**
SQLite database containing one row each for recording, metadata, artwork, job,
EPG, recording cache and associated state. It applies the current
`database/schema/metadata-platform-v1.sql` **additively in a transaction**,
checks legacy rows, schema object definitions, foreign keys and integrity,
repeats the SQL for idempotency, injects a forced failure to prove rollback of
that test transaction, and verifies restoration from an earlier SQLite backup
into a fresh offline database.

This is real historical **SQL** provenance, **not** an older released binary
package or a complete production migration. The current daemon initializes many
additional tables through repository-owned `ensureSchema()` calls; this
rehearsal does not execute those calls, start the daemon, or prove that old
runtime state is compatible with current binaries. Neither package upgrades nor
a database restore have been authorized on the productive yaVDR. Regression:
`python3 -m unittest -v tools.test_packaging_p7_historical_schema`.

## Real C++ repository schema initialization on a historical fixture

The `test-packaging-p7-runtime-schema` Make target compiles the actual
`GenreIndexRepository::ensureSchema()` and
`GlobalSearchRepository::ensureSchema()` implementation against SQLite.
A non-root harness creates the proven tagged historical SQL database and
sample rows in a **fresh 0700 scratch directory**, runs the C++ initializers
twice per process and over two process starts, confirms the original columns,
row values and SQL objects remain intact, verifies SQLite integrity/foreign
keys and the domain-specific version markers, then restores the pre-upgrade
backup into another isolated file and validates the original schema and rows.
The test is now invoked inside the disposable Ubuntu 24.04 full-package CI
runner. It cannot operate on `/var/lib/vdr-suite` or any installed database
because the script does not accept a database path.

This now additionally exercises the real SecurityIdentity, HumanAccount,
CredentialVerifier, MutationOperation, TimerIntent and TimerAssignment
`ensureSchema()` methods. After the first initialization, the private harness
commits synthetic account, credential-verifier and operation/payload records;
a fresh process must preserve their values and all existing SQL definitions.
No real password or productive user database is accessed. Timer tables are
initialized and replayed, but preservation of **historical timer data** is
not yet established.

This improves migration evidence for the listed repository owners only.
It does **not** execute every daemon initializer, test historical Debian
package upgrades, start services, or establish production rollback approval.

## Focused real C++ older-column migration rehearsal

The disposable runtime test also creates a **reconstructed**, explicitly
non-historical fixture `tools/fixtures/packaging/p7-pre-revision-security-operations.sql`.
It models legacy SQLite `security_human_accounts` without `revision` and
`mutation_operations` without `expected_resource_fingerprint`, alongside
one synthetic user/credential/verifier and one synthetic queued operation.
The test calls the current real C++ repository `ensureSchema()` methods,
requires both missing columns to be added with expected default values,
proves each original column and row value survives both C++ reopens, and
allows newly seeded current-schema rows to coexist on the second open while
independently checking that every original tuple is still present. Full
current-schema rows must additionally remain identical on reinitialization.
confirms the pre-upgrade backup restores the original missing-column shapes.

The SQL file **was not recovered from an older release**; it combines
table shapes from two different older commits with invented test-only data.
A new provenance gate verifies that both legacy CREATE TABLE statements
(including their constraints) match **byte-exact source snapshots** of
`HumanAccountRepository.cpp` at commit
`e63623d80be9b9d69f5219edd379a973968cd282` and
`MutationOperationRepository.cpp` at commit
`6df73c326f32ae8ddd739caf1452b6ccddc16b1e`, both by Git blob SHA-1.
Run `python3 -m unittest -v tools.test_packaging_p7_schema_provenance`
to verify the attestation without modifying a database. A fake, unusable password-hash
sentinel is used. This is evidence for specific old-column migrations only,
not a supported old-package upgrade, complete daemon-start migration, or
production restore procedure.


## P7 versioned package upgrade experiment (unreleased)

The verified CI #41 commit `bc4623374a4df4c08118c424ea19aaf552a4964c` contains `0.14.1-1~exp1`
as an **experimental source-packaging state**, not a distributed package release.
A fresh disposable Ubuntu 24.04 CI runner now builds its three binary packages
from that immutable commit. The active packaging branch produces **a distinct**
`0.14.1-1~exp2` binary revision while keeping the application version
`0.14.1`. Both builds resolve the Seahawk VDR 2.8.3 ABI.

The new `p7-versioned-upgrade-noble` job first installs `exp1` in a throwaway
Ubuntu container with service activation blocked, constructs a synthetic
`vdr-suite.db` from tagged historical SQL and independently source-attested
older-column test SQL, and adds a test-only operator-managed configuration and
unowned persistent-data sentinel. It records a read-only hash and an offline
backup. It then runs `dpkg --unpack` on all three `exp2` binaries, checks
the unconfigured state, resumes with `dpkg --configure -a` and compares the
original SQLite **bytes, rows and schema**, conffiles and user sentinel.
Finally it verifies the backup, alters only the disposable test database,
restores its older snapshot and explicitly downgrades the three binaries to
`exp1`, verifying recovered state. This **experimental dpkg package-version
transition passed** on [CI #46](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38640654487)
(commit `098a14046d346cda5f03bbd3abaf4b386ae03428`) and again on
[CI #47](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38645349742)
(commit `08b0fbfdfb3c9ea959d93d7010801b53d46dbbd1`).
Both completed all five jobs successfully. Local validation of the latter
commit passed 68 packaging tests, version checks and the tracked-artifact audit.

**Important limits:** Neither older `exp1` nor `exp2` is a published
supported Debian package release. The older test data is fabricated from
historical SQL and placed on disk *after* installing the prior binaries; the
old daemon is not started and did not generate those data. The real package
maintainer scripts deliberately do not perform schema migration. Thus even
passing CI proves a package manager upgrade plus persistence/recovery behavior,
not upgrade compatibility of active daemon databases, nontrivial schema
migration, fail-after-mutation recovery, or a production downtime procedure.
The failed-interruption exercise is an unpacked-but-unconfigured dpkg state;
it does not assert resilience to arbitrary power loss.

## Remaining before P7 acceptance

The successful `exp1 → exp2` experiment verifies that the package manager
preserves a synthetic offline database and operator-managed configuration when
service startup is blocked. It **does not** provide a support matrix for
previously published packages or for databases used by a running daemon.
Until an explicit compatibility table covers the actual runtime schema owners
and release pairs, treat unknown source schema, future markers and unknown
binary/database combinations as **unsupported**; neither automatic migration
nor a downgrade of a newer live database is approved.
The first packaging release does not need a fictitious published predecessor;
its upgrade gate concerns only explicitly supported earlier **manual-install**
databases. Do not assume that every development checkout can be migrated.

- Schema migration registry and compatible version boundaries.
- An approved quiesce/snapshot method for the real daemon and all SQLite/WAL writers. The scratch rehearsal is not an integrated production backup.
- For the first package release, isolated migration tests for explicitly supported existing manual-install versions and daemon-created databases; after a previous supported package release exists, versioned binary-package upgrade tests for those supported predecessors.
- Full upgrade/rollback matrix, including compatibility and restoration of all state in an isolated Debian/Ubuntu install root.
- Concurrent service start/stop and admin onboarding acceptance on real yaVDR.

No maintainer script or automated installer currently performs a database migration.

The scratch rehearsal now rejects `/var`, `/etc` and other protected system namespaces as well as the two known VDR-Suite source checkouts. A new regression exercises a race in which a second process creates the restore target just before the exclusive open; the tool must preserve the other process's file rather than treat it as its own cleanup target. No production directory permissions or ownerships are changed.

The disposable Ubuntu noble package lifecycle exercise passed in [GitHub Actions #29](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38588255607). It verifies same-version reinstall, configuration and an unowned data sentinel across remove/purge, **not** upgrade from a previous version or database schema compatibility. Do not confuse a successful no-migration package reinstall with approval to downgrade or restore a live database.

## P7 incremental C++ owners: device pairing and channel cache

The isolated C++ schema rehearsal now additionally executes the real
`DevicePairingRequestRepository::ensureSchema()` and
`VdrChannelCacheRepository::ensureSchema()` twice per process and on two
fresh process invocations. The pairing fixture reconstructs an older table
without `revision`, `decided_by_actor_id` or `decided_at`; the test checks
that existing fake pairing data survives column addition with correct defaults.
It also seeds one new pairing record and one channel cache record, checks every
row and SQL object survives reopening, and verifies the independently restored
backup retains the pre-alter table shape. The fixture is **not an attested
historical binary/database**; this does not close the all-owners P7 gate.

CI confirmation: [GitHub Actions #48](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38676138135)
passed all five jobs for commit `db10fba7164b43ecea8832f5353da9ad1cd0a10c`.
The completed C++ test log contains
`P7_PAIRING_PRECOLUMN_AND_CHANNEL_PRESERVATION=PASS` and
`P7_HISTORICAL_CPP_SCHEMA_REHEARSAL=PASS`. This confirms the two new
repository owners' isolated, synthetic fixture behavior, **not** the
compatibility of an active database, a released older package, or an automatic
production migration. No real SQLite/WAL writer was stopped or accessed.

## P7 incremental C++ owners: browser sessions and playback progress

The C++ offline rehearsal now additionally runs the real schema initializers
for `BrowserSessionCredentialRepository`, `ContinueWatchingRepository`, and
`RecentlyWatchedRepository`. Pre-column fixtures contain synthetic historical-
shape browser-session and playback-state rows. Checks assert that browser
`last_seen_at` is backfilled from `created_at`, the two playback tables
receive an empty `backend_native_id` default, and pre-existing columns and
records survive both a fresh-process reinitialization and offline restoration.
The synthetic browser identity has valid SQLite foreign-key relationships but
no usable secrets. These tests are not a published-package migration, a live
session upgrade or complete runtime schema-owner coverage.
