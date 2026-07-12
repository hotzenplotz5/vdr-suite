#!/usr/bin/env python3
from pathlib import Path

ROOT = Path("/home/yavdr/vdr-suite")


def replace_once(relative_path: str, old: str, new: str) -> None:
    path = ROOT / relative_path
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(
            f"{relative_path}: expected exactly one replacement match, found {count}"
        )
    path.write_text(text.replace(old, new, 1), encoding="utf-8")
    print(f"updated {relative_path}")


replace_once(
    "apps/tools/restfulapi_recording_action_real_move_smoke.cpp",
    """    if (!hasTestMarker(options.source) || !hasTestMarker(options.target))
    {
        std::cerr
            << "Refusing move smoke without VDR-SUITE-TEST marker "
            << "in both --source and --target.\n";
        return 8;
    }
""",
    """    if (options.execute &&
        (!hasTestMarker(options.source) ||
         !hasTestMarker(options.target)))
    {
        std::cerr
            << "Refusing real move execution without VDR-SUITE-TEST marker "
            << "in both --source and --target.\n";
        return 8;
    }
"""
)

print("recording MOVE preview marker gate fix applied")
