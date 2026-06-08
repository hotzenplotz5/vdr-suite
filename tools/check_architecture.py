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

SQLITE_ALLOWED_PREFIXES = [
    "core/recordings/src/",
    "core/sqlite/",
]

def repo_path(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()

def is_source_file(path: Path) -> bool:
    return path.is_file() and path.suffix in SOURCE_SUFFIXES

def is_allowed_sqlite_file(path: Path) -> bool:
    rel = repo_path(path)
    return any(rel.startswith(prefix) for prefix in SQLITE_ALLOWED_PREFIXES)

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
        "core/sqlite/ and core/recordings/src/*Repository.cpp"
    )

    return errors

def main() -> int:
    errors = []

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
