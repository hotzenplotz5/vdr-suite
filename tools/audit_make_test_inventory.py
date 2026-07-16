#!/usr/bin/env python3
"""Audit the GNU Make test inventory without executing product tests.

The audit intentionally distinguishes hard structural errors from migration
warnings.  Hard errors fail --check.  Warnings document existing organization
debt so the Make/test consolidation can proceed without deleting useful tests.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import defaultdict, deque
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parents[1]
MAKEFILES = [ROOT / "Makefile", *sorted((ROOT / "mk").rglob("*.mk"))]

TARGET_RE = re.compile(r"^([A-Za-z0-9_.%/+:-]+(?:\s+[A-Za-z0-9_.%/+:-]+)*)\s*:(?![=])\s*(.*)$")
PATH_RE = re.compile(
    r"(?<![$(])\b(?:[A-Za-z0-9_.-]+/)+(?:[A-Za-z0-9_.-]+\.)"
    r"(?:cpp|cc|c|h|hpp|py|js|json|sql|md|service|html|css|svg)\b"
)

PUBLIC_GROUPS = (
    "test",
    "test-all",
    "test-ci-fast",
    "test-ci-frontend",
    "test-ci-packaging",
    "test-fast",
    "test-vdr",
    "test-frontend-contracts",
    "test-frontend-i18n",
    "test-install-staging",
    "real-vdr-regression",
)

MANUAL_PREFIXES = (
    "real-",
    "test-real-",
    "restfulapi-real-",
    "searchtimer-real-",
    "vdr-timer-real-",
)


@dataclass
class Target:
    name: str
    dependencies: set[str] = field(default_factory=set)
    definitions: list[str] = field(default_factory=list)
    recipe_locations: list[str] = field(default_factory=list)


def logical_lines(path: Path) -> Iterable[tuple[int, str]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    index = 0
    while index < len(lines):
        start = index + 1
        line = lines[index]
        while line.rstrip().endswith("\\") and index + 1 < len(lines):
            line = line.rstrip()[:-1] + " " + lines[index + 1].strip()
            index += 1
        yield start, line
        index += 1


def parse_makefiles() -> tuple[dict[str, Target], list[str], list[str]]:
    targets: dict[str, Target] = {}
    referenced_paths: list[str] = []
    duplicate_ldflags: list[str] = []

    for path in MAKEFILES:
        relative = path.relative_to(ROOT).as_posix()
        previous_nonempty = ""
        current_targets: list[str] = []

        for lineno, line in logical_lines(path):
            stripped = line.strip()
            referenced_paths.extend(PATH_RE.findall(line))

            if stripped == "$(LDFLAGS) \\" and previous_nonempty == "$(LDFLAGS) \\" :
                duplicate_ldflags.append(f"{relative}:{lineno}")
            if stripped:
                previous_nonempty = stripped

            match = TARGET_RE.match(line)
            if match and not line.startswith("\t"):
                names = match.group(1).split()
                dependencies = {
                    token
                    for token in match.group(2).split()
                    if token and not token.startswith(("$", "|"))
                }
                current_targets = names
                for name in names:
                    target = targets.setdefault(name, Target(name=name))
                    target.dependencies.update(dependencies)
                    target.definitions.append(f"{relative}:{lineno}")
                continue

            if line.startswith("\t") and current_targets:
                for name in current_targets:
                    targets[name].recipe_locations.append(f"{relative}:{lineno}")
            elif stripped and not stripped.startswith("#"):
                current_targets = []

    return targets, referenced_paths, duplicate_ldflags


def transitive_dependencies(targets: dict[str, Target], root: str) -> set[str]:
    seen: set[str] = set()
    queue: deque[str] = deque([root])
    while queue:
        current = queue.popleft()
        if current in seen:
            continue
        seen.add(current)
        target = targets.get(current)
        if not target:
            continue
        queue.extend(dep for dep in target.dependencies if dep not in seen)
    seen.discard(root)
    return seen


def expected_target_for_test_file(path: Path) -> str:
    stem = path.stem
    return stem.replace("_", "-")


def production_test_support_warnings() -> list[str]:
    warnings: list[str] = []
    production_files = [ROOT / "mk" / "vdr-sources.mk", ROOT / "mk" / "daemon-sources.mk"]
    marker = re.compile(r"(?:^|/)(?:Mock|Test)[A-Za-z0-9_]*\.(?:cpp|cc|c)$")
    for path in production_files:
        if not path.exists():
            continue
        relative = path.relative_to(ROOT).as_posix()
        for lineno, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
            for candidate in PATH_RE.findall(line):
                if marker.search(candidate):
                    warnings.append(f"{relative}:{lineno}: production aggregate references {candidate}")
    return warnings


def build_report() -> dict[str, object]:
    targets, referenced_paths, duplicate_ldflags = parse_makefiles()
    errors: list[str] = []
    warnings: list[str] = []

    for path_text in sorted(set(referenced_paths)):
        if not (ROOT / path_text).exists():
            errors.append(f"Make source reference does not exist: {path_text}")

    for target in targets.values():
        recipe_files = {location.split(":", 1)[0] for location in target.recipe_locations}
        if len(recipe_files) > 1:
            errors.append(
                f"Target {target.name} has recipes in multiple files: "
                + ", ".join(sorted(recipe_files))
            )

    test_targets = sorted(name for name in targets if name.startswith("test-"))
    group_closures = {
        group: sorted(
            dependency
            for dependency in transitive_dependencies(targets, group)
            if dependency.startswith("test-")
        )
        for group in PUBLIC_GROUPS
        if group in targets
    }
    grouped_targets = {target for closure in group_closures.values() for target in closure}
    ungrouped_targets = [
        target
        for target in test_targets
        if target not in grouped_targets
        and not target.startswith(MANUAL_PREFIXES)
        and target not in PUBLIC_GROUPS
    ]
    if ungrouped_targets:
        warnings.append(
            f"{len(ungrouped_targets)} test targets are not reachable from a public test group"
        )

    test_files = sorted(
        path.relative_to(ROOT).as_posix()
        for pattern in ("**/test_*.cpp", "**/test_*.cc", "**/test_*.js")
        for path in ROOT.glob(pattern)
        if ".git" not in path.parts
    )
    missing_target_files: list[str] = []
    for path_text in test_files:
        expected = expected_target_for_test_file(Path(path_text))
        if expected not in targets:
            missing_target_files.append(path_text)
    if missing_target_files:
        warnings.append(
            f"{len(missing_target_files)} test source files have no convention-matching Make target"
        )

    if duplicate_ldflags:
        warnings.append(f"{len(duplicate_ldflags)} consecutive duplicate $(LDFLAGS) entries found")

    production_leaks = production_test_support_warnings()
    if production_leaks:
        warnings.append(
            f"{len(production_leaks)} test-support-looking sources are present in production aggregates"
        )

    repeated_group_definitions = {
        group: targets[group].definitions
        for group in PUBLIC_GROUPS
        if group in targets and len(targets[group].definitions) > 1
    }
    if repeated_group_definitions:
        warnings.append(
            f"{len(repeated_group_definitions)} public test groups are defined in multiple locations"
        )

    return {
        "makefiles": [path.relative_to(ROOT).as_posix() for path in MAKEFILES],
        "counts": {
            "makefiles": len(MAKEFILES),
            "targets": len(targets),
            "test_targets": len(test_targets),
            "test_source_files": len(test_files),
            "ungrouped_test_targets": len(ungrouped_targets),
            "test_files_without_convention_target": len(missing_target_files),
            "duplicate_ldflags": len(duplicate_ldflags),
            "production_test_support_references": len(production_leaks),
            "multiply_defined_public_groups": len(repeated_group_definitions),
        },
        "public_groups": group_closures,
        "multiply_defined_public_groups": repeated_group_definitions,
        "ungrouped_test_targets": ungrouped_targets,
        "test_files_without_convention_target": missing_target_files,
        "duplicate_ldflags_locations": duplicate_ldflags,
        "production_test_support_references": production_leaks,
        "errors": errors,
        "warnings": warnings,
    }


def print_human(report: dict[str, object]) -> None:
    counts = report["counts"]
    print("Make/test inventory audit")
    print("=========================")
    for key, value in counts.items():
        print(f"{key}: {value}")

    errors = report["errors"]
    warnings = report["warnings"]
    if errors:
        print("\nErrors:")
        for item in errors:
            print(f"- {item}")
    if warnings:
        print("\nMigration warnings:")
        for item in warnings:
            print(f"- {item}")

    print("\nDetailed JSON can be produced with --json.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="return non-zero for hard structural errors",
    )
    parser.add_argument("--json", action="store_true", help="print the full report as JSON")
    args = parser.parse_args()

    report = build_report()
    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        print_human(report)

    if args.check and report["errors"]:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
