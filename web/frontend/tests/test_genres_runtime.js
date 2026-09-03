'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const source = [
  fs.readFileSync(path.join(frontendRoot, 'platform', 'helpers.js'), 'utf8'),
  fs.readFileSync(path.join(frontendRoot, 'modules', 'genres.js'), 'utf8')
].join('\n');

class FakeElement {
  constructor(tagName) {
    this.tagName = String(tagName || '').toUpperCase();
    this.children = [];
    this.listeners = Object.create(null);
    this.className = '';
    this.id = '';
    this.type = '';
    this.disabled = false;
    this._textContent = '';
    this.classList = {
      add: name => this.setClass(name, true),
      remove: name => this.setClass(name, false),
      toggle: (name, enabled) => this.setClass(name, enabled),
      contains: name => this.className.split(/\s+/).includes(name)
    };
  }

  setClass(name, enabled) {
    const names = new Set(this.className.split(/\s+/).filter(Boolean));
    if (enabled) names.add(name);
    else names.delete(name);
    this.className = Array.from(names).join(' ');
  }

  set textContent(value) {
    this._textContent = String(
      value === undefined || value === null ? '' : value
    );
  }

  get textContent() {
    return this._textContent +
      this.children.map(child => child.textContent || '').join('');
  }

  append() {
    Array.from(arguments).forEach(child => this.appendChild(child));
  }

  appendChild(child) {
    if (child) this.children.push(child);
    return child;
  }

  replaceChildren() {
    this.children = [];
    this._textContent = '';
    this.append.apply(this, arguments);
  }

  addEventListener(type, handler) {
    if (!this.listeners[type]) this.listeners[type] = [];
    this.listeners[type].push(handler);
  }

  dispatch(type) {
    (this.listeners[type] || []).forEach(handler => handler({target: this}));
  }
}

function findButton(root, expected, contains) {
  if (root.tagName === 'BUTTON') {
    const matches = contains
      ? root.textContent.includes(expected)
      : root.textContent === expected;
    if (matches) return root;
  }
  for (const child of root.children) {
    const found = findButton(child, expected, contains);
    if (found) return found;
  }
  return null;
}

function findElement(root, tagName, className) {
  if ((!tagName || root.tagName === tagName.toUpperCase()) &&
      (!className || root.classList.contains(className))) {
    return root;
  }
  for (const child of root.children) {
    const found = findElement(child, tagName, className);
    if (found) return found;
  }
  return null;
}

function flush() {
  return new Promise(resolve => setImmediate(resolve));
}

function recording(nativeId, title, pathValue) {
  const preferred = '/api/recordings/metadata/image?backend=default&backendNativeId=' +
    encodeURIComponent(nativeId) + '&kind=preferred&index=0';
  return {
    id:nativeId,
    backendId:'default',
    backendNativeId:nativeId,
    title:title,
    path:pathValue,
    startTime:'1780000000',
    durationSeconds:7200,
    sizeMb:4096,
    metadata:{
      presentation:{
        title:title,
        subtitle:'',
        summary:'',
        posterUrl:preferred,
        placeholderVariant:1
      },
      provider:{},
      native:{},
      artwork:{preferredUrl:preferred}
    }
  };
}

function richMetadata(nativeId, title) {
  return {
    available:true,
    provider:'tvscraper',
    mediaType:'movie',
    providerId:14400,
    title:title,
    episodeName:'',
    overview:'TVScraper-Beschreibung zu ' + title,
    preferredArtwork:{
      available:true,
      url:'/api/vdr/recordings/metadata/image?backend=default&backendNativeId=' +
        encodeURIComponent(nativeId) + '&kind=preferred&index=0',
      width:1920,
      height:1080
    },
    images:[
      {
        orientation:'landscape',
        image:{
          available:true,
          url:'/api/vdr/recordings/metadata/image?backend=default&backendNativeId=' +
            encodeURIComponent(nativeId) + '&kind=gallery&index=0',
          width:1920,
          height:1080
        }
      },
      {
        orientation:'portrait',
        image:{
          available:true,
          url:'/api/vdr/recordings/metadata/image?backend=default&backendNativeId=' +
            encodeURIComponent(nativeId) + '&kind=gallery&index=1',
          width:500,
          height:750
        }
      }
    ]
  };
}

(async function main() {
  const mount = new FakeElement('main');
  const elementsById = Object.create(null);
  const head = new FakeElement('head');
  head.appendChild = function (child) {
    FakeElement.prototype.appendChild.call(this, child);
    if (child && child.id) elementsById[child.id] = child;
    return child;
  };

  const document = {
    head: head,
    createElement: tag => new FakeElement(tag),
    getElementById: id => elementsById[id] || null,
    querySelector: () => null,
    querySelectorAll: () => [],
    addEventListener: () => {}
  };

  let registeredModule = null;
  let channelRequests = 0;
  let overviewRequests = 0;
  let epgItemRequests = 0;
  const epgQueries = [];
  const recordingCards = [];
  const openedRecordings = [];
  const recordingMetadataCalls = [];
  let activeMetadataCalls = 0;
  let maximumActiveMetadataCalls = 0;

  const genreRecordings = [
    recording(
      'native-largo',
      'Action/Largo Winch - Tödliches Erbe',
      '/Action/Largo_Winch_-_Tödliches_Erbe/2026-08-16.01.00.2-0.rec'
    ),
    recording(
      'native-no-match',
      'Action/Todliche Weihnachten',
      '/Action/Todliche_Weihnachten/2026-05-20.19.41.1-0.rec'
    ),
    recording('native-48hrs', 'Action/48 Hrs', '/Action/48 Hrs/example.rec'),
    recording('native-diehard', 'Action/Stirb langsam', '/Action/Stirb_langsam/example.rec'),
    recording('native-greatwall', 'Fantasy/The Great Wall 2016', '/Fantasy/The_Great_Wall/example.rec'),
    recording('native-bond', 'Thriller/James Bond 007 - Dr. No', '/Thriller/James_Bond/example.rec')
  ];

  const client = {
    fetchClientGenres: options => {
      overviewRequests += 1;
      return Promise.resolve(
        options.scope === 'epg'
          ? {
              backendId: 'default',
              scope: 'epg',
              from: 1000,
              until: 2000,
              totalItems: 554,
              categories: [
                {
                  id: 'movie',
                  label: 'Film',
                  count: 2,
                  children: [
                    {id: 'thriller', label: 'Thriller', count: 1},
                    {id: 'drama', label: 'Drama', count: 1}
                  ]
                },
                {id: 'series', label: 'Serie', count: 550, children: []},
                {
                  id: 'documentary',
                  label: 'Dokumentation',
                  count: 1,
                  children: []
                },
                {id: 'sports', label: 'Sport', count: 1, children: []}
              ]
            }
          : {
              backendId: 'default',
              scope: 'recordings',
              totalItems: genreRecordings.length,
              genres: [
                {id: 'action', label: 'Action', count: genreRecordings.length, known: true}
              ]
            }
      );
    },
    fetchClientChannels: () => {
      channelRequests += 1;
      return new Promise(() => {});
    },
    fetchClientGenreRecordings: () => Promise.resolve({
      items: genreRecordings,
      total: genreRecordings.length,
      hasMore:false
    }),
    fetchClientGenreEpg: options => {
      epgItemRequests += 1;
      epgQueries.push({
        contentClass: options.contentClass,
        genreId: options.genreId,
        from: options.from,
        until: options.until
      });
      const series = options.contentClass === 'series';
      return Promise.resolve({
        items: [{
          eventId: series ? '101' : '100',
          channelId: 'C-1',
          channelName: 'Das Erste HD',
          title: series ? 'Criminal Intent' : 'Testfilm',
          startTime: 1784839299,
          artwork: series
            ? {
                available:true,
                url:'/api/epg/cache/artwork?backend=default&channelId=C-1&eventId=101',
                width:1280,
                height:720
              }
            : {
                available:true,
                url:'/api/epg/cache/metadata/image?backend=default&channelId=C-1&eventId=100&kind=gallery&index=1',
                width:500,
                height:750
              }
        }],
        total: series ? 1084 : 1,
        hasMore: false
      });
    }
  };

  const platform = {
    registerModule: (name, moduleApi) => {
      assert.strictEqual(name, 'genres');
      registeredModule = moduleApi;
    },
    hasModule: () => false,
    getClientApi: () => client,
    getMountTarget: name => name === 'genres' ? mount : null,
    getSelectedBackendId: () => 'default'
  };

  const context = {
    window: null,
    document: document,
    console: console,
    Date: Date,
    Error: Error,
    Intl: Intl,
    Map: Map,
    Math: Math,
    Number: Number,
    Object: Object,
    Promise: Promise,
    Set: Set,
    String: String,
    setTimeout: setTimeout,
    clearTimeout: clearTimeout,
    VdrSuitePlatform: platform,
    VdrSuiteRecordings2BrowserView: {
      createRecordingCard(projected, onSelect) {
        const card = new FakeElement('button');
        const presentation = projected && projected.metadata &&
          projected.metadata.presentation || {};
        card.textContent = presentation.title || projected.title || 'Aufnahme';
        card.addEventListener('click', function () {
          if (typeof onSelect === 'function') onSelect(projected);
        });
        recordingCards.push({projected, card});
        return card;
      }
    },
    VdrSuiteRecordings2: {
      openRecording(item, options) {
        openedRecordings.push({item, options});
      }
    },
    VdrSuiteRecordings2MetadataDetail: {
      fetchMetadata(item, backendId) {
        assert.strictEqual(backendId, 'default');
        recordingMetadataCalls.push(item.backendNativeId);
        activeMetadataCalls += 1;
        maximumActiveMetadataCalls = Math.max(
          maximumActiveMetadataCalls,
          activeMetadataCalls
        );
        const result = item.backendNativeId === 'native-no-match'
          ? {available:false, status:'not-found'}
          : richMetadata(
              item.backendNativeId,
              item.backendNativeId === 'native-largo'
                ? 'Largo Winch - Tödliches Erbe'
                : item.title.replace(/^[^/]+\//, '')
            );
        return Promise.resolve(result).finally(function () {
          activeMetadataCalls -= 1;
        });
      }
    }
  };
  context.window = context;

  vm.createContext(context);
  vm.runInContext(source, context);

  assert(registeredModule, 'genres module was not registered');
  registeredModule.activate();
  await flush();
  await flush();

  const style = elementsById['vdr-suite-genres-style'];
  assert(style, 'Genres stylesheet was not installed');
  assert(style.textContent.includes('.genres-epg-card img.genres-epg-artwork-poster'));
  assert(style.textContent.includes('aspect-ratio:2/3'));
  assert(style.textContent.includes('object-fit:contain'));

  const actionButton = findButton(mount, 'Action', true);
  assert(actionButton, 'Action recording genre was not rendered');
  actionButton.dispatch('click');
  await flush();
  await flush();
  await flush();

  assert.strictEqual(recordingMetadataCalls.length, genreRecordings.length);
  assert(
    maximumActiveMetadataCalls <= 4,
    'recording metadata enrichment must stay bounded to four parallel reads'
  );
  assert.strictEqual(recordingCards.length, genreRecordings.length);

  const largoCard = recordingCards.find(entry =>
    entry.projected.backendNativeId === 'native-largo');
  assert(largoCard, 'enriched Largo Winch card was not rendered');
  assert.strictEqual(largoCard.projected.path, '');
  assert.strictEqual(
    largoCard.projected.metadata.presentation.title,
    'Largo Winch - Tödliches Erbe'
  );
  const portraitUrl = '/api/vdr/recordings/metadata/image?backend=default&backendNativeId=' +
    'native-largo&kind=gallery&index=1';
  assert.strictEqual(
    largoCard.projected.metadata.presentation.posterUrl,
    portraitUrl
  );
  assert(!largoCard.projected.metadata.presentation.posterUrl.includes('kind=preferred'));

  const fallbackCard = recordingCards.find(entry =>
    entry.projected.backendNativeId === 'native-no-match');
  assert(fallbackCard, 'no-match fallback card was not rendered');
  assert.strictEqual(
    fallbackCard.projected.path,
    genreRecordings[1].path
  );
  assert.strictEqual(
    fallbackCard.projected.metadata.presentation.title,
    'Action/Todliche Weihnachten'
  );

  largoCard.card.dispatch('click');
  assert.strictEqual(openedRecordings.length, 1);
  assert.strictEqual(
    openedRecordings[0].item.path,
    genreRecordings[0].path,
    'genre card click must hand the original recording to Recordings 2'
  );
  assert.strictEqual(
    openedRecordings[0].item.backendNativeId,
    'native-largo'
  );

  const epgButton = findButton(mount, 'EPG', false);
  assert(epgButton, 'EPG scope button was not rendered');
  epgButton.dispatch('click');
  await flush();
  await flush();

  assert(mount.textContent.includes('554 Einträge'));
  assert(mount.textContent.includes('4 Hauptkategorien'));
  for (const label of ['Film', 'Serie', 'Dokumentation', 'Sport']) {
    assert(mount.textContent.includes(label), label + ' was not rendered');
  }
  for (const hidden of ['Nachrichten', 'Talkshow', 'Reality']) {
    assert(!mount.textContent.includes(hidden), hidden + ' must stay hidden');
  }

  for (let index = 0; index < 12; index += 1) {
    const filmButton = findButton(mount, 'Film', true);
    assert(filmButton, 'Film category disappeared');
    filmButton.dispatch('click');

    assert(mount.textContent.includes('Filmgenres'));
    assert(mount.textContent.includes('Alle Filme'));
    assert(mount.textContent.includes('Thriller'));
    assert(!mount.textContent.includes('Nachrichten'));

    const thrillerButton = findButton(mount, 'Thriller', true);
    assert(thrillerButton, 'Thriller child disappeared');
    thrillerButton.dispatch('click');
    await flush();
    await flush();

    assert(mount.textContent.includes('Das Erste HD'));
    assert.strictEqual(
      epgQueries[epgQueries.length - 1].contentClass,
      'movie'
    );
    assert.strictEqual(
      epgQueries[epgQueries.length - 1].genreId,
      'thriller'
    );
    assert.strictEqual(epgQueries[epgQueries.length - 1].from, 1000);
    assert.strictEqual(epgQueries[epgQueries.length - 1].until, 2000);

    const poster = findElement(mount, 'IMG', 'genres-epg-artwork-poster');
    assert(poster, 'Genre EPG movie card must render the portrait poster class');
    assert.strictEqual(
      poster.src,
      '/api/epg/cache/metadata/image?backend=default&channelId=C-1&eventId=100&kind=gallery&index=1'
    );

    const filmBack = findButton(mount, '← Filmgenres', false);
    assert(filmBack, 'Film result back button was not rendered');
    filmBack.dispatch('click');

    const mainBack = findButton(mount, '← EPG-Hauptkategorien', false);
    assert(mainBack, 'Film hierarchy back button was not rendered');
    mainBack.dispatch('click');
  }

  const seriesButton = findButton(mount, 'Serie', true);
  assert(seriesButton, 'Series category was not rendered');
  assert(seriesButton.textContent.includes('550 Treffer'));
  seriesButton.dispatch('click');
  await flush();
  await flush();

  const seriesQuery = epgQueries[epgQueries.length - 1];
  assert.strictEqual(seriesQuery.contentClass, 'series');
  assert.strictEqual(seriesQuery.genreId, '');
  assert.strictEqual(seriesQuery.from, 1000);
  assert.strictEqual(seriesQuery.until, 2000);
  assert(mount.textContent.includes('Criminal Intent'));
  assert(mount.textContent.includes('Serie · 1084 Treffer'));

  const fallbackImage = findElement(mount, 'IMG', '');
  assert(fallbackImage, 'Genre EPG fallback artwork must remain visible');
  assert.strictEqual(
    fallbackImage.src,
    '/api/epg/cache/artwork?backend=default&channelId=C-1&eventId=101'
  );
  assert(!fallbackImage.classList.contains('genres-epg-artwork-poster'));

  const seriesBack = findButton(mount, '← EPG-Hauptkategorien', false);
  assert(seriesBack, 'Series result back button was not rendered');
  seriesBack.dispatch('click');
  const refreshedSeriesButton = findButton(mount, 'Serie', true);
  assert(refreshedSeriesButton, 'Series category disappeared after returning');
  assert(
    refreshedSeriesButton.textContent.includes('1084 Treffer'),
    'detail total must update the cached overview category'
  );
  assert(mount.textContent.includes('1088 Einträge'));

  registeredModule.deactivate();
  registeredModule.activate();
  await flush();
  await flush();
  assert.strictEqual(
    overviewRequests,
    3,
    'reactivating EPG genres must refresh the cached overview'
  );

  assert.strictEqual(epgItemRequests, 13);
  assert.strictEqual(
    channelRequests,
    0,
    'Genre browser must never issue a supplementary VDR channel request'
  );

  console.log(
    'genres runtime native recording metadata and database-only EPG navigation ok'
  );
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
