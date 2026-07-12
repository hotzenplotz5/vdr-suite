#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path("/home/yavdr/vdr-suite")
PATH = ROOT / "apps/tools/restfulapi_recording_action_real_move_smoke.cpp"

text = PATH.read_text(encoding="utf-8")

if "if (options.execute &&" in text and "Refusing real move execution without VDR-SUITE-TEST marker" in text:
    print("recording MOVE preview marker gate already fixed")
    raise SystemExit(0)

pattern = re.compile(
    r"if\s*\(\s*!hasTestMarker\(options\.source\)\s*\|\|\s*"
    r"!hasTestMarker\(options\.target\)\s*\)"
)

replacement = (
    "if (options.execute &&\n"
    "        (!hasTestMarker(options.source) ||\n"
    "         !hasTestMarker(options.target)))"
)

text, count = pattern.subn(replacement, text, count=1)
if count != 1:
    raise SystemExit(
        "apps/tools/restfulapi_recording_action_real_move_smoke.cpp: "
        f"expected one marker guard, found {count}"
    )

old_message = "Refusing move smoke without VDR-SUITE-TEST marker "
new_message = "Refusing real move execution without VDR-SUITE-TEST marker "

if old_message in text:
    text = text.replace(old_message, new_message, 1)
elif new_message not in text:
    raise SystemExit(
        "apps/tools/restfulapi_recording_action_real_move_smoke.cpp: "
        "marker refusal message not found"
    )

PATH.write_text(text, encoding="utf-8")
print("recording MOVE preview marker gate robust fix applied")
