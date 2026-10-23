#!/usr/bin/env python3
"""Reject drift between pinned patched epgsearch Service(void*) ABI and SuiteBridge."""
import argparse
from pathlib import Path
import re


TYPES = (
    "Epgsearch_searchtimer_observe_v1_0",
    "Epgsearch_searchtimer_observe_v1_1",
    "eEpgsearchNativeToggleCasStatus",
    "Epgsearch_searchtimer_toggle_cas_v1_0",
    "eEpgsearchNativeUpdateCasStatus",
    "Epgsearch_searchtimer_update_cas_v1_0",
    "eEpgsearchNativeDeleteCasStatus",
    "Epgsearch_searchtimer_delete_cas_v1_0",
)


def declaration(source, name):
    cleaned = re.sub(r"//[^\n]*", "", source)
    cleaned = re.sub(r"/\*.*?\*/", "", cleaned, flags=re.S)
    expression = rf"\b(?:struct|enum)\s+{re.escape(name)}\b[^{{;]*\{{[^}}]*\}}\s*;"
    found = re.findall(expression, cleaned, flags=re.S)
    assert len(found) == 1, f"missing or duplicated ABI declaration: {name}"
    return re.sub(r"\s+", "", found[0])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--bridge", required=True, type=Path)
    args = parser.parse_args()
    provider = (args.source / "services.h").read_text()
    mirror = args.bridge.read_text()
    for name in TYPES:
        native = declaration(provider, name)
        bridged = declaration(mirror, name)
        assert native == bridged, f"ABI DRIFT: {name}"
    print("PASS: all eight native epgsearch Service(void*) ABI declarations identical")


if __name__ == "__main__":
    main()
