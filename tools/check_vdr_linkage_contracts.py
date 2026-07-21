#!/usr/bin/env python3
from pathlib import Path
import re
import sys

FILES = [
    Path("Makefile"),
    Path("mk/common.mk"),
    Path("mk/vdr-tests.mk"),
    Path("mk/epg-metadata-tests.mk"),
    Path("mk/local-test-groups.mk"),
]

TARGET_PATTERN = re.compile(r"(?m)^([A-Za-z0-9_.-]+):[^\n]*\n")


def target_blocks(text):
    matches = list(TARGET_PATTERN.finditer(text))
    for index, match in enumerate(matches):
        start = match.start()
        end = matches[index + 1].start() if index + 1 < len(matches) else len(text)
        yield match.group(1), text[start:end]


def has_any(block, needles):
    return any(needle in block for needle in needles)


def provides_cache_repository(block):
    return has_any(block, [
        "core/vdr/src/VdrRecordingCacheRepository.cpp",
        "$(DAEMON_SRC)",
    ])


def main():
    errors = []
    checked_targets = 0

    for path in FILES:
        if not path.exists():
            errors.append(f"{path}: missing file")
            continue

        text = path.read_text(encoding="utf-8")

        for target, block in target_blocks(text):
            if "$(CXX)" not in block and "g++" not in block:
                continue

            checked_targets += 1

            uses_vdr_src = "$(VDR_SRC)" in block
            uses_sqlite = has_any(block, [
                "$(SQLITE_SRC)",
                "core/sqlite/src/Database.cpp",
                "core/vdr/src/VdrRecordingCacheRepository.cpp",
                "$(DAEMON_SRC)",
            ])
            uses_router = has_any(block, [
                "$(REST_ROUTER_SRC)",
                "api/rest/src/ApiRouter.cpp",
            ])

            if uses_vdr_src:
                if not provides_cache_repository(block):
                    errors.append(
                        f"{path}:{target}: uses $(VDR_SRC) but misses a cache repository provider"
                    )

                if not has_any(block, ["$(SQLITE_SRC)", "core/sqlite/src/Database.cpp"]):
                    errors.append(
                        f"{path}:{target}: uses $(VDR_SRC) but misses $(SQLITE_SRC)"
                    )

                if not has_any(block, ["$(LDFLAGS)", "-lsqlite3"]):
                    errors.append(
                        f"{path}:{target}: uses $(VDR_SRC) but misses $(LDFLAGS)"
                    )

            if uses_sqlite:
                if not has_any(block, ["$(LDFLAGS)", "-lsqlite3"]):
                    errors.append(
                        f"{path}:{target}: links SQLite-backed code but misses $(LDFLAGS)"
                    )

            if uses_router:
                if "api/rest/src/VdrRecordingFolderController.cpp" not in block:
                    errors.append(
                        f"{path}:{target}: links ApiRouter/REST_ROUTER_SRC but misses "
                        "api/rest/src/VdrRecordingFolderController.cpp"
                    )

    print(f"checked C++ build targets: {checked_targets}")

    if errors:
        print("VDR linkage contract violations:")
        for error in errors:
            print(" - " + error)
        raise SystemExit(1)

    print("VDR linkage contracts ok")


if __name__ == "__main__":
    main()
