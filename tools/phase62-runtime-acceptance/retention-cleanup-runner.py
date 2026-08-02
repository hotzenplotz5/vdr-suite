#!/usr/bin/env python3
from __future__ import annotations

from retention_cleanup_runtime_runner import AcceptanceError, main


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AcceptanceError as error:
        print("PHASE_62_SLICE_2W_RUNTIME_ACCEPTANCE=FAIL")
        print(f"FAILURE_REASON={error}")
        raise SystemExit(1)
