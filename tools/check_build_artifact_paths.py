#!/usr/bin/env python3
"""Validate that Make compiler outputs stay inside the repository build directory."""

from __future__ import annotations

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
OUTPUT_RE = re.compile(r"-o\s+([^\s\\]+)")


def makefiles() -> list[Path]:
    paths = [ROOT / "Makefile"]
    paths.extend(sorted((ROOT / "mk").glob("*.mk")))
    paths.extend(sorted((ROOT / "apps").glob("**/Makefile")))
    return [path for path in paths if path.is_file()]


def main() -> int:
    violations: list[str] = []
    output_count = 0

    for path in makefiles():
        relative = path.relative_to(ROOT)
        for line_number, line in enumerate(
            path.read_text(encoding="utf-8").splitlines(), 1
        ):
            for match in OUTPUT_RE.finditer(line):
                output_count += 1
                output = match.group(1).strip('"\'')
                if not output.startswith("$(BUILD_DIR)/"):
                    violations.append(
                        f"{relative}:{line_number}: compiler output must use "
                        f"$(BUILD_DIR): {output}"
                    )

    common = (ROOT / "mk/common.mk").read_text(encoding="utf-8")
    if "BUILD_DIR ?= $(CURDIR)/.build" not in common:
        violations.append("mk/common.mk: missing canonical BUILD_DIR default")

    cache = (ROOT / "mk/object-cache.mk").read_text(encoding="utf-8")
    if "OBJECT_CACHE_DIR ?= $(BUILD_DIR)/obj" not in cache:
        violations.append(
            "mk/object-cache.mk: missing canonical OBJECT_CACHE_DIR default"
        )
    expected_wrapper = (
        'BUILD_CXX = python3 tools/build_cpp_cached.py --compiler "$(CXX)" '
        '--cache-dir "$(OBJECT_CACHE_DIR)" --'
    )
    if expected_wrapper not in cache:
        violations.append("mk/object-cache.mk: missing cached BUILD_CXX wrapper")

    ignored = (ROOT / ".gitignore").read_text(encoding="utf-8").splitlines()
    if ".build/" not in ignored:
        violations.append(".gitignore: missing .build/")

    # Every make-driven database fixture must stay in the selected build
    # directory; never use or remove another user's /tmp database.
    if 'VDR_SUITE_TEST_DATABASE_PATH := $(abspath $(BUILD_DIR))/vdr-suite-test.db' not in common:
        violations.append("mk/common.mk: test DB must follow BUILD_DIR")
    if '"$(VDR_SUITE_TEST_DATABASE_PATH)"' not in common:
        violations.append("mk/common.mk: prepare-test-db must use the build DB path")
    maintenance = (ROOT / "mk/maintenance-tests.mk").read_text(encoding="utf-8")
    if "rm -f /tmp/vdr-suite-test.db" in maintenance:
        violations.append("mk/maintenance-tests.mk: must not remove a global /tmp DB")
    for folder in ("api/rest/tests", "core/recordings/tests", "core/http/tests"):
        for source in (ROOT / folder).glob("*.cpp"):
            if 'db.open("/tmp/vdr-suite-test.db")' in source.read_text(encoding="utf-8"):
                violations.append(f"{source.relative_to(ROOT)}: shared /tmp DB")
    for source_name in ("apps/dashboard/main.cpp", "core/http/src/TestHttpServer.cpp",
                        "core/sqlite/tests/test_database.cpp"):
        source = ROOT / source_name
        content = source.read_text(encoding="utf-8")
        if "/tmp/vdr-suite-test.db" in content or "/tmp/vdr-suite-cpp-test.db" in content:
            violations.append(f"{source_name}: shared /tmp DB")

    if output_count == 0:
        violations.append("no compiler outputs found; path audit cannot prove coverage")

    if violations:
        for violation in violations:
            print(violation, file=sys.stderr)
        return 1

    print(f"build artifact path contract ok ({output_count} compiler outputs)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
