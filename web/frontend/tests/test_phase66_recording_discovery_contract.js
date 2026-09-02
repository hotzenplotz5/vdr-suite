'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const frontendRoot = path.join(__dirname, '..');
const repoRoot = path.join(frontendRoot, '..', '..');

function read(relativePath) {
  return fs.readFileSync(path.join(repoRoot, relativePath), 'utf8');
}

const source = read('web/frontend/home-recording-discovery.js');
const bootstrap = read('web/frontend/home-recording-discovery-bootstrap.js');
const clientApi = read('web/frontend/api/client-api.js');
const genreClientApi = read('web/frontend/api/genre-client-api.js');
const genres = read('web/frontend/modules/genres.js');
const continueWatching = read('web/frontend/home-continue-watching.js');
const httpPaths = read('core/http/src/TestHttpServerPaths.inc');
const makefile = read('Makefile');
const sliceMake = read('mk/phase66-recording-discovery.mk');

// Newly Recorded consumes the existing bounded recording query and explicit backend scope.
assert(source.includes("fetchClientRecordings({"));
assert(source.includes("backend: backendId"));
assert(source.includes("sort: 'startTime'"));
assert(source.includes("order: 'desc'"));
assert(source.includes('limit: NEW_LIMIT'));
assert(clientApi.includes("requestJson('/api/vdr/recordings/query', options)"));

// Folder discovery consumes the existing recording hierarchy and canonical folder owner.
assert(source.includes('fetchClientRecordingFolder({'));
assert(source.includes("path: ''"));
assert(source.includes('limit: FOLDER_LIMIT'));
assert(source.includes("global.VdrSuiteRecordings2.openFolder(path)"));
assert(clientApi.includes("requestJson('/api/vdr/recordings/folder', backendQueryOptions(options))"));

// Genre discovery consumes the existing metadata genre truth and existing genre browser.
assert(source.includes("scope: 'recordings'"));
assert(source.includes('fetchClientGenreRecordings({'));
assert(source.includes("global.VdrSuiteGenres.openRecordingGenre(entry"));
assert(genreClientApi.includes("base.requestJson('/api/metadata/genres'"));
assert(genreClientApi.includes("base.requestJson('/api/metadata/genres/recordings'"));
assert(genres.includes('client.fetchClientGenreRecordings(options)'));
assert(genres.includes('openRecordingGenre: function (entry, options)'));

// Series membership is projection-only: the canonical `series` endpoint remains the sole membership authority.
assert(source.includes("text(entry.id).toLowerCase() === 'series'"));
assert(source.includes('canonicalRecordings(payload, backendId)'));
assert(source.includes('limit: SERIES_PAGE_LIMIT'));
assert(source.includes('offset: offset'));
assert(source.includes('page.hasMore') || source.includes('payload.hasMore'));
assert(!source.includes('filter(isSeriesRecording)'));
assert(!source.includes('function isSeriesRecording'));
assert(!source.includes("text(value.contentKind) === 'series-episode'"));
assert(!source.includes('homeSeriesId'));
assert(!source.includes('seriesCatalog'));
assert(!source.includes('SERIES_MEMBER_LIMIT'));
assert(!source.includes('SERIES_LIMIT'));

// Persisted Native Recording Metadata enriches canonical members only, via backendNativeId and bounded concurrency.
assert(source.includes("client.requestJson('/api/vdr/recordings/metadata'"));
assert(source.includes('backendNativeId: candidate.nativeId'));
assert(source.includes("cache: 'no-store'"));
assert(source.includes("credentials: 'same-origin'"));
assert(source.includes('const SERIES_METADATA_CONCURRENCY = 4'));
assert(source.includes('Math.min(SERIES_METADATA_CONCURRENCY, queue.length)'));
assert(source.includes('richProviderId !== 0'));
assert(source.includes('rich.get(nativeId) || null'));
assert(!source.includes('fetch("https://'));
assert(!source.includes('tmdb.org'));
assert(!source.includes('tvmaze'));

// Recording identity and navigation remain owned by Recordings 2; Home does not synthesize a second ID.
assert(source.includes("return text(recording && (recording.recordingId || recording.id))"));
assert(source.includes("global.VdrSuiteRecordings2.openRecording(recording"));
assert(source.includes("selectShellModule('recordings2')"));
assert(source.includes("backLabel: config.backLabel || '← Zurück zu Home'"));
assert(source.includes("onClose: typeof config.onClose === 'function' ? config.onClose : returnHome"));
assert(source.includes("backLabel: '← Zurück zur Staffel'"));
assert(source.includes('renderSeriesDetail(series, selectedSeason, backendId)'));
assert(!source.includes('homeRecordingId'));

// Existing metadata/artwork projection is reused with browser-native lazy image loading and fallback.
assert(source.includes('presentation(recording).posterUrl || artwork.preferredUrl'));
assert(source.includes('member.posterUrl || recordingPosterUrl(recording)'));
assert(source.includes("image.loading = 'lazy'"));
assert(!source.includes('resolveArtwork'));
assert(!source.includes('fetchArtwork'));

// Below-the-fold discovery is bounded, deferred, and each rail settles independently.
assert(source.includes('new global.IntersectionObserver'));
assert(source.includes("rootMargin: '320px 0px'"));
assert(source.includes('return Promise.allSettled(['));
assert(source.includes('loadNewly(client, backendId, generation)'));
assert(source.includes('loadGenres(client, backendId, generation)'));
assert(source.includes('loadFolders(client, backendId, generation)'));
assert(source.includes("'Neu aufgenommene Inhalte sind vorübergehend nicht verfügbar.'"));
assert(source.includes("'Genres sind vorübergehend nicht verfügbar.'"));
assert(source.includes("'Aufnahmeordner sind vorübergehend nicht verfügbar.'"));

// The discovery runtime is itself deferred from the established production loader.
assert(bootstrap.includes("loadVdrSuiteDeferredRuntime("));
assert(bootstrap.includes("'/frontend/home-recording-discovery.js'"));
assert(httpPaths.includes('{"/frontend/home-recording-discovery.js", "home-recording-discovery.js"'));
assert(httpPaths.includes('{"/frontend/platform/deferred-runtime-loader.js", "platform/deferred-runtime-loader.js", "application/javascript; charset=utf-8", "home-recording-discovery-bootstrap.js"}'));

// 66.4 stays on its existing composition path and is not repurposed as the 66.5 owner.
assert(httpPaths.includes('{"/frontend/home-live-hero.js", "home-live-hero.js", "application/javascript; charset=utf-8", "home-continue-watching.js"}'));
assert(!continueWatching.includes('VdrSuiteHomeRecordingDiscovery'));

// No new playback/history/recommendation owner is introduced by this follow-up.
assert(!source.includes('MediaSession'));
assert(!source.includes('navigator.mediaSession'));
assert(!source.includes('localStorage'));
assert(!source.includes('/history'));
assert(!source.includes('recommendation'));
assert(!source.includes('ranking'));

// The ordinary frontend and packaging gates include the actual production assets.
assert(makefile.includes('include mk/phase66-recording-discovery.mk'));
assert(sliceMake.includes('test-ci-frontend: test-phase66-recording-discovery-frontend'));
assert(sliceMake.includes('test-ci-packaging: test-phase66-recording-discovery-install-staging'));
assert(sliceMake.includes('web/frontend/home-recording-discovery-bootstrap.js'));
assert(sliceMake.includes('web/frontend/home-recording-discovery.js'));

console.log('phase66 recording discovery production composition contract ok');