#!/usr/bin/env python3

from pathlib import Path
import argparse
import re
import subprocess
import sys


ROOT = Path.cwd()


STALE_RULES = [
    (
        "old SearchTimer progress block",
        re.compile(r"SearchTimer Foundation\s+.*95%\s+in progress"),
    ),
    (
        "old SearchTimer progress source",
        re.compile(r"SearchTimer Foundation\|95\|in progress"),
    ),
    (
        "old SearchTimer foundation current milestone",
        re.compile(r"Phase 47\.0 - SearchTimer Foundation"),
    ),
    (
        "old real VDR regression next phase",
        re.compile(r"Phase 47\.67 - Add real VDR read-only regression helper"),
    ),
    (
        "old EPG person search current milestone wording",
        re.compile(r"EPG Person Search Foundation"),
    ),
    (
        "old EPGSearch parameter regression current focus",
        re.compile(r"Phase 49\.5 - EPGSearch parameter regression expansion"),
    ),
    (
        "old Phase 50 backend-management roadmap position",
        re.compile(r"Phase 50 - Backend Management Foundation"),
    ),
]


ALLOWED_HISTORICAL_MATCHES = [
    (
        Path("docs/development/completed-phases.md"),
        re.compile(r"### EPG Person Search Foundation"),
    ),
    (
        Path("docs/development/completed-phases.md"),
        re.compile(r"## Phase 49\.5 - EPGSearch parameter regression expansion"),
    ),
    (
        Path("docs/development/real-vdr-regression-coverage-audit.md"),
        re.compile(r"Phase 47\.67 originally recommended"),
    ),
]


DOC_SUFFIXES = {".md"}
DOC_ROOTS = [
    Path("README.md"),
    Path("docs"),
]


def run(command, check=False):
    result = subprocess.run(
        command,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    if check and result.returncode != 0:
        print(result.stdout, end="")
        print(result.stderr, end="", file=sys.stderr)
        raise SystemExit(result.returncode)

    return result


def git_lines(command):
    result = run(command, check=True)
    return [line.strip() for line in result.stdout.splitlines() if line.strip()]


def tracked_doc_files():
    files = []

    result = run(["git", "ls-files"], check=True)

    for raw in result.stdout.splitlines():
        path = Path(raw.strip())

        if not raw.strip():
            continue

        if path == Path("README.md"):
            files.append(path)
            continue

        if path.parts and path.parts[0] == "docs" and path.suffix in DOC_SUFFIXES:
            files.append(path)

    return files


def is_allowed(path, line):
    for allowed_path, pattern in ALLOWED_HISTORICAL_MATCHES:
        if path == allowed_path and pattern.search(line):
            return True

    return False


def scan_stale_patterns(files):
    findings = []
    allowed = []

    for path in files:
        full_path = ROOT / path

        if not full_path.exists():
            continue

        try:
            lines = full_path.read_text(encoding="utf-8").splitlines()
        except UnicodeDecodeError:
            continue

        for number, line in enumerate(lines, start=1):
            for label, pattern in STALE_RULES:
                if pattern.search(line):
                    item = (label, path, number, line)

                    if is_allowed(path, line):
                        allowed.append(item)
                    else:
                        findings.append(item)

    return findings, allowed


def check_progress_source():
    source = ROOT / "docs/planning/project-progress.md"

    if not source.exists():
        return ["missing docs/planning/project-progress.md"]

    text = source.read_text(encoding="utf-8")
    problems = []

    required = [
        "overall|90",
        "SearchTimer Backend Foundation|100|completed",
        "SearchTimer User Workflow|100|completed",
        "Phase 51.10 - Live parity discovery foundation completion",
    ]

    forbidden = [
        "overall|77",
        "overall|78",
        "SearchTimer Foundation|95|in progress",
        "SearchTimer User Workflow|0|in progress",
        "Phase 47.0 - SearchTimer Foundation",
        "Phase 50.0 - SearchTimer user workflow foundation",
    ]

    for marker in required:
        if marker not in text:
            problems.append(f"missing progress marker: {marker}")

    for marker in forbidden:
        if marker in text:
            problems.append(f"forbidden progress marker still present: {marker}")

    return problems


def check_generated_progress_blocks():
    result = run(["git", "diff", "--quiet"])

    if result.returncode != 0:
        return [
            "working tree has changes; skip generator consistency check to avoid mixing audit with uncommitted edits"
        ]

    before = run(["git", "status", "--short"], check=True).stdout

    generated = run(["python3", "tools/update_project_progress.py"])

    after_diff = run(["git", "diff", "--name-only"]).stdout.splitlines()

    if after_diff:
        run(["git", "restore", "README.md", "docs/development/current-status.md", "docs/project-status-dashboard.md"])
        return [
            "project progress generator would modify: " + ", ".join(after_diff)
        ]

    after = run(["git", "status", "--short"], check=True).stdout

    if before != after:
        return ["working tree state changed during generator check"]

    if generated.returncode != 0:
        return ["project progress generator failed"]

    return []


def run_optional_tests():
    problems = []

    for target in ["test-docs", "test-phase"]:
        result = run(["make", target])

        if result.returncode != 0:
            problems.append(f"make {target} failed")
            print(result.stdout, end="")
            print(result.stderr, end="", file=sys.stderr)

    return problems


def print_section(title):
    print()
    print("== " + title + " ==")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--commits", type=int, default=20)
    parser.add_argument("--run-tests", action="store_true")
    args = parser.parse_args()

    print_section("Recent commits")
    recent = git_lines(["git", "log", f"--oneline", f"-{args.commits}"])
    for line in recent:
        print(line)

    print_section("Files changed in recent commit window")
    base = f"HEAD~{args.commits}"
    diff_result = run(["git", "diff", "--name-only", f"{base}..HEAD"])

    if diff_result.returncode != 0:
        print(f"Could not diff {base}..HEAD, falling back to HEAD~1..HEAD")
        diff_result = run(["git", "diff", "--name-only", "HEAD~1..HEAD"], check=True)

    changed_files = [line.strip() for line in diff_result.stdout.splitlines() if line.strip()]

    for file_name in changed_files:
        print(file_name)

    print_section("Stale documentation marker scan")
    files = tracked_doc_files()
    findings, allowed = scan_stale_patterns(files)

    if allowed:
        print("Allowed historical matches:")
        for label, path, number, line in allowed:
            print(f"ALLOW {path}:{number}: {label}: {line}")

    if findings:
        print()
        print("Unexpected stale matches:")
        for label, path, number, line in findings:
            print(f"FAIL {path}:{number}: {label}: {line}")
    else:
        print("No unexpected stale markers found.")

    print_section("Progress source check")
    progress_problems = check_progress_source()

    if progress_problems:
        for problem in progress_problems:
            print("FAIL " + problem)
    else:
        print("Progress source markers are consistent.")

    print_section("Generated progress block check")
    generator_problems = check_generated_progress_blocks()

    if generator_problems:
        for problem in generator_problems:
            print("FAIL " + problem)
    else:
        print("Generated progress blocks are consistent.")

    test_problems = []

    if args.run_tests:
        print_section("Tests")
        test_problems = run_optional_tests()

        if test_problems:
            for problem in test_problems:
                print("FAIL " + problem)
        else:
            print("make test-docs and make test-phase passed.")

    all_problems = findings or progress_problems or generator_problems or test_problems

    print_section("Result")

    if all_problems:
        print("RESULT: FAIL")
        return 1

    print("RESULT: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
