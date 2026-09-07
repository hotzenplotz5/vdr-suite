#!/usr/bin/env python3
"""Finish the exact Recording-Cut activation repair without discarding local work.

Only the demonstrated regression expectation is changed. The original source
repair is reconstructed from the checked-in, checksum-verified payload. This
helper never installs services, modifies production data, or starts a cut.
"""
import argparse
import ast
import base64
import hashlib
import importlib.util
import os
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BASE = '68439dd9b3641f72a97ff55584a750bccac158ae'
DELIVERY = '96393a4ddc2d172968c53a9d3445ae31f50f7ea8'
BRANCH = 'work/post-phase66-native-recording-editing'
SELF = 'tools/repairs/recording_cut_activation_finish.py'
ORIGINAL = 'tools/repairs/recording_cut_activation_repair.py'
TEST = 'core/agent/tests/test_backend_agent_recording_cut_reconciliation.cpp'
COMMIT_MESSAGE = 'fix(recording-cut): activate fenced command delivery'


def git(*args, capture=True):
    result = subprocess.run(['git', *args], cwd=ROOT, text=True,
                            capture_output=capture, check=True)
    return result.stdout.strip() if capture else ''


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def replace_one(content, old, new):
    require(content.count(old) == 1, 'expected exactly one source anchor')
    return content.replace(old, new, 1)


def payload():
    spec = importlib.util.spec_from_file_location('cut_activation_original', ROOT / ORIGINAL)
    require(spec is not None and spec.loader is not None, 'original repair unavailable')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    encoded = zlib.decompress(base64.b64decode(module.PAYLOAD)).decode('utf-8')
    require(hashlib.sha256(encoded.encode()).hexdigest() == module.PAYLOAD_SHA256,
            'original repair payload checksum mismatch')
    patches = ast.literal_eval(encoded)
    require(set(patches) == {
        'core/agent/src/BackendAgentCommandDelivery.cpp',
        'core/agent/src/BackendAgentRecordingCutReconciliationRepository.cpp',
        TEST}, 'unexpected original repair scope')
    return patches


def expected_states():
    patches = payload()
    expected = {}
    for name, replacements in patches.items():
        content = git('show', BASE + ':' + name) + '\n'
        for old, new in replacements:
            content = replace_one(content, old, new)
        expected[name] = content
    corrected = expected[TEST]
    corrected = replace_one(corrected,
        '    assert(!duplicatePoll.assignment.present);\n'
        '    acceptReceipt(commands, delivered, clock + 3);',
        '    assert(duplicatePoll.assignment.present);\n'
        '    assert(duplicatePoll.assignment.commandId == delivered.commandId);\n'
        '    assert(duplicatePoll.assignment.requestFingerprint ==\n'
        '        delivered.requestFingerprint);\n'
        '    assert(commands.summaryForBackend("default").deliveryCount == 2);\n'
        '    acceptReceipt(commands, delivered, clock + 3);\n'
        '    const auto acknowledgedPoll = pollCut(commands, deliveryFacts, clock + 3);\n'
        '    assert(acknowledgedPoll.accepted);\n'
        '    assert(!acknowledgedPoll.assignment.present);')
    corrected = replace_one(corrected,
        '    assert(storedResult.deliveryCount == 1);',
        '    assert(storedResult.deliveryCount == 2);')
    final = dict(expected)
    final[TEST] = corrected
    return expected, final


def verify_checkout():
    require(git('rev-parse', '--show-toplevel') == str(ROOT), 'unexpected checkout root')
    require(git('branch', '--show-current') == BRANCH, 'unexpected branch')
    require(git('rev-parse', 'HEAD^') == DELIVERY, 'unexpected delivery ancestry')
    require(git('diff', '--name-only', DELIVERY, 'HEAD').splitlines() == [SELF],
            'unexpected delivery commit contents')
    require(not git('diff', '--cached', '--name-only'), 'staged changes present')


def verify_worktree(expected, final, allow_old):
    names = set(final)
    changed = set(git('diff', '--name-only').splitlines())
    require(changed == names, 'tracked worktree differs from the exact repair scope')
    actual = {name: (ROOT / name).read_text(encoding='utf-8') for name in names}
    for name in names:
        allowed = {final[name]}
        if allow_old:
            allowed.add(expected[name])
        require(actual[name] in allowed, 'unexpected content in ' + name)
    return actual


def apply_correction(expected, final):
    actual = verify_worktree(expected, final, True)
    for name in final:
        if name != TEST:
            require(actual[name] == final[name], 'production repair differs from expected')
    if actual[TEST] == final[TEST]:
        print('REGRESSION_CORRECTION=ALREADY_APPLIED')
        return
    require(actual[TEST] == expected[TEST], 'unexpected regression test content')
    path = ROOT / TEST
    descriptor, temporary = tempfile.mkstemp(prefix='.cut-repair-', dir=path.parent)
    try:
        with os.fdopen(descriptor, 'w', encoding='utf-8') as stream:
            stream.write(final[TEST])
        os.chmod(temporary, path.stat().st_mode & 0o777)
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
    verify_worktree(expected, final, False)
    git('diff', '--check')
    print('REGRESSION_CORRECTION=APPLIED')


def checks():
    targets = [
        'test-backend-agent-recording-cut-reconciliation',
        'test-backend-agent-recording-cut-executor',
        'test-backend-agent-recording-cut-local-state',
        'check-recording-cut-runtime-wiring',
        'backend-agent',
    ]
    for target in targets:
        print('=== ' + target + ' ===', flush=True)
        subprocess.run(['make', target], cwd=ROOT, check=True)
    git('diff', '--check')
    print('FOCUSED_TESTS=PASS')
    print('BACKEND_AGENT_BUILD=PASS')


def publish(final):
    verify_worktree(final, final, False)
    git('fetch', 'origin', BRANCH, capture=False)
    head = git('rev-parse', 'HEAD')
    require(git('rev-parse', 'FETCH_HEAD') == head,
            'remote changed; no commit or push')
    git('add', '--', *sorted(final))
    git('diff', '--cached', '--check')
    require(set(git('diff', '--cached', '--name-only').splitlines()) == set(final),
            'unexpected staged scope')
    git('commit', '-m', COMMIT_MESSAGE, capture=False)
    new_head = git('rev-parse', 'HEAD')
    print('SOURCE_COMMIT=' + new_head, flush=True)
    git('push', 'origin', 'HEAD:refs/heads/' + BRANCH, capture=False)
    require(git('rev-parse', 'HEAD') == new_head, 'local HEAD changed unexpectedly')
    require(not git('status', '--porcelain', '--untracked-files=no'),
            'tracked worktree not clean after commit')
    print('SOURCE_FIX=COMMITTED_AND_PUSHED')
    print('RUNTIME_INSTALL=not_executed')
    print('NCUT_EXEC=not_executed')
    print('SERVICE_RESTART=not_executed')
    print('SLICE3_REAL_CUT_48HRS=NOT_YET_PASS')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--apply', action='store_true')
    parser.add_argument('--finish', action='store_true')
    args = parser.parse_args()
    verify_checkout()
    expected, final = expected_states()
    if args.apply or args.finish:
        apply_correction(expected, final)
    else:
        verify_worktree(expected, final, True)
        print('REGRESSION_CORRECTION=CHECK_PASS')
    if args.finish:
        checks()
        verify_worktree(expected, final, False)
        publish(final)


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, OSError, subprocess.CalledProcessError, ValueError) as exc:
        print('REPAIR_ERROR=' + str(exc), file=sys.stderr)
        sys.exit(1)
