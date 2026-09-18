const assert = require('assert');
const fs = require('fs');

const teletext = fs.readFileSync('web/frontend/teletext-view.js', 'utf8');
const live = fs.readFileSync('web/frontend/live-tv-view.js', 'utf8');
const client = fs.readFileSync('web/frontend/api/client-api.js', 'utf8');
const index = fs.readFileSync('web/frontend/index.html', 'utf8');
const paths = fs.readFileSync('core/http/src/TestHttpServerPaths.inc', 'utf8');
const install = fs.readFileSync('mk/install.mk', 'utf8');

assert(teletext.includes('global.VdrSuiteTeletextView = api'));
assert(teletext.includes('fetchClientTeletextService'));
assert(teletext.includes('fetchClientTeletextPage'));
assert(teletext.includes('pageNumber: 100'));
assert(teletext.includes('number < 100 || number > 899'));
assert(teletext.includes('number < 0 || number > 65534'));
assert(teletext.includes('cells.length === 1000'));
assert(teletext.includes('rowIndex < 25'));
assert(teletext.includes('column < 40'));
assert(teletext.includes("event.key === 'Escape'"));
assert(teletext.includes("'Unterseite Auto'"));
assert(teletext.includes("credentials: 'same-origin'"));
assert(teletext.includes("cache: 'no-store'"));
assert(teletext.includes("body.vdr-suite-teletext-open .vdr-suite-teletext-overlay"));
assert(teletext.includes("body.vdr-suite-teletext-open .vdr-suite-live-tv-player"));
assert(teletext.includes("right:32vw"));
assert(teletext.includes("width:32vw"));
assert(teletext.includes("right:30vw"));
assert(teletext.includes("width:30vw"));
assert(teletext.includes("max-height:66vh"));
assert(teletext.includes("setDesktopCompanionActive(true)"));
assert(teletext.includes("setDesktopCompanionActive(false)"));

assert(!teletext.includes('fetch('));
assert(!teletext.includes('TTXC 1'));
assert(!teletext.includes('TTXP 1'));
assert(!teletext.includes('/var/cache/vdr/vtx'));
assert(!teletext.includes('OsdTeletext::'));
assert(!teletext.includes('VdrSuitePlaybackShell'));
assert(!teletext.includes('createLivePanel'));
assert(!teletext.includes('.destroy('));
assert(!teletext.includes('appendChild(state.playback'));
assert(!teletext.includes('createMediaSession'));

assert(client.includes("function fetchClientTeletextService(options)"));
assert(client.includes("requestJson('/api/vdr/broadcast/teletext/service', options)"));
assert(client.includes("function fetchClientTeletextPage(options)"));
assert(client.includes("requestJson('/api/vdr/broadcast/teletext/page', options)"));
assert(client.includes('fetchClientTeletextService: fetchClientTeletextService'));
assert(client.includes('fetchClientTeletextPage: fetchClientTeletextPage'));

assert(live.includes('global.VdrSuiteTeletextView'));
assert(live.includes('teletext.createLauncher('));
assert(live.includes('head.appendChild(teletextButton)'));

const teletextPosition = index.indexOf('<script src="../frontend/teletext-view.js"></script>');
const livePosition = index.indexOf('<script src="../frontend/live-tv-view.js"></script>');
assert(teletextPosition >= 0);
assert(livePosition > teletextPosition);

assert(paths.includes('{"/frontend/teletext-view.js", "teletext-view.js", "application/javascript; charset=utf-8", nullptr}'));
assert(install.includes('web/frontend/teletext-view.js $(DESTDIR)$(DATADIR)/web/frontend/teletext-view.js'));
assert(install.includes('node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/teletext-view.js'));

console.log('Phase 67 Teletext frontend ownership and navigation contract ok');
