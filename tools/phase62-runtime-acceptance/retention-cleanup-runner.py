#!/usr/bin/env python3
from __future__ import annotations

import retention_cleanup_runtime_execution as runtime_execution
from retention_cleanup_runtime_process import (
    stop_service,
    verify_runtime_process,
    wait_service,
)

runtime_execution.stop_service = stop_service
runtime_execution.verify_runtime_process = verify_runtime_process
runtime_execution.wait_service = wait_service

from retention_cleanup_runtime_runner import AcceptanceError, main  # noqa: E402


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AcceptanceError as error:
        print("PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE=FAIL")
        print(f"FAILURE_REASON={error}")
        raise SystemExit(1)
