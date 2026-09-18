'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const metadataViewSource = fs.readFileSync('web/frontend/recordings2-metadata-view.js', 'utf8');
const metadataAssignmentSource = fs.readFileSync('web/frontend/recordings2-metadata-assignment.js', 'utf8');
const metadataDetailSource = fs.readFileSync('web/frontend/recordings2-metadata-detail.js', 'utf8');

assert(metadataViewSource.includes('.recordings2-metadata-tabs{display:grid;grid-template-columns:repeat(4,minmax(0,1fr))'),
  'metadata tabs must render as a visible four-button grid instead of a horizontal scrollbar');
assert(metadataViewSource.includes('@media(max-width:720px){.recordings2-metadata-tabs{grid-template-columns:repeat(2,minmax(0,1fr))}'),
  'metadata tabs must reflow to a usable two-column grid on mobile');
assert(metadataViewSource.indexOf('root.appendChild(panels.images);') <
  metadataViewSource.indexOf('root.appendChild(tabs);'),
  'canonical metadata view must place the tab bar after all metadata panels');
assert(metadataAssignmentSource.includes("root.querySelector('.recordings2-metadata-tabs')") &&
  metadataAssignmentSource.includes('root.insertBefore(section, tabs)'),
  'manual metadata assignment must stay above the bottom tab bar');
assert(metadataDetailSource.includes('root.insertBefore(box, tabs)'),
  'metadata assignment load errors must stay above the bottom tab bar');
assert(metadataViewSource.includes("panel.scrollIntoView({behavior: 'smooth', block: 'start'})"),
  'selecting a bottom metadata tab must bring its content panel into view');

let requestedPath = '';
let requestedOptions = null;
let personSearchOptions = null;
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
    },
    fetchClientRecordingPersons(options) {
      personSearchOptions = options;
      return Promise.resolve({
        matches: [
          {recording: {id: 'recording-1', title: 'Film A'}},
          {recording: null},
          null
        ]
      });
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
  'web/frontend/platform/helpers.js',
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
assert.strictEqual(
  api.isPublicMetadataImageUrl(
    '/recording-artwork/default/0123456789abcdef0123456789abcdef'
  ),
  true
);
assert.strictEqual(
  api.isPublicMetadataImageUrl('/recording-artwork/default/not-an-asset-id'),
  false
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
const versionedMetadata = api.versionManualMetadataArtwork({
  available: true,
  preferredArtwork: {available: true, url: metadataImagePath},
  manualAssignment: {active: true, revision: 7}
});
assert.strictEqual(
  versionedMetadata.preferredArtwork.url,
  metadataImagePath + '&assignmentRevision=7'
);
assert.strictEqual(
  api.versionManualMetadataArtwork(versionedMetadata).preferredArtwork.url,
  metadataImagePath + '&assignmentRevision=7'
);
const automaticMetadata = {
  preferredArtwork: {available: true, url: metadataImagePath}
};
assert.strictEqual(api.versionManualMetadataArtwork(automaticMetadata), automaticMetadata);

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

function detailField(label, value) {
  const labelNode = {textContent: label};
  const valueNode = {textContent: value};
  return {
    labelNode,
    valueNode,
    querySelector(selector) {
      if (selector === 'span') return labelNode;
      if (selector === 'strong') return valueNode;
      return null;
    }
  };
}

const heading = {textContent: 'Alter technischer Titel'};
const summary = {textContent: 'Alte Beschreibung'};
const detailPoster = {
  children: [],
  textContent: '▶',
  replaceChildren() { this.children = Array.from(arguments); this.textContent = ''; }
};
const genreField = detailField('Genre', 'EPG-Genre');
const releaseField = detailField('Veröffentlichung', 'EPG-Datum');
const ratingField = detailField('Bewertung', 'EPG-Bewertung');
const sourceField = detailField('Metadatenquelle', 'VDR');
const detailFields = [genreField, releaseField, ratingField, sourceField];
const detailRoot = {
  querySelector(selector) {
    if (selector === '.recordings2-detail-copy h3') return heading;
    if (selector === '.recordings2-detail-description') return summary;
    if (selector === '.recordings2-detail-poster') return detailPoster;
    if (selector === '.recordings2-detail-poster img') return detailPoster.children[0] || null;
    return null;
  },
  querySelectorAll(selector) {
    return selector === '.recordings2-detail-field' ? detailFields : [];
  }
};
api.applyMetadataToDetail(detailRoot, {
  available: true,
  title: 'Face/Off – Im Körper des Feindes',
  overview: 'Manuell ausgewählte Beschreibung',
  preferredArtwork: {
    available: true,
    url: versionedMetadata.preferredArtwork.url
  },
  images: [{
    orientation: 'portrait',
    image: {
      available: true,
      url: '/api/vdr/recordings/metadata/image?backend=default&kind=image&index=9'
    }
  }],
  manualAssignment: {
    active: true,
    relationshipLocked: true
  }
});
assert.strictEqual(heading.textContent, 'Face/Off – Im Körper des Feindes');
assert.strictEqual(summary.textContent, 'Manuell ausgewählte Beschreibung');
assert.strictEqual(detailPoster.children.length, 1);
assert.strictEqual(
  detailPoster.children[0].src,
  '/vdr-suite' + metadataImagePath + '&assignmentRevision=7'
);
assert.strictEqual(
  detailPoster.children[0].alt,
  'Poster zu Face/Off – Im Körper des Feindes'
);
api.prioritizeDetailPoster(detailRoot);
assert.strictEqual(detailPoster.children[0].loading, 'eager');
assert.strictEqual(detailPoster.children[0].decoding, 'async');
assert.strictEqual(detailPoster.children[0].fetchPriority, 'high');

const preferredStillPath =
  '/api/vdr/recordings/metadata/image?backend=default&kind=preferred';
const portraitPosterPath =
  '/api/vdr/recordings/metadata/image?backend=default&kind=image&index=3';
heading.textContent = 'Pfadtitel bleibt';
summary.textContent = 'EPG-Fallback';
api.applyMetadataToDetail(detailRoot, {
  available: true,
  provider: 'tvscraper',
  title: 'Automatischer Titel',
  overview: 'Automatische Beschreibung',
  genres: ['Drama', 'War'],
  releaseDate: '2001-09-09',
  firstAired: '2001-09-23',
  voteAverage: 8.7,
  voteCount: 123,
  preferredArtwork: {
    available: true,
    url: preferredStillPath
  },
  images: [
    {
      orientation: 'landscape',
      image: {
        available: true,
        url: '/api/vdr/recordings/metadata/image?backend=default&kind=image&index=0'
      }
    },
    {
      orientation: 'portrait',
      image: {
        available: true,
        url: portraitPosterPath
      }
    }
  ]
});
assert.strictEqual(heading.textContent, 'Pfadtitel bleibt');
assert.strictEqual(summary.textContent, 'Automatische Beschreibung');
assert.strictEqual(genreField.valueNode.textContent, 'Drama, War');
assert.strictEqual(releaseField.valueNode.textContent, '23.09.2001');
assert.strictEqual(ratingField.valueNode.textContent, '8.7 / 10');
assert.strictEqual(sourceField.valueNode.textContent, 'TVScraper');
assert.strictEqual(
  detailPoster.children[0].src,
  '/vdr-suite' + portraitPosterPath
);

const preferredFallbackPath =
  '/api/vdr/recordings/metadata/image?backend=default&kind=preferred&fallback=1';
api.applyMetadataToDetail(detailRoot, {
  available: true,
  provider: 'tvscraper',
  preferredArtwork: {
    available: true,
    url: preferredFallbackPath
  },
  images: [{
    orientation: 'landscape',
    image: {
      available: true,
      url: '/api/vdr/recordings/metadata/image?backend=default&kind=image&index=0'
    }
  }]
});
assert.strictEqual(
  detailPoster.children[0].src,
  '/vdr-suite' + preferredFallbackPath
);

summary.textContent = 'EPG bleibt ohne Scraper-Text';
api.applyMetadataToDetail(detailRoot, {
  available: true,
  title: 'Automatischer Titel ohne Overview',
  overview: '',
  tagline: '',
  preferredArtwork: {available: false}
});
assert.strictEqual(heading.textContent, 'Pfadtitel bleibt');
assert.strictEqual(summary.textContent, 'EPG bleibt ohne Scraper-Text');
assert.strictEqual(genreField.valueNode.textContent, 'Drama, War');
assert.strictEqual(releaseField.valueNode.textContent, '23.09.2001');
assert.strictEqual(ratingField.valueNode.textContent, '8.7 / 10');
assert.strictEqual(sourceField.valueNode.textContent, 'TVScraper');

api.applyMetadataToDetail(detailRoot, {
  available: false,
  provider: 'manual',
  genres: ['Falsch'],
  firstAired: '1999-01-01',
  voteAverage: 1
});
assert.strictEqual(genreField.valueNode.textContent, 'Drama, War');
assert.strictEqual(releaseField.valueNode.textContent, '23.09.2001');
assert.strictEqual(ratingField.valueNode.textContent, '8.7 / 10');
assert.strictEqual(sourceField.valueNode.textContent, 'TVScraper');

window.VdrSuitePublicUrl = null;
assert.strictEqual(
  api.assignmentRuntimePath(),
  '/frontend/recordings2-metadata-assignment.js'
);
metadataImage.src = 'unchanged';
api.repairMetadataImagePaths({querySelectorAll() { return [metadataImage]; }});
assert.strictEqual(metadataImage.src, 'unchanged');

Promise.all([
  api.fetchMetadata({backendNativeId: '/srv/vdr/video/Inferno.rec'}, 'remote'),
  personView.findRecordings({name: 'Harrison Ford'}, 'remote', 12)
]).then(results => {
  const recordings = results[1];
  assert.strictEqual(requestedPath, '/api/vdr/recordings/metadata');
  assert.strictEqual(requestedOptions.query.backend, 'remote');
  assert.strictEqual(requestedOptions.query.backendNativeId, '/srv/vdr/video/Inferno.rec');
  assert.strictEqual(personSearchOptions.backendId, 'remote');
  assert.strictEqual(personSearchOptions.query.name, 'Harrison Ford');
  assert.strictEqual(personSearchOptions.query.limit, 12);
  assert.strictEqual(personSearchOptions.cache, 'no-store');
  assert.strictEqual(personSearchOptions.credentials, 'same-origin');
  assert.strictEqual(recordings.length, 1);
  assert.strictEqual(recordings[0].id, 'recording-1');
  console.log('recordings2 modular metadata detail ok');
}).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
