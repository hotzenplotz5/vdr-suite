'use strict';

const assert = require('assert');
const fs = require('fs');

const client = fs.readFileSync('web/frontend/api/client-api.js', 'utf8');
const live = fs.readFileSync('web/frontend/live-tv-view.js', 'utf8');

assert(client.includes('function fetchClientHbbtvApplications(options)'));
assert(client.includes(
  "requestJson('/api/vdr/broadcast/hbbtv/applications', options)"
));
assert(client.includes(
  'fetchClientHbbtvApplications: fetchClientHbbtvApplications'
));

assert(live.includes("typeof client.fetchClientHbbtvApplications !== 'function'"));
assert(live.includes('vdr-suite-hbbtv-availability'));
assert(live.includes("hbbtvIndicator.setAttribute('aria-live', 'polite')"));
assert(live.includes("state.hbbtvResult === 'no_applications'"));
assert(live.includes('attempt < 4'));
assert(live.includes("state.liveChannelId !== id"));
assert(live.includes('sequence !== state.hbbtvRequestSequence'));
assert(live.includes("'HbbTV verfügbar'"));
assert(live.includes("'Kein HbbTV signalisiert'"));
assert(live.includes("'HbbTV Senderkontext nicht aktiv'"));
assert(live.includes("'HbbTV Empfänger nicht aktiv'"));

assert(!live.includes('fetchClientHbbtvLaunch'));
assert(!live.includes('openArbitraryUrl'));
assert(!live.includes('executeJavascript'));
assert(!live.includes('rawKey('));

console.log('Phase 67 HbbTV discovery frontend contract ok');
