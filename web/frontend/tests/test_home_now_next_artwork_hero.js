'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');

const source = fs.readFileSync(
  path.join(frontendRoot, 'home-live-hero.js'),
  'utf8'
);

assert(source.includes('loadArtworkPage'));
assert(source.includes('artworkForEvent'));
assert(source.includes('void loadProgrammeArtworkPage('));
assert(source.includes('media-home-live-guide-logo-badge'));
assert(source.includes(
  '.media-home-live-guide-logo-badge{position:absolute;left:.45rem;bottom:.45rem;'
));
assert(source.includes(
  "appendChannelLogo(artwork, channel, 'media-home-live-guide-logo');"
));
assert(source.includes(
  "image.addEventListener('error', fallback);"
));
assert(source.includes(
  "'media-home-live-guide-logo-badge-logo'"
));
assert(source.includes(
  'if (reuseWarmPrograms) return Promise.resolve(null);'
));
assert(source.includes('card.dataset.eventId = eventId(entry.event);'));
assert(source.includes('function programmeSignature(events)'));
assert(source.includes('config.renderUnchanged !== false'));
assert(source.includes('sync(false, {retainVisible: true, revalidatePrograms: true});'));

assert(!source.includes('fetchClientMetadata'));
assert(!source.includes('fetchClientEpgArtwork'));
assert(!source.includes('/api/epg/cache/artwork?'));

const pageStart =
  source.indexOf(
    'function loadProgrammePage(sequence, offset, reset, options)'
  );

const pageEnd =
  source.indexOf(
    'function loadPrograms(sequence, options)',
    pageStart
  );

assert(pageStart >= 0);
assert(pageEnd > pageStart);

const pageBody =
  source.slice(pageStart, pageEnd);

const successMarker =
  pageBody.indexOf(
    'state.programmeLoadedAt = Date.now();'
  );

const renderMarker =
  pageBody.indexOf(
    'render();',
    successMarker
  );

const artworkMarker =
  pageBody.indexOf(
    'void loadProgrammeArtworkPage(',
    successMarker
  );

assert(successMarker >= 0);
assert(renderMarker > successMarker);
assert(artworkMarker > renderMarker);

let nowNextRequests = 0;
let manifestRequests = 0;
let channelRequests = 0;

const nowNextPages = [];
const manifestPages = [];
const manifestControls = [];

const owner = {
  loadPage(options) {
    nowNextRequests += 1;

    const ids =
      Array.from(options.channelIds || []);

    nowNextPages.push(ids);

    return Promise.resolve({
      backendId: options.backendId,
      eventCount: 0,
      events: []
    });
  },

  loadArtworkPage(options) {
    manifestRequests += 1;

    const ids =
      Array.from(options.channelIds || []);

    manifestPages.push(ids);

    return new Promise((resolve, reject) => {
      manifestControls.push({
        resolve,
        reject
      });
    });
  },

  artworkForEvent(
    backendId,
    channelId,
    eventId
  ) {
    if (backendId === 'backend-a' &&
        channelId === 'C1' &&
        eventId === 'event-1') {
      return {
        available: true,
        provider: 'tvscraper',
        url: '/artwork/event-1.jpg'
      };
    }

    return null;
  }
};

const window = {
  console,
  Date,
  Promise,
  VdrSuiteHomeNowNext: owner,
  VdrSuitePlatform: {
    getClientApi() {
      return {
        fetchClientChannels() {
          channelRequests += 1;
          return Promise.resolve({channels: []});
        }
      };
    },
    getSelectedBackendId() { return 'backend-a'; },
    getSelectedModule() { return 'overview'; }
  }
};

const context = vm.createContext({
  window,
  console,
  Date,
  Promise,
  Object,
  Array,
  String,
  Number,
  Boolean,
  Math,
  Map,
  Set,
  Error
});

vm.runInContext(
  source,
  context,
  {
    filename:
      'web/frontend/home-live-hero.js'
  }
);

assert.ok(window.VdrSuiteHomeLiveHero);

(async function () {
  const hero =
    window.VdrSuiteHomeLiveHero;

  hero.__test.setActive(true);
  hero.__test.setBackendId('backend-a');

  const channels = [];

  for (let number = 1; number <= 25; number += 1) {
    channels.push({
      id: 'C' + String(number),
      name: 'Kanal ' + String(number),
      number,
      radio: false,
      enabled: true
    });
  }

  hero.__test.applyChannels({
    channels
  });

  /*
   * First 24-channel H2 page.
   *
   * The manifest promise intentionally remains unresolved. The H2 page must
   * still resolve, proving enrichment is not part of the first-render await.
   */
  const firstPage =
    await hero.__test.loadProgrammePage(
      0,
      0,
      true
    );

  assert.ok(firstPage);

  assert.strictEqual(
    nowNextRequests,
    1
  );

  assert.strictEqual(
    manifestRequests,
    1
  );

  assert.strictEqual(
    nowNextPages[0].length,
    24
  );

  assert.strictEqual(
    manifestPages[0].length,
    24
  );

  assert.deepStrictEqual(
    manifestPages[0],
    nowNextPages[0]
  );

  assert.strictEqual(
    hero.__test.eventArtwork({
      id: 'event-1',
      channelId: 'C1'
    }),
    '/artwork/event-1.jpg'
  );

  manifestControls[0].resolve({
    artworkCount: 1
  });

  await Promise.resolve();
  await Promise.resolve();

  /*
   * Progressive page 2 contains only the remaining channel and therefore
   * starts one separate bounded manifest request.
   */
  const secondPage =
    await hero.__test.loadProgrammePage(
      0,
      24,
      false
    );

  assert.ok(secondPage);

  assert.strictEqual(
    nowNextRequests,
    2
  );

  assert.strictEqual(
    manifestRequests,
    2
  );

  assert.deepStrictEqual(
    nowNextPages[1],
    ['C25']
  );

  assert.deepStrictEqual(
    manifestPages[1],
    ['C25']
  );

  /*
   * Artwork is optional. A failed manifest must not promote itself into the
   * programme error state or reject the already completed H2 page.
   */
  manifestControls[1].reject(
    new Error('optional manifest failure')
  );

  await Promise.resolve();
  await Promise.resolve();

  /*
   * Canonical Home return revalidates Now/Next directly from the retained
   * channel IDs. It must bypass both the 60-second warm shortcut and the
   * slower channel-list request.
   */
  const beforeResumeRequests = nowNextRequests;
  await hero.__test.load(false, {
    retainVisible: true,
    revalidatePrograms: true
  });
  assert.strictEqual(channelRequests, 0);
  assert.strictEqual(nowNextRequests, beforeResumeRequests + 1);

  const snapshot =
    hero.snapshot();

  assert.strictEqual(
    snapshot.programError,
    ''
  );

  assert.strictEqual(
    snapshot.dataError,
    ''
  );

  console.log(
    'Home H2.1 post-render artwork enrichment contract ok'
  );
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
