#!/usr/bin/env python3
"""Bounded Recording-Cut source repair, delivered through Git.

The source baseline is exact. The only permitted successor is the repository
commit adding this repair file. No production data or services are modified.
"""
import argparse
import ast
import base64
import difflib
import hashlib
import subprocess
import sys
import zlib
from pathlib import Path

ROOT = Path('/home/yavdr/vdr-suite')
BASE = '68439dd9b3641f72a97ff55584a750bccac158ae'
BRANCH = 'work/post-phase66-native-recording-editing'
SELF = 'tools/repairs/recording_cut_activation_repair.py'
PAYLOAD_SHA256 = '09508088182bcbca7e0fd27562e0734f638f5796d50fa3da4c2c812bac02e621'
PAYLOAD = (
    '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'
)

def git(*args):
    return subprocess.run(['git', *args], cwd=ROOT, text=True,
                          capture_output=True, check=True).stdout.strip()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--apply', action='store_true')
    args = parser.parse_args()

    if git('rev-parse', '--show-toplevel') != str(ROOT):
        raise RuntimeError('unexpected checkout root')
    if git('branch', '--show-current') != BRANCH:
        raise RuntimeError('unexpected branch')
    head = git('rev-parse', 'HEAD')
    if head != BASE:
        if git('rev-parse', 'HEAD^') != BASE:
            raise RuntimeError('unexpected source ancestry; no changes made')
        changed = git('diff', '--name-only', BASE, 'HEAD').splitlines()
        if changed != [SELF]:
            raise RuntimeError('delivery commit contains unexpected changes')
    git('diff', '--quiet')
    git('diff', '--cached', '--quiet')

    payload = zlib.decompress(base64.b64decode(PAYLOAD)).decode('utf-8')
    if hashlib.sha256(payload.encode()).hexdigest() != PAYLOAD_SHA256:
        raise RuntimeError('repair payload checksum mismatch')
    patches = ast.literal_eval(payload)
    before = {}
    after = {}
    for name, replacements in patches.items():
        original = (ROOT / name).read_text(encoding='utf-8')
        updated = original
        for old, new in replacements:
            count = updated.count(old)
            if count != 1:
                raise RuntimeError(
                    f'{name}: expected one exact anchor, found {count}')
            updated = updated.replace(old, new, 1)
        before[name], after[name] = original, updated

    patch = ''.join(
        ''.join(difflib.unified_diff(
            before[name].splitlines(keepends=True),
            after[name].splitlines(keepends=True),
            fromfile='a/' + name, tofile='b/' + name))
        for name in before)
    if not patch:
        raise RuntimeError('empty patch')
    output = Path('/var/tmp/vdr-suite-cut-activation.patch')
    output.write_text(patch, encoding='utf-8')
    git('apply', '--check', '--', str(output))
    print('PATCH_CHECK=PASS')
    print('PATCH=' + str(output))
    print('CHANGED_FILES=' + str(len(before)))
    if args.apply:
        git('apply', '--', str(output))
        git('diff', '--check')
        print('SOURCE_PATCH=APPLIED')
    else:
        print('SOURCE_PATCH=NOT_APPLIED')
    print('DATABASE_WRITE=not_executed')
    print('SERVICE_MUTATION=not_executed')
    print('NCUT_EXEC=not_executed')

if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, OSError, subprocess.CalledProcessError) as exc:
        print('REPAIR_ERROR=' + str(exc), file=sys.stderr)
        sys.exit(1)
