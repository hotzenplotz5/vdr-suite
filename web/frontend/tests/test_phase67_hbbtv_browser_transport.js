'use strict';

const assert = require('assert');
const fs = require('fs');

const client = fs.readFileSync('web/frontend/api/client-api.js', 'utf8');
const index = fs.readFileSync('web/frontend/index.html', 'utf8');
const paths = fs.readFileSync('core/http/src/TestHttpServerPaths.inc', 'utf8');
const install = fs.readFileSync('mk/install.mk', 'utf8');
const live = fs.readFileSync('web/frontend/live-tv-view.js', 'utf8');
const worker = fs.readFileSync('web/frontend/hbbtv-qoi-worker.js', 'utf8');

for (const token of [
  'function fetchClientHbbtvSessionLaunch(options)',
  'function fetchClientHbbtvSessionStatus(options)',
  'function fetchClientHbbtvSessionInput(options)',
  'function fetchClientHbbtvSessionClose(options)',
  'function fetchClientHbbtvMedia(options)',
  'function mutateClientHbbtvMedia(options)',
  "'/api/vdr/broadcast/hbbtv/sessions/media'",
  'function fetchClientHbbtvPresentation(options)',
  "'/api/vdr/broadcast/hbbtv/sessions/presentation'",
  "Accept: 'image/qoi'",
  'activeSessionCsrfHeaders()'
]) {
  assert(client.includes(token), token);
}

const qoiPosition = index.indexOf(
  '<script src="../frontend/hbbtv-qoi.js"></script>'
);
const livePosition = index.indexOf(
  '<script src="../frontend/live-tv-view.js"></script>'
);
assert(qoiPosition >= 0);
assert(livePosition > qoiPosition);
assert(paths.includes(
  '{"/frontend/hbbtv-qoi.js", "hbbtv-qoi.js", "application/javascript; charset=utf-8", nullptr}'
));
assert(install.includes(
  'web/frontend/hbbtv-qoi.js $(DESTDIR)$(DATADIR)/web/frontend/hbbtv-qoi.js'
));
assert(install.includes(
  'node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/hbbtv-qoi.js'
));
assert(paths.includes(
  '{"/frontend/hbbtv-qoi-worker.js", "hbbtv-qoi-worker.js", "application/javascript; charset=utf-8", nullptr}'
));
assert(install.includes(
  'web/frontend/hbbtv-qoi-worker.js $(DESTDIR)$(DATADIR)/web/frontend/hbbtv-qoi-worker.js'
));
assert(install.includes(
  'node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/hbbtv-qoi-worker.js'
));
assert(worker.includes("importScripts('hbbtv-qoi.js')"));
assert(live.includes("new global.Worker('../frontend/hbbtv-qoi-worker.js')"));

for (const forbidden of [
  'RedButton',
  'LoadUrl',
  'ProcessKey',
  'executeJavascript',
  'VK_LEFT',
  'VK_ENTER'
]) {
  assert(!client.includes(forbidden), forbidden);
}

console.log('Phase 67 HbbTV browser transport contract ok');

const openStart = live.indexOf('function openHbbtvSession()');
const closeStart = live.indexOf('function closeHbbtvSession()', openStart);
assert(openStart >= 0);
assert(closeStart > openStart);

const openSource = live.slice(openStart, closeStart);
const refreshFunctionPosition = openSource.indexOf(
  'function launchFreshApplication(staleRetry) {'
);
const refreshPosition = openSource.indexOf(
  'return beginHbbtvAvailability(currentChannel).then(function() {',
  refreshFunctionPosition
);
const freshApplicationPosition = openSource.indexOf(
  'const application = launchableHbbtvApplication();',
  refreshPosition
);
const launchPosition = openSource.indexOf(
  'return client.fetchClientHbbtvSessionLaunch({',
  freshApplicationPosition
);
const staleRetryPosition = openSource.indexOf(
  "text(error.message) === 'hbbtv_application_context_stale'",
  launchPosition
);
const boundedRetryPosition = openSource.indexOf(
  'return launchFreshApplication(true);',
  staleRetryPosition
);
const initialLaunchPosition = openSource.indexOf(
  'return launchFreshApplication(false).catch(function(error) {',
  boundedRetryPosition
);

assert(refreshFunctionPosition >= 0);
assert(refreshPosition > refreshFunctionPosition);
assert(freshApplicationPosition > refreshPosition);
assert(launchPosition > freshApplicationPosition);
assert(staleRetryPosition > launchPosition);
assert(boundedRetryPosition > staleRetryPosition);
assert(initialLaunchPosition > boundedRetryPosition);
