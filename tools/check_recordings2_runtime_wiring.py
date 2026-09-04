#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

runtime_paths = {
    'shared': ROOT / 'web/frontend/recordings2-shared.js',
    'folder_artwork': ROOT / 'web/frontend/recordings2-folder-artwork.js',
    'actions': ROOT / 'web/frontend/recordings2-actions.js',
    'browser_view': ROOT / 'web/frontend/recordings2-browser-view.js',
    'marks_detail': ROOT / 'web/frontend/recordings2-marks-detail.js',
    'marks_timeline': ROOT / 'web/frontend/recordings2-marks-timeline.js',
    'person_view': ROOT / 'web/frontend/recordings2-person-search-view.js',
    'metadata_view': ROOT / 'web/frontend/recordings2-metadata-view.js',
    'metadata_detail': ROOT / 'web/frontend/recordings2-metadata-detail.js',
    'metadata_assignment': ROOT / 'web/frontend/recordings2-metadata-assignment.js',
    'runtime': ROOT / 'web/frontend/recordings2.js',
}
runtimes = {
    name: path.read_text(encoding='utf-8')
    for name, path in runtime_paths.items()
}
loader = (ROOT / 'web/frontend/platform/deferred-runtime-loader.js').read_text(encoding='utf-8')
client_api = (ROOT / 'web/frontend/api/client-api.js').read_text(encoding='utf-8')
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
install_makefile = (ROOT / 'mk/install.mk').read_text(encoding='utf-8')

required_tokens = {
    'shared': (
        'global.VdrSuiteRecordings2Shared',
        'recordingNativeTitle',
        'recordingMetadataTitle',
        'recordingTitle',
        'recordingPosterUrl',
        'normalizePath',
        'installStyles',
    ),
    'folder_artwork': (
        'global.VdrSuiteRecordings2FolderArtwork',
        'forFolderName',
        'isSingleRecordingLeaf',
        'resolveLeaves',
        'LEAF_CONCURRENCY',
        "action: {slug: 'action'}",
        'recording-genre-',
        'recording-genre-sprite.svg',
    ),
    'actions': (
        'global.VdrSuiteRecordings2Actions',
        'fetchClientRecordingActionValidation',
        'fetchClientRecordingActionExecution',
        'fetchClientRecordingFolder',
        "action: String(action || '').toUpperCase()",
        'findMatchingRecording',
        'requestBrowsableFolder',
        'isDryRunReady',
        'READBACK_ATTEMPTS',
    ),
    'browser_view': (
        'global.VdrSuiteRecordings2BrowserView',
        'renderFolder',
        'renderDetail',
        'VdrSuiteRecordings2FolderArtwork',
        'VdrSuiteRecordings2Actions',
        'folderArtwork.create',
        'actionView.createPanel',
        'VdrSuiteRecordings2MetadataDetail',
        'metadataDetail.enhance',
        'root.__vdrSuiteRecordingPlaybackOwner = activePlayback',
    ),
    'marks_detail': (
        'global.VdrSuiteRecordings2MarksDetail',
        "'/api/vdr/recordings/marks'",
        'recordingId: id',
        'Schnitt / Schnittmarken',
        'marksRevision',
        'VdrSuiteRecordings2BrowserView',
        'view.renderDetail()',
        "'/frontend/recordings2-marks-timeline.js'",
        'timeline.bind(root, recording, payload)',
    ),
    'marks_timeline': (
        'global.VdrSuiteRecordings2MarksTimeline',
        'input[aria-label="Wiedergabeposition"]',
        '__vdrSuiteRecordingPlaybackOwner',
        'owner.subscribe',
        'nativeMarksRevision',
        'transport',
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
        'assignmentRuntimePath',
        'loadAssignmentRuntime',
        'VdrSuiteRecordings2MetadataAssignment',
        'refreshDetailAddon',
    ),
    'metadata_assignment': (
        'global.VdrSuiteRecordings2MetadataAssignment',
        "'/recordings/metadata/'",
        'csrfHeaders',
        'Metadaten suchen',
        'Manuelle Zuordnung entfernen',
    ),
    'runtime': (
        'global.VdrSuiteRecordings2 = moduleApi;',
        'fetchClientRecordingFolder',
        'recordingFolder !== true',
        'serverRecordings',
        'promotedRecordings',
        'resolveSingleRecordingLeaves',
        'data-module="recordings2"',
        'browserView.create',
        'refreshDetailAddon',
    ),
}
for owner, tokens in required_tokens.items():
    for token in tokens:
        if token not in runtimes[owner]:
            raise SystemExit(f'missing Recordings 2 {owner} contract: {token}')

for external_scheme in ('http://', 'https://'):
    if external_scheme in runtimes['folder_artwork']:
        raise SystemExit(
            'Recordings 2 folder artwork must use only locally served suite assets'
        )

for token in (
    'web/frontend/assets/recording-genre-action.svg',
    'channel-logos/vdr-suite-brand/recording-genre-action.svg',
):
    if token not in install_makefile:
        raise SystemExit(
            f'Recordings 2 Action genre artwork installation contract missing: {token}'
        )

compact_folder_style_tokens = (
    'repeat(auto-fit,minmax(18rem,1fr))',
    'grid-template-columns:5.2rem minmax(0,1fr) auto;min-height:0',
    '.recordings2-folder-artwork{width:5.2rem}',
)
for token in compact_folder_style_tokens:
    if token not in runtimes['folder_artwork']:
        raise SystemExit(
            f'Recordings 2 compact desktop folder style missing: {token}'
        )
for forbidden in ('minmax(26rem,1fr)', 'width:8.9rem', 'min-height:13.7rem'):
    if forbidden in runtimes['folder_artwork']:
        raise SystemExit(
            f'Recordings 2 oversized desktop folder style returned: {forbidden}'
        )

line_limits = {
    'runtime': 330,
    'metadata_detail': 140,
    'metadata_assignment': 360,
    'shared': 320,
    'folder_artwork': 220,
    'actions': 620,
    'browser_view': 400,
    'marks_detail': 260,
    'marks_timeline': 180,
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

for owner in (
    'folder_artwork',
    'actions',
    'browser_view',
    'marks_detail',
    'marks_timeline',
    'metadata_assignment',
    'runtime',
):
    if 'fetch(' in runtimes[owner]:
        raise SystemExit(
            f'Recordings 2 {owner} must use the Client API and not call fetch() directly'
        )

marks_source = runtimes['marks_detail']
for forbidden in (
    'backendNativeId',
    'recordingPath',
    "method: 'POST'",
    'fetchClientRecordingActionExecution',
    'fetchClientRecordingActionValidation',
):
    if forbidden in marks_source:
        raise SystemExit(
            f'Recordings 2 marks read addon violates read-only public identity boundary: {forbidden}'
        )
for required in (
    "query: {\n        backend: backend,\n        recordingId: id\n      }",
    "cache: 'no-store'",
    "credentials: 'same-origin'",
):
    if required not in marks_source:
        raise SystemExit(f'Recordings 2 marks read request contract missing: {required}')

required_backend_tokens = (
    'path == "/api/vdr/recordings/metadata"',
    'path == "/api/vdr/recordings/metadata/image"',
    'path == "/api/vdr/recordings/actions/validate"',
    'path == "/api/vdr/recordings/actions/execute"',
    'getMetadataImage(',
)
for token in required_backend_tokens:
    if token not in router:
        raise SystemExit(f'missing recording route contract: {token}')

for token in (
    'fetchClientRecordingFolder',
    'fetchClientRecordingActionValidation',
    'fetchClientRecordingActionExecution',
    'requestJson: requestJson',
):
    if token not in client_api:
        raise SystemExit(f'missing Web Client API recording contract: {token}')

recording_action_start = client_api.index(
    'function fetchClientRecordingActionValidation(options)'
)
recording_action_end = client_api.index(
    'function fetchClientSearchTimers(options)',
    recording_action_start,
)
recording_action_client = client_api[recording_action_start:recording_action_end]
if 'requestJsonWithFallback' in recording_action_client:
    raise SystemExit(
        'Recording action mutations must not use speculative route fallback'
    )
for canonical_path in (
    "'/api/vdr/recordings/actions/validate'",
    "'/api/vdr/recordings/actions/execute'",
):
    if canonical_path not in recording_action_client:
        raise SystemExit(
            f'missing canonical Recording action client route: {canonical_path}'
        )

for token in ('findByBackendNativeId(', 'recordingMetadataRepository'):
    if token not in daemon:
        raise SystemExit(f'missing recording metadata daemon wiring: {token}')

runtime_assets = (
    ('recordings2-shared.js', 'VdrSuiteRecordings2Shared'),
    ('recordings2-folder-artwork.js', 'VdrSuiteRecordings2FolderArtwork'),
    ('recordings2-actions.js', 'VdrSuiteRecordings2Actions'),
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

marks_asset = 'recordings2-marks-detail.js'
if f'"/frontend/{marks_asset}"' not in server:
    raise SystemExit(f'HTTP server does not allow /frontend/{marks_asset}')
if f'"{marks_asset}"' not in server:
    raise SystemExit(f'HTTP server does not serve {marks_asset}')
if '"recordings2-browser-view.js", "application/javascript; charset=utf-8", "recordings2-marks-detail.js"' not in server:
    raise SystemExit('Recordings 2 marks addon is not bundled with the browser view owner')
if f'web/frontend/{marks_asset}' not in module_makefile:
    raise SystemExit(f'Recordings 2 install rule is missing {marks_asset}')
if 'test_recordings2_marks_detail.js' not in module_makefile:
    raise SystemExit('Recordings 2 marks detail contract test is not wired')

marks_timeline_asset = 'recordings2-marks-timeline.js'
if f'"/frontend/{marks_timeline_asset}"' not in server:
    raise SystemExit(f'HTTP server does not allow /frontend/{marks_timeline_asset}')
if f'"{marks_timeline_asset}"' not in server:
    raise SystemExit(f'HTTP server does not serve {marks_timeline_asset}')
if f'web/frontend/{marks_timeline_asset}' not in module_makefile:
    raise SystemExit(f'Recordings 2 install rule is missing {marks_timeline_asset}')
if f"'/frontend/{marks_timeline_asset}'" not in marks_source:
    raise SystemExit('Recordings 2 marks detail does not load its timeline lifecycle runtime')

timeline_bundle = module_makefile.find(
    '\t\tweb/frontend/recordings2-marks-timeline.js \\\n'
)
detail_bundle = module_makefile.find(
    '\t\tweb/frontend/recordings2-marks-detail.js \\\n'
)
if timeline_bundle < 0 or detail_bundle < 0 or timeline_bundle >= detail_bundle:
    raise SystemExit(
        'Recordings 2 production bundle must load marks timeline before marks detail'
    )
if 'global.VdrSuiteRecordings2MarksTimeline = Object.freeze' not in module_makefile:
    raise SystemExit('Recordings 2 install staging does not assert the marks timeline runtime')

assignment_asset = 'recordings2-metadata-assignment.js'
assignment_path = f"'/frontend/{assignment_asset}'"
if assignment_path not in runtimes['metadata_detail']:
    raise SystemExit(f'missing dynamic assignment runtime path: {assignment_path}')
if f'"/frontend/{assignment_asset}"' not in server:
    raise SystemExit(
        f'HTTP server does not allow dynamic assignment runtime /frontend/{assignment_asset}'
    )
if f'"{assignment_asset}"' not in server:
    raise SystemExit(f'HTTP server does not serve dynamic assignment runtime {assignment_asset}')
if f'web/frontend/{assignment_asset}' not in module_makefile:
    raise SystemExit(f'Recordings 2 install rule is missing {assignment_asset}')

folder_artwork_runtime = loader.index('const folderArtworkRuntime =')
actions_runtime = loader.index('const actionsRuntime =')
browser_runtime = loader.index('const browserViewRuntime =')
core_runtime = loader.index('const recordings2Runtime =')
metadata_runtime = loader.index('const metadataDetailRuntime =')
if not (
    folder_artwork_runtime < browser_runtime < core_runtime < metadata_runtime
    and actions_runtime < browser_runtime
):
    raise SystemExit(
        'Recordings 2 folder artwork and actions must load before the browser view and core runtime'
    )

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
