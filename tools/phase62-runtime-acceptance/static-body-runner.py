#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import json
from pathlib import Path
from types import ModuleType
from typing import Any


RUNNER_PATH = Path(__file__).with_name("runner.py")


def load_runner() -> ModuleType:
    spec = importlib.util.spec_from_file_location(
        "phase62_runtime_acceptance_runner",
        RUNNER_PATH,
    )

    if spec is None or spec.loader is None:
        raise RuntimeError("runtime_acceptance_runner_unavailable")

    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


runner = load_runner()
base_validate_manifest = runner.validate_manifest
base_request = runner.RuntimeAcceptance.request


def validate_static_body_manifest(
    manifest: dict[str, Any],
) -> list[str]:
    errors = [
        error
        for error in base_validate_manifest(manifest)
        if error != "safeBody must be exactly {}"
    ]

    body = manifest.get("safeBody")

    if not isinstance(body, dict) or not body:
        errors.append(
            "safeBody must be a non-empty object"
        )
        return errors

    try:
        encoded = json.dumps(
            body,
            separators=(",", ":"),
            sort_keys=True,
        )
    except (TypeError, ValueError):
        errors.append("safeBody must be JSON serializable")
        return errors

    if len(encoded.encode("utf-8")) > 2048:
        errors.append("safeBody exceeds 2048 bytes")

    if body.get("backendId") != manifest.get("backendId"):
        errors.append(
            "safeBody.backendId must equal backendId"
        )

    if "operationId" in body:
        errors.append(
            "safeBody must not contain operationId"
        )

    query_scoped = manifest.get("queryScopedRoutes", False)
    if not isinstance(query_scoped, bool):
        errors.append("queryScopedRoutes must be a boolean")
    elif query_scoped:
        query_suffix = manifest.get("querySuffix", "")
        if "backend=" not in query_suffix:
            errors.append(
                "query-scoped routes require backend in querySuffix"
            )

    return errors


def request_with_query_scope(
    self: Any,
    method: str,
    path: str,
    **kwargs: Any,
) -> tuple[int, dict[str, str], str]:
    if method == "POST" and self.manifest.get(
        "queryScopedRoutes",
        False,
    ):
        routes = self.manifest.get("routes", [])
        exact_targets = set(routes)
        exact_targets.update(route + "/" for route in routes)
        if path in exact_targets:
            path += self.manifest["querySuffix"]

    return base_request(
        self,
        method,
        path,
        **kwargs,
    )


def main() -> int:
    runner.validate_manifest = validate_static_body_manifest
    runner.RuntimeAcceptance.request = request_with_query_scope
    return runner.main()


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except runner.AcceptanceError as error:
        print(
            "phase62_runtime_acceptance_error="
            f"{error}"
        )
        raise SystemExit(1)
