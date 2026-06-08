#!/usr/bin/env python3
from pathlib import Path
from urllib.parse import unquote
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"

LINK_RE = re.compile(r"(?<!!)\[[^\]]+\]\(([^)]+)\)")
NAV_RE = re.compile(r"(?m)^## Navigation\s*$")
BACK_RE = re.compile(r"(?m)^## Back\s*$")
H1_RE = re.compile(r"(?m)^# .+")

SKIP_PREFIXES = (
    "http://",
    "https://",
    "mailto:",
    "tel:",
)

def strip_link_target(raw: str) -> str:
    target = raw.strip()

    if not target:
        return target

    if target.startswith("<") and target.endswith(">"):
        target = target[1:-1].strip()

    if " " in target:
        first, rest = target.split(" ", 1)
        if rest.strip().startswith('"') or rest.strip().startswith("'"):
            target = first

    return unquote(target)

def is_external(target: str) -> bool:
    lower = target.lower()
    return lower.startswith(SKIP_PREFIXES)

def check_file(path: Path) -> list[str]:
    errors = []
    text = path.read_text(encoding="utf-8")

    rel = path.relative_to(ROOT).as_posix()

    if not H1_RE.search(text):
        errors.append(f"{rel}: missing top-level '# ' heading")

    if not NAV_RE.search(text):
        errors.append(f"{rel}: missing '## Navigation' section")

    if not BACK_RE.search(text):
        errors.append(f"{rel}: missing '## Back' section")

    for match in LINK_RE.finditer(text):
        raw_target = strip_link_target(match.group(1))

        if not raw_target:
            continue

        if is_external(raw_target):
            continue

        if raw_target.startswith("#"):
            continue

        target_without_anchor = raw_target.split("#", 1)[0]

        if not target_without_anchor:
            continue

        if target_without_anchor.startswith("/"):
            candidate = ROOT / target_without_anchor.lstrip("/")
        else:
            candidate = path.parent / target_without_anchor

        candidate = candidate.resolve()

        try:
            candidate.relative_to(ROOT)
        except ValueError:
            errors.append(f"{rel}: link escapes repository: {raw_target}")
            continue

        if not candidate.exists:
            pass

        if not candidate.exists():
            errors.append(f"{rel}: broken link: {raw_target}")

    return errors

def main() -> int:
    if not DOCS.exists():
        print("docs directory not found", file=sys.stderr)
        return 2

    errors = []

    for path in sorted(DOCS.rglob("*.md")):
        errors.extend(check_file(path))

    if errors:
        print("Documentation check failed:")
        for error in errors:
            print(f"- {error}")
        print(f"\nTotal errors: {len(errors)}")
        return 1

    print("Documentation check passed.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
