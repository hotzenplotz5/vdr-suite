# P7 — Upgrade/recovery policy and preflight

Status: policy and **read-only inspection** implemented. No automated database migrations or tested downgrade path.

## Safety contract

1. Stop write producers only during a separately approved controlled maintenance window. Package installation must not assume it can safely stop a running VDR on first install.
2. Inspect SQLite database with `python3 tools/packaging_upgrade_preflight.py --database /path/to/offline-copy.db`. The preflight is strictly read-only and uses SQLite immutable mode; if the database is live, **results may be inconsistent**. It is not a consistent backup.
3. For an actual backup use SQLite's supported online backup API or safely shut down writers, and verify integrity of the resulting backup. Include SQLite WAL changes; never simply copy a live main `.db` file while a WAL is active.
4. Record source version, target package version, schema version, backup location and file hashes; protect secrets and backups using least privilege.
5. Install/upgrade only in an isolated test environment until schema migration is proven idempotent, transactional and safely resumable.
6. Recover by stopping application writes and restoring a verified, compatible backup before reverting binaries. A newer DB schema must not be opened by an older unsupported binary.
7. Do not delete persistent databases during package `remove`; `purge` must never silently destroy recorded content.

## Remaining before P7 acceptance

- Schema migration registry and compatible version boundaries.
- Snapshot/backup implementation and WAL-safe integration tests.
- Upgrade tests from at least one supported previous package version.
- Rollback/recovery dry-run with integrity verification.
- Concurrent service start/stop and admin onboarding acceptance on real yaVDR.

No maintainer script or automated installer currently performs a database migration.
