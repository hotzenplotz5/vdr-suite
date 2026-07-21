#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

recordings2 = (ROOT / 'web/frontend/recordings2.js').read_text(encoding='utf-8')
loader = (ROOT / 'web/frontend/platform/deferred-runtime-loader.js').read_text(encoding='utf-8')
server = (ROOT / 'core/http/src/TestHttpServer.cpp').read_text(encoding='utf-8')
makefile = (ROOT / 'Makefile').read_text(encoding='utf-8')
module_makefile = (ROOT / 'mk/recordings2.mk').read_text(encoding='utf-8')

required_module_tokens = (
    'global.VdrSuiteRecordings2 = moduleApi;',
    'fetchClientRecordingFolder',
    'recordingFolder !== true',
    'data-module="recordings2"',
    'Aufnahmeordner konnte nicht geladen werden',
    'recordingPosterUrl',
    'renderDetail',
)
for token in required_module_tokens:
    if token not in recordings2:
        raise SystemExit(f'missing Recordings 2 module contract: {token}')

for forbidden in ('VdrSuiteRecordingBrowser', 'configureRecordingBrowser', 'renderRecordingsThroughModule'):
    if forbidden in recordings2:
        raise SystemExit(f'Recordings 2 depends on legacy recording browser: {forbidden}')

required_loader_tokens = (
    "'/frontend/recordings2.js'",
    'window.VdrSuiteRecordings2',
    "'vdr-suite-recordings2-runtime'",
)
for token in required_loader_tokens:
    if token not in loader:
        raise SystemExit(f'missing deferred runtime wiring: {token}')

if 'path == "/frontend/recordings2.js"' not in server:
    raise SystemExit('HTTP server does not allow /frontend/recordings2.js')
if '"recordings2.js"' not in server:
    raise SystemExit('HTTP server does not serve recordings2.js')
if 'include mk/recordings2.mk' not in makefile:
    raise SystemExit('root Makefile does not include mk/recordings2.mk')
if 'web/frontend/recordings2.js' not in module_makefile:
    raise SystemExit('Recordings 2 install rule is missing')

print('recordings2 runtime wiring ok')
