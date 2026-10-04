#!/usr/bin/env python3
"""Guard the bounded MU.9B Account lifecycle mutation UI slice."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FILES = {
    "adapter": ROOT / "web/frontend/api/account-admin-client-api.js",
    "ui": ROOT / "web/frontend/settings-account-admin.js",
    "test": ROOT / "web/frontend/tests/test_mu9b_account_lifecycle_mutation_ui.js",
    "mu9a_test": ROOT / "web/frontend/tests/test_mu9a_account_admin_read_ui.js",
    "style": ROOT / "web/frontend/style.css",
    "de": ROOT / "web/frontend/locales/de.js",
    "en": ROOT / "web/frontend/locales/en.js",
    "make": ROOT / "mk/mu9-account-admin-ui.mk",
    "mu9a_guard": ROOT / "tools/check_mu9a_account_admin_read_ui.py",
    "current": ROOT / "docs/CURRENT.md",
    "workstream": ROOT / "docs/development/post-phase69-multiuser-workstream.md",
    "slice_doc": ROOT / "docs/development/post-phase69-mu9b-account-lifecycle-ui.md",
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
        "VdrSuiteBrowserSession", "csrfHeaders", "restore",
        "updateAccountDisplayName(", "activateAccount(", "deactivateAccount(",
        "ifMatch", "credentials: 'same-origin'",
    ):
        require("adapter", marker)

    for marker in (
        "settings-account-admin-display-name-input",
        "settings-account-admin-save-name",
        "settings-account-admin-toggle-active",
        "accountAdminDeactivateConfirm",
        "mutationErrorText",
        "status === 409",
        "status === 412",
        "loadAccount(",
    ):
        require("ui", marker)

    for name in ("adapter", "ui"):
        for marker in (
            "createAccount(", "setAccountGrant(",
            "revokeAccountCredential(", "revokeAccountSession(",
        ):
            forbid(name, marker)

    forbid("ui", ".innerHTML")

    for marker in (
        "X-CSRF-Token", "csrf-mu9b", '"rev-1"', '"rev-2"', '"rev-3"',
        "updateAccountDisplayName", "deactivateAccount", "activateAccount",
        "test_mu9b_account_lifecycle_mutation_ui passed",
    ):
        require("test", marker)

    for marker in (
        "settings-account-admin-lifecycle",
        "settings-account-admin-display-name-input",
    ):
        require("style", marker)

    for marker in (
        "settings.accountAdminSaveDisplayName",
        "settings.accountAdminDeactivateConfirm",
        "settings.accountAdminFinalAdministrator",
        "settings.accountAdminRevisionConflict",
    ):
        require("de", marker)
        require("en", marker)

    require("make", "test-mu9b-account-lifecycle-ui:")
    require("make", "node web/frontend/tests/test_mu9b_account_lifecycle_mutation_ui.js")
    require("make", "python3 tools/check_mu9b_account_lifecycle_ui.py")
    require("mu9a_guard",
        "MU9B_STATUS=SUCCESSOR_STATUS_NOT_OWNED_BY_MU9A_GUARD")

    require("current",
        "MU.9B - Account Lifecycle Mutation UI [IMPLEMENTATION MERGED - PR #420 / REAL YAVDR PASS / HOSTED CI GREEN]")
    require("workstream",
        "MU.9B - Account Lifecycle Mutation UI [IMPLEMENTATION MERGED - PR #420 / REAL YAVDR PASS / HOSTED CI GREEN]")
    require("slice_doc",
        "Status: **IMPLEMENTATION MERGED — PR #420 / REAL YAVDR PASS / HOSTED CI GREEN**")
    require("current", "successor MU.9 slice - exact scope not yet selected")
    require("workstream", "successor MU.9 slice - exact scope not yet selected")

    print("MU.9B Account lifecycle mutation UI contracts passed")
    print("MU9A=IMPLEMENTATION_MERGED_PR_418_EXACT_MERGED_MAIN_YAVDR_PASS_HOSTED_CI_GREEN")
    print("MU9B=IMPLEMENTATION_MERGED_PR_420_REAL_YAVDR_PASS_HOSTED_CI_GREEN")
    print("MU9_SUCCESSOR_STATUS=NOT_SELECTED")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print("MU.9B Account lifecycle mutation UI check failed:")
        print(f"- {error}")
        raise SystemExit(1)
