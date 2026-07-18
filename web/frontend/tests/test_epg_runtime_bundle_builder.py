#!/usr/bin/env python3

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
BUILDER = ROOT / "tools" / "build_frontend_epg_runtime.py"


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="vdr-suite-epg-runtime-") as directory:
        output = Path(directory) / "epg-runtime.js"
        subprocess.run(
            [
                "python3",
                str(BUILDER),
                "--root",
                str(ROOT),
                "--output",
                str(output),
            ],
            check=True,
        )
        source = output.read_text(encoding="utf-8")

        deferred_marker = "function bootVdrSuiteLegacyEpgRuntime()"
        legacy_marker = "function ensureCachedEpgDetailStyles()"
        day_marker = "global.VdrSuiteChannelDayProgram = api"
        compat_marker = "global.VdrSuiteChannelDayProgramCompat = Object.freeze"

        assert deferred_marker in source
        assert "document.addEventListener('DOMContentLoaded', bootVdrSuiteLegacyEpgRuntime" in source
        assert "global.__vdrSuiteLegacyEpgRuntimeStarted = true" in source
        assert legacy_marker in source
        assert day_marker in source
        assert compat_marker in source
        assert source.index(deferred_marker) < source.index(legacy_marker)
        assert source.index(legacy_marker) < source.index(day_marker)
        assert source.index(day_marker) < source.index(compat_marker)

        subprocess.run(["node", "--check", str(output)], check=True)

    print("test_epg_runtime_bundle_builder passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
