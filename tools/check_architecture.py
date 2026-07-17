#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

SOURCE_DIRS = [
    ROOT / "apps",
    ROOT / "api",
    ROOT / "core",
]

SOURCE_SUFFIXES = {
    ".h",
    ".hpp",
    ".cpp",
    ".cc",
    ".cxx",
}

SQLITE_INFRASTRUCTURE_PREFIXES = [
    "core/sqlite/",
]

SQLITE_DOMAIN_REPOSITORY_PREFIXES = [
    "core/recordings/src/",
    "core/vdr/src/",
    "core/metadata/src/",
]

SQLITE_ALLOWED_CONTRACT_TESTS = {
    "core/metadata/tests/test_metadata_schema_contract.cpp",
}


def repo_path(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def is_source_file(path: Path) -> bool:
    return path.is_file() and path.suffix in SOURCE_SUFFIXES


def is_sqlite_infrastructure_file(path: Path) -> bool:
    rel = repo_path(path)
    return any(
        rel.startswith(prefix)
        for prefix in SQLITE_INFRASTRUCTURE_PREFIXES
    )


def is_domain_repository_implementation(path: Path) -> bool:
    rel = repo_path(path)

    if not any(
        rel.startswith(prefix)
        for prefix in SQLITE_DOMAIN_REPOSITORY_PREFIXES
    ):
        return False

    return path.name.endswith("Repository.cpp")


def is_registered_sqlite_contract_test(path: Path) -> bool:
    return repo_path(path) in SQLITE_ALLOWED_CONTRACT_TESTS


def is_allowed_sqlite_file(path: Path) -> bool:
    return (
        is_sqlite_infrastructure_file(path)
        or is_domain_repository_implementation(path)
        or is_registered_sqlite_contract_test(path)
    )


def collect_source_files() -> list[Path]:
    result = []

    for directory in SOURCE_DIRS:
        if not directory.exists():
            continue

        for path in directory.rglob("*"):
            if is_source_file(path):
                result.append(path)

    return sorted(result)


def check_sqlite_boundary(path: Path, text: str) -> list[str]:
    errors = []

    if "sqlite3.h" not in text and "sqlite3_" not in text:
        return errors

    if is_allowed_sqlite_file(path):
        return errors

    errors.append(
        f"{repo_path(path)}: direct SQLite usage is only allowed in "
        "core/sqlite/, approved domain *Repository.cpp implementations, "
        "and explicitly registered SQLite/schema contract tests"
    )

    return errors


def check_sqlite_boundary_contract() -> list[str]:
    errors = []
    sqlite_source = "#include <sqlite3.h>\nsqlite3_stmt* stmt = nullptr;\n"

    allowed_paths = [
        "core/sqlite/src/Database.cpp",
        "core/recordings/src/RecordingRepository.cpp",
        "core/vdr/src/EpgEventRepository.cpp",
        "core/vdr/src/EpgSearchNativeFuzzyCapabilityRepository.cpp",
        "core/vdr/src/VdrRecordingCacheRepository.cpp",
        "core/metadata/src/MetadataEntityRepository.cpp",
        "core/metadata/tests/test_metadata_schema_contract.cpp",
    ]

    rejected_paths = [
        "core/recordings/src/RecordingActionService.cpp",
        "core/vdr/src/VdrService.cpp",
        "core/vdr/src/RepositoryHelper.cpp",
        "core/metadata/src/MetadataResolver.cpp",
        "core/metadata/tests/test_metadata_identity.cpp",
        "api/rest/src/FakeRepository.cpp",
        "apps/example/src/FakeRepository.cpp",
    ]

    for rel in allowed_paths:
        path = ROOT / rel
        path_errors = check_sqlite_boundary(path, sqlite_source)
        if path_errors:
            errors.append(
                "SQLite boundary contract expected allowed path: " + rel
            )

    for rel in rejected_paths:
        path = ROOT / rel
        path_errors = check_sqlite_boundary(path, sqlite_source)
        if len(path_errors) != 1:
            errors.append(
                "SQLite boundary contract expected rejected path: " + rel
            )

    return errors


def main() -> int:
    errors = check_sqlite_boundary_contract()

    for path in collect_source_files():
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue

        errors.extend(check_sqlite_boundary(path, text))

    if errors:
        print("Architecture check failed:")
        for error in errors:
            print(f"- {error}")
        print(f"\nTotal errors: {len(errors)}")
        return 1

    print("Architecture check passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
