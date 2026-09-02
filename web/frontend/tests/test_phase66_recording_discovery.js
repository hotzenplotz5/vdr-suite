'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const source = [
  fs.readFileSync(path.join(frontendRoot, 'platform', 'helpers.js'), 'utf8'),
  fs.readFileSync(path.join(frontendRoot, 'home-recording-discovery.js'), 'utf8')
].join('\n');

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
assert.strictEqual(api._test.recordingBackendNativeId(seriesRecording), '/srv/vdr/video/Serien/The_Walking_Dead/S10E15.rec');
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
  mediaType: 'series',
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

const stargateTimestampRecording = {
  recordingId: 'stargate-timestamp-1',
  backendId: 'default',
  backendNativeId: '/srv/vdr/video/Serien/Stargate_Universe/2016-01-05.10.52.3-0.rec',
  path: 'Serien/Stargate Universe/2016-01-05.10.52.3-0.rec',
  metadata: {
    provider: {},
    presentation: {posterUrl: '/fallback/stargate-universe.jpg'}
  }
};
const stargatePreferredArtwork =
  '/api/vdr/recordings/metadata/image?backend=default&backendNativeId=stargate-timestamp-1&kind=preferred';
const stargatePortraitArtwork =
  '/api/vdr/recordings/metadata/image?backend=default&backendNativeId=stargate-timestamp-1&kind=image&index=2';
const projectedStargateTimestamp = api._test.seriesMemberProjection(stargateTimestampRecording, {
  available: true,
  mediaType: 'series',
  provider: 'tvscraper',
  providerId: 4242,
  title: 'Stargate Universe',
  episodeName: 'Die Rückkehr',
  seasonNumber: 2,
  episodeNumber: 3,
  preferredArtwork: {available: true, url: stargatePreferredArtwork},
  images: [
    {
      orientation: 'landscape',
      image: {
        available: true,
        url: '/api/vdr/recordings/metadata/image?backend=default&backendNativeId=stargate-timestamp-1&kind=image&index=0'
      }
    },
    {
      orientation: 'portrait',
      image: {available: true, url: stargatePortraitArtwork}
    }
  ]
}, 'default');
assert.strictEqual(projectedStargateTimestamp.seriesTitle, 'Stargate Universe');
assert.strictEqual(projectedStargateTimestamp.seasonNumber, 2);
assert.strictEqual(projectedStargateTimestamp.episodeNumber, 3);
assert.strictEqual(projectedStargateTimestamp.episodeTitle, 'Die Rückkehr');
assert.strictEqual(projectedStargateTimestamp.posterUrl, stargatePortraitArtwork);
assert.notStrictEqual(projectedStargateTimestamp.posterUrl, stargatePreferredArtwork);

const stargateFallbackSeason1 = api._test.seriesMemberProjection({
  recordingId: 'stargate-fallback-s01e01',
  backendId: 'default',
  path: 'Serien/Stargate Universe/S01E01 Air',
  metadata: {
    provider: {},
    presentation: {posterUrl: '/fallback/stargate-season-1.jpg'}
  }
}, null, 'default');
const stargateFallbackSeason3 = api._test.seriesMemberProjection({
  recordingId: 'stargate-fallback-s03e01',
  backendId: 'default',
  path: 'Serien/Stargate Universe/S03E01 Zukunft',
  metadata: {
    provider: {},
    presentation: {posterUrl: '/fallback/stargate-season-3.jpg'}
  }
}, null, 'default');
assert.strictEqual(stargateFallbackSeason1.seasonNumber, 1);
assert.strictEqual(stargateFallbackSeason1.episodeNumber, 1);
const groupedStargate = api._test.buildSeriesProjection([
  stargateFallbackSeason1,
  projectedStargateTimestamp,
  stargateFallbackSeason3
]);
assert.strictEqual(groupedStargate.length, 1);
assert.strictEqual(groupedStargate[0].posterUrl, stargatePortraitArtwork);
assert.deepStrictEqual(
  Array.from(groupedStargate[0].seasons, (season) => season.number),
  [1, 2, 3]
);

const projectedFromNegativeProviderId = api._test.seriesMemberProjection({
  recordingId: 'negative-series-1',
  backendId: 'default',
  backendNativeId: 'native-negative-series-1',
  title: 'EPG Serienname',
  metadata: {provider: {
    seriesTitle: 'EPG Serienname',
    episodeTitle: 'EPG Episodentitel',
    seasonNumber: 9,
    episodeNumber: 99
  }}
}, {
  available: true,
  mediaType: 'series',
  provider: 'tvscraper',
  providerId: -74205,
  title: 'Band of Brothers',
  episodeName: 'Currahee',
  seasonNumber: 1,
  episodeNumber: 1,
  preferredArtwork: {available: true, url: '/metadata/band-of-brothers.jpg'}
}, 'default');
assert(projectedFromNegativeProviderId);
assert.strictEqual(projectedFromNegativeProviderId.seriesKey, 'native:tvscraper:-74205');
assert.strictEqual(projectedFromNegativeProviderId.seriesTitle, 'Band of Brothers');
assert.strictEqual(projectedFromNegativeProviderId.seasonNumber, 1);
assert.strictEqual(projectedFromNegativeProviderId.episodeNumber, 1);
assert.strictEqual(projectedFromNegativeProviderId.episodeTitle, 'Currahee');
assert.strictEqual(projectedFromNegativeProviderId.posterUrl, '/metadata/band-of-brothers.jpg');

const projectedFromZeroProviderId = api._test.seriesMemberProjection({
  recordingId: 'zero-series-1',
  backendId: 'default',
  title: 'Fallback Serienname',
  metadata: {provider: {}}
}, {
  available: true,
  mediaType: 'series',
  provider: 'tvscraper',
  providerId: 0,
  title: 'Rich ohne Identität'
}, 'default');
assert.strictEqual(projectedFromZeroProviderId.seriesKey, 'title:rich ohne identität');

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

const projectedFlatCanonicalMember = api._test.seriesMemberProjection({
  recordingId: 'flat-series-1',
  backendId: 'default',
  title: 'Flat Canonical Series Member',
  metadata: {provider: {}}
}, null, 'default');
assert(projectedFlatCanonicalMember);
assert.strictEqual(projectedFlatCanonicalMember.seriesTitle, 'Flat Canonical Series Member');

const grouped = api._test.buildSeriesProjection([
  projectedFromRichMetadata,
  projectedFromCanonicalPath,
  api._test.seriesMemberProjection({
    recordingId: 'twd-s02e03',
    backendId: 'default',
    path: 'Serien/The Walking Dead/S02E03 Alte Wunden',
    metadata: {provider: {}}
  }, null, 'default'),
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
assert.strictEqual(walkingDead.episodes.length, 3);
assert.deepStrictEqual(Array.from(walkingDead.seasons, (season) => season.number), [2, 10]);
assert.strictEqual(walkingDead.seasons[1].episodes[0].episodeNumber, 14);
assert.strictEqual(walkingDead.seasons[1].episodes[1].episodeNumber, 15);

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

function findElements(root, predicate, found) {
  const result = found || [];
  for (const child of root.children || []) {
    if (predicate(child)) result.push(child);
    findElements(child, predicate, result);
  }
  return result;
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

function findSeriesCards(root) {
  if (!root) return [];
  return findElements(root, (element) =>
    element.dataset && Boolean(element.dataset.seriesKey));
}

function findSeasonButtons(root) {
  if (!root) return [];
  return findElements(root, (element) =>
    element.dataset && element.dataset.seasonNumber !== undefined);
}

function findSeasonButton(root, seasonNumber) {
  return findSeasonButtons(root).find((element) =>
    element.dataset.seasonNumber === String(seasonNumber)) || null;
}

function findEpisodeCards(root) {
  if (!root) return [];
  return findElements(root, (element) =>
    element.dataset && element.dataset.episodeNumber !== undefined &&
    element.dataset.recordingId);
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
  let metadataInFlight = 0;
  let metadataMaxInFlight = 0;

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
      if (config.seriesError ||
          (config.seriesErrorOffset !== undefined && request.offset >= config.seriesErrorOffset)) {
        return Promise.reject(new Error('series unavailable'));
      }
      const all = config.seriesItems || [];
      const serverLimit = Math.min(Number(request.limit || 0) || 48, 100);
      const offset = Number(request.offset || 0);
      const items = all.slice(offset, offset + serverLimit);
      return Promise.resolve({
        backendId: config.backendId || 'default',
        genreId: 'series',
        total: all.length,
        limit: serverLimit,
        offset,
        hasMore: offset + items.length < all.length,
        items
      });
    },
    fetchClientRecordingFolder(request) {
      calls.folders.push(request);
      return Promise.resolve(config.folders || {folders: []});
    },
    requestJson(route, request) {
      calls.metadata.push({route, request});
      const nativeId = request && request.query && request.query.backendNativeId;
      metadataInFlight += 1;
      metadataMaxInFlight = Math.max(metadataMaxInFlight, metadataInFlight);
      const response = Array.isArray(config.metadataPending) && config.metadataPending.includes(nativeId)
        ? new Promise(function () {})
        : Promise.resolve().then(function () {
          if (Array.isArray(config.metadataErrors) && config.metadataErrors.includes(nativeId)) {
            throw new Error('metadata unavailable');
          }
          if (config.metadataByNativeId &&
              Object.prototype.hasOwnProperty.call(config.metadataByNativeId, nativeId)) {
            return config.metadataByNativeId[nativeId];
          }
          return {available: false};
        });
      return response.finally(function () {
        metadataInFlight -= 1;
      });
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
    modules,
    metadataMaxInFlight() { return metadataMaxInFlight; }
  };
}

function makeEpisode(seriesTitle, seasonNumber, episodeNumber, idPrefix) {
  const season = String(seasonNumber).padStart(2, '0');
  const episode = String(episodeNumber).padStart(2, '0');
  const id = (idPrefix || seriesTitle.toLowerCase().replace(/[^a-z0-9]+/g, '-')) +
    '-s' + season + 'e' + episode;
  return {
    recordingId: id,
    backendId: 'default',
    backendNativeId: 'native-' + id,
    path: 'Serien/' + seriesTitle + '/S' + season + 'E' + episode + ' Folge ' + episode,
    title: 'Serien/' + seriesTitle + '/S' + season + 'E' + episode + ' Folge ' + episode,
    metadata: {provider: {}, presentation: {posterUrl: ''}, artwork: {preferredUrl: ''}}
  };
}

function buildLargeCanonicalSeriesSet() {
  const items = [];
  for (let season = 1; season <= 10; season += 1) {
    for (let episode = 1; episode <= 15; episode += 1) {
      if (season === 9 && episode > 10) continue;
      items.push(makeEpisode('The Walking Dead', season, episode, 'twd'));
    }
  }
  assert.strictEqual(items.length, 145);
  for (let seriesIndex = 1; seriesIndex <= 13; seriesIndex += 1) {
    const title = 'Serie ' + String(seriesIndex).padStart(2, '0');
    for (let episode = 1; episode <= 9; episode += 1) {
      items.push(makeEpisode(title, 1, episode, 'series-' + seriesIndex));
    }
  }
  assert.strictEqual(items.length, 262);
  return items;
}

async function proveCanonicalSeriesHierarchyProductionPath() {
  const canonicalSeriesItems = buildLargeCanonicalSeriesSet();
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
      {id: 'series', label: 'Serien', count: canonicalSeriesItems.length}
    ]},
    seriesItems: canonicalSeriesItems,
    folders: {folders: [{name: 'Filme', path: 'Filme', count: 1}]}
  });

  assert.strictEqual(await production.api.refresh(), true);
  assert.strictEqual(production.calls.genreRecordings.length, 3);
  assert.deepStrictEqual(
    production.calls.genreRecordings.map((call) => call.offset),
    [0, 100, 200]
  );
  assert(production.calls.genreRecordings.every((call) => call.limit === 100));
  assert(production.calls.genreRecordings.every((call) => call.backendId === 'default'));
  assert(production.calls.genreRecordings.every((call) => call.genreId === 'series'));
  assert.strictEqual(production.calls.metadata.length, canonicalSeriesItems.length);
  assert(production.metadataMaxInFlight() <= 4);
  assert(production.metadataMaxInFlight() >= 2);
  assert(production.calls.metadata.every((call) => call.route === '/api/vdr/recordings/metadata'));
  assert(production.calls.metadata.every((call) => call.request.cache === 'no-store'));
  assert(production.calls.metadata.every((call) => call.request.credentials === 'same-origin'));
  assert(production.calls.metadata.every((call) => call.request.query.backend === 'default'));
  assert.strictEqual(new Set(
    production.calls.metadata.map((call) => call.request.query.backendNativeId)
  ).size, canonicalSeriesItems.length);

  const seriesRail = findRail(production.host, 'series');
  assert(seriesRail);
  assert.strictEqual(findSeriesCards(seriesRail).length, 14);
  const twdCard = findSeriesCard(seriesRail, 'folder:serien/the walking dead');
  const thirteenthSeries = findSeriesCard(seriesRail, 'folder:serien/serie 13');
  assert(twdCard);
  assert(thirteenthSeries);
  assert(findElement(twdCard, (element) => String(element.textContent).includes('145 Folgen')));
  assert.strictEqual(findImage(twdCard), null);
  assert.strictEqual(findRecordingCard(seriesRail, canonicalSeriesItems[0].recordingId), null);
  assert.strictEqual(findRecordingCard(seriesRail, heuristicOnly.recordingId), null);

  twdCard.listeners.click[0]();
  const seasonButtons = findSeasonButtons(findRail(production.host, 'series'));
  assert.deepStrictEqual(
    seasonButtons.map((button) => Number(button.dataset.seasonNumber)),
    [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
  );
  const season10 = findSeasonButton(findRail(production.host, 'series'), 10);
  assert(season10);
  season10.listeners.click[0]();
  const episodeCards = findEpisodeCards(findRail(production.host, 'series'));
  assert.strictEqual(episodeCards.length, 15);
  assert.deepStrictEqual(
    episodeCards.map((card) => Number(card.dataset.episodeNumber)),
    [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]
  );

  const episode15 = canonicalSeriesItems.find((recording) =>
    recording.recordingId === 'twd-s10e15');
  const episode15Card = findRecordingCard(
    findRail(production.host, 'series'),
    episode15.recordingId
  );
  assert(episode15Card);
  episode15Card.listeners.click[0]();
  await Promise.resolve();
  await Promise.resolve();
  assert.strictEqual(production.openedRecordings.length, 1);
  assert.strictEqual(production.openedRecordings[0].recording, episode15);
  assert.strictEqual(production.openedRecordings[0].options.backendId, 'default');
  assert.strictEqual(production.openedRecordings[0].options.backLabel, '← Zurück zur Staffel');
  assert.strictEqual(typeof production.openedRecordings[0].options.onClose, 'function');
  assert.strictEqual(production.modules[0], 'recordings2');

  const seriesRequestsBeforeReturn = production.calls.genreRecordings.length;
  production.openedRecordings[0].options.onClose();
  assert.strictEqual(production.modules[production.modules.length - 1], 'overview');
  assert.strictEqual(production.calls.genreRecordings.length, seriesRequestsBeforeReturn);
  const returnedSeasonRail = findRail(production.host, 'series');
  assert.strictEqual(findEpisodeCards(returnedSeasonRail).length, 15);
  const returnedSeason10 = findSeasonButton(returnedSeasonRail, 10);
  assert(returnedSeason10);
  assert(String(returnedSeason10.className).includes('selected'));
  const backToSeries = findElement(returnedSeasonRail, (element) =>
    element.tagName === 'BUTTON' && element.textContent === '← Serien');
  assert(backToSeries);
  backToSeries.listeners.click[0]();
  assert.strictEqual(findSeriesCards(findRail(production.host, 'series')).length, 14);

  const scopedEpisode = makeEpisode('Scoped Series', 1, 1, 'scoped');
  const foreignSeries = Object.assign({}, makeEpisode('Foreign Series', 1, 1, 'foreign'), {
    backendId: 'remote-b'
  });
  const scoped = createProductionHarness({
    genres: {genres: [{id: 'series', label: 'Serien', count: 2}]},
    seriesItems: [scopedEpisode, foreignSeries]
  });
  assert.strictEqual(await scoped.api.refresh(), true);
  const scopedRail = findRail(scoped.host, 'series');
  assert(findSeriesCard(scopedRail, 'folder:serien/scoped series'));
  assert.strictEqual(findSeriesCard(scopedRail, 'folder:serien/foreign series'), null);
  assert.strictEqual(scoped.calls.metadata.length, 1);
  assert.strictEqual(scoped.calls.metadata[0].request.query.backendNativeId, scopedEpisode.backendNativeId);

  const bandEpisode1 = makeEpisode('Band of Brothers', 1, 1, 'band');
  bandEpisode1.metadata.provider = {
    seriesTitle: 'EPG Band of Brothers',
    episodeTitle: 'EPG Currahee',
    seasonNumber: 9,
    episodeNumber: 99
  };
  const bandEpisode2 = makeEpisode('Band of Brothers', 1, 2, 'band');
  const richPreferredArtworkUrl = '/api/vdr/recordings/metadata/image?backend=default&backendNativeId=' +
    encodeURIComponent(bandEpisode1.backendNativeId) + '&kind=preferred&index=0';
  const richPortraitArtworkUrl = '/api/vdr/recordings/metadata/image?backend=default&backendNativeId=' +
    encodeURIComponent(bandEpisode1.backendNativeId) + '&kind=image&index=2';
  const enriched = createProductionHarness({
    genres: {genres: [{id: 'series', label: 'Serien', count: 2}]},
    seriesItems: [bandEpisode1, bandEpisode2],
    metadataByNativeId: {
      [bandEpisode1.backendNativeId]: {
        available: true,
        provider: 'tvscraper',
        mediaType: 'series',
        providerId: -74205,
        title: 'Band of Brothers',
        episodeName: 'Currahee',
        seasonNumber: 1,
        episodeNumber: 1,
        overview: 'Rich TVScraper overview',
        people: [{role: 'actor', name: 'Damian Lewis', characterName: 'Richard Winters'}],
        images: [{orientation: 'portrait', image: {available: true, url: richPortraitArtworkUrl}}],
        preferredArtwork: {available: true, url: richPreferredArtworkUrl}
      }
    },
    metadataErrors: [bandEpisode2.backendNativeId]
  });
  assert.strictEqual(await enriched.api.refresh(), true);
  assert.strictEqual(enriched.calls.metadata.length, 2);
  assert(enriched.metadataMaxInFlight() <= 4);
  const bandRail = findRail(enriched.host, 'series');
  const bandCard = findSeriesCard(bandRail, 'folder:serien/band of brothers');
  assert(bandCard);
  assert(findElement(bandCard, (element) => element.textContent === 'Band of Brothers'));
  assert.strictEqual(findImage(bandCard).src, richPortraitArtworkUrl);
  assert.notStrictEqual(findImage(bandCard).src, richPreferredArtworkUrl);
  bandCard.listeners.click[0]();
  const bandSeason1 = findSeasonButton(findRail(enriched.host, 'series'), 1);
  assert(bandSeason1);
  bandSeason1.listeners.click[0]();
  const bandEpisodes = findEpisodeCards(findRail(enriched.host, 'series'));
  assert.strictEqual(bandEpisodes.length, 2);
  const bandEpisode1Card = findRecordingCard(findRail(enriched.host, 'series'), bandEpisode1.recordingId);
  const bandEpisode2Card = findRecordingCard(findRail(enriched.host, 'series'), bandEpisode2.recordingId);
  assert(bandEpisode1Card);
  assert(bandEpisode2Card);
  assert.strictEqual(Number(bandEpisode1Card.dataset.episodeNumber), 1);
  assert(findElement(bandEpisode1Card, (element) => element.textContent === 'Currahee'));
  assert.strictEqual(findImage(bandEpisode1Card).src, richPortraitArtworkUrl);
  assert.strictEqual(Number(bandEpisode2Card.dataset.episodeNumber), 2);
  assert(findElement(bandEpisode2Card, (element) => element.textContent === 'Folge 02'));

  const progressiveItems = [1, 2, 3, 4].map(function (episodeNumber) {
    const day = String(episodeNumber).padStart(2, '0');
    return {
      recordingId: 'stargate-progressive-' + String(episodeNumber),
      backendId: 'default',
      backendNativeId: '/srv/vdr/video/Serien/Stargate_Universe/2016-01-' + day + '.10.52.3-0.rec',
      path: 'Serien/Stargate Universe/2016-01-' + day + '.10.52.3-0.rec',
      title: 'Stargate Universe',
      metadata: {provider: {}, presentation: {posterUrl: ''}, artwork: {preferredUrl: ''}}
    };
  });
  const progressiveMetadata = {};
  progressiveItems.slice(1).forEach(function (recording, index) {
    progressiveMetadata[recording.backendNativeId] = {
      available: true,
      provider: 'tvscraper',
      mediaType: 'series',
      providerId: 4242,
      title: 'Stargate Universe',
      episodeName: 'Folge ' + String(index + 2),
      seasonNumber: 2,
      episodeNumber: index + 2,
      preferredArtwork: {available: false},
      images: []
    };
  });
  const progressive = createProductionHarness({
    genres: {genres: [{id: 'series', label: 'Serien', count: progressiveItems.length}]},
    seriesItems: progressiveItems,
    metadataByNativeId: progressiveMetadata,
    metadataPending: [progressiveItems[0].backendNativeId]
  });
  let progressiveRefreshSettled = false;
  progressive.api.refresh().then(function () {
    progressiveRefreshSettled = true;
  });
  await new Promise((resolve) => setImmediate(resolve));
  assert.strictEqual(progressiveRefreshSettled, false);
  assert.strictEqual(progressive.calls.metadata.length, 4);
  const progressiveRail = findRail(progressive.host, 'series');
  const progressiveCard = findSeriesCard(progressiveRail, 'folder:serien/stargate universe');
  assert(progressiveCard);
  progressiveCard.listeners.click[0]();
  assert(findSeasonButton(findRail(progressive.host, 'series'), 2));
  assert(findSeasonButton(findRail(progressive.host, 'series'), 0));

  const failingSeries = createProductionHarness({
    newly: {recordings: [sameBackend]},
    genres: {genres: [{id: 'series', label: 'Serien', count: 1}]},
    seriesItems: [scopedEpisode],
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
    seriesItems: [scopedEpisode],
    folders: {folders: [{name: 'Filme', path: 'Filme', count: 1}]}
  });
  assert.strictEqual(await noCanonicalSeries.api.refresh(), true);
  assert.strictEqual(noCanonicalSeries.calls.genreRecordings.length, 0);
  assert.strictEqual(noCanonicalSeries.calls.metadata.length, 0);
  assert.strictEqual(findRail(noCanonicalSeries.host, 'series'), null);

  const emptyCanonicalSeries = createProductionHarness({
    genres: {genres: [{id: 'series', label: 'Serien', count: 1}]},
    seriesItems: []
  });
  assert.strictEqual(await emptyCanonicalSeries.api.refresh(), true);
  assert.strictEqual(emptyCanonicalSeries.calls.genreRecordings.length, 1);
  assert.strictEqual(emptyCanonicalSeries.calls.metadata.length, 0);
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

  console.log('phase66 recording discovery canonical series metadata enrichment coverage ok');
}()).catch(function (error) {
  console.error(error);
  process.exitCode = 1;
});
