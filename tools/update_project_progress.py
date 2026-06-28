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
        if line in ('## Overall Progress', '## Overall Foundation Progress'):
            section = 'overall'
            continue
        if line == '## Progress Items':
            section = 'items'
            continue
        if line == '## Current Milestone':
            section = 'current'
            continue
        if line.startswith('```'):
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


def progress_link_for(path):
    if path.name == 'README.md':
        return 'docs/planning/project-progress.md'
    if path.name == 'current-status.md':
        return '../planning/project-progress.md'
    if path.name == 'project-status-dashboard.md':
        return 'planning/project-progress.md'
    raise SystemExit('No progress link rule for ' + str(path))


def render_item(name, percent, state):
    if percent == 100:
        return '- ' + name + ' — complete'
    if percent == 0:
        return '- ' + name + ' — planned'
    return '- ' + name + ' — ' + str(percent) + '% ' + state


def render_group(title, items):
    lines = []
    if not items:
        return lines
    lines.append('### ' + title)
    lines.append('')
    for name, percent, state in items:
        lines.append(render_item(name, percent, state))
    lines.append('')
    return lines


def render_block(overall, items, current, source_link):
    completed = []
    in_progress = []
    planned = []
    other = []

    for item in items:
        name, percent, state = item
        normalized = state.strip().lower()
        if percent == 100 or normalized == 'completed':
            completed.append(item)
        elif percent == 0 or normalized == 'planned':
            planned.append(item)
        elif normalized == 'in progress':
            in_progress.append(item)
        else:
            other.append(item)

    lines = []
    lines.append(START)
    lines.append('## Project Progress')
    lines.append('')
    lines.append('Overall foundation progress, not product completion: **' + str(overall) + '%**')
    lines.append('')
    lines.extend(render_group('Completed foundations', completed))
    lines.extend(render_group('In progress', in_progress))
    lines.extend(render_group('Planned foundations', planned))
    lines.extend(render_group('Other tracked work', other))
    lines.append('Current milestone:')
    lines.append('')
    lines.append('```text')
    lines.append(current)
    lines.append('```')
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
