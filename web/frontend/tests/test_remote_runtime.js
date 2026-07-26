'use strict';

const fs = require('fs');
const path = require('path');
const vm = require('vm');
const assert = require('assert');

const clientSource = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'live-remote-client-api.js'),
  'utf8'
);
const source = fs.readFileSync(
  path.join(__dirname, '..', 'modules', 'remote.js'),
  'utf8'
);
const svg = fs.readFileSync(
  path.join(__dirname, '..', 'assets', 'vdr-remote-photorealistic.svg'),
  'utf8'
);

assert(!source.includes('/remote/seq'));
assert(!source.includes('/remote/kbd'));
assert(!source.includes('/osd'));
assert(!source.includes('restfulapi'));
assert(!source.includes('/api/vdr/remote/actions'));
assert(!source.includes('/api/vdr/live/overlay'));
assert(!source.includes('/api/vdr/live'));
assert(clientSource.includes("'/api/vdr/remote/actions'"));
assert(clientSource.includes("'/api/vdr/live/overlay'"));
assert(clientSource.includes("'/api/vdr/live'"));

assert(source.includes(
  "'/channel-logos/vdr-suite-brand/vdr-remote-photorealistic.svg'"
));
assert(source.includes("createElement('section', 'remote-stage')"));
assert(source.includes("createElement('img', 'remote-image')"));
assert(source.includes("createElement('button', 'remote-key')"));
assert(source.includes(
  "'Fotorealistische VDR-Fernbedienung'"
));
assert(source.includes(
  "'Groß · scrollbar · vollständig bedienbar'"
));

assert(source.includes('let actionInFlight = false'));
assert(source.includes("classList.add('is-pressed')"));
assert(source.includes("classList.remove('is-pressed')"));
assert(source.includes("classList.add('is-sending')"));
assert(source.includes("classList.remove('is-sending')"));
assert(source.includes('actionInFlight)'));
assert(!source.includes('lockControls(true)'));
assert(!source.includes('button.disabled = locked'));

assert(source.includes('.remote-key.is-pressed::before'));
assert(source.includes(
  '.remote-key:not(.is-disabled):active::before'
));
assert(source.includes(
  'transform: translateY(2px) scale(.985)'
));
assert(!source.includes('.remote-key:hover'));
assert(!source.includes('@media (hover: hover)'));
assert(!source.includes('element.title ='));

assert(source.includes(
  'width: min(27rem, calc(100vw - 1rem))'
));
assert(source.includes(
  'width: min(25rem, calc(100vw - .7rem))'
));
assert(source.includes('overflow-y: auto'));
assert(source.includes('scrollbar-gutter: stable'));
assert(!source.includes('max-height: 760px'));

assert(source.includes(
  "feature.classList.add('brand-feature-remote')"
));
assert(source.includes(
  "'VDR - Fernbedienung öffnen'"
));
assert(source.includes(
  "createElement('h2', '', 'VDR - Fernbedienung')"
));
assert(source.includes(
  "modal.setAttribute('aria-label', 'VDR - Fernbedienung')"
));
assert(source.includes(
  "title.textContent = 'VDR - Fernbedienung'"
));

[
  'INPUT',
  'SETUP',
  '>TV<',
  '>VCR<',
  '>DVD<',
  '>SAT<',
  '>AMP<',
  '>HI-FI<',
  '>MP3<',
  '>DVB-T<',
  '>AV<',
  '>GUIDE<'
].forEach(function (unsupportedLabel) {
  assert(
    !svg.includes(unsupportedLabel),
    'unsupported decorative key remains in SVG: ' + unsupportedLabel
  );
});
assert(svg.includes('>MENU<'));
assert(svg.includes('>INFO<'));
assert(svg.includes('>BACK<'));
assert(svg.includes('>MUTE<'));
assert(svg.includes('>PREV<'));
assert(svg.includes('>NEXT<'));
assert(svg.includes('>PLAY<'));
assert(svg.includes('>PAUSE<'));
assert(svg.includes('>REMOTE CONTROL<'));

const serverPaths = fs.readFileSync(
  path.join(
    __dirname,
    '..',
    '..',
    '..',
    'core',
    'http',
    'src',
    'TestHttpServerPaths.inc'
  ),
  'utf8'
);
const liveRemoteMake = fs.readFileSync(
  path.join(__dirname, '..', '..', '..', 'mk', 'live-remote.mk'),
  'utf8'
);
assert(serverPaths.includes(
  '{"/frontend/app.js", "app.js", "application/javascript; charset=utf-8", "modules/remote.js"}'
));
assert(serverPaths.includes(
  '{"/frontend/api/client-api.js", "api/client-api.js", "application/javascript; charset=utf-8", "api/live-remote-client-api.js"}'
));
assert(liveRemoteMake.includes('web/frontend/modules/remote.js'));
assert(liveRemoteMake.includes(
  'web/frontend/api/live-remote-client-api.js'
));
assert(liveRemoteMake.includes(
  'web/frontend/assets/vdr-remote-photorealistic.svg'
));
assert(liveRemoteMake.includes(
  'channel-logos/vdr-suite-brand/vdr-remote-photorealistic.svg'
));

const document = {
  head: {appendChild: function () {}},
  body: {append: function () {}},
  createElement: function (tag) {
    return {
      tagName: tag.toUpperCase(),
      dataset: {},
      style: {},
      classList: {
        add: function () {},
        remove: function () {},
        toggle: function () {}
      },
      setAttribute: function () {},
      getAttribute: function () { return null; },
      removeAttribute: function () {},
      appendChild: function () {},
      append: function () {},
      replaceChildren: function () {},
      addEventListener: function () {},
      querySelector: function () { return null; },
      querySelectorAll: function () { return []; }
    };
  },
  querySelector: function () { return null; },
  querySelectorAll: function () { return []; },
  getElementById: function () { return null; },
  addEventListener: function () {}
};

const requests = [];
const baseApi = Object.freeze({
  requestJson: function (url, options) {
    requests.push({url: url, options: options});
    return Promise.resolve({success: true});
  },
  fetchClientBackends: function () {
    return Promise.resolve({backends: []});
  }
});

const context = {
  window: null,
  document: document,
  console: console,
  Date: Date,
  JSON: JSON,
  Object: Object,
  Promise: Promise,
  Set: Set,
  URLSearchParams: URLSearchParams,
  setTimeout: setTimeout,
  clearTimeout: clearTimeout,
  VdrSuiteClientApi: baseApi
};
context.window = context;
vm.createContext(context);
vm.runInContext(clientSource, context);
vm.runInContext(source, context);

assert(context.VdrSuiteRemote);
assert.strictEqual(
  context.VdrSuiteRemote.remoteImagePath,
  '/channel-logos/vdr-suite-brand/vdr-remote-photorealistic.svg'
);

const expectedActions = [
  'up', 'down', 'left', 'right', 'ok', 'back', 'menu', 'info',
  'red', 'green', 'yellow', 'blue',
  'zero', 'one', 'two', 'three', 'four', 'five', 'six', 'seven',
  'eight', 'nine',
  'channelUp', 'channelDown', 'volumeUp', 'volumeDown', 'mute',
  'play', 'pause', 'stop', 'record', 'fastForward', 'rewind',
  'next', 'previous', 'switchChannel'
];
assert.deepStrictEqual(
  Array.from(context.VdrSuiteRemote.actions),
  expectedActions
);

const hotspotActions = Array.from(
  context.VdrSuiteRemote.hotspotActions
);
assert.strictEqual(hotspotActions.length, 35);
assert.strictEqual(new Set(hotspotActions).size, 35);
assert.deepStrictEqual(
  hotspotActions.slice().sort(),
  expectedActions
    .filter(function (action) { return action !== 'switchChannel'; })
    .sort()
);

assert.strictEqual(
  typeof context.VdrSuiteClientApi.fetchClientRemoteAction,
  'function'
);
assert.strictEqual(
  typeof context.VdrSuiteClientApi.fetchClientLiveOverlay,
  'function'
);
assert.strictEqual(
  context.VdrSuiteRemote.canControlBackend({
    enabled: true,
    online: true,
    frontendSelector: {canWrite: false},
    capabilities: {remoteControl: true}
  }),
  false
);
assert.strictEqual(
  context.VdrSuiteRemote.canControlBackend({
    enabled: true,
    online: true,
    frontendSelector: {canWrite: true},
    capabilities: {remoteControl: true}
  }),
  true
);

context.VdrSuiteClientApi.fetchClientRemoteAction({
  payload: {
    backendId: 'default',
    operationId: 'remote-1',
    action: 'ok'
  }
});
context.VdrSuiteClientApi.fetchClientLiveOverlay({
  backendId: 'default'
});

assert.strictEqual(requests[0].url, '/api/vdr/remote/actions');
assert.strictEqual(requests[0].options.method, 'POST');
assert.strictEqual(requests[1].url, '/api/vdr/live/overlay');

console.log('remote frontend contract ok');
