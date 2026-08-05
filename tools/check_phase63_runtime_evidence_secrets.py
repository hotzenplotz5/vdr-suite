#!/usr/bin/env python3
"""Fail closed when Phase-63 runtime evidence logs contain secret material."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re
import sys
import tempfile
from typing import Iterable

PATTERNS: tuple[tuple[str, re.Pattern[str]], ...] = (
    (
        "enrollment-token-key",
        re.compile(r"(?i)(?<![A-Za-z0-9_])enrollment_token\s*[=:]"),
    ),
    (
        "credential-secret-key",
        re.compile(r"(?i)(?<![A-Za-z0-9_])(?:pending_)?credential_secret\s*[=:]"),
    ),
    (
        "authorization-header",
        re.compile(r"(?i)(?<![A-Za-z0-9_])authorization\s*:"),
    ),
    (
        "enrollment-token-value",
        re.compile(r"(?<![A-Za-z0-9_-])ent_[A-Za-z0-9_-]{16,}(?![A-Za-z0-9_-])"),
    ),
)

ASSIGNMENT_VALUE = re.compile(
    r"(?i)((?:pending_)?credential_secret|enrollment_token)(\s*[=:]\s*)[^\s,;\"']+"
)
AUTHORIZATION_VALUE = re.compile(r"(?i)(authorization\s*:\s*).*$")
ENROLLMENT_TOKEN_VALUE = re.compile(
    r"(?<![A-Za-z0-9_-])ent_[A-Za-z0-9_-]{16,}(?![A-Za-z0-9_-])"
)


def redact(line: str) -> str:
    line = AUTHORIZATION_VALUE.sub(r"\1[REDACTED]", line)
    line = ASSIGNMENT_VALUE.sub(r"\1\2[REDACTED]", line)
    line = ENROLLMENT_TOKEN_VALUE.sub("ent_[REDACTED]", line)
    return line[:500]


def log_files(evidence_dir: Path) -> Iterable[Path]:
    for path in sorted(evidence_dir.glob("*.log")):
        if path.is_symlink() or not path.is_file():
            raise RuntimeError(f"unsafe evidence log path: {path.name}")
        yield path


def scan(evidence_dir: Path) -> list[tuple[str, int, tuple[str, ...], str, str]]:
    findings: list[tuple[str, int, tuple[str, ...], str, str]] = []
    for path in log_files(evidence_dir):
        text = path.read_text(encoding="utf-8", errors="replace")
        for line_number, line in enumerate(text.splitlines(), 1):
            categories = tuple(name for name, pattern in PATTERNS if pattern.search(line))
            if not categories:
                continue
            digest = hashlib.sha256(line.encode("utf-8")).hexdigest()[:16]
            findings.append((path.name, line_number, categories, digest, redact(line)))
    return findings


def emit_report(evidence_dir: Path) -> int:
    if not evidence_dir.is_dir():
        print("EVIDENCE_SECRET_SCAN=ERROR")
        print("REASON=evidence_directory_missing")
        return 2
    try:
        findings = scan(evidence_dir)
    except (OSError, RuntimeError) as error:
        print("EVIDENCE_SECRET_SCAN=ERROR")
        print(f"REASON={str(error).replace(chr(10), ' ')[:300]}")
        return 2
    if not findings:
        print("EVIDENCE_SECRET_SCAN=PASS")
        print("FINDINGS=0")
        return 0
    print("EVIDENCE_SECRET_SCAN=FAIL")
    print(f"FINDINGS={len(findings)}")
    for filename, line_number, categories, digest, excerpt in findings:
        print(
            f"FILE={filename} LINE={line_number} "
            f"CATEGORY={','.join(categories)} LINE_SHA256={digest}"
        )
        print(f"REDACTED={excerpt}")
    return 1


def self_test() -> int:
    with tempfile.TemporaryDirectory(prefix="phase63-evidence-secret-test-") as temporary:
        root = Path(temporary)
        benign = root / "benign"
        benign.mkdir()
        (benign / "backend-agent.journal.log").write_text(
            "Backend Agent synchronized\n"
            "Backend Agent credential rotation succeeded\n"
            '{"agentId":"agt_0123456789abcdef0123456789abcdef",'
            '"credentialGeneration":2,"state":"online"}\n'
            "Controlled Backend Agent enrollment package created\n",
            encoding="utf-8",
        )
        if scan(benign):
            print("self-test benign evidence produced a false positive", file=sys.stderr)
            return 1

        malicious = root / "malicious"
        malicious.mkdir()
        secret = "ent_0123456789abcdefghijklmnopqrstuv"
        (malicious / "leak.log").write_text(
            "enrollment_token=" + secret + "\n"
            "credential_secret=ags_0123456789abcdefghijklmnopqrstuv\n"
            "Authorization: Basic dXNlcjpzZWNyZXQ=\n",
            encoding="utf-8",
        )
        findings = scan(malicious)
        categories = {category for finding in findings for category in finding[2]}
        expected = {
            "enrollment-token-key",
            "credential-secret-key",
            "authorization-header",
            "enrollment-token-value",
        }
        if categories != expected:
            print(
                f"self-test missed secret categories: expected={sorted(expected)} "
                f"actual={sorted(categories)}",
                file=sys.stderr,
            )
            return 1
        rendered = "\n".join(finding[4] for finding in findings)
        if secret in rendered or "dXNlcjpzZWNyZXQ=" in rendered:
            print("self-test report exposed secret material", file=sys.stderr)
            return 1
    print("Phase-63 runtime evidence secret scanner self-test passed")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("evidence_dir", nargs="?", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if args.evidence_dir is None:
        parser.error("evidence_dir is required unless --self-test is used")
    return emit_report(args.evidence_dir)


if __name__ == "__main__":
    raise SystemExit(main())
