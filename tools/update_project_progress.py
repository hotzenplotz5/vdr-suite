#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'docs/planning/project-progress.md'

TARGETS = [
    ROOT / 'README.md',
    ROOT / 'docs/development/current-status.md',
    ROOT / 'docs/project-status-dashboard.md',
]

START = '<!-- PROJECT_PROGRESS_START -->'
END = '<!-- PROJECT_PROGRESS_END -->'

def parse_source():
    text = SOURCE.read_text(encoding='utf-8')
    overall = None
    items = []
    current = None
    section = None

    for raw in text.splitlines():
        line = raw.strip()
        if line == '## Overall Progress':
            section = 'overall'
            continue
        if line == '## Progress Items':
            section = 'items'
            continue
        if line == '## Current Milestone':
            section = 'current'
            continue
        if line.startswith('## ') or line == '---':
            if section != 'current':
                section = None
            continue
        if not line or line.startswith('- '):
            continue

        if section == 'overall' and line.startswith('overall|'):
            overall = int(line.split('|', 1)[1])
        elif section == 'items' and '|' in line:
            parts = line.split('|')
            if len(parts) != 3:
                raise SystemExit('Invalid progress item: ' + line)
            items.append((parts[0], int(parts[1]), parts[2]))
        elif section == 'current' and current is None:
            current = line

    if overall is None:
        raise SystemExit('Missing overall progress')
    if current is None:
        raise SystemExit('Missing current milestone')
    if not items:
        raise SystemExit('Missing progress items')

    return overall, items, current

def bar(percent):
    filled = round(percent / 10)
    empty = 10 - filled
    return '█' * filled + '░' * empty

def progress_link_for(path):
    if path.name == 'README.md':
        return 'docs/planning/project-progress.md'
    if path.name == 'current-status.md':
        return '../planning/project-progress.md'
    if path.name == 'project-status-dashboard.md':
        return 'planning/project-progress.md'
    raise SystemExit('No progress link rule for ' + str(path))


def render_block(overall, items, current, source_link):
    lines = []
    lines.append(START)
    lines.append('## Project Progress')
    lines.append('')
    lines.append('Overall project progress:')
    lines.append('')
    lines.append('    ' + bar(overall) + ' ' + str(overall) + '%')
    lines.append('')
    lines.append('Milestone progress:')
    lines.append('')
    width = max(len(name) for name, _, _ in items)
    for name, percent, state in items:
        label = name.ljust(width)
        lines.append('    ' + label + '  ' + bar(percent) + ' ' + str(percent).rjust(3) + '%  ' + state)
    lines.append('')
    lines.append('Current milestone:')
    lines.append('')
    lines.append('    ' + current)
    lines.append('')
    lines.append('Progress source: [Project Progress](' + source_link + ')')
    lines.append(END)
    return '\n'.join(lines)

def ensure_block(path, block):
    text = path.read_text(encoding='utf-8')
    if START in text and END in text:
        before = text.split(START, 1)[0]
        after = text.split(END, 1)[1]
        path.write_text(before + block + after, encoding='utf-8')
        return

    if path.name == 'README.md':
        marker = '## Current Project State'
    elif path.name == 'current-status.md':
        marker = '## Current Verified State'
    else:
        marker = '## Current Release State'

    if marker not in text:
        raise SystemExit('Marker not found in ' + str(path) + ': ' + marker)

    text = text.replace(marker, block + '\n\n---\n\n' + marker, 1)
    path.write_text(text, encoding='utf-8')

def main():
    overall, items, current = parse_source()
    for target in TARGETS:
        block = render_block(overall, items, current, progress_link_for(target))
        ensure_block(target, block)
    print('project progress blocks updated')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
