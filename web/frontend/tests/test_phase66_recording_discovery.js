'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const source = fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery.js'), 'utf8');

const transitions = [];
const openedRecordings = [];
const openedFolders = [];
const openedGenres = [];
const previewReasons = [];
const scheduled = [];

const context = {
  window: {},
  console
};
context.window.window = context.window;
context.window.document = null;
context.window.setTimeout = function (callback) {
  scheduled.push(callback);
  return scheduled.length;
};
context.window.selectModule = function (moduleName) {
  transitions.push(moduleName);
  return true;
};
context.window.VdrSuiteHomeLivePreview = {
  cancel(reason) {
    previewReasons.push(reason);
  }
};
context.window.VdrSuiteRecordings2 = {
  openRecording(recording, options) {
    openedRecordings.push({recording, options});
  },
  openFolder(folderPath) {
    openedFolders.push(folderPath);
  }
};
context.window.VdrSuiteGenres = {
  openRecordingGenre(entry, options) {
    openedGenres.push({entry, options});
    return true;
  }
};
context.window.VdrSuitePlatform = {
  getSelectedBackendId() { return 'default'; },
  getSelectedModule() { return 'overview'; },
  getClientApi() { return null; }
};

vm.createContext(context);
vm.runInContext(source, context);

const api = context.window.VdrSuiteHomeRecordingDiscovery;
assert(api && api._test);

const sameBackend = {
  recordingId: 'recording-1',
  backendId: 'default',
  title: 'Film Eins'
};
const inheritedBackend = {
  id: 'recording-2',
  title: 'Film Zwei'
};
const foreignBackend = {
  recordingId: 'recording-3',
  backendId: 'remote-b',
  title: 'Fremde Aufnahme'
};
const missingIdentity = {
  backendId: 'default',
  title: 'Ohne ID'
};

const canonical = api._test.canonicalRecordings({
  recordings: [sameBackend, inheritedBackend, foreignBackend, missingIdentity]
}, 'default');
assert.strictEqual(canonical.length, 2);
assert.strictEqual(canonical[0], sameBackend);
assert.strictEqual(canonical[1], inheritedBackend);

const genres = api._test.canonicalGenres({genres: [
  {id: 'movie', count: 3},
  {id: 'series', count: 1},
  {id: 'empty', count: 0},
  {id: '', count: 4}
]});
assert.strictEqual(genres.length, 2);
assert.strictEqual(genres[0].id, 'movie');
assert.strictEqual(genres[1].id, 'series');

const folders = api._test.canonicalFolders({folders: [
  {name: 'Filme', path: 'Filme'},
  {name: 'Serien', path: 'Serien'},
  {name: '', path: ''}
]});
assert.strictEqual(folders.length, 2);
assert.strictEqual(folders[0].path, 'Filme');
assert.strictEqual(folders[1].path, 'Serien');

const seriesRecording = {
  recordingId: 'series-1',
  backendId: 'default',
  backendNativeId: '/srv/vdr/video/Serien/The_Walking_Dead/S10E15.rec',
  path: 'Serien/The Walking Dead/S10E15 Prinzessin',
  metadata: {
    provider: {
      contentKind: 'series-episode',
      seriesId: 'series:42',
      seriesTitle: 'The Walking Dead',
      episodeTitle: 'Prinzessin',
      seasonNumber: 10,
      episodeNumber: 15
    },
    presentation: {
      posterUrl: '/api/vdr/recordings/metadata/image?kind=preferred'
    }
  }
};
assert.strictEqual(api._test.recordingPosterUrl(seriesRecording), '/api/vdr/recordings/metadata/image?kind=preferred');
assert.strictEqual(api._test.recordingPosterUrl({
  metadata: {artwork: {preferredUrl: '/cached/poster.jpg'}}
}), '/cached/poster.jpg');
assert.strictEqual(api._test.canonicalSeriesPath(seriesRecording), 'Serien/The Walking Dead');

const projectedFromRichMetadata = api._test.seriesMemberProjection({
  recordingId: 'rich-1',
  backendId: 'default',
  backendNativeId: 'native-rich-1',
  path: 'Serien/The Walking Dead/S10E14 Abschiede',
  metadata: {provider: {}}
}, {
  available: true,
  mediaType: 'episode',
  provider: 'tvscraper',
  providerId: 1402,
  title: 'The Walking Dead',
  episodeName: 'Abschiede',
  seasonNumber: 10,
  episodeNumber: 14,
  preferredArtwork: {
    available: true,
    url: '/api/vdr/recordings/metadata/image?backend=default&backendNativeId=native-rich-1&kind=preferred&index=0'
  }
}, 'default');
assert(projectedFromRichMetadata);
assert.strictEqual(projectedFromRichMetadata.seriesTitle, 'The Walking Dead');
assert.strictEqual(projectedFromRichMetadata.seasonNumber, 10);
assert.strictEqual(projectedFromRichMetadata.episodeNumber, 14);
assert.strictEqual(projectedFromRichMetadata.episodeTitle, 'Abschiede');
assert(projectedFromRichMetadata.posterUrl.includes('kind=preferred'));

const projectedFromCanonicalPath = api._test.seriesMemberProjection({
  recordingId: 'path-1',
  backendId: 'default',
  backendNativeId: 'native-path-1',
  path: 'Serien/The Walking Dead/S10E15 Prinzessin',
  metadata: {provider: {}}
}, null, 'default');
assert(projectedFromCanonicalPath);
assert.strictEqual(projectedFromCanonicalPath.seriesTitle, 'The Walking Dead');
assert.strictEqual(projectedFromCanonicalPath.seasonNumber, 10);
assert.strictEqual(projectedFromCanonicalPath.episodeNumber, 15);
assert.strictEqual(projectedFromCanonicalPath.episodeTitle, 'Prinzessin');

const grouped = api._test.buildSeriesProjection([
  projectedFromRichMetadata,
  projectedFromCanonicalPath,
  api._test.seriesMemberProjection({
    recordingId: 'other-1',
    backendId: 'default',
    path: 'Serien/Andor/S01E03 Abrechnung',
    metadata: {provider: {}}
  }, null, 'default')
]);
assert.strictEqual(grouped.length, 2);
const walkingDead = grouped.find((entry) => entry.title === 'The Walking Dead');
assert(walkingDead);
assert.strictEqual(walkingDead.episodes.length, 2);
assert.strictEqual(walkingDead.seasons.length, 1);
assert.strictEqual(walkingDead.seasons[0].number, 10);
assert.strictEqual(walkingDead.seasons[0].episodes[0].episodeNumber, 14);
assert.strictEqual(walkingDead.seasons[0].episodes[1].episodeNumber, 15);

class FakeElement {
  constructor(tagName) {
    this.tagName = String(tagName || '').toUpperCase();
    this.children = [];
    this.attributes = {};
    this.dataset = {};
    this.listeners = {};
    this.parentNode = null;
    this.className = '';
    this.textContent = '';
    this.src = '';
    this.alt = '';
    this.loading = '';
    this.type = '';
  }

  setAttribute(name, value) {
    this.attributes[name] = String(value);
  }

  appendChild(child) {
    if (!child) return child;
    child.parentNode = this;
    this.children.push(child);
    return child;
  }

  append() {
    Array.from(arguments).forEach((child) => this.appendChild(child));
  }

  replaceChildren() {
    this.children.forEach((child) => { child.parentNode = null; });
    this.children = [];
    Array.from(arguments).forEach((child) => this.appendChild(child));
  }

  addEventListener(type, handler) {
    if (!this.listeners[type]) this.listeners[type] = [];
    this.listeners[type].push(handler);
  }

  remove() {
    if (!this.parentNode) return;
    const index = this.parentNode.children.indexOf(this);
    if (index >= 0) this.parentNode.children.splice(index, 1);
    this.parentNode = null;
  }

  querySelector(selector) {
    const match = String(selector || '').match(/^\[data-home-discovery-rail="([^"]+)"\]$/);
    if (!match) return null;
    return findElement(this, (element) =>
      element.attributes['data-home-discovery-rail'] === match[1]);
  }

  closest() {
    return null;
  }
}

function findElement(root, predicate) {
  for (const child of root.children || []) {
    if (predicate(child)) return child;
    const nested = findElement(child, predicate);
    if (nested) return nested;
  }
  return null;
}

function findRail(host, key) {
  return host.querySelector('[data-home-discovery-rail="' + key + '"]');
}

function findRecordingCard(root, recordingId) {
  if (!root) return null;
  return findElement(root, (element) =>
    element.dataset && element.dataset.recordingId === recordingId);
}

function findSeriesCard(root, seriesKey) {
  if (!root) return null;
  return findElement(root, (element) =>
    element.dataset && element.dataset.seriesKey === seriesKey);
}

function findSeasonButton(root, seasonNumber) {
  if (!root) return null;
  return findElement(root, (element) =>
    element.dataset && element.dataset.seasonNumber === String(seasonNumber));
}

function findImage(root) {
  if (!root) return null;
  return findElement(root, (element) => element.tagName === 'IMG');
}

function createProductionHarness(options) {
  const config = options || {};
  const host = new FakeElement('div');
  const calls = {
    recordings: [],
    genres: [],
    genreRecordings: [],
    folders: [],
    metadata: []
  };
  const productionOpenedRecordings = [];
  const modules = [];
  let selectedModule = 'overview';

  const client = {
    fetchClientRecordings(request) {
      calls.recordings.push(request);
      return Promise.resolve(config.newly || {recordings: []});
    },
    fetchClientGenres(request) {
      calls.genres.push(request);
      return Promise.resolve(config.genres || {genres: []});
    },
    fetchClientGenreRecordings(request) {
      calls.genreRecordings.push(request);
      if (config.seriesError) return Promise.reject(new Error('series unavailable'));
      return Promise.resolve(config.series || {recordings: []});
    },
    fetchClientRecordingFolder(request) {
      calls.folders.push(request);
      return Promise.resolve(config.folders || {folders: []});
    },
    requestJson(route, request) {
      assert.strictEqual(route, '/api/vdr/recordings/metadata');
      calls.metadata.push(request);
      const nativeId = request.query.backendNativeId;
      if (config.metadataErrors && config.metadataErrors.includes(nativeId)) {
        return Promise.reject(new Error('metadata unavailable'));
      }
      const payload = config.metadata && config.metadata[nativeId];
      return Promise.resolve(payload || {available: false, status: 'not-found'});
    }
  };

  const document = {
    readyState: 'loading',
    head: null,
    querySelector(selector) {
      return selector === '[data-home-zone="additional-sections"]' ? host : null;
    },
    createElement(tagName) {
      return new FakeElement(tagName);
    },
    addEventListener() {},
    getElementById() { return null; }
  };

  const productionContext = {window: {}, console};
  productionContext.window.window = productionContext.window;
  productionContext.window.document = document;
  productionContext.window.setTimeout = function () { return 1; };
  productionContext.window.selectModule = function (moduleName) {
    modules.push(moduleName);
    selectedModule = moduleName;
    return true;
  };
  productionContext.window.VdrSuiteHomeLivePreview = {cancel() {}};
  productionContext.window.VdrSuiteRecordings2 = {
    openRecording(recording, openOptions) {
      productionOpenedRecordings.push({recording, options: openOptions});
    },
    openFolder() {}
  };
  productionContext.window.VdrSuitePlatform = {
    getSelectedBackendId() { return config.backendId || 'default'; },
    getSelectedModule() { return selectedModule; },
    getClientApi() { return client; }
  };

  vm.createContext(productionContext);
  vm.runInContext(source, productionContext);

  return {
    api: productionContext.window.VdrSuiteHomeRecordingDiscovery,
    host,
    calls,
    openedRecordings: productionOpenedRecordings,
    modules
  };
}

async function proveCanonicalSeriesHierarchyProductionPath() {
  const episode14 = {
    recordingId: 'twd-s10e14',
    backendId: 'default',
    backendNativeId: 'native-twd-14',
    path: 'Serien/The Walking Dead/S10E14 Abschiede',
    title: 'Serien/The Walking Dead/S10E14 Abschiede',
    metadata: {provider: {}}
  };
  const episode15 = {
    recordingId: 'twd-s10e15',
    backendId: 'default',
    backendNativeId: 'native-twd-15',
    path: 'Serien/The Walking Dead/S10E15 Prinzessin',
    title: 'Serien/The Walking Dead/S10E15 Prinzessin',
    metadata: {provider: {}}
  };
  const andor = {
    recordingId: 'andor-s01e03',
    backendId: 'default',
    backendNativeId: 'native-andor-3',
    path: 'Serien/Andor/S01E03 Abrechnung',
    title: 'Serien/Andor/S01E03 Abrechnung',
    metadata: {provider: {}}
  };
  const foreignSeries = {
    recordingId: 'series-foreign',
    backendId: 'remote-b',
    backendNativeId: 'native-foreign',
    path: 'Serien/Fremd/S01E01 Fremd',
    title: 'Fremde Serie',
    metadata: {provider: {}}
  };
  const heuristicOnly = {
    recordingId: 'heuristic-only',
    backendId: 'default',
    title: 'Serienähnlicher Film',
    metadata: {
      provider: {
        contentKind: 'series-episode',
        seriesId: 'not-authoritative',
        seriesTitle: 'Keine kanonische Serie'
      }
    }
  };

  const production = createProductionHarness({
    newly: {recordings: [heuristicOnly]},
    genres: {genres: [
      {id: 'movie', label: 'Film', count: 1},
      {id: 'series', label: 'Serien', count: 4}
    ]},
    series: {recordings: [episode14, episode15, andor, foreignSeries]},
    folders: {folders: [{name: 'Filme', path: 'Filme', count: 1}]},
    metadata: {
      'native-twd-14': {
        available: true,
        mediaType: 'episode',
        provider: 'tvscraper',
        providerId: 1402,
        title: 'The Walking Dead',
        episodeName: 'Abschiede',
        seasonNumber: 10,
        episodeNumber: 14,
        preferredArtwork: {available: true, url: '/poster/twd.jpg'}
      },
      'native-twd-15': {
        available: true,
        mediaType: 'episode',
        provider: 'tvscraper',
        providerId: 1402,
        title: 'The Walking Dead',
        episodeName: 'Prinzessin',
        seasonNumber: 10,
        episodeNumber: 15,
        preferredArtwork: {available: true, url: '/poster/twd.jpg'}
      },
      'native-andor-3': {
        available: true,
        mediaType: 'episode',
        provider: 'tvscraper',
        providerId: 83867,
        title: 'Andor',
        episodeName: 'Abrechnung',
        seasonNumber: 1,
        episodeNumber: 3,
        preferredArtwork: {available: true, url: '/poster/andor.jpg'}
      }
    }
  });

  assert.strictEqual(await production.api.refresh(), true);
  assert.strictEqual(production.calls.genreRecordings.length, 1);
  assert.strictEqual(production.calls.genreRecordings[0].backendId, 'default');
  assert.strictEqual(production.calls.genreRecordings[0].genreId, 'series');
  assert.strictEqual(production.calls.genreRecordings[0].limit, 240);
  assert.strictEqual(production.calls.metadata.length, 3);
  assert(production.calls.metadata.every((call) => call.query.backend === 'default'));

  const seriesRail = findRail(production.host, 'series');
  assert(seriesRail);
  const twdCard = findSeriesCard(seriesRail, 'folder:serien/the walking dead');
  const andorCard = findSeriesCard(seriesRail, 'folder:serien/andor');
  assert(twdCard);
  assert(andorCard);
  assert.strictEqual(findRecordingCard(seriesRail, episode14.recordingId), null);
  assert.strictEqual(findRecordingCard(seriesRail, episode15.recordingId), null);
  assert.strictEqual(findRecordingCard(seriesRail, foreignSeries.recordingId), null);
  assert.strictEqual(findRecordingCard(seriesRail, heuristicOnly.recordingId), null);
  const twdPoster = findImage(twdCard);
  assert(twdPoster);
  assert.strictEqual(twdPoster.src, '/poster/twd.jpg');

  twdCard.listeners.click[0]();
  const season10 = findSeasonButton(findRail(production.host, 'series'), 10);
  assert(season10);
  assert(String(season10.textContent).includes('Staffel 10'));
  assert.strictEqual(findRecordingCard(findRail(production.host, 'series'), episode14.recordingId), null);

  season10.listeners.click[0]();
  const episode14Card = findRecordingCard(findRail(production.host, 'series'), episode14.recordingId);
  const episode15Card = findRecordingCard(findRail(production.host, 'series'), episode15.recordingId);
  assert(episode14Card);
  assert(episode15Card);

  episode15Card.listeners.click[0]();
  await Promise.resolve();
  await Promise.resolve();
  assert.strictEqual(production.openedRecordings.length, 1);
  assert.strictEqual(production.openedRecordings[0].recording, episode15);
  assert.strictEqual(production.openedRecordings[0].options.backendId, 'default');
  assert.strictEqual(production.modules[0], 'recordings2');

  const failingMetadata = createProductionHarness({
    genres: {genres: [{id: 'series', label: 'Serien', count: 2}]},
    series: {recordings: [episode14, episode15]},
    metadataErrors: ['native-twd-14', 'native-twd-15']
  });
  assert.strictEqual(await failingMetadata.api.refresh(), true);
  const fallbackRail = findRail(failingMetadata.host, 'series');
  assert(fallbackRail);
  const fallbackTwd = findSeriesCard(fallbackRail, 'folder:serien/the walking dead');
  assert(fallbackTwd);
  fallbackTwd.listeners.click[0]();
  const fallbackSeason10 = findSeasonButton(findRail(failingMetadata.host, 'series'), 10);
  assert(fallbackSeason10);
  fallbackSeason10.listeners.click[0]();
  assert(findRecordingCard(findRail(failingMetadata.host, 'series'), episode14.recordingId));
  assert(findRecordingCard(findRail(failingMetadata.host, 'series'), episode15.recordingId));

  const failingSeries = createProductionHarness({
    newly: {recordings: [sameBackend]},
    genres: {genres: [{id: 'series', label: 'Serien', count: 1}]},
    seriesError: true,
    folders: {folders: [{name: 'Filme', path: 'Filme', count: 1}]}
  });
  assert.strictEqual(await failingSeries.api.refresh(), true);
  assert(findRail(failingSeries.host, 'newly-recorded'));
  assert(findRail(failingSeries.host, 'genres'));
  assert(findRail(failingSeries.host, 'folders'));
  assert(findElement(findRail(failingSeries.host, 'series'), (element) =>
    String(element.className).includes('error')));

  const noCanonicalSeries = createProductionHarness({
    newly: {recordings: [heuristicOnly]},
    genres: {genres: [{id: 'movie', label: 'Film', count: 1}]},
    series: {recordings: [episode14]},
    folders: {folders: [{name: 'Filme', path: 'Filme', count: 1}]}
  });
  assert.strictEqual(await noCanonicalSeries.api.refresh(), true);
  assert.strictEqual(noCanonicalSeries.calls.genreRecordings.length, 0);
  assert.strictEqual(noCanonicalSeries.calls.metadata.length, 0);
  assert.strictEqual(findRail(noCanonicalSeries.host, 'series'), null);

  const emptyCanonicalSeries = createProductionHarness({
    genres: {genres: [{id: 'series', label: 'Serien', count: 1}]},
    series: {recordings: []}
  });
  assert.strictEqual(await emptyCanonicalSeries.api.refresh(), true);
  assert.strictEqual(emptyCanonicalSeries.calls.genreRecordings.length, 1);
  assert.strictEqual(findRail(emptyCanonicalSeries.host, 'series'), null);
}

(async function () {
  assert.strictEqual(await api._test.openRecording(seriesRecording, 'default'), true);
  assert.strictEqual(openedRecordings.length, 1);
  assert.strictEqual(openedRecordings[0].recording, seriesRecording);
  assert.strictEqual(openedRecordings[0].options.backendId, 'default');
  assert.strictEqual(openedRecordings[0].options.backLabel, '← Zurück zu Home');
  assert.strictEqual(typeof openedRecordings[0].options.onClose, 'function');
  assert.strictEqual(transitions[0], 'recordings2');

  assert.strictEqual(await api._test.openFolder({name: 'Filme', path: 'Filme'}, 'default'), true);
  assert.strictEqual(openedFolders.length, 1);
  assert.strictEqual(openedFolders[0], 'Filme');
  assert.strictEqual(transitions[1], 'recordings2');

  const genre = {id: 'movie', label: 'Film', count: 3};
  assert.strictEqual(await api._test.openGenre(genre, 'default'), true);
  assert.strictEqual(openedGenres.length, 1);
  assert.strictEqual(openedGenres[0].entry, genre);
  assert.strictEqual(openedGenres[0].options.backendId, 'default');
  assert.strictEqual(transitions[2], 'genres');

  assert.strictEqual(previewReasons.length, 3);

  openedRecordings[0].options.onClose();
  assert.strictEqual(transitions[3], 'overview');
  assert.strictEqual(scheduled.length, 1);

  await proveCanonicalSeriesHierarchyProductionPath();

  console.log('phase66 recording discovery ownership and canonical series hierarchy coverage ok');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
