'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

let requestedPath = '';
let requestedOptions = null;
const document = {
  querySelector() { return null; },
  getElementById() { return null; },
  head: { appendChild() {} },
  createElement() {
    return {
      className: '',
      dataset: {},
      hidden: false,
      children: [],
      textContent: '',
      classList: { add() {} },
      setAttribute() {},
      addEventListener() {},
      appendChild(child) { this.children.push(child); return child; },
      append() { this.children.push(...arguments); },
      replaceChildren() { this.children = Array.from(arguments); },
      remove() {},
      querySelector() { return null; },
      insertBefore(child) { this.children.push(child); return child; }
    };
  }
};

const window = {
  VdrSuiteClientApi: {
    requestJson(path, options) {
      requestedPath = path;
      requestedOptions = options;
      return Promise.resolve({available: false});
    }
  },
  VdrSuitePublicUrl: {
    resolvePath(path) {
      const value = String(path || '');
      if (value === '/vdr-suite' || value.startsWith('/vdr-suite/')) return value;
      return '/vdr-suite' + value;
    }
  }
};
const context = vm.createContext({
  window,
  document,
  console,
  Date,
  Number,
  String,
  Object,
  Array,
  Promise,
  Math,
  parseInt
});

[
  'web/frontend/recordings2-shared.js',
  'web/frontend/recordings2-person-search-view.js',
  'web/frontend/recordings2-metadata-view.js',
  'web/frontend/recordings2-metadata-detail.js'
].forEach(path => {
  vm.runInContext(fs.readFileSync(path, 'utf8'), context, {filename: path});
});

const personView = window.VdrSuiteRecordings2PersonSearchView;
const api = window.VdrSuiteRecordings2MetadataDetail;
assert.ok(personView);
assert.ok(window.VdrSuiteRecordings2MetadataView);
assert.ok(api);
assert.strictEqual(api.roleLabel('actor'), 'Schauspiel');
assert.strictEqual(api.roleLabel('director'), 'Regie');
assert.strictEqual(api.mediaTypeLabel('movie'), 'Film');
assert.strictEqual(api.mediaTypeLabel('series'), 'Serie');
assert.strictEqual(api.orientationLabel('portrait'), 'Hochformat');
assert.strictEqual(api.formatDate('2026-07-22'), '22.07.2026');
assert.strictEqual(
  api.isPublicMetadataImageUrl('/api/vdr/recordings/metadata/image?backend=default'),
  true
);
assert.strictEqual(api.isPublicMetadataImageUrl('https://image.tmdb.org/example.jpg'), false);
assert.strictEqual(
  api.assignmentRuntimePath(),
  '/vdr-suite/frontend/recordings2-metadata-assignment.js'
);

const manualCastPanel = document.createElement('section');
personView.renderCast(
  manualCastPanel,
  {
    available: true,
    provider: 'manual',
    people: [
      {
        role: 'actor',
        name: 'Tom Hanks',
        characterName: 'Forrest Gump',
        image: {available: false}
      }
    ],
    manualAssignment: {
      active: true,
      relationshipLocked: true
    }
  },
  'default',
  api.isPublicMetadataImageUrl
);
assert.strictEqual(manualCastPanel.children.length, 1);
const manualCastGrid = manualCastPanel.children[0];
assert.strictEqual(manualCastGrid.className, 'recordings2-metadata-cast');
assert.strictEqual(manualCastGrid.children.length, 1);
const manualCastCard = manualCastGrid.children[0].children[0];
assert.strictEqual(manualCastCard.className, 'recordings2-person-card');
assert.strictEqual(manualCastCard.title, 'Tom Hanks in vorhandenen Aufnahmen suchen');
assert.strictEqual(manualCastCard.children[0].className, 'recordings2-person-placeholder');
const manualCastCopy = manualCastCard.children[1];
assert.strictEqual(manualCastCopy.children[0].textContent, 'Tom Hanks');
assert.strictEqual(manualCastCopy.children[1].textContent, 'Forrest Gump');
assert.strictEqual(manualCastCopy.children[2].textContent, 'Schauspiel');

const metadataImagePath = '/api/vdr/recordings/metadata/image?backend=default';
const metadataImage = {
  src: '',
  getAttribute(name) { return name === 'src' ? metadataImagePath : null; }
};
api.repairMetadataImagePaths({
  querySelectorAll(selector) {
    assert.strictEqual(
      selector,
      '.recordings2-metadata-image img,.recordings2-person-image'
    );
    return [metadataImage];
  }
});
assert.strictEqual(metadataImage.src, '/vdr-suite' + metadataImagePath);

const heading = {textContent: 'Alter technischer Titel'};
const summary = {textContent: 'Alte Beschreibung'};
const detailPoster = {
  children: [],
  textContent: '▶',
  replaceChildren() { this.children = Array.from(arguments); this.textContent = ''; }
};
const detailRoot = {
  querySelector(selector) {
    if (selector === '.recordings2-detail-copy h3') return heading;
    if (selector === '.recordings2-detail-description') return summary;
    if (selector === '.recordings2-detail-poster') return detailPoster;
    return null;
  }
};
api.applyMetadataToDetail(detailRoot, {
  available: true,
  title: 'Face/Off – Im Körper des Feindes',
  overview: 'Manuell ausgewählte Beschreibung',
  preferredArtwork: {
    available: true,
    url: metadataImagePath
  },
  manualAssignment: {
    active: true,
    relationshipLocked: true
  }
});
assert.strictEqual(heading.textContent, 'Face/Off – Im Körper des Feindes');
assert.strictEqual(summary.textContent, 'Manuell ausgewählte Beschreibung');
assert.strictEqual(detailPoster.children.length, 1);
assert.strictEqual(detailPoster.children[0].src, '/vdr-suite' + metadataImagePath);
assert.strictEqual(
  detailPoster.children[0].alt,
  'Poster zu Face/Off – Im Körper des Feindes'
);

heading.textContent = 'Pfadtitel bleibt';
summary.textContent = 'Fallback bleibt';
api.applyMetadataToDetail(detailRoot, {
  available: true,
  title: 'Automatischer Titel',
  overview: 'Automatische Beschreibung',
  preferredArtwork: {
    available: true,
    url: '/api/vdr/recordings/metadata/image?backend=default&kind=preferred'
  }
});
assert.strictEqual(heading.textContent, 'Pfadtitel bleibt');
assert.strictEqual(summary.textContent, 'Fallback bleibt');
assert.strictEqual(
  detailPoster.children[0].src,
  '/vdr-suite/api/vdr/recordings/metadata/image?backend=default&kind=preferred'
);

window.VdrSuitePublicUrl = null;
assert.strictEqual(
  api.assignmentRuntimePath(),
  '/frontend/recordings2-metadata-assignment.js'
);
metadataImage.src = 'unchanged';
api.repairMetadataImagePaths({querySelectorAll() { return [metadataImage]; }});
assert.strictEqual(metadataImage.src, 'unchanged');

api.fetchMetadata({backendNativeId: '/srv/vdr/video/Inferno.rec'}, 'remote').then(() => {
  assert.strictEqual(requestedPath, '/api/vdr/recordings/metadata');
  assert.strictEqual(requestedOptions.query.backend, 'remote');
  assert.strictEqual(requestedOptions.query.backendNativeId, '/srv/vdr/video/Inferno.rec');
  console.log('recordings2 modular metadata detail ok');
}).catch(error => {
  console.error(error);
  process.exitCode = 1;
});