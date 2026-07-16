#!/usr/bin/env python3
"""Print the complete non-fatal Make/test debt inventory."""

from __future__ import annotations

from pathlib import Path

import check_make_test_manifest as audit


def main() -> int:
    paths = audit.make_files()
    graph, _definitions = audit.parse_targets(paths)

    grouped_targets: set[str] = set()
    for group in audit.PUBLIC_GROUPS:
        grouped_targets.update(audit.reachable(graph, group))

    test_targets = {
        target
        for target in graph
        if target == "test" or target.startswith("test-")
    }
    ungrouped_targets = sorted(
        target
        for target in test_targets - grouped_targets
        if target != "test-make-test-manifest-strict"
    )

    referenced = audit.referenced_test_sources(paths)
    discovered = audit.discovered_test_sources()
    orphan_sources = sorted(discovered - referenced)

    aggregates = audit.parse_source_aggregates(paths)
    production_leaks: list[str] = []
    for name in ("VDR_SRC", "DAEMON_SRC", "REST_ROUTER_SRC"):
        for source in aggregates.get(name, []):
            if (
                Path(source).name.startswith(("Mock", "Test"))
                and (name, source) not in audit.INTENTIONAL_RUNTIME_AGGREGATES
            ):
                production_leaks.append(f"{name}: {source}")

    duplicate_flags = audit.duplicate_ldflags(paths)

    print("\nComplete ungrouped test target list:")
    for target in ungrouped_targets:
        print(target)

    print("\nComplete orphan test source list:")
    for path in orphan_sources:
        print(path)

    print("\nComplete production aggregate test-support list:")
    for finding in sorted(production_leaks):
        print(finding)

    print("\nIntentional runtime aggregate allowlist:")
    for (name, source), reason in sorted(
        audit.INTENTIONAL_RUNTIME_AGGREGATES.items()
    ):
        print(f"{name}: {source} — {reason}")

    print("\nComplete duplicate LDFLAGS list:")
    for path, line in duplicate_flags:
        print(f"{path}:{line}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
