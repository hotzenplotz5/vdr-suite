#!/usr/bin/env python3
"""Guard the bounded MU.9A read-only Account administration UI slice."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FILES = {
    "index": ROOT / "web/frontend/index.html",
    "app": ROOT / "web/frontend/app.js",
    "style": ROOT / "web/frontend/style.css",
    "adapter": ROOT / "web/frontend/api/account-admin-client-api.js",
    "ui": ROOT / "web/frontend/settings-account-admin.js",
    "test": ROOT / "web/frontend/tests/test_mu9a_account_admin_read_ui.js",
    "paths": ROOT / "core/http/src/TestHttpServerPaths.inc",
    "install": ROOT / "mk/install.mk",
    "makefile": ROOT / "Makefile",
    "slice_make": ROOT / "mk/mu9-account-admin-ui.mk",
    "mu8_guard": ROOT / "tools/check_mu8c_credential_revoke.py",
    "current": ROOT / "docs/CURRENT.md",
    "workstream": ROOT / "docs/development/post-phase69-multiuser-workstream.md",
    "slice_doc": ROOT / "docs/development/post-phase69-mu9a-account-admin-read-ui.md",
}
TEXT = {}
for name, path in FILES.items():
    if not path.exists():
        raise SystemExit(f"missing file: {path.relative_to(ROOT)}")
    TEXT[name] = path.read_text(encoding="utf-8")

public_browser = ROOT / "web/frontend/api/public-v1-client.js"
public_canonical = ROOT / "clients/reference-js/public-v1-client.js"
if not public_browser.is_symlink():
    raise SystemExit("MU.9A public-v1 browser asset must remain a symlink")
if public_browser.resolve() != public_canonical.resolve():
    raise SystemExit("MU.9A public-v1 browser asset drifted from accepted reference client")

def require(name, marker):
    if marker not in TEXT[name]:
        raise AssertionError(f"{name} missing marker: {marker}")

def forbid(name, marker):
    if marker in TEXT[name]:
        raise AssertionError(f"{name} contains forbidden marker: {marker}")

def main():
    scripts = [
        "../frontend/api/public-v1-client.js",
        "../frontend/api/account-admin-client-api.js",
        "../frontend/settings-account-admin.js",
        "../frontend/app.js",
    ]
    positions = [TEXT["index"].find(script) for script in scripts]
    if any(position < 0 for position in positions) or positions != sorted(positions):
        raise AssertionError("MU.9A script ownership/order drifted")

    require("app", "window.VdrSuiteAccountAdminSettings")
    require("app", "accountAdminSettings.render(panel)")

    for marker in (
        '"/frontend/api/public-v1-client.js"', '"api/public-v1-client.js"',
        '"/frontend/api/account-admin-client-api.js"', '"api/account-admin-client-api.js"',
        '"/frontend/settings-account-admin.js"', '"settings-account-admin.js"',
    ):
        require("paths", marker)

    for marker in (
        "web/frontend/api/public-v1-client.js",
        "web/frontend/api/account-admin-client-api.js",
        "web/frontend/settings-account-admin.js",
    ):
        require("install", marker)

    require("makefile", "include mk/mu9-account-admin-ui.mk")
    require("slice_make", "test-mu9a-account-admin-read-ui:")
    require("slice_make", "node web/frontend/tests/test_mu9a_account_admin_read_ui.js")
    require("slice_make", "python3 tools/check_mu9a_account_admin_read_ui.py")

    for marker in (
        "VdrSuitePublicV1Client", "credentials: 'same-origin'",
        "getAccounts(", "getAccount(", "getAccountGrants(",
        "getAccountCredentials(", "getAccountSessions(", "cache: 'no-store'",
    ):
        require("adapter", marker)

    require("ui", "VdrSuiteAccountAdminClientApi")
    require("ui", "listAccounts(")
    require("ui", "loadAccount(")
    require("ui", "settings-account-admin")
    forbid("ui", ".innerHTML")

    for marker in (
        "fs.realpathSync(publicBrowserPath)", "getAccountGrants",
        "getAccountCredentials", "getAccountSessions",
        "test_mu9a_account_admin_read_ui passed",
    ):
        require("test", marker)

    require("current",
        "MU.9A merged evidence: [MU.9A Account Administration Read UI]")
    forbid("current", "The successor MU.9 slice has not yet been selected.")
    require("workstream",
        "MU.9A - Account Administration Read UI [IMPLEMENTATION MERGED - PR #418 / EXACT MERGED-MAIN YAVDR PASS / HOSTED CI GREEN]")
    require("slice_doc",
        "Status: **IMPLEMENTATION MERGED — PR #418 / EXACT MERGED-MAIN YAVDR PASS / HOSTED CI GREEN**")
    require("mu8_guard",
        "MU9_STATUS=SUCCESSOR_STATUS_NOT_OWNED_BY_MU8C_GUARD")

    print("MU.9A Account administration read UI contracts passed")
    print("MU9A=IMPLEMENTATION_MERGED_PR_418_EXACT_MERGED_MAIN_YAVDR_PASS_HOSTED_CI_GREEN")
    print("MU9B_STATUS=SUCCESSOR_STATUS_NOT_OWNED_BY_MU9A_GUARD")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print("MU.9A Account administration read UI check failed:")
        print(f"- {error}")
        raise SystemExit(1)
