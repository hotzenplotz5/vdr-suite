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
      return '/vdr-suite' + path;
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

const api = window.VdrSuiteRecordings2MetadataDetail;
assert.ok(window.VdrSuiteRecordings2PersonSearchView);
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
