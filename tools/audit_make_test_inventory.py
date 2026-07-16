#!/usr/bin/env python3
"""Audit GNU Make test coverage and group ownership without executing tests."""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import defaultdict, deque
from dataclasses import dataclass, field
from pathlib import Path
from typing import DefaultDict, Iterable

ROOT = Path(__file__).resolve().parents[1]
GROUP_FILE = Path("mk/test-groups.mk")
MAKEFILES = [ROOT / "Makefile", *sorted((ROOT / "mk").rglob("*.mk"))]

CANONICAL_GROUPS = (
    "test-ci-fast",
    "test-ci-frontend",
    "test-ci-packaging",
    "test-vdr",
    "test-all",
    "test-manual-real",
    "test-make-inventory",
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
    "test-manual-real",
    "real-vdr-regression",
)

REQUIRED_REACHABILITY = {
    "test-ci-fast": {
        "test-make-inventory",
        "test-recording-mutation-safety-policy",
        "test-restfulapi-recording-trash-contract",
        "test-systemd-unit-contract",
    },
    "test-ci-frontend": {"test-frontend-contracts"},
    "test-ci-packaging": {
        "test-install-staging",
        "test-systemd-unit-contract",
    },
    "test-all": {
        "test",
        "test-vdr",
        "test-ci-frontend",
        "test-ci-packaging",
        "test-make-inventory",
    },
}

MANUAL_PREFIXES = (
    "real-",
    "test-real-",
    "restfulapi-real-",
    "searchtimer-real-",
    "vdr-timer-real-",
)

INTENTIONAL_RUNTIME_AGGREGATES = {
    (
        "DAEMON_SRC",
        "core/http/src/TestHttpServer.cpp",
    ): "legacy-named production HTTP dispatcher constructed by DaemonRuntime",
    (
        "VDR_SRC",
        "core/vdr/src/MockVdrAdapter.cpp",
    ): "explicit mock runtime mode provided by VdrAdapterFactory",
    (
        "VDR_SRC",
        "core/vdr/src/TestLiveTransport.cpp",
    ): "explicit test runtime mode provided by LiveTransportFactory",
    (
        "REST_ROUTER_SRC",
        "core/vdr/src/MockVdrTimerActionExecutor.cpp",
    ): "router test-fixture aggregate; compatibility alias pending source split",
}

TARGET_RE = re.compile(
    r"^([A-Za-z0-9_.%/+\-]+(?:\s+[A-Za-z0-9_.%/+\-]+)*)\s*:(?!=)\s*(.*)$"
)
VARIABLE_ASSIGNMENT_RE = re.compile(
    r"^([A-Za-z0-9_]+)\s*(\+=|:=|=|\?=)\s*(.*)$"
)
VARIABLE_REFERENCE_RE = re.compile(r"^\$\(([A-Za-z0-9_]+)\)$")
SOURCE_ASSIGNMENT_RE = re.compile(
    r"^([A-Za-z0-9_]+)\s*(?::=|=|\+=|\?=)\s*(.*)$"
)
TEST_SOURCE_RE = re.compile(
    r"(?:^|\s)((?:api|core|web)/[^\s\\]+/test_[^\s\\]+\.(?:cpp|cc|js))"
)


@dataclass
class Target:
    name: str
    dependencies: set[str] = field(default_factory=set)
    definitions: list[str] = field(default_factory=list)
    recipe_locations: list[str] = field(default_factory=list)


def relative(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def logical_lines(lines: Iterable[str]) -> list[tuple[int, str]]:
    result: list[tuple[int, str]] = []
    current = ""
    start = 0
    for number, raw in enumerate(lines, start=1):
        line = raw.rstrip("\n")
        if current:
            current += line.lstrip()
        else:
            current = line
            start = number
        if current.rstrip().endswith("\\"):
            current = current.rstrip()[:-1] + " "
            continue
        result.append((start, current))
        current = ""
    if current:
        result.append((start, current))
    return result


def parse_make_variables() -> dict[str, list[str]]:
    variables: dict[str, list[str]] = {}
    for path in MAKEFILES:
        for _number, line in logical_lines(
            path.read_text(encoding="utf-8").splitlines()
        ):
            match = VARIABLE_ASSIGNMENT_RE.match(line.strip())
            if not match:
                continue
            name, operator, value = match.groups()
            tokens = value.split()
            if operator == "+=":
                variables.setdefault(name, []).extend(tokens)
            elif operator == "?=":
                variables.setdefault(name, tokens)
            else:
                variables[name] = tokens
    return variables


def dependency_tokens(
    text: str,
    variables: dict[str, list[str]],
) -> set[str]:
    result: set[str] = set()
    queue: deque[str] = deque(text.split())
    expanded_variables: set[str] = set()

    while queue:
        token = queue.popleft()
        if token == "|":
            continue
        if token.startswith("#"):
            break

        variable_match = VARIABLE_REFERENCE_RE.match(token)
        if variable_match:
            name = variable_match.group(1)
            if name in expanded_variables:
                continue
            expanded_variables.add(name)
            queue.extendleft(reversed(variables.get(name, [])))
            continue

        if "$" in token or "%" in token or "=" in token:
            continue
        result.add(token)

    return result


def parse_targets() -> tuple[dict[str, Target], set[Path]]:
    variables = parse_make_variables()
    targets: dict[str, Target] = {}
    referenced_test_sources: set[Path] = set()

    for path in MAKEFILES:
        lines = path.read_text(encoding="utf-8").splitlines()
        current_targets: list[str] = []

        for raw in lines:
            for match in TEST_SOURCE_RE.finditer(raw):
                referenced_test_sources.add(Path(match.group(1)))

        for number, line in logical_lines(lines):
            stripped = line.strip()
            if not stripped or stripped.startswith("#"):
                current_targets = []
                continue

            match = TARGET_RE.match(line)
            if match and not line.startswith("\t"):
                names = match.group(1).split()
                if names == [".PHONY"]:
                    current_targets = []
                    continue
                dependencies = dependency_tokens(match.group(2), variables)
                current_targets = names
                for name in names:
                    target = targets.setdefault(name, Target(name=name))
                    target.dependencies.update(dependencies)
                    target.definitions.append(f"{relative(path)}:{number}")
                continue

            if line.startswith("\t") and current_targets:
                for name in current_targets:
                    targets[name].recipe_locations.append(
                        f"{relative(path)}:{number}"
                    )
            else:
                current_targets = []

    return targets, referenced_test_sources


def transitive_dependencies(
    targets: dict[str, Target],
    root: str,
) -> set[str]:
    seen: set[str] = set()
    queue: deque[str] = deque([root])
    while queue:
        current = queue.popleft()
        if current in seen:
            continue
        seen.add(current)
        target = targets.get(current)
        if target:
            queue.extend(
                dependency
                for dependency in target.dependencies
                if dependency not in seen
            )
    return seen


def discovered_test_sources() -> set[Path]:
    result: set[Path] = set()
    for pattern in ("**/test_*.cpp", "**/test_*.cc", "**/test_*.js"):
        for path in ROOT.glob(pattern):
            if ".git" in path.parts or "build" in path.parts:
                continue
            if "tests" not in path.parts:
                continue
            result.add(path.relative_to(ROOT))
    return result


def parse_source_aggregates() -> dict[str, list[str]]:
    aggregates: DefaultDict[str, list[str]] = defaultdict(list)
    for path in MAKEFILES:
        for _number, line in logical_lines(
            path.read_text(encoding="utf-8").splitlines()
        ):
            match = SOURCE_ASSIGNMENT_RE.match(line.strip())
            if not match:
                continue
            name, value = match.groups()
            if not name.endswith("_SRC"):
                continue
            aggregates[name].extend(
                token for token in value.split() if token.endswith(".cpp")
            )
    return dict(aggregates)


def duplicate_ldflags() -> list[str]:
    duplicates: list[str] = []
    for path in MAKEFILES:
        previous = ""
        for number, line in enumerate(
            path.read_text(encoding="utf-8").splitlines(),
            start=1,
        ):
            normalized = line.strip().rstrip("\\").strip()
            if normalized == "$(LDFLAGS)" and previous == normalized:
                duplicates.append(f"{relative(path)}:{number}")
            previous = normalized
    return duplicates


def build_report() -> dict[str, object]:
    targets, referenced_test_sources = parse_targets()
    errors: list[str] = []

    for group in CANONICAL_GROUPS:
        target = targets.get(group)
        if target is None:
            errors.append(f"Missing canonical test group: {group}")
            continue
        definition_files = sorted(
            {definition.split(":", 1)[0] for definition in target.definitions}
        )
        if definition_files != [GROUP_FILE.as_posix()]:
            errors.append(
                f"Canonical group {group} must be defined only in "
                f"{GROUP_FILE}: {', '.join(definition_files)}"
            )

    for target in targets.values():
        recipe_files = {
            location.split(":", 1)[0]
            for location in target.recipe_locations
        }
        if len(recipe_files) > 1:
            errors.append(
                f"Target {target.name} has recipes in multiple files: "
                + ", ".join(sorted(recipe_files))
            )

    for group, required_targets in REQUIRED_REACHABILITY.items():
        closure = transitive_dependencies(targets, group)
        for required in sorted(required_targets - closure):
            errors.append(f"{group} does not reach required target {required}")

    grouped_targets: set[str] = set()
    public_group_closures: dict[str, list[str]] = {}
    for group in PUBLIC_GROUPS:
        if group not in targets:
            continue
        closure = transitive_dependencies(targets, group)
        public_group_closures[group] = sorted(
            target for target in closure if target.startswith("test-")
        )
        grouped_targets.update(closure)

    test_targets = {
        name for name in targets if name == "test" or name.startswith("test-")
    }
    ungrouped_targets = sorted(
        target
        for target in test_targets - grouped_targets
        if target not in PUBLIC_GROUPS
        and target not in CANONICAL_GROUPS
        and not target.startswith(MANUAL_PREFIXES)
    )
    if ungrouped_targets:
        errors.append(
            f"{len(ungrouped_targets)} test targets are not reachable "
            "from a public group"
        )

    test_sources = discovered_test_sources()
    orphan_sources = sorted(test_sources - referenced_test_sources)
    stale_test_references = sorted(referenced_test_sources - test_sources)
    if orphan_sources:
        errors.append(
            f"{len(orphan_sources)} test source files have no Make reference"
        )
    if stale_test_references:
        errors.append(
            f"{len(stale_test_references)} Make references point to missing "
            "test source files"
        )

    duplicate_flags = duplicate_ldflags()
    if duplicate_flags:
        errors.append(
            f"{len(duplicate_flags)} consecutive duplicate $(LDFLAGS) entries found"
        )

    aggregates = parse_source_aggregates()
    production_findings: list[str] = []
    accepted_runtime_variants: list[str] = []
    for name in ("VDR_SRC", "DAEMON_SRC", "REST_ROUTER_SRC"):
        for source in aggregates.get(name, []):
            if not Path(source).name.startswith(("Mock", "Test")):
                continue
            key = (name, source)
            reason = INTENTIONAL_RUNTIME_AGGREGATES.get(key)
            if reason:
                accepted_runtime_variants.append(
                    f"{name}: {source} — {reason}"
                )
            else:
                production_findings.append(f"{name}: {source}")
    if production_findings:
        errors.append(
            f"{len(production_findings)} unclassified Mock/Test sources "
            "are present in broad runtime aggregates"
        )

    return {
        "makefiles": [relative(path) for path in MAKEFILES],
        "counts": {
            "makefiles": len(MAKEFILES),
            "targets": len(targets),
            "test_targets": len(test_targets),
            "test_source_files": len(test_sources),
            "referenced_test_source_files": len(referenced_test_sources),
            "ungrouped_test_targets": len(ungrouped_targets),
            "orphan_test_source_files": len(orphan_sources),
            "stale_test_source_references": len(stale_test_references),
            "duplicate_ldflags": len(duplicate_flags),
            "unclassified_runtime_test_sources": len(production_findings),
        },
        "public_groups": public_group_closures,
        "ungrouped_test_targets": ungrouped_targets,
        "orphan_test_source_files": [str(path) for path in orphan_sources],
        "stale_test_source_references": [
            str(path) for path in stale_test_references
        ],
        "duplicate_ldflags_locations": duplicate_flags,
        "unclassified_runtime_test_sources": production_findings,
        "accepted_runtime_variants": sorted(accepted_runtime_variants),
        "errors": errors,
    }


def print_human(report: dict[str, object]) -> None:
    print("Make/test inventory audit")
    print("=========================")
    for key, value in report["counts"].items():
        print(f"{key}: {value}")

    accepted = report["accepted_runtime_variants"]
    if accepted:
        print("\nIntentional runtime variants:")
        for item in accepted:
            print(f"- {item}")

    errors = report["errors"]
    if errors:
        print("\nErrors:")
        for item in errors:
            print(f"- {item}")
    else:
        print("\nStrict Make/test inventory passed.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="return non-zero for any inventory violation",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="print the full report as JSON",
    )
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
