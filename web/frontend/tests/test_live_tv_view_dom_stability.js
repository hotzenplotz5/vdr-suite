'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const frontend = path.resolve(__dirname, '..');
const source = fs.readFileSync(path.join(frontend, 'live-tv-view.js'), 'utf8');
const recordingsSource = fs.readFileSync(path.join(frontend, 'recordings2.js'), 'utf8');
const indexSource = fs.readFileSync(path.join(frontend, 'index.html'), 'utf8');
const installSource = fs.readFileSync(path.resolve(frontend, '..', '..', 'mk', 'install.mk'), 'utf8');

const renderStart = source.indexOf('function render()');
const renderEnd = source.indexOf('\n  function applyChannels', renderStart);
assert.ok(renderStart >= 0 && renderEnd > renderStart, 'Live-TV render function must exist');
const renderSource = source.slice(renderStart, renderEnd);
const guardPosition = renderSource.indexOf('if (playbackMountedIn(mount)) return;');
const replacePosition = renderSource.indexOf('mount.replaceChildren();');
assert.ok(guardPosition >= 0, 'mounted playback must short-circuit ordinary full renders');
assert.ok(replacePosition > guardPosition, 'mounted-player guard must run before replaceChildren');
assert.ok(source.includes('mount.contains(element)'), 'stability guard must verify the exact playback element is already mounted');

const createPlaybackStart = source.indexOf('function createPlayback(');
const startChannelStart = source.indexOf('\n  function startChannel(', createPlaybackStart);
assert.ok(createPlaybackStart >= 0 && startChannelStart > createPlaybackStart);
const createPlaybackSource = source.slice(createPlaybackStart, startChannelStart);
assert.ok(
  createPlaybackSource.includes("if (state.active && sequence === state.switchSequence) scrollPlayerIntoView();"),
  'successful media-session startup may scroll the existing player without re-rendering it'
);
assert.ok(
  !createPlaybackSource.includes("if (state.active && sequence === state.switchSequence) {\n        render();"),
  'successful startup must not disconnect/reinsert the just-started media element'
);

const ensurePlaybackStart = recordingsSource.indexOf('function ensurePlaybackRuntime()');
const requestFolderStart = recordingsSource.indexOf('\n  function requestFolder(', ensurePlaybackStart);
assert.ok(ensurePlaybackStart >= 0 && requestFolderStart > ensurePlaybackStart, 'Recordings2 playback loader must exist');
const ensurePlaybackSource = recordingsSource.slice(ensurePlaybackStart, requestFolderStart);
const deferredPlaybackPosition = ensurePlaybackSource.indexOf("'/frontend/recordings2-playback.js'");
const lateShellInstallPosition = ensurePlaybackSource.indexOf(
  'global.VdrSuitePlaybackShell.install();',
  deferredPlaybackPosition
);
assert.ok(deferredPlaybackPosition >= 0, 'Recordings2 must defer-load the shared playback runtime');
assert.ok(
  lateShellInstallPosition > deferredPlaybackPosition,
  'persistent playback shell must bind after the deferred playback owner becomes available'
);
assert.ok(
  ensurePlaybackSource.indexOf('global.VdrSuitePlaybackShell.install();') >= 0,
  'already-loaded playback owners must also be adopted by the persistent shell'
);

const liveViewPosition = indexSource.indexOf('<script src="../frontend/live-tv-view.js"></script>');
const compatPosition = indexSource.indexOf('<script src="../frontend/channel-day-program-compat.js"></script>');
assert.ok(liveViewPosition >= 0, 'index must load the stable Live-TV product runtime');
assert.ok(compatPosition > liveViewPosition, 'stable Live-TV runtime must preempt the compat fallback');

assert.ok(
  installSource.includes('web/frontend/live-tv-view.js $(DESTDIR)$(DATADIR)/web/frontend/live-tv-view.js'),
  'install-runtime must deploy the stable Live-TV runtime directly'
);
assert.ok(
  installSource.includes('test -f /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/live-tv-view.js'),
  'install staging must assert the stable Live-TV runtime is packaged'
);
assert.ok(
  installSource.includes('node --check /tmp/vdr-suite-pkgroot/usr/share/vdr-suite/web/frontend/live-tv-view.js'),
  'install staging must syntax-check the stable Live-TV runtime'
);

console.log('Live-TV mounted player DOM stability and deferred shell binding contract ok');
