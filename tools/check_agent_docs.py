#!/usr/bin/env python3
"""Read-only, advisory audit of VDR-Suite agent instructions (never a CI gate)."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[1]
REGISTER = Path('docs/development/agent-rule-register.md')
RULE_ROW = re.compile(r'^\s*\|\s*(AG-\d{3})\s*\|')
LINK = re.compile(r'(?<!!)\[[^\]\n]+\]\(([^)\n]+)\)')
FENCE = re.compile(r'^\s*(`{3,}|~{3,})\s*([a-zA-Z0-9_-]*)')
STALE_PROSE = re.compile(
    r'^\s*(?:[-*]\s*)?(?:No root compilation\.|'
    r'Build still runs only (?:under|as) [`\x27\"]?yavdr\b|'
    r'Build exactly once as [`\x27\"]?yavdr\b)', re.IGNORECASE
)
STALE_SHELL = re.compile(
    r'^\s*(?:sudo\s+-u\s+yavdr\b|runuser\s+-u\s+yavdr\b|'
    r'su\s+-\s+yavdr\b|set\s+-(?:euo\s+pipefail|e|u|o\s+pipefail)\s*(?:$|[;#]))'
)


@dataclass(frozen=True)
class Finding:
    kind: str
    source: str
    line: int
    explanation: str


def source_files(root: Path) -> list[Path]:
    """Only agent instructions, not all repository docs or generated files."""
    agents = (p for p in root.rglob('AGENTS.md')
              if not any(part.startswith('.') for part in p.relative_to(root).parts)
              and not any(part in ('node_modules', 'vendor') for part in p.relative_to(root).parts))
    guides = (root / 'docs' / 'development').glob('agent-*.md')
    return sorted(set(agents) | set(guides))


def destination(raw: str) -> str | None:
    """Return relative Markdown destination, or None for an external/anchor link."""
    value = raw.strip()
    if value.startswith('<'):
        value = value[1:].split('>', 1)[0]
    else:
        value = value.split()[0] if value else ''
    if (not value or value.startswith(('#', '//'))
            or re.match(r'^[a-z][a-z\d+.-]*:', value, re.IGNORECASE)):
        return None
    return unquote(value.split('#', 1)[0].split('?', 1)[0])


def scan(root: Path) -> tuple[list[Finding], int]:
    root = root.resolve()
    findings: list[Finding] = []
    targets = source_files(root)
    reg = root / REGISTER
    if not reg.is_file():
        findings.append(Finding('REGISTER_MISSING', str(REGISTER), 0,
                                'Agent rule index is absent'))
    else:
        seen: dict[str, int] = {}
        for number, line in enumerate(reg.read_text(encoding='utf-8').splitlines(), 1):
            match = RULE_ROW.match(line)
            if match:
                rule_id = match.group(1)
                if rule_id in seen:
                    findings.append(Finding('DUPLICATE_ID', REGISTER.as_posix(), number,
                                            f'{rule_id}; first declaration on line {seen[rule_id]}'))
                else:
                    seen[rule_id] = number

    for file in targets:
        name = file.relative_to(root).as_posix()
        lines = file.read_text(encoding='utf-8').splitlines()
        fence_char = ''
        fence_length = 0
        executable = False
        for number, line in enumerate(lines, 1):
            fence = FENCE.match(line)
            if fence:
                marker, lang = fence.groups()
                if not fence_char:
                    fence_char, fence_length = marker[0], len(marker)
                    executable = lang.lower() in ('bash', 'sh', 'shell', 'console')
                elif marker[0] == fence_char and len(marker) >= fence_length and not lang:
                    fence_char, fence_length, executable = '', 0, False
                continue
            if not fence_char:
                if STALE_PROSE.match(line):
                    findings.append(Finding('OBSOLETE_RULE', name, number, line.strip()))
                for link in LINK.finditer(line):
                    relative = destination(link.group(1))
                    if relative is None:
                        continue
                    target = (file.parent / relative).resolve()
                    if not target.is_relative_to(root) or not target.exists():
                        findings.append(Finding('BROKEN_LINK', name, number,
                                                f'{link.group(1)}'))
            elif executable and STALE_SHELL.match(line):
                findings.append(Finding('OBSOLETE_COMMAND', name, number, line.strip()))
    return sorted(findings, key=lambda f: (f.source, f.line, f.kind)), len(targets)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT,
                        help='repository root (default: script checkout)')
    args = parser.parse_args(argv)
    if not args.root.is_dir():
        parser.error(f'no such repository directory: {args.root}')
    try:
        findings, scanned = scan(args.root)
    except (OSError, UnicodeError) as exc:
        print(f'AGENT_DOC_AUDIT=ERROR: {exc}')
        return 2
    for finding in findings:
        print(f'{finding.kind} {finding.source}:{finding.line}: {finding.explanation}')
    print(f'AGENT_DOC_AUDIT={"WARNINGS" if findings else "PASS"} '
          f'FILES={scanned} FINDINGS={len(findings)} ADVISORY_ONLY=YES')
    return 0  # Intentional: diagnostics never fail CI, builds, or installation.


if __name__ == '__main__':
    raise SystemExit(main())
