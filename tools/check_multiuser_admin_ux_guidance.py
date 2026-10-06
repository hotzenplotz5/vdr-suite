#!/usr/bin/env python3
"""Guard the post-MU.9 administration usability productization slice."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FILES = {
    "security_h": ROOT / "core/security/include/HumanAccountGrantAdministrationService.h",
    "security_cpp": ROOT / "core/security/src/HumanAccountGrantAdministrationService.cpp",
    "api_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "api_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "http": ROOT / "core/http/src/TestHttpServer.cpp",
    "adapter": ROOT / "web/frontend/api/account-admin-client-api.js",
    "ui": ROOT / "web/frontend/settings-account-admin.js",
    "css": ROOT / "web/frontend/style.css",
    "de": ROOT / "web/frontend/locales/de.js",
    "en": ROOT / "web/frontend/locales/en.js",
    "mu9e_test": ROOT / "web/frontend/tests/test_mu9e_grant_mutation_ui.js",
    "current": ROOT / "docs/CURRENT.md",
    "workstream": ROOT / "docs/development/post-phase69-multiuser-workstream.md",
    "slice": ROOT / "docs/development/post-phase69-multiuser-administration-usability.md",
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
        "supportedGrantPermissions",
        "supportedGrantPermissionOptions",
        "supportedGrantScopeKinds",
    ):
        require("security_h", marker)
        require("security_cpp", marker)

    require("security_cpp", 'permission.rfind("role.", 0U)')
    require("security_cpp", 'return {"global", "backend"}')

    for marker in (
        "supportedPermissions",
        "supportedPermissionOptions",
        "supportedScopeKinds",
    ):
        require("api_h", marker)
        require("api_cpp", marker)
        require("http", marker)

    for marker in (
        "listBackends",
        "getBackends",
        "supportedPermissionOptions",
        "supportedScopeKinds",
    ):
        require("adapter", marker)

    for marker in (
        "settings-account-admin-guide",
        "settings-account-admin-guide-step",
        "settings-account-admin-technical",
        "settings-account-admin-grant-permission-input",
        "settings-account-admin-grant-backend-input",
        "document.createElement('select')",
        "supportedPermissionOptions",
        "supportedScopeKinds",
        "settings.accountAdminPermissionLabel.",
    ):
        require("ui", marker)

    forbid("ui", "permissionInput.type = 'text'")
    forbid("ui", "backendInput.type = 'text'")
    forbid("ui", ".innerHTML")

    # The browser may translate server-supplied presentation keys, but it must
    # not contain a copied authorization allowlist.
    for marker in (
        "role.admin",
        "role.read-only",
        "channels.view",
        "timers.view",
        "recordings.rename",
    ):
        forbid("ui", marker)

    for marker in (
        ".settings-account-admin-guide",
        ".settings-account-admin-callout",
        ".settings-account-admin-technical",
        ".settings-account-admin-grant-picker",
    ):
        require("css", marker)

    for locale in ("de", "en"):
        for marker in (
            "settings.accountAdminHowTo",
            "settings.accountAdminSetupTitle",
            "settings.accountAdminGrants",
            "settings.accountAdminChoosePermission",
            "settings.accountAdminTechnicalDetails",
            "settings.accountAdminPermissionLabel.role.admin",
            "settings.accountAdminPermissionLabel.media.live.play",
            "settings.accountAdminSessionsHint",
        ):
            require(locale, marker)

    for marker in (
        "grant permission must be a server-backed selector, not free text",
        "grant scope must be a selector, not free text",
        "backend selector must use canonical Public-v1 backend discovery",
        "supportedPermissionOptions",
    ):
        require("mu9e_test", marker)

    require("slice", "Status: **CANDIDATE**")
    require("slice", "It is not")
    require("slice", "MU.9G")
    require("slice", "browser must not own or invent the permission")
    require("slice", "MU.10 remains the planned Device/app pairing successor")
    require("slice", "Phase 70 remains not started")

    require("current", "Post-MU.9 Administration Usability [CANDIDATE]")
    require("current", "MU.9 - Account/access administration UI [COMPLETED - CLOSEOUT PR #431]")
    require("workstream", "Post-MU.9 — Administration usability [CURRENT CANDIDATE]")

    print("Post-MU.9 administration usability contracts passed")
    print("MULTIUSER_ADMIN_UX=CANDIDATE")
    print("MU9=COMPLETED_CLOSEOUT_PR_431")
    print("MU10=NOT_STARTED")
    print("PHASE70=NOT_STARTED")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print("Post-MU.9 administration usability check failed:")
        print(f"- {error}")
        raise SystemExit(1)
