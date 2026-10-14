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

Schema versions are not globally unified: `PRAGMA user_version` alone is insufficient to assess Suite metadata, search, security, timer, and other repository-owned migration state. In particular, `database/migrations/README.md` explicitly reserves that directory; no complete migration runner is present there. Never infer backward compatibility or authorize a package upgrade solely from the read-only preflight.

## Remaining before P7 acceptance

- Schema migration registry and compatible version boundaries.
- An approved quiesce/snapshot method for the real daemon and all SQLite/WAL writers. The scratch rehearsal is not an integrated production backup.
- Upgrade tests from at least one supported previous package version.
- Full upgrade/rollback matrix, including compatibility and restoration of all state in an isolated Debian/Ubuntu install root.
- Concurrent service start/stop and admin onboarding acceptance on real yaVDR.

No maintainer script or automated installer currently performs a database migration.

The scratch rehearsal now rejects `/var`, `/etc` and other protected system namespaces as well as the two known VDR-Suite source checkouts. A new regression exercises a race in which a second process creates the restore target just before the exclusive open; the tool must preserve the other process's file rather than treat it as its own cleanup target. No production directory permissions or ownerships are changed.

The disposable Ubuntu noble package lifecycle exercise passed in [GitHub Actions #29](https://github.com/hotzenplotz5/vdr-suite/actions/runs/38588255607). It verifies same-version reinstall, configuration and an unowned data sentinel across remove/purge, **not** upgrade from a previous version or database schema compatibility. Do not confuse a successful no-migration package reinstall with approval to downgrade or restore a live database.
