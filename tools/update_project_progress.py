#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'docs/planning/project-progress.md'

TARGETS = [
    ROOT / 'README.md',
    ROOT / 'docs/development/current-status.md',
    ROOT / 'docs/project-status-dashboard.md',
]

TARGET_START = '<!-- PROJECT_PROGRESS_START -->'
TARGET_END = '<!-- PROJECT_PROGRESS_END -->'
SOURCE_START = '<!-- PROJECT_STATE_SNAPSHOT_START -->'
SOURCE_END = '<!-- PROJECT_STATE_SNAPSHOT_END -->'
SOURCE_LINK_TOKEN = '{{PROJECT_PROGRESS_SOURCE_LINK}}'


def progress_link_for(path):
    if path.name == 'README.md':
        return 'docs/planning/project-progress.md'
    if path.name == 'current-status.md':
        return '../planning/project-progress.md'
    if path.name == 'project-status-dashboard.md':
        return 'planning/project-progress.md'
    raise SystemExit('No progress link rule for ' + str(path))


def load_snapshot_template():
    text = SOURCE.read_text(encoding='utf-8')
    if SOURCE_START not in text or SOURCE_END not in text:
        raise SystemExit('Project state snapshot markers missing in ' + str(SOURCE))

    snapshot = text.split(SOURCE_START, 1)[1].split(SOURCE_END, 1)[0].strip()
    if not snapshot:
        raise SystemExit('Project state snapshot is empty in ' + str(SOURCE))

    return snapshot


def render_block(snapshot, source_link):
    block = snapshot.replace(SOURCE_LINK_TOKEN, source_link)
    return TARGET_START + '\n' + block + '\n' + TARGET_END


def ensure_block(path, block):
    text = path.read_text(encoding='utf-8')
    if TARGET_START in text and TARGET_END in text:
        before = text.split(TARGET_START, 1)[0]
        after = text.split(TARGET_END, 1)[1]
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
    snapshot = load_snapshot_template()
    for target in TARGETS:
        block = render_block(snapshot, progress_link_for(target))
        ensure_block(target, block)
    print('project state snapshot blocks updated')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
