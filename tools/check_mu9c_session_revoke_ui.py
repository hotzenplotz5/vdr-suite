#!/usr/bin/env python3
"""Guard the bounded MU.9C Session revoke UI slice."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FILES = {
    "adapter": ROOT / "web/frontend/api/account-admin-client-api.js",
    "ui": ROOT / "web/frontend/settings-account-admin.js",
    "test": ROOT / "web/frontend/tests/test_mu9c_session_revoke_ui.js",
    "make": ROOT / "mk/mu9-account-admin-ui.mk",
    "de": ROOT / "web/frontend/locales/de.js",
    "en": ROOT / "web/frontend/locales/en.js",
    "mu9b_guard": ROOT / "tools/check_mu9b_account_lifecycle_ui.py",
    "current": ROOT / "docs/CURRENT.md",
    "workstream": ROOT / "docs/development/post-phase69-multiuser-workstream.md",
    "slice_doc": ROOT / "docs/development/post-phase69-mu9c-session-revoke-ui.md",
}
TEXT = {}
for name, path in FILES.items():
    if not path.exists():
        raise SystemExit(f"missing file: {path.relative_to(ROOT)}")
    TEXT[name] = path.read_text(encoding="utf-8")


def require(name, marker):
    if marker not in TEXT[name]:
        raise AssertionError(f"{name} missing marker: {marker}")


def forbid(name, marker):
    if marker in TEXT[name]:
        raise AssertionError(f"{name} contains forbidden marker: {marker}")


def main():
    for marker in (
        "getAccountSession(", "revokeAccountSession(",
        "VdrSuiteBrowserSession", "csrfHeaders", "restore",
        "ifMatch", "credentials: 'same-origin'",
    ):
        require("adapter", marker)

    for marker in (
        "settings-account-admin-session-revoke",
        "accountAdminRevokeSessionConfirm",
        "sessionMutationErrorText",
        "status === 412",
        "revokeSession",
    ):
        require("ui", marker)

    for name in ("adapter", "ui"):
        for marker in (
            "createAccount(", "setAccountGrant(", "revokeAccountCredential(",
        ):
            forbid(name, marker)

    forbid("ui", ".innerHTML")

    for marker in (
        "getAccountSession", "revokeAccountSession",
        "session-rev-1", "csrf-mu9c",
        "stale Session revoke must not retry implicitly",
        "cancelled confirmation must not mutate",
        "test_mu9c_session_revoke_ui passed",
    ):
        require("test", marker)

    for marker in (
        "settings.accountAdminRevokeSession",
        "settings.accountAdminRevokeSessionConfirm",
        "settings.accountAdminSessionRevisionConflict",
        "settings.accountAdminSessionRevisionRequired",
    ):
        require("de", marker)
        require("en", marker)

    require("make", "test-mu9c-session-revoke-ui:")
    require("make", "node web/frontend/tests/test_mu9c_session_revoke_ui.js")
    require("make", "python3 tools/check_mu9c_session_revoke_ui.py")
    require("mu9b_guard",
        "MU9C_STATUS=SUCCESSOR_STATUS_NOT_OWNED_BY_MU9B_GUARD")

    require("current",
        "MU.9C - Session Revoke UI [IMPLEMENTATION CANDIDATE]")
    forbid("current", "The successor MU.9 slice has not yet been selected.")
    require("workstream",
        "MU.9C - Session Revoke UI [IMPLEMENTATION CANDIDATE]")
    require("slice_doc", "Status: **IMPLEMENTATION CANDIDATE**")

    print("MU.9C Session revoke UI contracts passed")
    print("MU9B=IMPLEMENTATION_MERGED_PR_420_REAL_YAVDR_PASS_HOSTED_CI_GREEN")
    print("MU9C=IMPLEMENTATION_CANDIDATE")
    print("PHASE70=NOT_STARTED")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print("MU.9C Session revoke UI check failed:")
        print(f"- {error}")
        raise SystemExit(1)
