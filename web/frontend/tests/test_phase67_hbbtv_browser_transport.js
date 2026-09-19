'use strict';

const assert = require('assert');
const fs = require('fs');

const client = fs.readFileSync('web/frontend/api/client-api.js', 'utf8');
const index = fs.readFileSync('web/frontend/index.html', 'utf8');
const paths = fs.readFileSync('core/http/src/TestHttpServerPaths.inc', 'utf8');
const install = fs.readFileSync('mk/install.mk', 'utf8');

for (const token of [
  'function fetchClientHbbtvSessionLaunch(options)',
  'function fetchClientHbbtvSessionStatus(options)',
  'function fetchClientHbbtvSessionInput(options)',
  'function fetchClientHbbtvSessionClose(options)',
  'function fetchClientHbbtvMedia(options)',
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
