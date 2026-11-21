#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "adr": ROOT / "docs/adr/ADR-0067-human-account-backend-access-administration.md",
    "audit": ROOT / "docs/development/post-phase69-mu5-administration-architecture-acceptance.md",
    "workstream": ROOT / "docs/development/post-phase69-multiuser-workstream.md",
    "current": ROOT / "docs/CURRENT.md",
    "status": ROOT / "docs/development/current-status.md",
    "roadmap": ROOT / "docs/planning/roadmap.md",
    "phase_map": ROOT / "docs/planning/phase-map.md",
    "index": ROOT / "docs/adr/index.md",
    "security": ROOT / "docs/architecture/security-identity-foundation.md",
}

texts = {name: path.read_text(encoding="utf-8") for name, path in FILES.items()}


def require(name, marker):
    if marker not in texts[name]:
        raise AssertionError(f"{name} misses required marker: {marker}")


def forbid(name, marker):
    if marker in texts[name]:
        raise AssertionError(f"{name} retains forbidden stale marker: {marker}")


def main():
    require("adr", "## Status\n\nAccepted architecture.")
    require("adr", "MU.5 Administration Architecture Acceptance")

    for permission in (
        "accounts.view",
        "accounts.grants.view",
        "accounts.credentials.view",
        "accounts.sessions.view",
        "accounts.create",
        "accounts.modify",
        "accounts.activate",
        "accounts.deactivate",
        "accounts.grants.modify",
        "accounts.credentials.revoke",
        "accounts.sessions.revoke",
    ):
        require("adr", permission)

    for marker in (
        "All of these administration permissions use global authorization scope",
        "backend-scoped",
        "role.admin@backend-id",
        "role.read-only@*",
        "one server-generated Human Account ID",
        "one active",
        "human-password",
        "does **not** grant",
        "bounded durable create",
        "monotonic persisted Account revision",
        "Deactivation takes effect immediately",
        "revoke all still-active browser sessions",
        "Browser-session authentication/resolution must also consult current effective",
        "Final usable administrator is protected transactionally",
        "role.admin@*",
        "SecurityPermissionGrantRepository",
        "Class-A",
        "MU.6 — Human Account lifecycle administration",
        "Phase 70 Recommendation and Content Knowledge Graph remains not started",
    ):
        require("adr", marker)

    forbid("adr", "Proposed architecture.")
    forbid("adr", "must be accepted before the next Multiuser runtime slice")

    for marker in (
        "fb5169d38530374e3b05bcbe5431c34ab5ae102c",
        "HumanAccountRepository",
        "SecurityPermissionGrantRepository",
        "BrowserSessionAuthenticator",
        "would therefore block new password logins while potentially leaving",
        "MU.6 — Human Account lifecycle administration",
        "Phase 70 remains not started",
    ):
        require("audit", marker)

    for name in ("workstream", "current", "status", "roadmap", "phase_map"):
        forbid(name, "ADR-0067 [PROPOSED]")
        forbid(name, "ADR-0067 is proposed")

    require("workstream", "### MU.5 — Administration architecture contract [COMPLETED]")
    require("workstream", "ADR-0067 Account/Backend Access Administration [ACCEPTED]")

    require("current", "MU.5 - Administration architecture contract - ADR-0067 [COMPLETED]")
    require("status", "MU.5 - Administration architecture contract - ADR-0067 [COMPLETED]")
    require("roadmap", "MU.5 Administration architecture contract / ADR-0067            [DONE]")

    require("index", "Latest accepted canonical ADR:")
    require("index", "ADR-0067")
    require("index", "## Proposed Canonical ADRs\n\nNone.")

    require("security", "MU.5 is completed and ADR-0067 is accepted")

    print("MU.5 administration architecture contracts passed")
    print("ADR-0067=ACCEPTED")
    print("MU.5=COMPLETED")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except AssertionError as error:
        print("MU.5 administration architecture check failed:")
        print("- " + str(error))
        sys.exit(1)
