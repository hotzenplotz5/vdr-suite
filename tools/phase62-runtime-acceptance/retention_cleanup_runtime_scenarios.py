#!/usr/bin/env python3
from __future__ import annotations

from retention_cleanup_runtime_support import *  # noqa: F403


def prepare_disabled_database(path: Path) -> dict[str, object]:
    with closing(database_connection(path)) as database:
        required_security_schema(database)
        identity = provision_identity(database, "phase62-s2w-disabled")
        lifecycle = create_lifecycle(
            database,
            identity,
            "phase62-s2w-disabled-old",
            verifier_expires_at="2020-01-01 00:00:00",
            last_seen_at=timestamp(0),
        )
        verify_database(database)
    return {"identity": identity, "lifecycle": lifecycle}


def verify_disabled_database(
    path: Path,
    state: dict[str, object],
) -> dict[str, object]:
    identity = state["identity"]
    lifecycle = state["lifecycle"]
    require(
        isinstance(identity, dict) and isinstance(lifecycle, dict),
        "disabled_state_invalid",
    )
    with closing(database_connection(path)) as database:
        assert_lifecycle(
            database,
            lifecycle,
            verifier=True,
            session=True,
            credential=True,
        )
        assert_identity_preserved(database, identity, issuer_revoked=False)
        events = cleanup_events(database, identity["actor_id"])
        assert_cleanup_events(events, 0)
        quick, foreign_keys = verify_database(database)
    return {
        "disabled_preserved_lifecycles": 1,
        "disabled_cleanup_events": 0,
        "disabled_sqlite_quick_check": quick,
        "disabled_foreign_key_violations": foreign_keys,
    }


def prepare_rollback_database(path: Path) -> dict[str, object]:
    with closing(database_connection(path)) as database:
        required_security_schema(database)
        identity = provision_identity(database, "phase62-s2w-rollback")
        first = create_lifecycle(
            database,
            identity,
            "phase62-s2w-rollback-a",
            verifier_expires_at="2019-01-01 00:00:00",
            last_seen_at=timestamp(0),
        )
        second = create_lifecycle(
            database,
            identity,
            "phase62-s2w-rollback-b",
            verifier_expires_at="2020-01-01 00:00:00",
            last_seen_at=timestamp(0),
        )
        database.execute(
            """
            CREATE TRIGGER phase62_s2w_fail_cleanup_audit
            BEFORE INSERT ON accountability_events
            WHEN NEW.action = 'browser.session.cleanup'
            BEGIN
                SELECT RAISE(ABORT, 'forced cleanup audit failure');
            END
            """
        )
        verify_database(database)
    return {"identity": identity, "lifecycles": [first, second]}


def verify_rollback_database(
    path: Path,
    state: dict[str, object],
) -> dict[str, object]:
    identity = state["identity"]
    lifecycles = state["lifecycles"]
    require(
        isinstance(identity, dict) and isinstance(lifecycles, list),
        "rollback_state_invalid",
    )
    with closing(database_connection(path)) as database:
        for lifecycle in lifecycles:
            assert_lifecycle(
                database,
                lifecycle,
                verifier=True,
                session=True,
                credential=True,
            )
        assert_identity_preserved(database, identity, issuer_revoked=False)
        events = cleanup_events(database, identity["actor_id"])
        assert_cleanup_events(events, 0)
        quick, foreign_keys = verify_database(database)
    return {
        "rollback_preserved_lifecycles": len(lifecycles),
        "rollback_cleanup_events": 0,
        "rollback_sqlite_quick_check": quick,
        "rollback_foreign_key_violations": foreign_keys,
    }


def prepare_enabled_database(path: Path) -> dict[str, object]:
    with closing(database_connection(path)) as database:
        required_security_schema(database)
        identity = provision_identity(database, "phase62-s2w-enabled")
        active = create_lifecycle(
            database,
            identity,
            "phase62-s2w-enabled-active",
            verifier_expires_at=timestamp(7200),
            last_seen_at=timestamp(0),
        )
        revoked_recent = create_lifecycle(
            database,
            identity,
            "phase62-s2w-enabled-revoked-recent",
            verifier_expires_at=timestamp(7200),
            last_seen_at=timestamp(0),
            revoked_at=timestamp(-(RETENTION_SECONDS - 120)),
        )
        revoked_old = create_lifecycle(
            database,
            identity,
            "phase62-s2w-enabled-revoked-old",
            verifier_expires_at=timestamp(7200),
            last_seen_at=timestamp(0),
            revoked_at=timestamp(-(RETENTION_SECONDS + 120)),
        )
        expired_recent = create_lifecycle(
            database,
            identity,
            "phase62-s2w-enabled-expired-recent",
            verifier_expires_at=timestamp(-(RETENTION_SECONDS - 120)),
            last_seen_at=timestamp(0),
        )
        expired_old = create_lifecycle(
            database,
            identity,
            "phase62-s2w-enabled-expired-old",
            verifier_expires_at=timestamp(-(RETENTION_SECONDS + 120)),
            last_seen_at=timestamp(0),
        )
        idle_recent = create_lifecycle(
            database,
            identity,
            "phase62-s2w-enabled-idle-recent",
            verifier_expires_at=timestamp(7200),
            last_seen_at=timestamp(
                -(RETENTION_SECONDS + IDLE_SECONDS - 120)
            ),
        )
        idle_old = create_lifecycle(
            database,
            identity,
            "phase62-s2w-enabled-idle-old",
            verifier_expires_at=timestamp(7200),
            last_seen_at=timestamp(
                -(RETENTION_SECONDS + IDLE_SECONDS + 120)
            ),
        )
        non_browser = create_lifecycle(
            database,
            identity,
            "phase62-s2w-enabled-non-browser",
            verifier_expires_at="2020-01-01 00:00:00",
            last_seen_at=timestamp(0),
            credential_type="api-key",
        )
        shared_terminal = create_lifecycle(
            database,
            identity,
            "phase62-s2w-enabled-shared-terminal",
            verifier_expires_at="2020-01-01 00:00:00",
            last_seen_at=timestamp(0),
        )
        shared_active = {
            "token_id": "phase62-s2w-enabled-shared-active-token",
            "session_id": shared_terminal["session_id"],
            "credential_id": shared_terminal["credential_id"],
        }
        database.execute(
            """
            CREATE TRIGGER phase62_s2w_replace_shared_verifier
            AFTER DELETE ON security_browser_session_credentials
            WHEN OLD.token_id =
                'phase62-s2w-enabled-shared-terminal-token'
            BEGIN
                INSERT INTO security_browser_session_credentials
                    (
                        token_id,
                        session_id,
                        actor_id,
                        device_id,
                        credential_id,
                        issued_from_credential_id,
                        session_secret_hash,
                        csrf_secret_hash,
                        active,
                        expires_at,
                        last_seen_at,
                        revoked_at
                    )
                VALUES
                    (
                        'phase62-s2w-enabled-shared-active-token',
                        OLD.session_id,
                        OLD.actor_id,
                        OLD.device_id,
                        OLD.credential_id,
                        OLD.issued_from_credential_id,
                        '$6$phase62-s2w-runtime-session-replacement',
                        '$6$phase62-s2w-runtime-csrf-replacement',
                        1,
                        datetime(CURRENT_TIMESTAMP, '+7200 seconds'),
                        CURRENT_TIMESTAMP,
                        ''
                    );
            END
            """
        )
        revoke_issuer(database, identity["issuer_id"])
        verify_database(database)
    return {
        "identity": identity,
        "preserved": [
            active,
            revoked_recent,
            expired_recent,
            idle_recent,
            shared_active,
        ],
        "deleted": [
            revoked_old,
            expired_old,
            idle_old,
        ],
        "non_browser": non_browser,
        "shared_terminal": shared_terminal,
        "shared_active": shared_active,
    }


def verify_enabled_database(
    path: Path,
    state: dict[str, object],
) -> dict[str, object]:
    identity = state["identity"]
    preserved = state["preserved"]
    deleted = state["deleted"]
    non_browser = state["non_browser"]
    shared_terminal = state["shared_terminal"]
    shared_active = state["shared_active"]
    require(
        isinstance(identity, dict)
        and isinstance(preserved, list)
        and isinstance(deleted, list)
        and isinstance(non_browser, dict)
        and isinstance(shared_terminal, dict)
        and isinstance(shared_active, dict),
        "enabled_state_invalid",
    )
    with closing(database_connection(path)) as database:
        for lifecycle in preserved:
            assert_lifecycle(
                database,
                lifecycle,
                verifier=True,
                session=True,
                credential=True,
            )
        for lifecycle in deleted:
            assert_lifecycle(
                database,
                lifecycle,
                verifier=False,
                session=False,
                credential=False,
            )
        assert_lifecycle(
            database,
            non_browser,
            verifier=False,
            session=False,
            credential=True,
        )
        require(
            credential_type(database, non_browser["credential_id"])
            == "api-key",
            "non_browser_credential_type_changed",
        )
        require(
            not verifier_exists(database, shared_terminal["token_id"]),
            "shared_terminal_verifier_not_deleted",
        )
        assert_lifecycle(
            database,
            shared_active,
            verifier=True,
            session=True,
            credential=True,
        )
        assert_identity_preserved(database, identity, issuer_revoked=True)
        events = cleanup_events(database, identity["actor_id"])
        assert_cleanup_events(events, 5)
        quick, foreign_keys = verify_database(database)
    return {
        "enabled_deleted_lifecycles": 5,
        "enabled_preserved_lifecycles": len(preserved),
        "enabled_cleanup_events": len(events),
        "enabled_sqlite_quick_check": quick,
        "enabled_foreign_key_violations": foreign_keys,
        "issuer_only_cleanup": 0,
        "non_browser_credential_preserved": "yes",
        "shared_identity_preserved": "yes",
    }


def prepare_bounded_database(path: Path) -> dict[str, object]:
    with closing(database_connection(path)) as database:
        required_security_schema(database)
        identity = provision_identity(database, "phase62-s2w-bounded")
        lifecycles = []
        for index in range(BATCH_SIZE + 2):
            lifecycles.append(
                create_lifecycle(
                    database,
                    identity,
                    f"phase62-s2w-bounded-{index:03d}",
                    verifier_expires_at="2020-01-01 00:00:00",
                    last_seen_at=timestamp(0),
                )
            )
        verify_database(database)
    return {"identity": identity, "lifecycles": lifecycles}


def verify_bounded_database(
    path: Path,
    state: dict[str, object],
) -> dict[str, object]:
    identity = state["identity"]
    lifecycles = state["lifecycles"]
    require(
        isinstance(identity, dict) and isinstance(lifecycles, list),
        "bounded_state_invalid",
    )
    with closing(database_connection(path)) as database:
        remaining = [
            str(row[0])
            for row in database.execute(
                """
                SELECT token_id
                FROM security_browser_session_credentials
                WHERE actor_id = ?
                ORDER BY token_id
                """,
                (identity["actor_id"],),
            )
        ]
        expected_remaining = [
            lifecycles[BATCH_SIZE]["token_id"],
            lifecycles[BATCH_SIZE + 1]["token_id"],
        ]
        require(
            remaining == expected_remaining,
            "bounded_order_or_limit_mismatch",
        )
        for lifecycle in lifecycles[:BATCH_SIZE]:
            assert_lifecycle(
                database,
                lifecycle,
                verifier=False,
                session=False,
                credential=False,
            )
        for lifecycle in lifecycles[BATCH_SIZE:]:
            assert_lifecycle(
                database,
                lifecycle,
                verifier=True,
                session=True,
                credential=True,
            )
        assert_identity_preserved(database, identity, issuer_revoked=False)
        events = cleanup_events(database, identity["actor_id"])
        assert_cleanup_events(events, BATCH_SIZE)
        quick, foreign_keys = verify_database(database)
    return {
        "bounded_input_lifecycles": len(lifecycles),
        "bounded_deleted_lifecycles": BATCH_SIZE,
        "bounded_remaining_lifecycles": len(remaining),
        "bounded_cleanup_events": len(events),
        "bounded_sqlite_quick_check": quick,
        "bounded_foreign_key_violations": foreign_keys,
    }
