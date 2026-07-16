#!/usr/bin/env python3
"""Audit Make test targets, public groups and test-source reachability.

The default mode blocks structural regressions while reporting historical debt.
Use --strict to also fail on ungrouped targets, orphan test sources and test
support linked through production source aggregates.
"""

from __future__ import annotations

import argparse
import re
import sys
from collections import defaultdict, deque
from pathlib import Path
from typing import DefaultDict, Iterable

ROOT = Path(__file__).resolve().parents[1]
GROUP_FILE = Path("mk/test-groups.mk")
PUBLIC_GROUPS = (
    "test-ci-fast",
    "test-ci-frontend",
    "test-ci-packaging",
    "test-vdr",
    "test-all",
    "test-manual-real",
)

REQUIRED_REACHABILITY = {
    "test-ci-fast": {
        "test-make-test-manifest",
        "test-restfulapi-recording-trash-contract",
        "test-recording-mutation-safety-policy",
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
        "test-make-test-manifest",
    },
}

INTENTIONAL_RUNTIME_AGGREGATES = {
    (
        "DAEMON_SRC",
        "core/http/src/TestHttpServer.cpp",
    ): "legacy-named production HTTP dispatcher used by DaemonRuntime",
    (
        "VDR_SRC",
        "core/vdr/src/MockVdrAdapter.cpp",
    ): "explicit mock adapter mode provided by VdrAdapterFactory",
    (
        "VDR_SRC",
        "core/vdr/src/TestLiveTransport.cpp",
    ): "explicit test transport mode provided by LiveTransportFactory",
}

TARGET_RE = re.compile(r"^([A-Za-z0-9_.%/+\-]+(?:\s+[A-Za-z0-9_.%/+\-]+)*)\s*:(?!=)\s*(.*)$")
SOURCE_RE = re.compile(r"(?:^|\s)((?:api|core|web)/[^\s\\]+/test_[^\s\\]+\.(?:cpp|js))")
ASSIGNMENT_RE = re.compile(r"^([A-Za-z0-9_]+)\s*(?::=|=|\+=|\?=)\s*(.*)$")
VARIABLE_ASSIGNMENT_RE = re.compile(
    r"^([A-Za-z0-9_]+)\s*(\+=|:=|=|\?=)\s*(.*)$"
)
VARIABLE_REFERENCE_RE = re.compile(r"^\$\(([A-Za-z0-9_]+)\)$")


def make_files() -> list[Path]:
    files = [ROOT / "Makefile"]
    files.extend(sorted((ROOT / "mk").rglob("*.mk")))
    return [path for path in files if path.is_file()]


def relative(path: Path) -> Path:
    return path.relative_to(ROOT)


def logical_lines(lines: Iterable[str]) -> list[str]:
    result: list[str] = []
    current = ""
    for raw in lines:
        line = raw.rstrip("\n")
        if current:
            current += line.lstrip()
        else:
            current = line
        if current.rstrip().endswith("\\"):
            current = current.rstrip()[:-1] + " "
            continue
        result.append(current)
        current = ""
    if current:
        result.append(current)
    return result


def parse_make_variables(paths: Iterable[Path]) -> dict[str, list[str]]:
    variables: dict[str, list[str]] = {}
    for path in paths:
        lines = path.read_text(encoding="utf-8").splitlines()
        for line in logical_lines(lines):
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
    queue = deque(text.split())
    expanded_variables: set[str] = set()

    while queue:
        token = queue.popleft()
        if token == "|":
            continue
        if token.startswith("#"):
            break

        variable_match = VARIABLE_REFERENCE_RE.match(token)
        if variable_match:
            variable_name = variable_match.group(1)
            if variable_name in expanded_variables:
                continue
            expanded_variables.add(variable_name)
            queue.extendleft(reversed(variables.get(variable_name, [])))
            continue

        if "$" in token or "%" in token or "=" in token:
            continue
        result.add(token)

    return result


def parse_targets(paths: Iterable[Path]) -> tuple[
    dict[str, set[str]], DefaultDict[str, list[Path]]
]:
    graph: dict[str, set[str]] = defaultdict(set)
    definitions: DefaultDict[str, list[Path]] = defaultdict(list)
    variables = parse_make_variables(paths)

    for path in paths:
        lines = path.read_text(encoding="utf-8").splitlines()
        for line in logical_lines(lines):
            if not line or line[0].isspace() or line.startswith("#"):
                continue
            match = TARGET_RE.match(line)
            if not match:
                continue
            targets_text, prerequisites_text = match.groups()
            targets = targets_text.split()
            if targets == [".PHONY"]:
                continue
            prerequisites = dependency_tokens(
            prerequisites_text,
            variables,
        )
            for target in targets:
                graph.setdefault(target, set()).update(prerequisites)
                definitions[target].append(relative(path))

    return graph, definitions


def reachable(graph: dict[str, set[str]], start: str) -> set[str]:
    seen: set[str] = set()
    queue: deque[str] = deque([start])
    while queue:
        target = queue.popleft()
        if target in seen:
            continue
        seen.add(target)
        queue.extend(sorted(graph.get(target, ())))
    return seen


def referenced_test_sources(paths: Iterable[Path]) -> set[Path]:
    referenced: set[Path] = set()
    for path in paths:
        for line in path.read_text(encoding="utf-8").splitlines():
            for match in SOURCE_RE.finditer(line):
                referenced.add(Path(match.group(1)))
    return referenced


def discovered_test_sources() -> set[Path]:
    result: set[Path] = set()
    for suffix in ("*.cpp", "*.js"):
        for path in ROOT.rglob(f"test_{suffix}"):
            if ".git" in path.parts or "build" in path.parts:
                continue
            if "tests" not in path.parts:
                continue
            result.add(relative(path))
    return result


def parse_source_aggregates(paths: Iterable[Path]) -> dict[str, list[str]]:
    aggregates: dict[str, list[str]] = defaultdict(list)
    for path in paths:
        lines = path.read_text(encoding="utf-8").splitlines()
        for line in logical_lines(lines):
            match = ASSIGNMENT_RE.match(line.strip())
            if not match:
                continue
            name, value = match.groups()
            if not name.endswith("_SRC"):
                continue
            aggregates[name].extend(
                token for token in value.split() if token.endswith(".cpp")
            )
    return aggregates


def duplicate_ldflags(paths: Iterable[Path]) -> list[tuple[Path, int]]:
    duplicates: list[tuple[Path, int]] = []
    for path in paths:
        previous = ""
        for number, line in enumerate(
            path.read_text(encoding="utf-8").splitlines(), start=1
        ):
            normalized = line.strip().rstrip("\\").strip()
            if normalized == "$(LDFLAGS)" and previous == normalized:
                duplicates.append((relative(path), number))
            previous = normalized
    return duplicates


def report_list(title: str, values: Iterable[str], limit: int = 30) -> None:
    items = sorted(values)
    if not items:
        return
    print(f"\n{title} ({len(items)}):")
    for item in items[:limit]:
        print(f"  - {item}")
    if len(items) > limit:
        print(f"  ... {len(items) - limit} more")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--strict",
        action="store_true",
        help="also fail on historical inventory warnings",
    )
    args = parser.parse_args()

    paths = make_files()
    graph, definitions = parse_targets(paths)
    errors: list[str] = []
    warnings: list[str] = []

    for group in PUBLIC_GROUPS:
        locations = definitions.get(group, [])
        if not locations:
            errors.append(f"missing public test group: {group}")
            continue
        unique_locations = sorted(set(locations))
        if unique_locations != [GROUP_FILE]:
            joined = ", ".join(str(path) for path in unique_locations)
            errors.append(
                f"public test group {group} must be defined only in "
                f"{GROUP_FILE}, found: {joined}"
            )

    for group, required in REQUIRED_REACHABILITY.items():
        group_reachability = reachable(graph, group)
        for target in sorted(required - group_reachability):
            errors.append(f"{group} does not reach required target {target}")

    referenced = referenced_test_sources(paths)
    discovered = discovered_test_sources()
    orphan_sources = discovered - referenced
    stale_references = referenced - discovered

    grouped_targets: set[str] = set()
    for group in PUBLIC_GROUPS:
        grouped_targets.update(reachable(graph, group))

    test_targets = {
        target
        for target in graph
        if target == "test" or target.startswith("test-")
    }
    ungrouped_targets = {
        target
        for target in test_targets - grouped_targets
        if target not in {"test-make-test-manifest-strict"}
    }

    if stale_references:
        errors.extend(
            f"Make target references missing test source: {path}"
            for path in sorted(stale_references)
        )

    aggregates = parse_source_aggregates(paths)
    production_leaks: list[str] = []
    for name in ("VDR_SRC", "DAEMON_SRC", "REST_ROUTER_SRC"):
        for source in aggregates.get(name, []):
            basename = Path(source).name
            if (
                basename.startswith(("Mock", "Test"))
                and (name, source) not in INTENTIONAL_RUNTIME_AGGREGATES
            ):
                production_leaks.append(f"{name}: {source}")

    duplicate_flags = duplicate_ldflags(paths)

    if orphan_sources:
        warnings.append(f"{len(orphan_sources)} test source files have no Make reference")
    if ungrouped_targets:
        warnings.append(f"{len(ungrouped_targets)} test targets are outside public groups")
    if production_leaks:
        warnings.append(
            f"{len(production_leaks)} test-support-looking sources are in production aggregates"
        )
    if duplicate_flags:
        warnings.append(f"{len(duplicate_flags)} duplicate adjacent LDFLAGS entries remain")

    print("Make/test manifest audit")
    print(f"  Make fragments: {len(paths)}")
    print(f"  Parsed targets: {len(graph)}")
    print(f"  Test targets: {len(test_targets)}")
    print(f"  Test source files: {len(discovered)}")
    print(f"  Referenced test source files: {len(referenced)}")

    report_list("Ungrouped test targets", ungrouped_targets)
    report_list("Orphan test source files", (str(path) for path in orphan_sources))
    report_list("Production aggregate test-support findings", production_leaks)
    report_list(
        "Duplicate adjacent LDFLAGS entries",
        (f"{path}:{line}" for path, line in duplicate_flags),
    )

    if warnings:
        print("\nWarnings:")
        for warning in warnings:
            print(f"  - {warning}")

    if args.strict:
        errors.extend(f"strict: {warning}" for warning in warnings)

    if errors:
        print("\nErrors:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    print("\nMake/test manifest structural checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
