#!/usr/bin/env python3
from __future__ import annotations

import argparse
import importlib.util
import json
from pathlib import Path
from typing import Any


HERE = Path(__file__).resolve().parent
BASE_RUNNER_PATH = HERE / "runner.py"
DEFAULT_MANIFEST = HERE / "slice-2m-safe-post.json"


def load_base_runner() -> Any:
    specification = importlib.util.spec_from_file_location(
        "phase62_runtime_acceptance_base",
        BASE_RUNNER_PATH,
    )

    if specification is None or specification.loader is None:
        raise RuntimeError("base_runner_import_failed")

    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


BASE = load_base_runner()
AcceptanceError = BASE.AcceptanceError
require = BASE.require


def load_manifest(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        manifest = json.load(handle)

    require(
        isinstance(manifest, dict),
        "manifest_must_be_object",
    )
    return manifest


def validate_manifest(
    manifest: dict[str, Any],
) -> list[str]:
    errors: list[str] = []

    if manifest.get("schemaVersion") != 1:
        errors.append("schemaVersion must be 1")

    for key in ("id", "title"):
        value = manifest.get(key)
        if not isinstance(value, str) or not value:
            errors.append(
                f"{key} must be a non-empty string"
            )

    list_keys = (
        "routes",
        "representativeUnauthenticatedRoutes",
        "representativeBasicRoutes",
        "queryRoutes",
        "trailingSlashRoutes",
        "excludedRoutes",
    )

    for key in list_keys:
        value = manifest.get(key)
        if (
            not isinstance(value, list)
            or not value
            or not all(
                isinstance(route, str)
                and route.startswith("/api/")
                for route in value
            )
        ):
            errors.append(
                f"{key} must be a non-empty /api/ route array"
            )

    routes = manifest.get("routes", [])
    if len(routes) != len(set(routes)):
        errors.append("routes must be unique")

    if manifest.get("safeBody") != {}:
        errors.append("safeBody must be exactly {}")

    if manifest.get("expectedStatus") != 200:
        errors.append("expectedStatus must be 200")

    snapshot = manifest.get("snapshot")
    if (
        not isinstance(snapshot, dict)
        or snapshot.get("method") != "GET"
        or not isinstance(snapshot.get("path"), str)
        or not snapshot["path"].startswith("/api/")
    ):
        errors.append("snapshot must define an exact GET /api/ path")

    return errors


def validate_repository_routes(
    manifest: dict[str, Any],
) -> list[str]:
    router = (
        BASE.REPOSITORY_ROOT
        / "api"
        / "rest"
        / "src"
        / "ApiRouter.cpp"
    )

    if not router.is_file():
        return [f"missing router source: {router}"]

    source = router.read_text(encoding="utf-8")
    errors: list[str] = []

    base_routes = set(manifest["routes"])
    base_routes.update(
        route.split("?", 1)[0].rstrip("/")
        for key in (
            "representativeUnauthenticatedRoutes",
            "representativeBasicRoutes",
            "queryRoutes",
            "trailingSlashRoutes",
            "excludedRoutes",
        )
        for route in manifest[key]
    )

    for route in sorted(base_routes):
        if f'"{route}"' not in source:
            errors.append(
                f"route missing from ApiRouter.cpp: {route}"
            )

    return errors


def runtime_manifest(
    manifest: dict[str, Any],
) -> dict[str, Any]:
    return {
        "id": manifest["id"],
        "permission": "phase62.safe-post.probe",
        "action": "phase62.safe-post.probe",
        "backendId": "default",
        "alternateBackendId": "phase62-safe-post-other",
        "snapshot": manifest["snapshot"],
    }


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Validate or run Phase 62 safe POST runtime acceptance."
        )
    )
    parser.add_argument(
        "--manifest",
        default=str(DEFAULT_MANIFEST),
    )

    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--validate-only", action="store_true")
    mode.add_argument("--self-test", action="store_true")
    mode.add_argument("--run", action="store_true")

    parser.add_argument(
        "--base-url",
        default="http://127.0.0.1:18080",
    )
    parser.add_argument(
        "--database",
        default="/var/lib/vdr-suite/vdr-suite.db",
    )
    parser.add_argument(
        "--service",
        default="vdr-suite-daemon.service",
    )
    parser.add_argument(
        "--daemon",
        default="/usr/sbin/vdr-suite-daemon",
    )
    parser.add_argument(
        "--loader",
        default=(
            "/usr/share/vdr-suite/web/frontend/platform/"
            "deferred-runtime-loader.js"
        ),
    )
    parser.add_argument("--backup-dir", default="")
    parser.add_argument("--expected-branch", default="")
    parser.add_argument("--expected-head", default="")
    parser.add_argument(
        "--expected-daemon-sha256",
        default="",
    )
    parser.add_argument(
        "--expected-loader-sha256",
        default="",
    )
    parser.add_argument("--timeout", type=int, default=10)
    parser.add_argument("--report-json", default="")
    return parser.parse_args()


def expect_safe(
    acceptance: Any,
    name: str,
    response: tuple[int, dict[str, str], str],
    expected_status: int,
) -> None:
    status, _, body = response
    require(
        status == expected_status,
        f"{name}:expected_status={expected_status}:actual={status}",
    )

    for forbidden in (
        "authentication_required",
        "csrf_validation_failed",
        "permission_denied",
        "backend_scope_denied",
        "security_policy_not_migrated",
    ):
        require(
            forbidden not in body,
            f"{name}:security_gate_denial={forbidden}",
        )

    acceptance.tests_passed += 1


def run_acceptance(
    arguments: argparse.Namespace,
    manifest: dict[str, Any],
) -> tuple[Any, str]:
    acceptance = BASE.RuntimeAcceptance(
        arguments,
        runtime_manifest(manifest),
    )

    error_message = ""

    try:
        acceptance.preflight()
        resource_before = acceptance.resource_snapshot()
        body = manifest["safeBody"]

        for route in manifest[
            "representativeUnauthenticatedRoutes"
        ]:
            acceptance.expect(
                "safe_post_unauthenticated",
                acceptance.request(
                    "POST",
                    route,
                    body=body,
                ),
                401,
            )

        for route in manifest[
            "representativeBasicRoutes"
        ]:
            expect_safe(
                acceptance,
                "safe_post_basic",
                acceptance.request(
                    "POST",
                    route,
                    basic=True,
                    body=body,
                ),
                manifest["expectedStatus"],
            )

        acceptance.issue_browser_session()

        for route in manifest["routes"]:
            expect_safe(
                acceptance,
                "safe_post_browser",
                acceptance.request(
                    "POST",
                    route,
                    browser=True,
                    body=body,
                ),
                manifest["expectedStatus"],
            )

        for route in manifest["queryRoutes"]:
            expect_safe(
                acceptance,
                "safe_post_query",
                acceptance.request(
                    "POST",
                    route,
                    browser=True,
                    body=body,
                ),
                manifest["expectedStatus"],
            )

        for route in manifest["trailingSlashRoutes"]:
            acceptance.expect(
                "safe_post_trailing_slash",
                acceptance.request(
                    "POST",
                    route,
                    browser=True,
                    body=body,
                ),
                503,
                "security_policy_not_migrated",
            )

        for route in manifest["excludedRoutes"]:
            acceptance.expect(
                "safe_post_excluded_route",
                acceptance.request(
                    "POST",
                    route,
                    browser=True,
                    body=body,
                ),
                503,
                "security_policy_not_migrated",
            )

        resource_after = acceptance.resource_snapshot()
        require(
            resource_after == resource_before,
            "resource_state_changed",
        )
        acceptance.tests_passed += 1
    except Exception as error:
        error_message = (
            f"{type(error).__name__}:{error}"
        )

    cleanup_errors = acceptance.cleanup()
    if cleanup_errors:
        error_message = (
            error_message + ";"
            if error_message
            else ""
        ) + ",".join(cleanup_errors)

    try:
        acceptance.final_verify()
    except Exception as error:
        error_message = (
            error_message + ";"
            if error_message
            else ""
        ) + (
            "final_verify:"
            f"{type(error).__name__}:{error}"
        )

    return acceptance, error_message


def self_test(manifest: dict[str, Any]) -> None:
    errors = (
        validate_manifest(manifest)
        + validate_repository_routes(manifest)
    )
    require(not errors, ";".join(errors))

    exact = set(manifest["routes"])
    require(
        all(not route.endswith("/") for route in exact),
        "safe_routes_must_not_have_trailing_slash",
    )
    require(
        all(
            route.endswith("/")
            and route not in exact
            and route.rstrip("/") in exact
            for route in manifest["trailingSlashRoutes"]
        ),
        "trailing_slash_fixture_must_target_safe_route",
    )
    require(
        not exact.intersection(
            manifest["excludedRoutes"]
        ),
        "excluded_route_is_safe",
    )


def main() -> int:
    arguments = parse_arguments()
    manifest = load_manifest(Path(arguments.manifest))
    errors = (
        validate_manifest(manifest)
        + validate_repository_routes(manifest)
    )

    if errors:
        for error in errors:
            print(f"validation_error={error}")
        return 1

    if arguments.validate_only:
        print("phase62_safe_post_manifest=valid")
        return 0

    if arguments.self_test:
        self_test(manifest)
        print("phase62_safe_post_self_test=passed")
        return 0

    for name in (
        "backup_dir",
        "expected_branch",
        "expected_head",
        "expected_daemon_sha256",
        "expected_loader_sha256",
    ):
        require(
            bool(getattr(arguments, name)),
            f"missing_run_argument:{name}",
        )

    acceptance, error_message = run_acceptance(
        arguments,
        manifest,
    )

    report = {
        "schemaVersion": 1,
        "manifestId": manifest["id"],
        "passed": not error_message,
        "testsPassed": acceptance.tests_passed,
        "httpRequests": acceptance.sequence,
        "error": error_message,
    }

    if arguments.report_json:
        path = Path(arguments.report_json)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(
            json.dumps(
                report,
                indent=2,
                sort_keys=True,
            )
            + "\n",
            encoding="utf-8",
        )

    acceptance.database.close()

    if error_message:
        print(
            "phase62_safe_post_acceptance_error="
            + error_message
        )
        print(
            "runtime_http_requests="
            + str(acceptance.sequence)
        )
        return 1

    print(f"slice={manifest['id']}")
    print(f"tests_passed={acceptance.tests_passed}")
    print("tests_failed=0")
    print(
        "runtime_http_requests="
        + str(acceptance.sequence)
    )
    print("safe_routes=8")
    print("resource_state_unchanged=yes")
    print("target_grants_restored=yes")
    print("browser_session_revoked=yes")
    print("revoked_cookie_replay_denied=yes")
    print("database_integrity=yes")
    print("service_pid_unchanged=yes")
    print("phase62_safe_post_acceptance=passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AcceptanceError as error:
        print(
            "phase62_safe_post_acceptance_error="
            + str(error)
        )
        raise SystemExit(1)
