#!/usr/bin/env python3
from __future__ import annotations

import argparse
from collections.abc import Callable, Sequence
from typing import Any

AccountabilityRow = tuple[str, ...]
AccountabilityReader = Callable[[Any, list[str]], list[AccountabilityRow]]
Require = Callable[[bool, str], None]


def _matches(
    row: AccountabilityRow,
    *,
    event_type: str,
    permission: str,
    action: str,
    decision: str,
    reason_code: str,
    outcome: str,
) -> bool:
    return (
        len(row) >= 14
        and row[0] == event_type
        and row[5] == permission
        and row[6] == "*"
        and row[10] == action
        and row[11] == decision
        and row[12] == reason_code
        and row[13] == outcome
    )


def adapt_accountability_rows(
    delegate: AccountabilityReader,
    require: Require,
    database: Any,
    request_ids: list[str],
) -> list[AccountabilityRow]:
    rows = delegate(database, request_ids)

    if len(request_ids) != 2:
        return rows

    logout_ids = [value for value in request_ids if value.endswith("-logout")]
    replay_ids = [value for value in request_ids if value.endswith("-replay")]
    if len(logout_ids) != 1 or len(replay_ids) != 1:
        return rows

    logout_id = logout_ids[0]
    replay_id = replay_ids[0]
    logout_rows = [row for row in rows if len(row) >= 9 and row[8] == logout_id]
    replay_rows = [row for row in rows if len(row) >= 9 and row[8] == replay_id]

    require(len(logout_rows) == 2, "logout_accountability_count_mismatch")
    require(len(replay_rows) == 1, "replay_accountability_count_mismatch")

    require(
        any(
            _matches(
                row,
                event_type="authorization.allowed",
                permission="session.revoke.self",
                action="browser.session.revoke",
                decision="allowed",
                reason_code="self_service_session_lifecycle_allowed",
                outcome="dispatch_authorized",
            )
            for row in logout_rows
        ),
        "logout_authorization_accountability_missing",
    )

    successful_logout = next(
        (
            row
            for row in logout_rows
            if _matches(
                row,
                event_type="operation.succeeded",
                permission="session.revoke.self",
                action="browser.session.revoke",
                decision="allowed",
                reason_code="browser_session_revoked",
                outcome="succeeded",
            )
        ),
        None,
    )
    require(
        successful_logout is not None,
        "logout_success_accountability_missing",
    )

    replay = replay_rows[0]
    require(
        _matches(
            replay,
            event_type="authorization.denied",
            permission="legacy.compatibility.access",
            action="http.access",
            decision="denied",
            reason_code="credential_revoked",
            outcome="dispatch_denied",
        ),
        "replay_accountability_missing",
    )

    # The existing guarded runner expects the two outcome-bearing rows here.
    # The lifecycle authorization row was validated above and remains present
    # in the later full evidence query.
    return [successful_logout, replay]


def _row(
    event_type: str,
    request_id: str,
    permission: str,
    action: str,
    decision: str,
    reason_code: str,
    outcome: str,
) -> AccountabilityRow:
    return (
        event_type,
        "actor",
        "device",
        "session",
        "authenticated",
        permission,
        "*",
        "",
        request_id,
        "",
        action,
        decision,
        reason_code,
        outcome,
    )


def self_test() -> None:
    logout_id = "phase62-test-logout"
    replay_id = "phase62-test-replay"
    rows = [
        _row(
            "authorization.allowed",
            logout_id,
            "session.revoke.self",
            "browser.session.revoke",
            "allowed",
            "self_service_session_lifecycle_allowed",
            "dispatch_authorized",
        ),
        _row(
            "operation.succeeded",
            logout_id,
            "session.revoke.self",
            "browser.session.revoke",
            "allowed",
            "browser_session_revoked",
            "succeeded",
        ),
        _row(
            "authorization.denied",
            replay_id,
            "legacy.compatibility.access",
            "http.access",
            "denied",
            "credential_revoked",
            "dispatch_denied",
        ),
    ]

    def require_for_test(condition: bool, message: str) -> None:
        if not condition:
            raise AssertionError(message)

    result = adapt_accountability_rows(
        lambda _database, _request_ids: list(rows),
        require_for_test,
        None,
        [logout_id, replay_id],
    )
    assert len(result) == 2
    assert result[0][0] == "operation.succeeded"
    assert result[1][12] == "credential_revoked"

    try:
        adapt_accountability_rows(
            lambda _database, _request_ids: list(rows[1:]),
            require_for_test,
            None,
            [logout_id, replay_id],
        )
    except AssertionError as error:
        assert str(error) == "logout_accountability_count_mismatch"
    else:
        raise AssertionError("missing logout authorization was accepted")


def main(arguments: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true")
    parsed = parser.parse_args(arguments)
    if not parsed.self_test:
        parser.error("--self-test is required")
    self_test()
    print("PHASE_62_IDLE_AUDIT_CONTRACT_SELF_TEST=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
