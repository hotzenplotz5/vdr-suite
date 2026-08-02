'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const runtime = fs.readFileSync(
  require('path').join(__dirname, '..', 'platform', 'public-url.js'),
  'utf8'
);

function propertyConstructor() {
  function Constructor() {}
  Object.defineProperty(Constructor.prototype, 'src', {
    configurable: true,
    enumerable: true,
    get() { return this._src || ''; },
    set(value) { this._src = String(value); }
  });
  Object.defineProperty(Constructor.prototype, 'href', {
    configurable: true,
    enumerable: true,
    get() { return this._href || ''; },
    set(value) { this._href = String(value); }
  });
  Object.defineProperty(Constructor.prototype, 'poster', {
    configurable: true,
    enumerable: true,
    get() { return this._poster || ''; },
    set(value) { this._poster = String(value); }
  });
  return Constructor;
}

function createContext(scriptPath) {
  const calls = {fetch: [], eventSource: []};
  const location = new URL('https://yavdr' + scriptPath.replace('/frontend/platform/public-url.js', '/frontend/'));

  function Element() {}
  Element.prototype.setAttribute = function (name, value) {
    this.attributes = this.attributes || {};
    this.attributes[name] = String(value);
  };

  function CSSStyleDeclaration() {}
  CSSStyleDeclaration.prototype.setProperty = function (name, value, priority) {
    this.values = this.values || {};
    this.values[name] = {value: String(value), priority: priority || ''};
  };
  ['background', 'backgroundImage', 'borderImage', 'content', 'cursor', 'listStyle', 'listStyleImage', 'mask', 'maskImage'].forEach(name => {
    Object.defineProperty(CSSStyleDeclaration.prototype, name, {
      configurable: true,
      enumerable: true,
      get() { return this['_' + name] || ''; },
      set(value) { this['_' + name] = String(value); }
    });
  });

  function EventSource(url, options) {
    calls.eventSource.push({url: String(url), options});
    this.url = String(url);
  }

  const Constructor = propertyConstructor();
  const context = {
    URL,
    location,
    document: {currentScript: {src: 'https://yavdr' + scriptPath}},
    fetch(input, init) {
      calls.fetch.push({input, init});
      return Promise.resolve({ok: true});
    },
    EventSource,
    Element,
    CSSStyleDeclaration,
    HTMLScriptElement: Constructor,
    HTMLImageElement: Constructor,
    HTMLSourceElement: Constructor,
    HTMLVideoElement: Constructor,
    HTMLAudioElement: Constructor,
    HTMLLinkElement: Constructor,
    HTMLAnchorElement: Constructor,
    HTMLIFrameElement: Constructor,
    Object,
    TypeError,
    Error,
    console
  };
  context.window = context;
  vm.createContext(context);
  vm.runInContext(runtime, context, {filename: 'public-url.js'});
  return {context, calls, Constructor, CSSStyleDeclaration, Element};
}

function assertThrowsPath(api, path) {
  assert.throws(() => api.resolvePath(path), TypeError);
}

async function testPublicPrefix() {
  const fixture = createContext('/vdr-suite/frontend/platform/public-url.js');
  const api = fixture.context.VdrSuitePublicUrl;

  assert.strictEqual(api.basePath, '/vdr-suite');
  assert.strictEqual(api.resolvePath('/api/vdr/status'), '/vdr-suite/api/vdr/status');
  assert.strictEqual(
    api.resolvePath('/recording-artwork/poster.jpg?size=large#image'),
    '/vdr-suite/recording-artwork/poster.jpg?size=large#image'
  );
  assert.strictEqual(
    api.resolvePath('/api/search?q=ARD%2FZDF#result%2F1'),
    '/vdr-suite/api/search?q=ARD%2FZDF#result%2F1'
  );
  assert(Object.isFrozen(api));
  assert.strictEqual(Object.getOwnPropertyDescriptor(fixture.context, 'VdrSuitePublicUrl').writable, false);

  [
    '', 'api/vdr/status', '//other.example/api', '/unknown/path',
    '/api/../secret', '/api/%2e%2e/secret', '/api/%2fsecret',
    '/api\\secret', '/vdr-suite/api/vdr/status'
  ].forEach(path => assertThrowsPath(api, path));

  await fixture.context.fetch('/api/vdr/status?backend=default');
  assert.strictEqual(fixture.calls.fetch[0].input, '/vdr-suite/api/vdr/status?backend=default');

  await fixture.context.fetch('/api/search?q=ARD%2FZDF');
  assert.strictEqual(fixture.calls.fetch[1].input, '/vdr-suite/api/search?q=ARD%2FZDF');

  await fixture.context.fetch('https://example.invalid/api/vdr/status');
  assert.strictEqual(fixture.calls.fetch[2].input, 'https://example.invalid/api/vdr/status');

  const source = new fixture.context.EventSource('/api/vdr/live', {withCredentials: true});
  assert.strictEqual(source.url, '/vdr-suite/api/vdr/live');

  const image = new fixture.context.HTMLImageElement();
  image.src = '/channel-logos/zdf.png';
  assert.strictEqual(image._src, '/vdr-suite/channel-logos/zdf.png');

  const script = new fixture.context.HTMLScriptElement();
  script.src = '/frontend/modules/genres.js?late=1';
  assert.strictEqual(script._src, '/vdr-suite/frontend/modules/genres.js?late=1');

  const element = new fixture.context.Element();
  element.setAttribute('src', '/recording-artwork/poster.jpg');
  assert.strictEqual(element.attributes.src, '/vdr-suite/recording-artwork/poster.jpg');

  const style = new fixture.context.CSSStyleDeclaration();
  style.backgroundImage = 'url("/channel-logos/vdr-suite-brand/recording-genre-action.svg")';
  assert.strictEqual(
    style._backgroundImage,
    'url("/vdr-suite/channel-logos/vdr-suite-brand/recording-genre-action.svg")'
  );
  style.setProperty('background-image', 'url(/recording-artwork/poster.jpg)');
  assert.strictEqual(
    style.values['background-image'].value,
    'url("/vdr-suite/recording-artwork/poster.jpg")'
  );
}

async function testDirectDaemon() {
  const fixture = createContext('/frontend/platform/public-url.js');
  const api = fixture.context.VdrSuitePublicUrl;
  assert.strictEqual(api.basePath, '');
  assert.strictEqual(api.resolvePath('/api/vdr/status'), '/api/vdr/status');

  await fixture.context.fetch('/api/vdr/status');
  assert.strictEqual(fixture.calls.fetch[0].input, '/api/vdr/status');

  const image = new fixture.context.HTMLImageElement();
  image.src = '/channel-logos/zdf.png';
  assert.strictEqual(image._src, '/channel-logos/zdf.png');
}

Promise.resolve()
  .then(testPublicPrefix)
  .then(testDirectDaemon)
  .then(() => console.log('public URL runtime tests ok'))
  .catch(error => {
    console.error(error);
    process.exitCode = 1;
  });
