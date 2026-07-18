#!/usr/bin/env python3

from pathlib import Path

updater = Path(__file__).with_name("apply_sb10d_embedded_runtime.py")
source = updater.read_text(encoding="utf-8")
patched = source.replace("\n    '''", "\n    r'''")

try:
    exec(
        compile(patched, str(updater), "exec"),
        {"__name__": "__main__", "__file__": str(updater)},
    )
except Exception as error:
    print(f"SB10D_IMPLEMENTATION_ERROR: {error}", flush=True)
    raise SystemExit(1)
