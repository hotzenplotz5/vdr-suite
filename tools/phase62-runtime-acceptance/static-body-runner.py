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

    return errors


def main() -> int:
    runner.validate_manifest = validate_static_body_manifest
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
