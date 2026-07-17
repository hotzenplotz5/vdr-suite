'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const sourcePath = path.resolve(__dirname, '..', 'recording-artwork.js');
const source = fs.readFileSync(sourcePath, 'utf8');

assert.ok(
  source.includes(
    '.recording-folder-list > .recording-list-item:not(.recording-folder-item) {'
  ),
  'Direct Recording rows must remain full width without matching folder cards'
);
assert.ok(
  !source.includes(
    "'  .recording-folder-list > .recording-list-item {'"
  ),
  'The broad full-width Recording selector must not match single-Recording folders'
);

console.log('test_recording_folder_grid_contract passed');
