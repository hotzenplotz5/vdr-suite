#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

runtime_paths = {
    'shared': ROOT / 'web/frontend/recordings2-shared.js',
    'browser_view': ROOT / 'web/frontend/recordings2-browser-view.js',
    'person_view': ROOT / 'web/frontend/recordings2-person-search-view.js',
    'metadata_view': ROOT / 'web/frontend/recordings2-metadata-view.js',
    'metadata_detail': ROOT / 'web/frontend/recordings2-metadata-detail.js',
    'runtime': ROOT / 'web/frontend/recordings2.js',
}
runtimes = {
    name: path.read_text(encoding='utf-8')
    for name, path in runtime_paths.items()
}
loader = (ROOT / 'web/frontend/platform/deferred-runtime-loader.js').read_text(encoding='utf-8')
router = (ROOT / 'api/rest/src/ApiRouter.cpp').read_text(encoding='utf-8')
daemon_sources = sorted(
    (ROOT / 'core/daemon/src').glob('DaemonRuntime*.cpp')
)
daemon = '\n'.join(
    path.read_text(encoding='utf-8')
    for path in daemon_sources
)
server_sources = [ROOT / 'core/http/src/TestHttpServer.cpp'] + sorted(
    (ROOT / 'core/http/src').glob('TestHttpServer*.inc')
)
server = '\n'.join(path.read_text(encoding='utf-8') for path in server_sources)
makefile = (ROOT / 'Makefile').read_text(encoding='utf-8')
module_makefile = (ROOT / 'mk/recordings2.mk').read_text(encoding='utf-8')

required_tokens = {
    'shared': (
        'global.VdrSuiteRecordings2Shared',
        'recordingTitle',
        'recordingPosterUrl',
        'normalizePath',
        'installStyles',
    ),
    'browser_view': (
        'global.VdrSuiteRecordings2BrowserView',
        'renderFolder',
        'renderDetail',
        'VdrSuiteRecordings2MetadataDetail',
        'metadataDetail.enhance',
    ),
    'person_view': (
        'global.VdrSuiteRecordings2PersonSearchView',
        'fetchClientRecordingPersons',
        'Gefundene Aufnahmen:',
        'roleLabel',
    ),
    'metadata_view': (
        'global.VdrSuiteRecordings2MetadataView',
        "'Scraper'",
        "'Schauspieler'",
        "'Bilder'",
        'isPublicMetadataImageUrl',
        'personView.renderCast',
    ),
    'metadata_detail': (
        'global.VdrSuiteRecordings2MetadataDetail',
        "'/api/vdr/recordings/metadata'",
        'metadataView.mount',
        'refreshDetailAddon',
    ),
    'runtime': (
        'global.VdrSuiteRecordings2 = moduleApi;',
        'fetchClientRecordingFolder',
        'recordingFolder !== true',
        'data-module="recordings2"',
        'browserView.create',
        'refreshDetailAddon',
    ),
}
for owner, tokens in required_tokens.items():
    for token in tokens:
        if token not in runtimes[owner]:
            raise SystemExit(f'missing Recordings 2 {owner} contract: {token}')

line_limits = {
    'runtime': 300,
    'metadata_detail': 120,
    'shared': 320,
    'browser_view': 400,
    'person_view': 240,
    'metadata_view': 340,
}
for owner, maximum in line_limits.items():
    count = len(runtimes[owner].splitlines())
    if count > maximum:
        raise SystemExit(
            f'Recordings 2 {owner} runtime is no longer modular: {count} lines > {maximum}'
        )

for forbidden in (
    'VdrSuiteRecordingBrowser',
    'configureRecordingBrowser',
    'renderRecordingsThroughModule',
):
    for owner, source in runtimes.items():
        if forbidden in source:
            raise SystemExit(f'Recordings 2 {owner} depends on legacy recording browser: {forbidden}')

required_backend_tokens = (
    'path == "/api/vdr/recordings/metadata"',
    'path == "/api/vdr/recordings/metadata/image"',
    'getMetadataImage(',
)
for token in required_backend_tokens:
    if token not in router:
        raise SystemExit(f'missing recording metadata route contract: {token}')

for token in ('findByBackendNativeId(', 'recordingMetadataRepository'):
    if token not in daemon:
        raise SystemExit(f'missing recording metadata daemon wiring: {token}')

runtime_assets = (
    ('recordings2-shared.js', 'VdrSuiteRecordings2Shared'),
    ('recordings2-browser-view.js', 'VdrSuiteRecordings2BrowserView'),
    ('recordings2-person-search-view.js', 'VdrSuiteRecordings2PersonSearchView'),
    ('recordings2-metadata-view.js', 'VdrSuiteRecordings2MetadataView'),
    ('recordings2-metadata-detail.js', 'VdrSuiteRecordings2MetadataDetail'),
    ('recordings2.js', 'VdrSuiteRecordings2'),
)
for filename, global_name in runtime_assets:
    path_token = f"'/frontend/{filename}'"
    if path_token not in loader:
        raise SystemExit(f'missing deferred runtime path: {path_token}')
    if f'window.{global_name}' not in loader:
        raise SystemExit(f'missing deferred runtime readiness check: {global_name}')
    if f'"/frontend/{filename}"' not in server:
        raise SystemExit(f'HTTP server does not allow /frontend/{filename}')
    if f'"{filename}"' not in server:
        raise SystemExit(f'HTTP server does not serve {filename}')
    if f'web/frontend/{filename}' not in module_makefile:
        raise SystemExit(f'Recordings 2 install rule is missing {filename}')

core_runtime = loader.index('const recordings2Runtime =')
metadata_runtime = loader.index('const metadataDetailRuntime =')
if core_runtime > metadata_runtime:
    raise SystemExit('Recordings 2 core runtime must start independently before optional metadata detail')

metadata_catch = loader.find('.catch(error => {', metadata_runtime)
promise_join = loader.find('return Promise.all([', metadata_runtime)
if metadata_catch < 0 or promise_join < 0 or metadata_catch > promise_join:
    raise SystemExit('optional Recordings 2 metadata runtime failure is not isolated')

core_slice = loader[core_runtime:metadata_runtime]
for forbidden_dependency in (
    'recordings2-person-search-view',
    'recordings2-metadata-view',
    'recordings2-metadata-detail',
):
    if forbidden_dependency in core_slice:
        raise SystemExit(
            f'Recordings 2 core runtime depends on optional metadata addon: {forbidden_dependency}'
        )

if 'include mk/recordings2.mk' not in makefile:
    raise SystemExit('root Makefile does not include mk/recordings2.mk')

print('recordings2 modular runtime wiring ok')
