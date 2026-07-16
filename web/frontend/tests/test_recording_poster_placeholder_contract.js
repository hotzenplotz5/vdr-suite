'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const indexPath = path.resolve(__dirname, '..', 'index.html');
const html = fs.readFileSync(indexPath, 'utf8');

assert.ok(
  html.includes('Phase 60.15d: deterministic provider-independent poster preparation.'),
  'Recording poster placeholder contract marker is missing'
);
assert.ok(
  html.includes('.recording-list-item::before'),
  'Recording list poster placeholder selector is missing'
);
assert.ok(
  html.includes('.recording-detail::before'),
  'Recording detail poster placeholder selector is missing'
);
assert.ok(
  html.includes('.recording-list-item:nth-of-type(6n + 2)::before'),
  'Deterministic list placeholder variants are missing'
);
assert.ok(
  html.includes('@media (max-width: 760px)'),
  'Mobile placeholder layout is missing'
);
assert.ok(
  !html.includes('http://') && !html.includes('https://'),
  'Frontend shell must not introduce an external poster dependency'
);

console.log('test_recording_poster_placeholder_contract passed');
