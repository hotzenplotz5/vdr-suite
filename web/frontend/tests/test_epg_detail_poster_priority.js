'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const detailSource = fs.readFileSync(
  path.resolve(__dirname, '..', 'epg-metadata-detail.js'),
  'utf8'
);
const hookSource = fs.readFileSync(
  path.resolve(__dirname, '..', 'epg-metadata-detail-hook.js'),
  'utf8'
);
const timelineSource = fs.readFileSync(
  path.resolve(__dirname, '..', 'epg-searchtimer-actions.js'),
  'utf8'
);

const headChildren = [];
const document = {
  getElementById() {
    return null;
  },
  createElement(tag) {
    return {
      tagName: String(tag).toUpperCase(),
      id: '',
      textContent: '',
      style: {},
      className: '',
      appendChild() {},
      setAttribute() {},
      addEventListener() {}
    };
  },
  head: {
    appendChild(element) {
      headChildren.push(element);
    }
  }
};

const context = vm.createContext({
  window: {},
  document,
  console,
  Map,
  Promise,
  URLSearchParams,
  setTimeout
});
context.window.window = context.window;

vm.runInContext(detailSource, context, {filename: 'epg-metadata-detail.js'});

const selectDetailArtwork = context.window.VdrSuiteEpgMetadataDetail.selectDetailArtwork;
assert.strictEqual(typeof selectDetailArtwork, 'function');

const preferredLandscape = {
  available: true,
  url: '/api/epg/cache/metadata/image?kind=preferred'
};
const portraitArtwork = {
  available: true,
  url: '/api/epg/cache/metadata/image?kind=image&index=2'
};

const portraitSelection = selectDetailArtwork({
  preferredArtwork: preferredLandscape,
  images: [
    {
      orientation: 'landscape',
      image: {
        available: true,
        url: '/api/epg/cache/metadata/image?kind=image&index=0'
      }
    },
    {
      orientation: 'portrait',
      image: portraitArtwork
    }
  ]
});
assert.strictEqual(portraitSelection.orientation, 'portrait');
assert.strictEqual(portraitSelection.artwork.url, portraitArtwork.url);

const preferredFallback = selectDetailArtwork({
  preferredArtwork: preferredLandscape,
  images: []
});
assert.strictEqual(preferredFallback.orientation, 'preferred');
assert.strictEqual(preferredFallback.artwork.url, preferredLandscape.url);

const invalidPortraitFallback = selectDetailArtwork({
  preferredArtwork: preferredLandscape,
  images: [
    {
      orientation: 'portrait',
      image: {
        available: true,
        url: 'https://example.invalid/poster.jpg'
      }
    }
  ]
});
assert.strictEqual(invalidPortraitFallback.orientation, 'preferred');
assert.strictEqual(invalidPortraitFallback.artwork.url, preferredLandscape.url);

assert.strictEqual(selectDetailArtwork({preferredArtwork: {available: false}, images: []}), null);

const styles = headChildren.map(element => element.textContent).join('\n');
assert.ok(styles.includes('.epg-detail-artwork.epg-detail-artwork-poster'));
assert.ok(styles.includes('aspect-ratio:2/3'));
assert.ok(styles.includes('background-size:contain'));
assert.ok(styles.includes('background-repeat:no-repeat'));

assert.ok(detailSource.includes("addTab('epg', 'EPG', false)"));
assert.ok(detailSource.includes("addTab('scraper', 'Scraper', true)"));
assert.ok(detailSource.includes("addTab('cast', 'Besetzung', true)"));
assert.ok(detailSource.includes("addTab('images', 'Bilder', true)"));
assert.ok(hookSource.includes('createEpgEventDetailCard = function (event, channel)'));
assert.ok(hookSource.includes('renderer.enhance(detail, event, channel)'));
assert.ok(timelineSource.includes('createEpgEventDetailCard = function (event, channel)'));
assert.ok(timelineSource.includes('decorateTimelineDetailArtwork(detail'));

console.log('EPG detail portrait artwork priority regression passed');
