#!/usr/bin/env python3
"""Guard the bounded MU.9D human-password Credential revoke UI slice."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FILES = {
    "adapter": ROOT / "web/frontend/api/account-admin-client-api.js",
    "ui": ROOT / "web/frontend/settings-account-admin.js",
    "test": ROOT / "web/frontend/tests/test_mu9d_credential_revoke_ui.js",
    "make": ROOT / "mk/mu9-account-admin-ui.mk",
    "de": ROOT / "web/frontend/locales/de.js",
    "en": ROOT / "web/frontend/locales/en.js",
    "mu9c_guard": ROOT / "tools/check_mu9c_session_revoke_ui.py",
    "current": ROOT / "docs/CURRENT.md",
    "workstream": ROOT / "docs/development/post-phase69-multiuser-workstream.md",
    "slice_doc": ROOT / "docs/development/post-phase69-mu9d-credential-revoke-ui.md",
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
        "getAccountCredential(", "revokeAccountCredential(",
        "VdrSuiteBrowserSession", "csrfHeaders", "restore",
        "ifMatch", "credentials: 'same-origin'",
    ):
        require("adapter", marker)

    for marker in (
        "settings-account-admin-credential-revoke",
        "accountAdminRevokeCredentialConfirm",
        "credentialMutationErrorText",
        "status === 409",
        "status === 412",
        "revokeCredential",
        "credential.credentialType === 'human-password'",
    ):
        require("ui", marker)

    for name in ("adapter", "ui"):
        forbid(name, "createAccount(")

    forbid("ui", ".innerHTML")

    for marker in (
        "getAccountCredential", "revokeAccountCredential",
        "credential-human-a", "credential-browser-a",
        "credential-rev-1", "csrf-mu9d",
        "only the active human-password Credential may expose revoke control",
        "stale Credential revoke must not retry implicitly",
        "final-admin rejection must not retry implicitly",
        "cancelled confirmation must not mutate",
        "test_mu9d_credential_revoke_ui passed",
    ):
        require("test", marker)

    for marker in (
        "settings.accountAdminRevokeCredential",
        "settings.accountAdminRevokeCredentialConfirm",
        "settings.accountAdminCredentialFinalAdministrator",
        "settings.accountAdminCredentialRevisionConflict",
        "settings.accountAdminCredentialRevisionRequired",
    ):
        require("de", marker)
        require("en", marker)

    require("make", "test-mu9d-credential-revoke-ui:")
    require("make", "node web/frontend/tests/test_mu9d_credential_revoke_ui.js")
    require("make", "python3 tools/check_mu9d_credential_revoke_ui.py")
    require("mu9c_guard",
        "MU9D_STATUS=SUCCESSOR_STATUS_NOT_OWNED_BY_MU9C_GUARD")

    require("current",
        "MU.9D - Human-password Credential Revoke UI [IMPLEMENTATION MERGED - PR #428 / FOCUSED YAVDR PASS / HOSTED CI GREEN]")
    require("workstream",
        "MU.9D - Human-password Credential Revoke UI [IMPLEMENTATION MERGED - PR #428 / FOCUSED YAVDR PASS / HOSTED CI GREEN]")
    require("slice_doc",
        "Status: **IMPLEMENTATION MERGED — PR #428 / FOCUSED YAVDR PASS / HOSTED CI GREEN**")
    require("slice_doc", "post-merge `main` CI run #9761")
    require("slice_doc", "final-usable-administrator")
    require("slice_doc", "issuer-session fencing remains server-owned")

    print("MU.9D Credential revoke UI contracts passed")
    print("MU9C=IMPLEMENTATION_MERGED_PR_426_FOCUSED_YAVDR_PASS_HOSTED_CI_GREEN")
    print("MU9D=IMPLEMENTATION_MERGED_PR_428_FOCUSED_YAVDR_PASS_HOSTED_CI_GREEN")
    print("MU9E_STATUS=SUCCESSOR_STATUS_NOT_OWNED_BY_MU9D_GUARD")
    print("PHASE70=NOT_STARTED")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print("MU.9D Credential revoke UI check failed:")
        print(f"- {error}")
        raise SystemExit(1)
