'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');

async function testPublicMembershipPage() {
  const requests = [];
  const id = 'rec_' + '1'.repeat(32);
  const item = {recordingId:id, backendId:'default', title:'Der Fall',
    cardMetadata:{mediaType:'series', title:'Serie', episodeName:'Der Fall'},
    preferredArtworkId:'2'.repeat(32), seriesGrouping:{groupKey:'sg1_' + '3'.repeat(32)}};
  const pages = [
    {items:[item], page:{limit:1, offset:0, totalCount:2, hasMore:true}, meta:{partial:false}},
    {items:[], page:{limit:1, offset:2, totalCount:2, hasMore:false}, meta:{partial:false}}
  ];
  const error = new Error('403');
  const window = {VdrSuiteClientApi:{requestJson(route, options) {
    requests.push({route, options});
    if (requests.length > pages.length) return Promise.reject(error);
    return Promise.resolve(pages[requests.length - 1]);
  }}};
  vm.runInNewContext(fs.readFileSync(path.join(frontendRoot, 'api', 'genre-client-api.js'), 'utf8'), {window, console});
  const api = window.VdrSuiteClientApi;
  const proofs = [];
  const first = await api.fetchPublicGenreRecordings({backendId:'default', genreId:'series',
    limit:1, offset:0, onPage:page => proofs.push(page)});
  assert.strictEqual(requests[0].route, '/api/v1/genres/recordings');
  assert.strictEqual(requests[0].options.query.backendId, 'default');
  assert.strictEqual(requests[0].options.query.genreId, 'series');
  assert.strictEqual(requests[0].options.query.locale, 'de');
  assert.strictEqual(requests[0].options.query.limit, 1);
  assert.strictEqual(requests[0].options.query.offset, 0);
  assert.strictEqual(first.items, pages[0].items);
  assert.strictEqual(first.recordings, first.items);
  assert.strictEqual(first.recordings[0], item);
  assert.strictEqual(first.recordings[0].backendNativeId, undefined);
  assert.strictEqual(first.total, 2);
  assert.strictEqual(first.totalCount, 2);
  assert.strictEqual(first.hasMore, true);
  assert.strictEqual(proofs[0], pages[0]);
  const empty = await api.fetchPublicGenreRecordings({backendId:'default', genreId:'series', limit:1, offset:2});
  assert.strictEqual(empty.recordings.length, 0);
  assert.strictEqual(empty.total, 2);
  assert.strictEqual(empty.hasMore, false);
  assert.strictEqual(requests[1].options.query.offset, 2);
  await assert.rejects(api.fetchPublicGenreRecordings({backendId:'default', genreId:'series'}),
    e => e === error);
  assert.strictEqual(requests.length, 3, '403 must not trigger a legacy fallback');
  const native = {id:'native-list-id', backendId:'default', backendNativeId:'/video/exact.rec'};
  const bridgeRequests = [];
  const bridgeWindow = {VdrSuiteClientApi:{requestJson(route, options) {
    bridgeRequests.push({route, options});
    return Promise.resolve({publicRecordingId:id, recording:native, metadataDetail:{available:true}});
  }}};
  vm.runInNewContext(fs.readFileSync(path.join(frontendRoot, 'api', 'genre-client-api.js'), 'utf8'), {window:bridgeWindow, console});
  const detail = await bridgeWindow.VdrSuiteClientApi.fetchClientRecordingForPublicId({backendId:'default', recordingId:id});
  assert.strictEqual(bridgeRequests[0].route, '/api/vdr/recordings/folder');
  assert.strictEqual(bridgeRequests[0].options.query.recordingId, id);
  assert.strictEqual(detail.recording.backendNativeId, native.backendNativeId);
  assert.strictEqual(detail.recording.id, native.id);
  assert.strictEqual(detail.recording.publicRecordingId, id);
  await assert.rejects(bridgeWindow.VdrSuiteClientApi.fetchClientRecordingForPublicId({
    backendId:'other', recordingId:id
  }), e => /Aufnahmeidentität/.test(e.message));
  assert.strictEqual(bridgeRequests.length, 2);
}


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

function publicDetail(member, title, absent) {
  const artworkUrl = id => '/api/v1/recordings/' + member.recordingId +
    '/artwork/' + id + '?backendId=' + member.backendId;
  return Object.assign({}, member, {
    metadata:absent ? {state:'absent'} : {
      state:'available', source:'restfulapi-scraper-bridge', contentKind:'movie',
      providerTitle:title, overview:'TVScraper-Beschreibung zu ' + title,
      preferredArtwork:{available:true, url:artworkUrl('a'.repeat(32)), width:1920, height:1080},
      images:[
        {orientation:'landscape', image:{available:true, url:artworkUrl('b'.repeat(32)), width:1920, height:1080}},
        {orientation:'portrait', image:{available:true, url:artworkUrl('c'.repeat(32)), width:500, height:750}}
      ]
    }
  });
}

(async function main() {
  await testPublicMembershipPage();
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
  const recordingOpenCalls = [];
  const membershipCalls = [];
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

  const publicMembers = genreRecordings.map((item, index) => ({
    recordingId:'rec_' + String(index + 1).repeat(32),
    backendId:'default', title:item.title.split('/').pop(),
    recordedAt:'2026-08-16T01:00:00Z', durationSeconds:3600, durationKnown:true
  }));
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
    fetchClientGenreRecordings: () => { throw new Error('legacy membership must not be called'); },
    fetchPublicGenreRecordings: options => {
      membershipCalls.push(options);
      return Promise.resolve({items:publicMembers, total:publicMembers.length, hasMore:false});
    },
    requestJson(route, options) {
      assert.strictEqual(options.query.backendId, 'default');
      const index = publicMembers.findIndex(item => route === '/api/v1/recordings/' + item.recordingId);
      assert(index >= 0, 'metadata must use the exact public member identity');
      const item = genreRecordings[index];
      recordingMetadataCalls.push(publicMembers[index].recordingId);
      activeMetadataCalls += 1;
      maximumActiveMetadataCalls = Math.max(maximumActiveMetadataCalls, activeMetadataCalls);
      return Promise.resolve(publicDetail(publicMembers[index],
        item.title.replace(/^[^/]+\//, ''), item.backendNativeId === 'native-no-match'))
        .finally(() => { activeMetadataCalls -= 1; });
    },
    fetchClientRecordingForPublicId(options) {
      assert.strictEqual(options.backendId, 'default');
      const index = publicMembers.findIndex(item => item.recordingId === options.recordingId);
      assert(index >= 0, 'private detail must use the exact public member identity');
      recordingOpenCalls.push(options.recordingId);
      return Promise.resolve({
        publicRecordingId:options.recordingId,
        recording:Object.assign({}, genreRecordings[index], {publicRecordingId:options.recordingId})
      });
    },
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
        card.textContent = projected.cardMetadata && projected.cardMetadata.title ||
          projected.title || 'Aufnahme';
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
      fetchMetadata() { throw new Error('Genre membership must enrich by public identity'); }
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
  assert(style.textContent.includes('object-fit:cover'));
  assert(style.textContent.includes('grid-template-columns:7.25rem minmax(0,1fr)'));
  assert(style.textContent.includes('grid-template-columns:6.25rem minmax(0,1fr)'));

  const actionButton = findButton(mount, 'Action', true);
  assert(actionButton, 'Action recording genre was not rendered');
  actionButton.dispatch('click');
  await flush();
  await flush();
  await flush();

  assert.strictEqual(membershipCalls.length, 1);
  assert.strictEqual(membershipCalls[0].limit, 30);
  assert.strictEqual(membershipCalls[0].genreId, 'action');
  assert.strictEqual(recordingMetadataCalls.length, genreRecordings.length);
  assert(
    maximumActiveMetadataCalls <= 4,
    'recording metadata enrichment must stay bounded to four parallel reads'
  );
  assert.strictEqual(recordingCards.length, genreRecordings.length);

  const largoCard = recordingCards.find(entry =>
    entry.projected.recordingId === publicMembers[0].recordingId);
  assert(largoCard, 'enriched Largo Winch card was not rendered');
  assert.strictEqual(largoCard.projected.backendNativeId, undefined);
  assert.strictEqual(largoCard.projected.path, undefined);
  assert.strictEqual(largoCard.projected.cardMetadata.title, 'Largo Winch - Tödliches Erbe');
  assert.strictEqual(largoCard.projected.preferredArtworkId, 'c'.repeat(32));
  assert.strictEqual(largoCard.projected.metadata.overview,
    'TVScraper-Beschreibung zu Largo Winch - Tödliches Erbe');

  const fallbackCard = recordingCards.find(entry =>
    entry.projected.recordingId === publicMembers[1].recordingId);
  assert(fallbackCard, 'no-match fallback card was not rendered');
  assert.strictEqual(fallbackCard.projected, publicMembers[1]);
  assert.strictEqual(recordingOpenCalls.length, 0, 'rendering cards must not resolve private paths');

  largoCard.card.dispatch('click');
  await flush();
  await flush();
  assert.strictEqual(openedRecordings.length, 1);
  assert.strictEqual(recordingMetadataCalls.length, genreRecordings.length);
  assert.deepStrictEqual(recordingOpenCalls, [publicMembers[0].recordingId]);
  assert.strictEqual(openedRecordings[0].item.publicRecordingId, publicMembers[0].recordingId);
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
    if (index === 0) {
      assert.strictEqual(typeof poster.onerror, 'function');
      poster.onerror();
      assert.strictEqual(
        poster.src,
        '/api/epg/cache/artwork?backend=default&channelId=C-1&eventId=100'
      );
    }

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
