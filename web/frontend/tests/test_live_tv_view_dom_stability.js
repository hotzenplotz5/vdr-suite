'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const frontend = path.resolve(__dirname, '..');
const source = fs.readFileSync(path.join(frontend, 'live-tv-view.js'), 'utf8');
const indexSource = fs.readFileSync(path.join(frontend, 'index.html'), 'utf8');
const phase65Source = fs.readFileSync(path.resolve(frontend, '..', '..', 'mk', 'phase65-media-tests.mk'), 'utf8');

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

const liveViewPosition = indexSource.indexOf('<script src="../frontend/live-tv-view.js"></script>');
const compatPosition = indexSource.indexOf('<script src="../frontend/channel-day-program-compat.js"></script>');
assert.ok(liveViewPosition >= 0, 'index must load the stable Live-TV product runtime');
assert.ok(compatPosition > liveViewPosition, 'stable Live-TV runtime must preempt the compat fallback');

assert.ok(
  phase65Source.includes('install-runtime: install-phase65d1-live-tv-view'),
  'install-runtime must depend on the stable Live-TV runtime install target'
);
assert.ok(
  phase65Source.includes('web/frontend/live-tv-view.js \\\n\t\t$(DESTDIR)$(DATADIR)/web/frontend/live-tv-view.js'),
  'Phase-65 install target must deploy the stable Live-TV runtime'
);
assert.ok(
  phase65Source.includes('test -f /tmp/vdr-suite-live-tv-view-install/usr/share/vdr-suite/web/frontend/live-tv-view.js'),
  'install staging must assert the stable Live-TV runtime is packaged'
);

console.log('Live-TV mounted player DOM stability contract ok');
