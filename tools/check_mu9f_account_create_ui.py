#!/usr/bin/env python3
"""Guard the bounded MU.9F Account CREATE UI slice."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FILES = {
    "adapter": ROOT / "web/frontend/api/account-admin-client-api.js",
    "ui": ROOT / "web/frontend/settings-account-admin.js",
    "test": ROOT / "web/frontend/tests/test_mu9f_account_create_ui.js",
    "make": ROOT / "mk/mu9-account-admin-ui.mk",
    "de": ROOT / "web/frontend/locales/de.js",
    "en": ROOT / "web/frontend/locales/en.js",
    "client": ROOT / "clients/reference-js/public-v1-client.js",
    "mu9e_guard": ROOT / "tools/check_mu9e_grant_mutation_ui.py",
    "current": ROOT / "docs/CURRENT.md",
    "workstream": ROOT / "docs/development/post-phase69-multiuser-workstream.md",
    "slice_doc": ROOT / "docs/development/post-phase69-mu9f-account-create-ui.md",
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
        "createAccount(", "VdrSuiteBrowserSession", "csrfHeaders", "restore",
        "idempotencyKey", "credentials: 'same-origin'",
    ):
        require("adapter", marker)

    for marker in (
        "createAccountErrorText",
        "createAccountIdempotencyKey",
        "cryptoApi.randomUUID",
        "settings-account-admin-create-login",
        "settings-account-admin-create-display-name",
        "settings-account-admin-create-password",
        "settings-account-admin-create-submit",
        "createPasswordInput.value = ''",
        "status === 409",
        "idempotency_conflict",
        "status === 400",
        "status === 415",
        "status === 422",
        "status === 503",
        "api.createAccount(",
        "loadAccounts(false)",
    ):
        require("ui", marker)

    for marker in (".innerHTML", "localStorage", "sessionStorage"):
        forbid("ui", marker)

    for marker in (
        "one user action must emit one Account CREATE",
        "Account CREATE must reuse the canonical browser/Public-v1 options",
        "password input must be cleared when CREATE is dispatched",
        "successful Account CREATE must refresh the Account list",
        "Account CREATE must not assign grants automatically",
        "failed Account CREATE must not retry implicitly",
        "each explicit create action must own a fresh Idempotency-Key",
        "blank Account CREATE fields must fail before mutation",
        "missing secure Idempotency-Key source must fail before mutation",
        "csrf-mu9f",
        "test_mu9f_account_create_ui passed",
    ):
        require("test", marker)

    for marker in (
        "settings.accountAdminCreateTitle",
        "settings.accountAdminCreateLoginName",
        "settings.accountAdminCreateDisplayName",
        "settings.accountAdminCreatePassword",
        "settings.accountAdminCreateNoAutomaticAccess",
        "settings.accountAdminCreateSubmit",
        "settings.accountAdminCreateFieldsRequired",
        "settings.accountAdminCreateIdempotencyUnavailable",
        "settings.accountAdminCreateOperationConflict",
        "settings.accountAdminCreateIdempotencyConflict",
        "settings.accountAdminCreateInvalidRequest",
        "settings.accountAdminCreateInvalidFields",
        "settings.accountAdminCreateUnavailable",
    ):
        require("de", marker)
        require("en", marker)

    for marker in (
        "createAccount(options)",
        "requestAccountCreate(",
        "idempotencyKey",
        "loginName: normalizedOptions.loginName",
        "displayName: normalizedOptions.displayName",
        "password: normalizedOptions.password",
    ):
        require("client", marker)

    require("make", "test-mu9f-account-create-ui:")
    require("make", "node web/frontend/tests/test_mu9f_account_create_ui.js")
    require("make", "python3 tools/check_mu9f_account_create_ui.py")
    require("mu9e_guard",
        "MU9E=IMPLEMENTATION_MERGED_PR_429_FOCUSED_YAVDR_PASS_POST_MERGE_CI_9763_GREEN")
    require("mu9e_guard",
        "MU9F_STATUS=SUCCESSOR_STATUS_NOT_OWNED_BY_MU9E_GUARD")

    require("current",
        "MU.9E - Backend Grant Mutation UI [IMPLEMENTATION MERGED - PR #429 / FOCUSED YAVDR PASS / POST-MERGE CI #9763 GREEN]")
    require("current", "MU.9F - Account CREATE UI [IMPLEMENTATION CANDIDATE]")
    require("workstream", "MU.9F - Account CREATE UI [IMPLEMENTATION CANDIDATE]")
    require("slice_doc", "Status: **IMPLEMENTATION CANDIDATE**")
    require("slice_doc", "caller-owned Idempotency-Key")
    require("slice_doc", "Phase 70 remains **NOT STARTED**")

    print("MU.9F Account CREATE UI contracts passed")
    print("MU9E=IMPLEMENTATION_MERGED_PR_429_FOCUSED_YAVDR_PASS_POST_MERGE_CI_9763_GREEN")
    print("MU9F=IMPLEMENTATION_CANDIDATE")
    print("PHASE70=NOT_STARTED")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print("MU.9F Account CREATE UI check failed:")
        print(f"- {error}")
        raise SystemExit(1)
