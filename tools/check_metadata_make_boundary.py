#!/usr/bin/env python3

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]


def read(relative_path: str) -> str:
    return (ROOT / relative_path).read_text(encoding="utf-8")


def count_assignment(text: str, variable: str) -> int:
    pattern = re.compile(rf"(?m)^{re.escape(variable)}\s*[:+?]?=")
    return len(pattern.findall(text))


def main() -> int:
    errors: list[str] = []

    recording_sources = read("mk/recording-sources.mk")
    platform_sources = read("mk/metadata-sources.mk")
    platform_tests = read("mk/metadata-tests.mk")
    smoke_targets = read("mk/smoke-targets.mk")

    if count_assignment(recording_sources, "METADATA_SRC") != 1:
        errors.append(
            "mk/recording-sources.mk must own exactly one METADATA_SRC assignment"
        )

    if count_assignment(platform_sources, "METADATA_SRC") != 0:
        errors.append(
            "mk/metadata-sources.mk must not redefine legacy METADATA_SRC"
        )

    if count_assignment(platform_sources, "METADATA_PLATFORM_SRC") != 1:
        errors.append(
            "mk/metadata-sources.mk must own exactly one METADATA_PLATFORM_SRC assignment"
        )

    if "$(METADATA_PLATFORM_SRC)" not in platform_tests:
        errors.append(
            "mk/metadata-tests.mk must compile the Suite metadata platform through METADATA_PLATFORM_SRC"
        )

    if "$(METADATA_SRC)" in platform_tests:
        errors.append(
            "mk/metadata-tests.mk must not consume the legacy METADATA_SRC variable"
        )

    if "test-metadata-service:" not in smoke_targets:
        errors.append(
            "legacy test-metadata-service target is missing from mk/smoke-targets.mk"
        )

    if "test-metadata-service" not in platform_tests:
        errors.append(
            "test-metadata-foundation must execute the legacy metadata service regression"
        )

    if errors:
        print("Metadata Make boundary check failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("Metadata Make boundary check passed.")
    print("Legacy source owner: mk/recording-sources.mk -> METADATA_SRC")
    print("Platform source owner: mk/metadata-sources.mk -> METADATA_PLATFORM_SRC")
    return 0


if __name__ == "__main__":
    sys.exit(main())
