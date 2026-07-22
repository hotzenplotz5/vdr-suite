 'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const document = {
  getElementById() { return null; },
  head: { appendChild() {} },
  createElement() {
    return {
      className: '',
      dataset: {},
      hidden: false,
      children: [],
      classList: { add() {} },
      setAttribute() {},
      addEventListener() {},
      appendChild(child) { this.children.push(child); return child; },
      replaceChildren() { this.children = Array.from(arguments); },
      remove() {},
      querySelector() { return null; }
    };
  }
};

const window = {};
const context = vm.createContext({
  window,
  document,
  console,
  Date,
  Number,
  String,
  Object,
  Array,
  Promise
});

const source = fs.readFileSync(
  'web/frontend/recordings2-metadata-detail.js',
  'utf8'
);
vm.runInContext(source, context, {
  filename: 'recordings2-metadata-detail.js'
});

const api = window.VdrSuiteRecordings2MetadataDetail;
assert.ok(api);
assert.strictEqual(api.roleLabel('actor'), 'Schauspiel');
assert.strictEqual(api.roleLabel('director'), 'Regie');
assert.strictEqual(api.mediaTypeLabel('movie'), 'Film');
assert.strictEqual(api.mediaTypeLabel('series'), 'Serie');
assert.strictEqual(api.orientationLabel('portrait'), 'Hochformat');
assert.strictEqual(api.formatDate('2026-07-22'), '22.07.2026');
assert.strictEqual(
  api.isPublicMetadataImageUrl(
    '/api/vdr/recordings/metadata/image?backend=default'
  ),
  true
);
assert.strictEqual(
  api.isPublicMetadataImageUrl(
    'https://image.tmdb.org/example.jpg'
  ),
  false
);

console.log('recordings2 metadata detail ok');
