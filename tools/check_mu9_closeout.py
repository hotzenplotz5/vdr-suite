#!/usr/bin/env python3
"""Guard the MU.9 closeout status after MU.9A-F merge."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path):
    return (ROOT / path).read_text(encoding="utf-8")

def require(label, text, marker):
    if marker not in text:
        raise SystemExit(f"FAIL: {label} missing: {marker}")

current = read("docs/CURRENT.md")
workstream = read("docs/development/post-phase69-multiuser-workstream.md")
closeout = read("docs/development/post-phase69-mu9-closeout.md")
mu9f = read("docs/development/post-phase69-mu9f-account-create-ui.md")
dependency = read("docs/planning/implementation-dependency-map.md")

require("MU.9F", mu9f, "IMPLEMENTATION MERGED")
require("closeout", closeout, "COMPLETED — MU.9A-F MERGED")
require("closeout", closeout, "PR #430")
require("closeout", closeout, "CI #9764")
require("closeout", closeout, "CI #9765")
require("closeout", closeout, "MU.8A")
require("workstream", workstream, "MU.9 — Account and access administration UI [COMPLETED — MU.9A-F MERGED]")
require("dependency", dependency, "MU.9 account/access admin UI [COMPLETED - MU.9A-F MERGED]")

for label, text in (("current", current), ("workstream", workstream), ("dependency", dependency)):
    if "MU.9F ACCOUNT CREATE UI CANDIDATE" in text:
        raise SystemExit(f"FAIL: stale MU.9F candidate status in {label}")

require("current", current, "Phase 70")
require("workstream", workstream, "MU.10")
print("MU.9 closeout contracts passed")
print("MU9=COMPLETED_MU9A_F_MERGED")
print("MU9F=IMPLEMENTATION_MERGED_PR_430_FOCUSED_YAVDR_PASS_HOSTED_CI_GREEN")
print("MU8A_RUNTIME_ACCEPTANCE=PENDING")
print("PHASE70=NOT_STARTED")
