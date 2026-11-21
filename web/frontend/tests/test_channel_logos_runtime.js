'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.resolve(__dirname, '..', 'channel-logos.js'), 'utf8'
);
const context = {};
vm.runInNewContext(source, context);

function candidates(name, channelId = '') {
  return Array.from(context.uniqueChannelLogoCandidates(name, channelId));
}

assert(candidates('Das Erste HD').includes('/channel-logos/das%20erste%20hd.png'));
assert(candidates('Das Erste HD').includes('/channel-logos/das%20erste.svg'));
assert(candidates('ZDFneo HD').includes('/channel-logos/zdf_neo.png'));
assert(candidates('Nick/Comedy Channel').includes(
  '/channel-logos/nick~comedy%20channel.png'
));
assert(candidates('Nick/Comedy Channel').includes(
  '/channel-logos/nick~comedy%20channel.svg'
));
assert.deepStrictEqual(candidates(''), []);
assert.strictEqual(context.channelLogoInitial('ZDF HD'), 'Z');
console.log('channel logo candidates: PASS');
