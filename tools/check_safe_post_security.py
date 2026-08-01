#!/usr/bin/env python3

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(path: str, needle: str) -> None:
    if needle not in read(path):
        raise AssertionError(
            f"{path}: missing safe POST contract: {needle}"
        )


def forbid(path: str, needle: str) -> None:
    if needle in read(path):
        raise AssertionError(
            f"{path}: forbidden safe POST contract: {needle}"
        )


def main() -> int:
    gate = "core/security/include/SecurityHttpGate.h"
    test = "core/security/tests/test_safe_post_security.cpp"
    fixture = (
        "core/security/tests/"
        "SecurityHttpGateBrowserTestFixture.h"
    )
    makefile = "mk/security-sources.mk"
    acceptance_makefile = "mk/phase62-runtime-acceptance.mk"
    safe_runner = (
        "tools/phase62-runtime-acceptance/"
        "safe-post-runner.py"
    )
    safe_manifest = (
        "tools/phase62-runtime-acceptance/"
        "slice-2m-safe-post.json"
    )
    recording_frontend_test = (
        "web/frontend/tests/"
        "test_recording_execution_security_runtime.js"
    )
    searchtimer_frontend_test = (
        "web/frontend/tests/"
        "test_searchtimer_maintenance_security_runtime.js"
    )
    document = (
        "docs/development/"
        "phase-62-slice-2m-safe-post-classification.md"
    )
    index = "docs/development/index.md"

    for relative in (
        test,
        fixture,
        safe_runner,
        safe_manifest,
        recording_frontend_test,
        searchtimer_frontend_test,
        document,
    ):
        if not (ROOT / relative).is_file():
            raise AssertionError(
                f"missing safe POST contract file: {relative}"
            )

    require(gate, "const bool isSafePost")

    for route in (
        '"/api/recordings/actions/validate"',
        '"/api/vdr/recordings/actions/validate"',
        '"/api/recordings/actions/preview"',
        '"/api/vdr/recordings/actions/preview"',
        '"/api/searchtimers/validate"',
        '"/api/vdr/searchtimers/validate"',
        '"/api/searchtimers/plan"',
        '"/api/vdr/searchtimers/plan"',
    ):
        require(gate, route)
        require(test, route)
        require(safe_manifest, route)

    require(gate, "if (isSafePost)")
    require(gate, "!gate.context.authenticated()")
    require(test, "!browserDecision.protectedMutation")
    require(test, "evidenceBefore")
    require(test, "security_policy_not_migrated")
    require(makefile, "test-security-safe-post:")
    require(makefile, "test_safe_post_security.cpp")
    require(safe_runner, "BASE.RuntimeAcceptance")
    require(safe_runner, "safe_post_browser")
    require(safe_runner, "resource_state_changed")
    require(safe_runner, "phase62_safe_post_acceptance=passed")
    require(
        acceptance_makefile,
        "phase62-runtime-acceptance-batch:",
    )
    require(
        acceptance_makefile,
        "slice-2m-safe-post.json",
    )
    require(
        recording_frontend_test,
        "/api/vdr/recordings/actions/validate",
    )
    require(
        searchtimer_frontend_test,
        "/api/vdr/searchtimers/validate",
    )
    require(
        searchtimer_frontend_test,
        "/api/vdr/searchtimers/plan",
    )
    require(
        index,
        "phase-62-slice-2m-safe-post-classification.md",
    )

    safe_section = read(gate).split(
        "const bool isSafePost =",
        1,
    )[1].split(
        "const bool isProtectedMutation =",
        1,
    )[0]

    for forbidden_route in (
        'path == "/api/searchtimers/execute"',
        'path == "/api/vdr/searchtimers/execute"',
        'path == "/api/searchtimers/real-test"',
        'path == "/api/vdr/searchtimers/real-test"',
        'path == "/api/epg/cache/refresh"',
        'path == "/api/epgsearch/native-fuzzy/refresh"',
        '"/api/epgsearch/native-fuzzy/stale-probes/delete"',
        '"/api/vdr/epgsearch/native-fuzzy/stale-probes/delete"',
    ):
        if forbidden_route in safe_section:
            raise AssertionError(
                "unsafe route entered safe POST classification: "
                + forbidden_route
            )

    forbid(document, "SearchTimer execute is safe")

    print("Safe POST security contracts passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
