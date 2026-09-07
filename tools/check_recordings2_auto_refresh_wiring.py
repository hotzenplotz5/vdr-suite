#!/usr/bin/env python3
"""Protect the canonical Recordings 2 automatic refresh composition."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def source(path):
    return (ROOT / path).read_text(encoding='utf-8')

def require(condition, message):
    if not condition:
        raise SystemExit(message)

owner = source('web/frontend/recordings2.js')
refresh = source('web/frontend/recordings2-folder-refresh.js')
make = source('mk/recordings2.mk')
server = source('core/http/src/TestHttpServerPaths.inc')

require(len(owner.splitlines()) <= 340, 'Recordings 2 owner exceeds its modularity budget')
require(len(refresh.splitlines()) <= 240, 'Recording folder refresh module is too large')
for token in (
    'global.VdrSuiteRecordings2FolderRefresh = Object.freeze({create})',
    'const INTERVAL_MS = 30000;',
    'options.fetchClientRecordingFolder({',
    "cache: 'no-store'",
    "credentials: 'same-origin'",
    'requestSequence',
    "source.addEventListener('update'",
    'options.createClientLiveUpdateSource()',
    "data.changedDomains.includes('recordings')",
    'source.close()',
    'serverSignature',
    'resolveLeaves(data, current)',
    'state().selectedRecording',
    'global.document.hidden',
    'global.clearTimeout(timer)',
):
    require(token in refresh, f'missing recording refresh contract: {token}')
for forbidden in ('fetch(', 'XMLHttpRequest', 'EventSource', 'WebSocket',
                  'localStorage', 'sessionStorage', "method: 'POST'", 'NCUT EXEC'):
    require(forbidden not in refresh, f'refresh module violates read-only Client API boundary: {forbidden}')
for token in (
    'global.VdrSuiteRecordings2FolderRefresh',
    'fetchClientRecordingFolder: function (options)',
    'folderRefresh.signature(data)',
    'stopFolderRefresh();',
    'scheduleFolderRefresh(0);',
    'resolveSingleRecordingLeaves',
    'updatePresentedFolderState',
):
    require(token in owner, f'missing recording owner refresh wiring: {token}')

bundle = make.index('\tcat \\')
helper = make.index('\t\tweb/frontend/recordings2-folder-refresh.js \\', bundle)
view = make.index('\t\tweb/frontend/recordings2-browser-view.js \\', bundle)
require(helper < view, 'refresh dependency must load before the browser view bundle')
require('global.VdrSuiteRecordings2FolderRefresh = Object.freeze' in make,
        'install staging must verify the bundled refresh runtime')
require('node --check web/frontend/recordings2-folder-refresh.js' in make,
        'refresh module syntax check missing')
require('node web/frontend/tests/test_recordings2_auto_refresh.js' in make,
        'automatic refresh regression is not in the frontend test graph')
require('python3 tools/check_recordings2_auto_refresh_wiring.py' in make,
        'automatic refresh architecture guard is not in the frontend test graph')
require('"/frontend/recordings2-browser-view.js"' in server,
        'existing production browser-view bundle route is missing')
for path in (
    'web/frontend/tests/test_recordings2_runtime.js',
    'web/frontend/tests/test_recordings2_auto_refresh.js',
    'web/frontend/tests/test_recordings2_detail_addon_playback_persistence.js',
):
    test = source(path)
    require("'web/frontend/recordings2-folder-refresh.js'" in test,
            f'production refresh dependency is missing from {path}')
    require(test.index("'web/frontend/recordings2-folder-refresh.js'") <
            test.index("'web/frontend/recordings2.js'"),
            f'refresh dependency order is wrong in {path}')

print('recordings2 automatic refresh production wiring and modularity ok')
