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
PAIRING_COLUMN_FIXTURE = (Path(__file__).resolve().parent / "fixtures" /
                          "packaging" / "p7-pre-revision-device-pairing.sql")
SESSION_PLAYBACK_FIXTURE = (Path(__file__).resolve().parent / "fixtures" /
                            "packaging" / "p7-pre-revision-sessions-playback.sql")
PRIOR_TABLES = (
    "security_actors", "security_credentials",
    "security_basic_credential_verifiers",
    "security_human_accounts", "mutation_operations",
    "security_device_pairing_requests",
    "vdr_recording_cache",
    "security_devices", "security_sessions",
    "security_browser_session_credentials",
    "continue_watching_state", "recently_watched_state",
)
ALTERED_TABLES = frozenset(
    ("security_human_accounts", "mutation_operations",
     "security_device_pairing_requests",
     "security_browser_session_credentials",
     "continue_watching_state", "recently_watched_state",
     "vdr_recording_cache")
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
        INSERT INTO security_device_pairing_requests(
          pairing_request_id,user_code_hash,pairing_token_hash,display_name,
          client_kind,expires_at,revision)
          VALUES('p7_new_pairing','SYNTHETIC_HASH_2','SYNTHETIC_TOKEN_2',
                 'P7 new test device','vidaa','2099-01-01 00:00:00',4);
        INSERT INTO vdr_channel_cache(
          backend_id,channel_id,channel_number,name,provider,group_name,
          radio,encrypted,enabled)
          VALUES('p7_test','p7_channel',17,'P7 demo','test','fixture',0,0,1);
        INSERT INTO security_devices(device_id,actor_id,display_name)
          VALUES('p7_new_device','p7_actor','P7 test-only device');
        INSERT INTO security_sessions(session_id,actor_id,device_id)
          VALUES('p7_new_session','p7_actor','p7_new_device');
        INSERT INTO security_browser_session_credentials(
          token_id,session_id,actor_id,device_id,credential_id,
          issued_from_credential_id,session_secret_hash,csrf_secret_hash,
          expires_at,last_seen_at)
          VALUES('p7_new_token','p7_new_session','p7_actor','p7_new_device',
                 'p7_credential','p7_credential','SYNTHETIC_SESSION_HASH',
                 'SYNTHETIC_CSRF_HASH','2099-01-01 00:00:00',
                 '2026-01-01 00:00:00');
        INSERT INTO continue_watching_state(
          actor_id,backend_id,recording_id,backend_native_id,
          position_seconds,last_activity_at,last_operation_id)
          VALUES('p7_actor','p7_backend','p7_recording','p7_native',120,
                 '2026-01-01T00:00:00Z','p7_op');
        INSERT INTO recently_watched_state(
          actor_id,backend_id,recording_id,backend_native_id,
          position_seconds,position_known,completion_known,completed,
          resume_relevance_known,resume_relevant,source_evidence,
          last_activity_at,last_operation_id)
          VALUES('p7_actor','p7_backend','p7_recording','p7_native',120,1,
                 1,0,1,1,'synthetic','2026-01-01T00:00:00Z','p7_op');
        INSERT INTO media_sessions(
          session_id,actor_id,backend_id,resource_kind,resource_id,state,
          presentation_profile_id)
          VALUES('p7_media_session','p7_actor','p7_backend','recording',
                 'p7_recording','active','hls-fmp4');
        INSERT INTO media_routes(
          route_id,session_id,route_epoch,provider_id,backend_id,state)
          VALUES('p7_route','p7_media_session',1,'p7_provider','p7_backend',
                 'active');
        INSERT INTO media_provider_stream_leases(
          lease_id,route_id,provider_id,workspace_id,state)
          VALUES('p7_lease','p7_route','p7_provider','fixture-workspace',
                 'active');
        INSERT INTO media_access_grants(
          grant_id,session_id,route_id,route_epoch,actor_id,secret_hash,
          expires_at,active)
          VALUES('p7_grant','p7_media_session','p7_route',1,'p7_actor',
                 'SYNTHETIC_NOT_A_REAL_SECRET','2099-01-01 00:00:00',1);
        INSERT INTO backend_media_transcode_settings(
          backend_id,video_encoder_mode)
          VALUES('p7_backend','vaapi');
        -- The tagged historical EPG and fuzzy-capability rows are part of
        -- the immutable source baseline, not runtime seed targets.
        -- The historical vdr_recording_cache row belongs to the immutable
        -- baseline. Do not add a synthetic row here: original_rows() must
        -- remain byte-for-byte equal on both C++ reopen passes.
        INSERT INTO public_recording_identity(public_id,backend_id,native_id)
          VALUES('rec_p7synthetic','p7_backend','p7_native');
        INSERT INTO recording_series_hierarchy_overrides(
          backend_id,recording_key,group_type,season_number,group_label)
          VALUES('p7_backend','p7_native','season',2,'P7 season');
        INSERT INTO epg_event_artwork(
          backend_id,channel_id,event_id,provider,path,width,height)
          VALUES('p7_backend','p7_channel','p7_epg','p7_test',
                 '/p7/fixture.jpg',300,400);
        INSERT INTO epg_scraper_metadata_cache(
          backend_id,channel_id,event_id,public_json)
          VALUES('p7_backend','p7_channel','p7_epg',
                 '{"title":"P7 EPG"}');
        INSERT INTO epg_scraper_metadata_images(
          backend_id,channel_id,event_id,kind,provider,path)
          VALUES('p7_backend','p7_channel','p7_epg','preferred',
                 'p7_test','/p7/image.jpg');
        INSERT INTO epg_scraper_metadata_people(
          backend_id,channel_id,event_id,ordinal,name,name_folded)
          VALUES('p7_backend','p7_channel','p7_epg',0,'P7 Person',
                 'p7 person');
        INSERT INTO security_actor_permission_grants(
          actor_id,permission,backend_id)
          VALUES('p7_actor','recordings.read','p7_backend');
        INSERT INTO security_actors(actor_id,actor_type,display_name)
          VALUES('p7_service_actor','service','P7 synthetic service');
        INSERT INTO security_devices(device_id,actor_id,display_name)
          VALUES('p7_service_device','p7_service_actor','P7 synthetic device');
        INSERT INTO security_credentials(credential_id,actor_id,credential_type)
          VALUES('p7_service_credential','p7_service_actor','device-app');
        INSERT INTO security_device_credential_verifiers(
          credential_id,device_id,verifier_hash)
          VALUES('p7_service_credential','p7_service_device',
                 'P7_SYNTHETIC_NOT_A_REAL_VERIFIER');
        INSERT INTO accountability_events(
          event_id,schema_version,classes,event_type,severity,occurred_at,
          actor_id,actor_type,device_id,session_id,authentication_state,
          permission,backend_id,operation_id,request_id,correlation_id,
          action,decision,reason_code,outcome)
          VALUES('p7_audit',1,'p7-fixture','p7.audit','info',
                 '2026-01-01T00:00:00Z','p7_actor','user','','','test-only',
                 'recordings.read','p7_backend','','p7_request','','read',
                 'allow','p7-synthetic','ok');
    """)
    connection.commit()


def current_owned_records(connection):
    tables = ("security_actors", "security_human_accounts",
              "security_credentials", "security_basic_credential_verifiers",
              "mutation_operations", "mutation_operation_payloads",
              "security_device_pairing_requests", "vdr_channel_cache",
              "security_devices", "security_sessions",
              "security_browser_session_credentials",
              "continue_watching_state", "recently_watched_state",
              "media_sessions", "media_routes", "media_provider_stream_leases",
              "media_access_grants", "backend_media_transcode_settings",
              "epg_events", "epgsearch_native_fuzzy_capability_probes",
              "vdr_recording_cache", "public_recording_identity",
              "recording_series_hierarchy_overrides", "epg_event_artwork",
              "epg_scraper_metadata_cache", "epg_scraper_metadata_images",
              "epg_scraper_metadata_people",
              "accountability_events", "security_device_credential_verifiers",
              "security_actor_permission_grants")
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
            preupgrade.executescript(
                PAIRING_COLUMN_FIXTURE.read_text(encoding="utf-8"))
            preupgrade.executescript(
                SESSION_PLAYBACK_FIXTURE.read_text(encoding="utf-8"))
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
            for absent in ("revision", "decided_by_actor_id", "decided_at"):
                assert absent not in prior_shape["security_device_pairing_requests"]
            assert "last_seen_at" not in prior_shape[
                "security_browser_session_credentials"]
            for table in ("continue_watching_state", "recently_watched_state"):
                assert "backend_native_id" not in prior_shape[table]
            for absent in ("metadata_payload", "recording_duration_known"):
                assert absent not in prior_shape["vdr_recording_cache"]
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
                historical_after = original_rows(connection, columns)
                assert historical_after == before, (
                    "historical records changed after runtime schema initializer "
                    "pass {} in tables {}".format(
                        count, sorted(table for table in TABLES
                                      if historical_after[table] != before[table])))
                after_objects = dict(connection.execute(
                    "SELECT name,sql FROM sqlite_master WHERE name NOT LIKE 'sqlite_%'"
                ).fetchall())
                assert all(after_objects.get(k) == v
                           for k, v in original_objects.items()
                           if k not in ALTERED_TABLES), (
                    "unexpected historical SQL object changed")
                assert prior_rows_preserved(
                    old_prior_rows, prior_rows(connection, prior_shape)), (
                    "pre-migration security, operation or pairing data changed")
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
                for added in ("revision", "decided_by_actor_id", "decided_at"):
                    assert added in updated_columns["security_device_pairing_requests"], (
                        "device pairing migration column missing: " + added)
                assert connection.execute(
                    "SELECT revision, decided_by_actor_id, decided_at "
                    "FROM security_device_pairing_requests "
                    "WHERE pairing_request_id='p7_pre_pairing'").fetchone() == (
                        1, "", ""), "pre-existing pairing defaults changed"
                assert "last_seen_at" in updated_columns[
                    "security_browser_session_credentials"]
                for table in ("continue_watching_state", "recently_watched_state"):
                    assert "backend_native_id" in updated_columns[table], (
                        "playback native recording ID column missing: " + table)
                    assert connection.execute(
                        "SELECT backend_native_id FROM " + table +
                        " WHERE recording_id='p7_pre_recording'").fetchone() == (
                            "",), "older playback row default changed: " + table
                assert connection.execute(
                    "SELECT last_seen_at=created_at "
                    "FROM security_browser_session_credentials "
                    "WHERE token_id='p7_pre_token'").fetchone() == (1,), (
                        "old browser session last_seen_at was not backfilled")
                for added, expected in (
                    ("metadata_payload", ""), ("recording_duration_known", 0)):
                    assert added in updated_columns["vdr_recording_cache"], (
                        "recording cache migration column missing: " + added)
                    assert connection.execute(
                        'SELECT "' + added + '" FROM vdr_recording_cache '
                        "WHERE backend_id='test' AND cache_key='key' LIMIT 1").fetchone() == (
                            expected,), "old recording cache default changed"
                assert connection.execute(
                    "SELECT count(*) FROM suite_metadata_schema_versions"
                ).fetchone()[0] >= 1
                assert connection.execute(
                    "SELECT count(*) FROM vdr_global_search_schema_versions"
                ).fetchone()[0] == 1
                if count == 1:
                    seed_owned_runtime_records(connection)
                    assert original_rows(connection, columns) == before, (
                        "test-only runtime seed changed historical source rows")
                    runtime_fixture = current_owned_records(connection)
                    runtime_objects = dict(connection.execute(
                        "SELECT name,sql FROM sqlite_master "
                        "WHERE name NOT LIKE 'sqlite_%'").fetchall())
                    require_consistent(connection)
                else:
                    assert runtime_fixture is not None
                    assert current_owned_records(connection) == runtime_fixture, (
                        "security/session/playback/channel records changed on restart")
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
        print("P7_PAIRING_PRECOLUMN_AND_CHANNEL_PRESERVATION=PASS", flush=True)
        print("P7_BROWSER_SESSION_AND_PLAYBACK_MIGRATION=PASS", flush=True)
        print("P7_MEDIA_SESSION_AND_EPG_SCHEMA_PRESERVATION=PASS", flush=True)
        print("P7_RECORDING_CACHE_AND_EPG_ARTWORK_PRESERVATION=PASS", flush=True)
        print("P7_SECURITY_GRANTS_AUDIT_PRESERVATION=PASS", flush=True)
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
