'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

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
assert.ok(
  createPlaybackSource.includes("state.liveError = liveErrorForChannel(error, channel, 'Live-TV konnte nicht gestartet werden.');"),
  'failed Live-TV startup must classify the error with the selected channel metadata in the owning view'
);
assert.ok(
  source.includes("const meta = addText(doc.createElement('span'), channelAvailabilityText(channel));"),
  'Live-TV tiles must expose known encryption state before playback starts'
);

const shellHelperStart = recordingsSource.indexOf('function installPlaybackShell()');
const ensurePlaybackStart = recordingsSource.indexOf('function ensurePlaybackRuntime()');
const requestFolderStart = recordingsSource.indexOf('\n  function requestFolder(', ensurePlaybackStart);
assert.ok(shellHelperStart >= 0 && ensurePlaybackStart > shellHelperStart, 'Recordings2 must expose the local shell-bind helper before its playback loader');
assert.ok(
  recordingsSource.slice(shellHelperStart, ensurePlaybackStart).includes('global.VdrSuitePlaybackShell') &&
    recordingsSource.slice(shellHelperStart, ensurePlaybackStart).includes('shell.install()'),
  'shell-bind helper must install the persistent shell when it becomes available'
);
assert.ok(ensurePlaybackStart >= 0 && requestFolderStart > ensurePlaybackStart, 'Recordings2 playback loader must exist');
const ensurePlaybackSource = recordingsSource.slice(ensurePlaybackStart, requestFolderStart);
const deferredPlaybackPosition = ensurePlaybackSource.indexOf("'/frontend/recordings2-playback.js'");
const lateShellInstallPosition = ensurePlaybackSource.indexOf('installPlaybackShell();', deferredPlaybackPosition);
assert.ok(deferredPlaybackPosition >= 0, 'Recordings2 must defer-load the shared playback runtime');
assert.ok(
  lateShellInstallPosition > deferredPlaybackPosition,
  'persistent playback shell must bind after the deferred playback owner becomes available'
);
assert.ok(
  ensurePlaybackSource.indexOf('installPlaybackShell();') >= 0,
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

function testNode() {
  return {
    id: '',
    className: '',
    dataset: {},
    style: {},
    textContent: '',
    hidden: false,
    appendChild() {},
    setAttribute() {},
    addEventListener() {},
    classList: {add() {}, remove() {}}
  };
}

const document = {
  readyState: 'complete',
  head: {appendChild() {}},
  createElement() { return testNode(); },
  getElementById() { return null; },
  querySelector() { return null; },
  querySelectorAll() { return []; },
  addEventListener() {}
};
const window = {document};

vm.runInNewContext(source, {
  window,
  document,
  Object,
  String,
  Number,
  Array,
  Boolean,
  Promise,
  RegExp,
  Error,
  Date,
  Math
}, {filename: 'live-tv-view.js'});

assert.ok(window.VdrSuiteLiveTvView);
const liveTest = window.VdrSuiteLiveTvView.__test;
assert.strictEqual(liveTest.channelHasEncryptionInfo({encrypted: true}), true);
assert.strictEqual(liveTest.channelHasEncryptionInfo({encrypted: false}), true);
assert.strictEqual(liveTest.channelHasEncryptionInfo({caids: ['09C7']}), true);
assert.strictEqual(liveTest.channelHasEncryptionInfo({}), false);
assert.strictEqual(liveTest.channelIsEncrypted({encrypted: true}), true);
assert.strictEqual(liveTest.channelIsEncrypted({encrypted: false}), false);
assert.strictEqual(liveTest.channelIsEncrypted({encrypted: 'false'}), false);
assert.strictEqual(liveTest.channelIsEncrypted({scrambled: '1'}), true);
assert.strictEqual(liveTest.channelIsEncrypted({caids: ['09C7', '09EF']}), true);
assert.strictEqual(liveTest.channelIsEncrypted({caids: []}), false);
assert.strictEqual(
  liveTest.channelAvailabilityText({number: 46, encrypted: true, enabled: true}),
  'Kanal 46 · verschlüsselt'
);
assert.strictEqual(
  liveTest.channelAvailabilityText({number: 1, encrypted: false, enabled: true}),
  'Kanal 1 · frei'
);
assert.strictEqual(
  liveTest.channelAvailabilityText({number: 2, enabled: true}),
  'Kanal 2 · verfügbar'
);
assert.strictEqual(
  liveTest.channelAvailabilityText({number: 3, encrypted: true, enabled: false}),
  'Kanal 3 · deaktiviert'
);

const encryptedError = liveTest.liveErrorForChannel(
  new Error('live_source_receiver_unavailable'),
  {name: 'Sky Test', encrypted: true},
  'fallback'
);
assert.ok(encryptedError.includes('Sky Test'));
assert.ok(encryptedError.includes('verschlüsselt'));
assert.ok(encryptedError.includes('VDR konnte aktuell keinen Live-Empfang'));
assert.ok(!encryptedError.includes('live_source_receiver_unavailable'));
assert.strictEqual(
  liveTest.liveErrorForChannel(
    new Error('live_source_receiver_unavailable'),
    {name: 'Das Erste HD', encrypted: false},
    'fallback'
  ),
  'live_source_receiver_unavailable'
);
assert.strictEqual(
  liveTest.liveErrorForChannel(
    new Error('Security accountability persistence is unavailable'),
    {name: 'Sky Test', encrypted: true},
    'fallback'
  ),
  'Security accountability persistence is unavailable'
);

console.log('Live-TV mounted player stability, encryption context and deferred shell binding contract ok');
