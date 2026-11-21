'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const sourcePath = path.resolve(__dirname, '..', 'epg-cache.js');
const source = fs.readFileSync(sourcePath, 'utf8');
const start = source.indexOf('function loadLiveNowNextEvents()');
const end = source.indexOf('function loadCachedNowNextEvents', start);

assert.ok(start >= 0 && end > start, 'cannot bound loadLiveNowNextEvents');
const body = source.slice(start, end);

assert.ok(body.includes("fetchJsonOrThrow('/api/epg/now-next?from=-1')"));
assert.ok(!body.includes('/api/vdr/events'));
assert.ok(body.includes(".catch(() => ({ events: [] }))"));
assert.strictEqual((body.match(/fetchJsonOrThrow\(/g) || []).length, 1);

console.log('test_phase69f_home_epg_fallback_removal passed');
