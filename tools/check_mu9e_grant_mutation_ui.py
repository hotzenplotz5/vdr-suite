#!/usr/bin/env python3
"""Guard the bounded MU.9E Backend Grant mutation UI slice."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FILES = {
    "adapter": ROOT / "web/frontend/api/account-admin-client-api.js",
    "ui": ROOT / "web/frontend/settings-account-admin.js",
    "test": ROOT / "web/frontend/tests/test_mu9e_grant_mutation_ui.js",
    "make": ROOT / "mk/mu9-account-admin-ui.mk",
    "de": ROOT / "web/frontend/locales/de.js",
    "en": ROOT / "web/frontend/locales/en.js",
    "mu9d_guard": ROOT / "tools/check_mu9d_credential_revoke_ui.py",
    "client": ROOT / "clients/reference-js/public-v1-client.js",
    "current": ROOT / "docs/CURRENT.md",
    "workstream": ROOT / "docs/development/post-phase69-multiuser-workstream.md",
    "slice_doc": ROOT / "docs/development/post-phase69-mu9e-grant-mutation-ui.md",
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
        "setAccountGrant(", "VdrSuiteBrowserSession", "csrfHeaders", "restore",
        "ifMatch", "credentials: 'same-origin'",
    ):
        require("adapter", marker)

    for marker in (
        "grantMutationErrorText",
        "settings-account-admin-grant-ensure",
        "settings-account-admin-grant-revoke",
        "overview.grantsEtag",
        "status === 409",
        "status === 412",
        "status === 422",
        "status === 428",
        "ensureGrant",
        "revokeGrant",
    ):
        require("ui", marker)

    forbid("adapter", "createAccount(")
    forbid("ui", "createAccount(")
    forbid("ui", ".innerHTML")

    # Product authorization remains server-owned; no copied permission catalogue
    # belongs in the browser source.
    for marker in (
        "role.admin",
        "role.read-only",
        "channels.view",
        "timers.view",
        "recordings.view",
    ):
        forbid("ui", marker)

    for marker in (
        "grant-set ETag must fence ensure",
        "stale Grant mutation must not retry implicitly",
        "unsupported Grant mutation must not retry implicitly",
        "final-admin Grant rejection must not retry implicitly",
        "missing grant-set ETag must fail before mutation",
        "cancelled Grant revoke must not mutate",
        "test_mu9e_grant_mutation_ui passed",
        "csrf-mu9e",
    ):
        require("test", marker)

    for marker in (
        "settings.accountAdminGrantPermission",
        "settings.accountAdminGrantBackend",
        "settings.accountAdminGrantServerPolicy",
        "settings.accountAdminEnsureGrant",
        "settings.accountAdminRevokeGrant",
        "settings.accountAdminGrantFinalAdministrator",
        "settings.accountAdminGrantRevisionConflict",
        "settings.accountAdminGrantUnsupported",
        "settings.accountAdminGrantRevisionRequired",
    ):
        require("de", marker)
        require("en", marker)

    require("client", "setAccountGrant(options)")
    require("client", "permission: normalizedOptions.permission")
    require("client", "backendId: normalizedOptions.backendId")
    require("client", "active: normalizedOptions.active")

    require("make", "test-mu9e-grant-mutation-ui:")
    require("make", "node web/frontend/tests/test_mu9e_grant_mutation_ui.js")
    require("make", "python3 tools/check_mu9e_grant_mutation_ui.py")
    require("mu9d_guard",
        "MU9E_STATUS=SUCCESSOR_STATUS_NOT_OWNED_BY_MU9D_GUARD")

    require("current",
        "MU.9D - Human-password Credential Revoke UI [IMPLEMENTATION MERGED - PR #428 / FOCUSED YAVDR PASS / HOSTED CI GREEN]")
    require("current",
        "MU.9E - Backend Grant Mutation UI [IMPLEMENTATION CANDIDATE]")
    require("workstream",
        "MU.9E - Backend Grant Mutation UI [IMPLEMENTATION CANDIDATE]")
    require("slice_doc", "Status: **IMPLEMENTATION CANDIDATE**")
    require("slice_doc", "does **not** hardcode that allowlist")
    require("slice_doc", "Phase 70 remains **NOT STARTED**")

    print("MU.9E Backend Grant mutation UI contracts passed")
    print("MU9D=IMPLEMENTATION_MERGED_PR_428_FOCUSED_YAVDR_PASS_HOSTED_CI_GREEN")
    print("MU9E=IMPLEMENTATION_CANDIDATE")
    print("PHASE70=NOT_STARTED")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print("MU.9E Backend Grant mutation UI check failed:")
        print(f"- {error}")
        raise SystemExit(1)
