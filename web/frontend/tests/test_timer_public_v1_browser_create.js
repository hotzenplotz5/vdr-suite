'use strict';

// F5.6 production-path test: real Timer module form -> browser client API ->
// durable Public-v1 Operation -> SSE readback, without a legacy CREATE.
const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

class Element {
  constructor(tag) {
    this.tagName = tag;
    this.children = [];
    this.listeners = {};
    this.attributes = {};
    this.dataset = {};
    this.classList = {add() {}};
    this.value = '';
    this.checked = false;
    this.disabled = false;
  }
  appendChild(item) {
    this.children.push(item);
    item.parent = this;
    return item;
  }
  replaceChildren(...items) {
    this.children = [];
    items.forEach(item => this.appendChild(item));
  }
  addEventListener(name, callback) { this.listeners[name] = callback; }
  setAttribute(name, value) { this.attributes[name] = String(value); }
  get elements() {
    const result = {};
    walk(this, item => {
      if (item.name && !Object.prototype.hasOwnProperty.call(result, item.name))
        result[item.name] = item;
    });
    return result;
  }
  querySelectorAll(selector) {
    if (selector !== 'input[name="weekday"]:checked')
      throw new Error('Unexpected selector: ' + selector);
    const selected = [];
    walk(this, item => {
      if (item.tagName === 'input' && item.name === 'weekday' && item.checked)
        selected.push(item);
    });
    return selected;
  }
}
function walk(root, visitor) {
  visitor(root);
  root.children.forEach(item => walk(item, visitor));
}
function find(root, condition) {
  let result = null;
  walk(root, item => {
    if (!result && condition(item)) result = item;
  });
  return result;
}
const root = new Element('main');
const calls = [];
const legacy = [];
const reads = [];
const sources = [];
let operationState = 'accepted';
let failManual = false;
let reloads = 0;
let randomSeed = 0;
const clientApi = {
  fetchPublicManualTimerCreate(input) {
    calls.push(input);
    if (failManual) return Promise.reject(new Error('backend unavailable'));
    return Promise.resolve({
      status: 202, location: '/api/v1/operations/op_manual_f56',
      data: {operationId: 'op_manual_f56', backendId: 'living-room',
        state: 'accepted', links: {self: '/api/v1/operations/op_manual_f56'}}
    });
  },
  fetchPublicNativeTimerOperation(input) {
    reads.push(input);
    return Promise.resolve({status: 200, data: {
      operationId: 'op_manual_f56', state: operationState,
      backendId: 'living-room'
    }});
  },
  createClientOperationUpdateSource(operationId) {
    assert.strictEqual(operationId, 'op_manual_f56');
    const source = {
      listeners: {}, closed: false,
      addEventListener(name, callback) { this.listeners[name] = callback; },
      emit(name, data) {
        if (this.listeners[name])
          this.listeners[name]({data: JSON.stringify(data)});
      },
      close() { this.closed = true; }
    };
    sources.push(source);
    return source;
  },
  fetchClientTimerCreateAction(options) {
    legacy.push(options.payload);
    return Promise.resolve({success: true});
  }
};
const contextWindow = {
  VdrSuiteClientApi: clientApi,
  crypto: {getRandomValues(bytes) {
    randomSeed++;
    bytes.fill(randomSeed);
    return bytes;
  }},
  setTimeout() {},
  VdrSuitePlatform: {
    hasModule() { return false; },
    registerModule() {}
  }
};
const document = {
  head: {appendChild() {}},
  createElement(tag) { return new Element(tag); },
  getElementById() { return null; }
};
const file = path.join(__dirname, '..', 'modules', 'timers.js');
vm.runInNewContext(fs.readFileSync(file, 'utf8'), {
  window: contextWindow, document, Date, Array, Boolean, Math,
  Number, Object, Promise, Set, String, Uint8Array
}, {filename: file});
const browser = contextWindow.VdrSuiteTimerBrowser;
assert.ok(browser);
browser.configureContext({
  detailDataElement: root, clientApi,
  getSelectedBackendId() { return 'living-room'; },
  reload() { reloads++; }
});
browser.renderList({timers: []});
const form = find(root, item => item.tagName === 'form');
assert.ok(form, 'real create form must be reachable from rendered list');
const fields = form.elements;
fields.title.value = 'One-shot news';
fields.manualChannelId.value = 'S19.2E-1-2-3';
fields.day.value = '2026-11-25';
fields.start.value = '20:15';
fields.stop.value = '20:45';
fields.manualDirectory.value = 'News~Evening';
fields.aux.value = '';
const feedback = find(form, item => item.className === 'timer-feedback');
assert.ok(feedback);
function submit() {
  assert.strictEqual(typeof form.listeners.submit, 'function');
  form.listeners.submit({preventDefault() {}});
}
const tick = () => new Promise(resolve => setImmediate(resolve));
(async function () {
  submit();
  await tick();
  assert.strictEqual(calls.length, 1, 'one Public-v1 CREATE dispatch');
  assert.strictEqual(legacy.length, 0, 'no legacy CREATE for one-shot');
  assert.strictEqual(calls[0].payload.backendId, 'living-room');
  assert.strictEqual(calls[0].payload.title, 'One-shot news');
  assert.strictEqual(calls[0].payload.directory, 'News~Evening');
  assert.strictEqual(calls[0].payload.startAt,
    new Date(2026, 10, 25, 20, 15).getTime() / 1000);
  assert.strictEqual(calls[0].payload.stopAt,
    new Date(2026, 10, 25, 20, 45).getTime() / 1000);
  assert.strictEqual(Object.prototype.hasOwnProperty.call(calls[0].payload, 'aux'), false);
  assert(calls[0].idempotencyKey.startsWith('manual-web-'));
  assert.strictEqual(reads.length, 1, 'initial durable Operation readback');
  assert.strictEqual(reloads, 0, 'HTTP 202 is not Timer success');

  operationState = 'outcome_unknown';
  sources[0].emit('operation', {operationId: 'op_manual_f56'});
  await tick();
  assert(feedback.textContent.includes('ungewiss'));
  assert.strictEqual(reloads, 0);
  assert.strictEqual(calls.length, 1, 'unknown outcome never resubmits CREATE');

  operationState = 'succeeded';
  sources[0].emit('operation', {operationId: 'op_manual_f56'});
  await tick();
  assert.strictEqual(reloads, 1, 'confirmed native success reloads Timer list');
  assert.strictEqual(sources[0].closed, true, 'completed SSE stream is closed');

  // A new explicit create after confirmed success receives a new key.
  submit();
  await tick();
  assert.strictEqual(calls.length, 2);
  assert.notStrictEqual(calls[1].idempotencyKey, calls[0].idempotencyKey);
  assert.strictEqual(legacy.length, 0);

  // A transport/backend failure never falls back to legacy CREATE.
  failManual = true;
  fields.title.value = 'No backend';
  submit();
  await tick();
  assert.strictEqual(calls.length, 3);
  assert.strictEqual(legacy.length, 0);
  assert(feedback.textContent.includes('nicht bestätigt'));
  const firstFailedKey = calls[2].idempotencyKey;
  submit();
  await tick();
  assert.strictEqual(calls.length, 4, 'only explicit submission may resend');
  assert.strictEqual(calls[3].idempotencyKey, firstFailedKey,
    'an uncertain retry must keep its original idempotency key');
  assert.strictEqual(legacy.length, 0, 'no legacy retry after Public-v1 error');
  failManual = false;

  // Repeating timers and opaque Aux must remain functional, not lose fields.
  const weekday = find(form, item => item.name === 'weekday');
  weekday.checked = true;
  fields.aux.value = '';
  submit();
  await tick();
  assert.strictEqual(calls.length, 4);
  assert.strictEqual(legacy.length, 1);
  assert.strictEqual(legacy[0].weekdays, 'M------');

  weekday.checked = false;
  fields.aux.value = '<epgsearch/>';
  submit();
  await tick();
  assert.strictEqual(calls.length, 4);
  assert.strictEqual(legacy.length, 2);
  assert.strictEqual(legacy[1].aux, '<epgsearch/>');
  // F5.6 production composition: same rendered Timer form, actual
  // browser client-api.js and bundled Public-v1 reference client.
  const httpRequests = [];
  const productionWindow = {
    location: {origin: 'https://suite.example'},
    VdrSuiteBrowserSession: {
      csrfHeaders() { return {'X-CSRF-Token': 'composition-csrf'}; }
    }
  };
  const httpContext = {
    window: productionWindow, Headers, URL, URLSearchParams, TextEncoder,
    Uint8Array, Array, Object, String, Boolean, Number, JSON, Promise,
    fetch(url, options) {
      httpRequests.push({url: String(url), options});
      if (String(url) === 'https://suite.example/api/v1/manual/timer-intents')
        return Promise.resolve({
          status: 202, ok: true,
          headers: {get(name) {
            return name === 'Location' ? '/api/v1/operations/op_manual_f56' : null;
          }},
          text: async () => JSON.stringify({
            operationId: 'op_manual_f56', state: 'accepted',
            backendId: 'living-room',
            links: {self: '/api/v1/operations/op_manual_f56'}
          })
        });
      if (String(url) === 'https://suite.example/api/v1/operations/op_manual_f56')
        return Promise.resolve({
          status: 200, ok: true,
          headers: {get() { return null; }},
          text: async () => JSON.stringify({
            operationId: 'op_manual_f56', state: 'succeeded',
            backendId: 'living-room'
          })
        });
      throw new Error('Unexpected production request: ' + String(url));
    }
  };
  // In a browser window.fetch is the same transport as global fetch.
  productionWindow.fetch = httpContext.fetch;
  const reference = path.join(__dirname, '..', '..', '..', 'clients',
    'reference-js', 'public-v1-client.js');
  const browserClient = path.join(__dirname, '..', 'api', 'client-api.js');
  vm.runInNewContext(fs.readFileSync(reference, 'utf8'), httpContext,
    {filename: reference});
  vm.runInNewContext(fs.readFileSync(browserClient, 'utf8'), httpContext,
    {filename: browserClient});
  const realClient = Object.assign({}, productionWindow.VdrSuiteClientApi, {
    createClientOperationUpdateSource:
      clientApi.createClientOperationUpdateSource
  });
  browser.configureContext({
    detailDataElement: root, clientApi: realClient,
    getSelectedBackendId() { return 'living-room'; },
    reload() { reloads++; }
  });
  fields.title.value = 'Composition news';
  fields.aux.value = '';
  operationState = 'accepted';
  submit();
  await tick();
  assert.strictEqual(httpRequests[0].url,
    'https://suite.example/api/v1/manual/timer-intents');
  assert.strictEqual(httpRequests[0].options.method, 'POST');
  assert.strictEqual(httpRequests[0].options.credentials, 'same-origin');
  assert.strictEqual(httpRequests[0].options.headers['X-CSRF-Token'],
    'composition-csrf');
  assert(httpRequests[0].options.headers['Idempotency-Key'].startsWith('manual-web-'));
  assert.strictEqual(httpRequests[0].options.headers['If-Match'], undefined);
  const submitted = JSON.parse(httpRequests[0].options.body);
  assert.strictEqual(submitted.title, 'Composition news');
  assert.strictEqual(submitted.backendId, 'living-room');
  assert.strictEqual(submitted.directory, 'News~Evening');
  assert.strictEqual(httpRequests[1].url,
    'https://suite.example/api/v1/operations/op_manual_f56');
  assert.strictEqual(httpRequests[1].options.method, 'GET');
  assert.strictEqual(httpRequests.length, 2);
  assert.strictEqual(legacy.length, 2,
    'real Browser+reference composition must not call legacy CREATE');

  console.log('Timer browser F5.6 one-shot Public-v1 Operation/SSE integration PASS');
}()).catch(error => { console.error(error); process.exitCode = 1; });
