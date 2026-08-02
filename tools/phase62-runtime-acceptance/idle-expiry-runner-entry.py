#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import sys
from pathlib import Path
from types import ModuleType

from idle_expiry_audit_contract import adapt_accountability_rows


def load_implementation(path: Path) -> ModuleType:
    specification = importlib.util.spec_from_file_location(
        "phase62_idle_expiry_runner_implementation",
        path,
    )
    if specification is None or specification.loader is None:
        raise RuntimeError("idle_expiry_runner_load_failed")

    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


def main() -> int:
    implementation_path = Path(__file__).with_name("idle-expiry-runner.py")
    implementation = load_implementation(implementation_path)
    original_reader = implementation.accountability_rows

    def validated_reader(database, request_ids):
        return adapt_accountability_rows(
            original_reader,
            implementation.require,
            database,
            request_ids,
        )

    implementation.accountability_rows = validated_reader

    try:
        return int(implementation.main())
    except implementation.AcceptanceError as error:
        print("PHASE_62_SLICE_2V_RUNTIME_ACCEPTANCE=FAIL")
        print(f"FAILURE_REASON={error}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
