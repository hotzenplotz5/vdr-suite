#!/usr/bin/env python3
"""P7 historical schema -> CURRENT C++ repository initializers, offline only.

Only creates databases in a fresh, auto-deleted private scratch directory.
No live service, apt, package installation, or production database access.
"""
import argparse
from contextlib import closing
import os
from pathlib import Path
import shutil
import sqlite3
import subprocess
import tempfile

from test_packaging_p7_historical_schema import (
    LEGACY, LEGACY_GIT_BLOB_SHA1, TABLES, create_legacy_database,
    require_consistent, source_git_blob_hash,
)

PRIOR_COLUMN_FIXTURE = (Path(__file__).resolve().parent / "fixtures" /
                        "packaging" / "p7-pre-revision-security-operations.sql")
PRIOR_TABLES = (
    "security_actors", "security_credentials",
    "security_basic_credential_verifiers",
    "security_human_accounts", "mutation_operations",
)
ALTERED_TABLES = frozenset(
    ("security_human_accounts", "mutation_operations")
)

GiB = 1024 ** 3
MiB = 1024 ** 2


def original_rows(connection, columns):
    """Compare only columns that existed in the historical Git schema."""
    snapshot = {}
    for table in TABLES:
        names = columns[table]
        quoted = ",".join('"' + name.replace('"', '""') + '"' for name in names)
        snapshot[table] = connection.execute(
            "SELECT " + quoted + " FROM " + table + " ORDER BY rowid"
        ).fetchall()
    return snapshot



def seed_owned_runtime_records(connection):
    """Synthetic account and operation records; never use real secrets."""
    connection.execute("PRAGMA foreign_keys=ON")
    connection.executescript("""
        INSERT INTO security_actors(actor_id,actor_type,display_name,active)
          VALUES('p7_actor','user','P7 fixture user',1);
        INSERT INTO security_human_accounts(account_id,actor_id,display_name,revision)
          VALUES('p7_account','p7_actor','P7 account',3);
        INSERT INTO security_credentials(credential_id,actor_id,credential_type)
          VALUES('p7_credential','p7_actor','basic');
        INSERT INTO security_basic_credential_verifiers(
          credential_id,login_name,password_hash)
          VALUES('p7_credential','p7_login','NOT_A_REAL_PASSWORD_HASH');
        INSERT INTO mutation_operations(
          operation_id,operation_revision,idempotency_key,actor_id,
          backend_id,backend_generation,resource_type,resource_id,
          expected_revision,expected_resource_fingerprint,action_family,
          request_fingerprint,requested_at,deadline,verification_policy,
          state,result_reference,updated_at)
          VALUES('p7_op',1,'p7_once','p7_actor','test',1,'timer','42',
                 '0','','create','p7_request',1000,2000,
                 'readback_required','queued','',1000);
        INSERT INTO mutation_operation_payloads(
          operation_id,payload_type,payload_version,payload,payload_fingerprint)
          VALUES('p7_op','test-only',1,'fixture','p7_payload');
    """)
    connection.commit()


def current_owned_records(connection):
    tables = ("security_actors", "security_human_accounts",
              "security_credentials", "security_basic_credential_verifiers",
              "mutation_operations", "mutation_operation_payloads")
    return {table: connection.execute('SELECT * FROM "' + table + '"').fetchall()
            for table in tables}


def prior_columns(connection):
    return {
        table: [row[1] for row in connection.execute(
            "PRAGMA table_info('" + table + "')")]
        for table in PRIOR_TABLES
    }


def prior_rows(connection, columns):
    result = {}
    for table, names in columns.items():
        selected = ",".join('"' + name + '"' for name in names)
        result[table] = connection.execute(
            "SELECT " + selected + " FROM " + table + " ORDER BY rowid"
        ).fetchall()
    return result


def prior_rows_preserved(before, after):
    """New test records may be added; every original tuple must survive."""
    return all(
        table in after and all(row in after[table] for row in rows)
        for table, rows in before.items()
    )


def run(binary):
    if os.geteuid() == 0:
        raise ValueError("P7 runtime fixture must not run as root")
    if source_git_blob_hash(LEGACY.read_bytes()) != LEGACY_GIT_BLOB_SHA1:
        raise ValueError("historical Git SQL fixture provenance mismatch")
    executable = Path(binary).resolve(strict=True)
    if not executable.is_file() or not os.access(executable, os.X_OK):
        raise ValueError("P7 runtime fixture executable missing")

    # The runner constructs every database filename inside this unique folder.
    with tempfile.TemporaryDirectory(prefix="p7-cpp-repository-") as scratch:
        folder = Path(scratch).resolve()
        mode = folder.stat().st_mode & 0o777
        if folder.stat().st_uid != os.geteuid() or mode & 0o077:
            raise ValueError("P7 fixture workspace must be caller-owned/private")
        disk = shutil.disk_usage(folder)
        reserve = max(4 * GiB, disk.total * 15 // 100)
        budget = 128 * MiB
        print("RESOURCE_TOTAL={} RESOURCE_FREE={} RESERVE={} BUDGET={}".format(
            disk.total, disk.free, reserve, budget), flush=True)
        if disk.free < reserve + budget:
            raise ValueError("P7 historical runtime fixture: insufficient space")
        print("RESOURCE_PREFLIGHT=PASS", flush=True)

        old, backup, restored = (
            folder / n for n in ("old.db", "before.db", "restored.db"))
        create_legacy_database(old)
        # A deliberately reconstructed shape exercises C++ ALTER guards.
        # It is NOT part of the tagged historical Git SQL.
        with closing(sqlite3.connect(old)) as preupgrade:
            preupgrade.executescript(
                PRIOR_COLUMN_FIXTURE.read_text(encoding="utf-8"))
            require_consistent(preupgrade)
        with closing(sqlite3.connect(old)) as source, closing(
                sqlite3.connect(backup)) as target:
            require_consistent(source)
            columns = {
                table: [r[1] for r in source.execute(
                    "PRAGMA table_info('" + table + "')")] for table in TABLES
            }
            before = original_rows(source, columns)
            prior_shape = prior_columns(source)
            assert "revision" not in prior_shape["security_human_accounts"]
            assert "expected_resource_fingerprint" not in prior_shape[
                "mutation_operations"]
            old_prior_rows = prior_rows(source, prior_shape)
            assert all(len(rows) == 1 for rows in old_prior_rows.values())
            original_objects = dict(source.execute(
                "SELECT name,sql FROM sqlite_master WHERE name NOT LIKE 'sqlite_%'"
            ).fetchall())
            source.backup(target)
        assert all(len(rows) == 1 for rows in before.values()), (
            "historical records missing in test fixture")

        # Reopen through fresh C++ instances; persist synthetic data between runs.
        runtime_fixture = None
        runtime_objects = None
        for count in (1, 2):
            p = subprocess.run([str(executable), str(old)],
                               text=True, capture_output=True, timeout=45)
            if p.returncode or "P7_RUNTIME_SCHEMA_REPOSITORIES=PASS" not in p.stdout:
                raise AssertionError("P7 C++ schema pass {} failed: {} {}".format(
                    count, p.stdout[-2000:], p.stderr[-2000:]))
            with closing(sqlite3.connect(old)) as connection:
                require_consistent(connection)
                assert original_rows(connection, columns) == before, (
                    "historical records changed after runtime schema initializer")
                after_objects = dict(connection.execute(
                    "SELECT name,sql FROM sqlite_master WHERE name NOT LIKE 'sqlite_%'"
                ).fetchall())
                assert all(after_objects.get(k) == v
                           for k, v in original_objects.items()
                           if k not in ALTERED_TABLES), (
                    "unexpected historical SQL object changed")
                assert prior_rows_preserved(
                    old_prior_rows, prior_rows(connection, prior_shape)), (
                    "pre-migration account, credential or operation data changed")
                updated_columns = prior_columns(connection)
                assert "revision" in updated_columns["security_human_accounts"], (
                    "account revision ALTER did not run")
                assert "expected_resource_fingerprint" in updated_columns[
                    "mutation_operations"], (
                    "mutation fingerprint ALTER did not run")
                assert connection.execute(
                    "SELECT revision FROM security_human_accounts "
                    "WHERE account_id='p7_pre_account'").fetchone()[0] == 1
                assert connection.execute(
                    "SELECT expected_resource_fingerprint "
                    "FROM mutation_operations WHERE operation_id="
                    "'p7_pre_operation'").fetchone()[0] == ""
                assert connection.execute(
                    "SELECT count(*) FROM suite_metadata_schema_versions"
                ).fetchone()[0] >= 1
                assert connection.execute(
                    "SELECT count(*) FROM vdr_global_search_schema_versions"
                ).fetchone()[0] == 1
                if count == 1:
                    seed_owned_runtime_records(connection)
                    runtime_fixture = current_owned_records(connection)
                    runtime_objects = dict(connection.execute(
                        "SELECT name,sql FROM sqlite_master "
                        "WHERE name NOT LIKE 'sqlite_%'").fetchall())
                    require_consistent(connection)
                else:
                    assert runtime_fixture is not None
                    assert current_owned_records(connection) == runtime_fixture, (
                        "security/credential/operation records changed on restart")
                    assert dict(connection.execute(
                        "SELECT name,sql FROM sqlite_master "
                        "WHERE name NOT LIKE 'sqlite_%'").fetchall()) == runtime_objects, (
                        "runtime SQL schema objects changed on second initialization")

        # Recovery remains completely separate from the migrated fixture.
        with closing(sqlite3.connect(backup)) as source, closing(
                sqlite3.connect(restored)) as target:
            source.backup(target)
            require_consistent(target)
            assert original_rows(target, columns) == before, (
                "offline restored rows differ from historical source")
            assert prior_rows(target, prior_shape) == old_prior_rows, (
                "offline restored prior account/operation rows differ")
            assert prior_columns(target) == prior_shape, (
                "offline backup did not restore old column shapes")
            assert dict(target.execute(
                "SELECT name,sql FROM sqlite_master WHERE name NOT LIKE 'sqlite_%'"
            ).fetchall()) == original_objects, (
                "offline restored schema differs from historical source")

        final = shutil.disk_usage(folder)
        if final.free < reserve:
            raise ValueError("P7 runtime rehearsal broke filesystem reserve")
        print("RESOURCE_POSTCHECK=PASS", flush=True)
        print("P7_PRECOLUMN_SECURITY_OPERATION_MIGRATION=PASS", flush=True)
        print("P7_HISTORICAL_CPP_SCHEMA_REHEARSAL=PASS", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True)
    args = parser.parse_args()
    try:
        run(args.binary)
    except (OSError, ValueError, AssertionError, sqlite3.Error,
            subprocess.TimeoutExpired) as error:
        parser.exit(1, "P7_HISTORICAL_CPP_SCHEMA_REHEARSAL=FAIL: " +
                    str(error) + "\n")


if __name__ == "__main__":
    main()
