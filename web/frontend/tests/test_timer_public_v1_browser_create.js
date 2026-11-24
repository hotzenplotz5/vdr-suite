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
    assert(['op_manual_f56', 'op_native_f56'].includes(operationId));
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
  confirm() { return true; }, // Browser confirmation for explicit delete fixture
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

  // An explicitly selected first Monday can now go via the same native
  // Public-v1 CREATE Operation, without a legacy timer write.
  fields.day.value = '2026-11-30';
  fields.aux.value = '';
  submit();
  await tick();
  assert.strictEqual(calls.length, 5);
  assert.strictEqual(legacy.length, 1);
  assert.strictEqual(calls[4].payload.weekdays, 'M------');
  assert.strictEqual(calls[4].payload.startAt,
    new Date(2026, 10, 30, 20, 15).getTime()/1000);

  weekday.checked = false;
  fields.day.value = '2026-11-25';
  fields.aux.value = '<epgsearch/>';
  submit();
  await tick();
  assert.strictEqual(calls.length, 6);
  assert.strictEqual(legacy.length, 1);
  assert.strictEqual(calls[5].payload.aux, '<epgsearch/>');
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
      if (String(url) === 'https://suite.example/api/v1/native-timers/17?backendId=living-room')
        return Promise.resolve({
          status: 200, ok: true,
          headers: {get(name) {
            return name === 'ETag' ? '"native-timer-binding/1:fixture:2:3"' : null;
          }},
          text: async () => JSON.stringify({
            backendId: 'living-room', timerId: '17', bindingState: nativeBindingState,
            directActions: nativeDirectAvailable
          })
        });
      if (String(url) ===
          'https://suite.example/api/v1/native-timers/17/modifications?backendId=living-room')
        return Promise.resolve({
          status: 202, ok: true,
          headers: {get(name) {
            return name === 'Location' ? '/api/v1/operations/op_native_f56' : null;
          }},
          text: async () => JSON.stringify({
            operation: {operationId: 'op_native_f56', state: 'accepted'}
          })
        });
      if (String(url) === 'https://suite.example/api/v1/operations/op_native_f56')
        return Promise.resolve({
          status: 200, ok: true,
          headers: {get() {return null;}},
          text: async () => JSON.stringify({
            operationId: 'op_native_f56', state: 'succeeded',
            backendId: 'living-room'
          })
        });
      if (String(url) ===
          'https://suite.example/api/v1/native-timers/17/direct-update?backendId=living-room' ||
          String(url) ===
          'https://suite.example/api/v1/native-timers/17/direct-delete?backendId=living-room')
        return Promise.resolve({
          status: 200, ok: true, headers: {get() {return null;}},
          text: async () => JSON.stringify({success: true})
        });
      if (String(url) === '/api/vdr/timers/actions/update' ||
          String(url) === '/api/vdr/timers/actions/delete' ||
          String(url) === 'https://suite.example/api/vdr/timers/actions/update' ||
          String(url) === 'https://suite.example/api/vdr/timers/actions/delete')
        return Promise.resolve({
          status: 200, ok: true, headers: {get() {return null;}},
          text: async () => JSON.stringify({success: true})
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
  let nativeBindingState = 'verified';
  let nativeDirectAvailable = false;
  // External and unbound native Timer reads retain the established writer.
  // Verify that the REAL browser/reference client routes their actions
  // through the existing authorized VDR endpoint, not a false v1 Operation.
    // A browser's window and globalThis are the same object (unlike
  // separately constructed Node VM objects). Keep UMD and client-api aligned.
  httpContext.location = productionWindow.location;
  httpContext.VdrSuiteBrowserSession = productionWindow.VdrSuiteBrowserSession;
  httpContext.window = httpContext;
  const reference = path.join(__dirname, '..', '..', '..', 'clients',
    'reference-js', 'public-v1-client.js');
  const browserClient = path.join(__dirname, '..', 'api', 'client-api.js');
  vm.runInNewContext(fs.readFileSync(reference, 'utf8'), httpContext,
    {filename: reference});
  vm.runInNewContext(fs.readFileSync(browserClient, 'utf8'), httpContext,
    {filename: browserClient});
  const realClient = Object.assign({}, httpContext.VdrSuiteClientApi, {
    createClientOperationUpdateSource:
      clientApi.createClientOperationUpdateSource
  });
  browser.configureContext({
    detailDataElement: root, clientApi: realClient,
    getSelectedBackendId() { return 'living-room'; },
    reload() { reloads++; }
  });
  fields.title.value = 'Composition news';
  fields.aux.value = '<epgsearch>plugin:data</epgsearch>';
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
  assert.strictEqual(submitted.aux, '<epgsearch>plugin:data</epgsearch>');
  assert.strictEqual(submitted.backendId, 'living-room');
  assert.strictEqual(submitted.directory, 'News~Evening');
  assert.strictEqual(httpRequests[1].url,
    'https://suite.example/api/v1/operations/op_manual_f56');
  assert.strictEqual(httpRequests[1].options.method, 'GET');
  assert.strictEqual(httpRequests.length, 2);
  assert.strictEqual(legacy.length, 1,
    'real Browser+reference composition must not call legacy CREATE');

  // Real production browser adapter + reference client for native TOGGLE:
  // GET strong ETag, POST with Browser CSRF and Idempotency-Key, SSE readback.
  browser.renderList({timers: [{
    timerId: '17', channelId: 'C-1', title: 'Native news',
    day: '2026-11-24', startTime: 2015, endTime: 2030,
    priority: 0, lifetime: 0, enabled: true, aux: ''
  }]});
  const disable = find(root, item =>
    item.tagName === 'button' && item.textContent === 'Deaktivieren');
  assert(disable, 'native Timer toggle visible');
  disable.listeners.click();
  await tick();
  const nativeRequests = httpRequests.slice(2);
  assert.equal(nativeRequests[0].url,
    'https://suite.example/api/v1/native-timers/17?backendId=living-room');
  assert.equal(nativeRequests[0].options.method, 'GET');
  assert.equal(nativeRequests[1].url,
    'https://suite.example/api/v1/native-timers/17/modifications?backendId=living-room');
  assert.equal(nativeRequests[1].options.method, 'POST');
  assert.equal(nativeRequests[1].options.credentials, 'same-origin');
  assert.equal(nativeRequests[1].options.headers['X-CSRF-Token'], 'composition-csrf');
  assert.equal(nativeRequests[1].options.headers['If-Match'],
    '"native-timer-binding/1:fixture:2:3"');
  assert(nativeRequests[1].options.headers['Idempotency-Key'].startsWith('manual-web-'));
  assert.deepStrictEqual(JSON.parse(nativeRequests[1].options.body),
    {kind: 'toggle', enabled: false});
  assert.equal(nativeRequests[2].url,
    'https://suite.example/api/v1/operations/op_native_f56');
  assert.equal(nativeRequests[2].options.method, 'GET');
  assert.equal(legacy.length, 1, 'native Public-v1 TOGGLE never dispatched legacy');

  // Production browser -> reference JS -> HTTP MODIFY composition for an
  // explicit Aux update. The previous plugin metadata is carried only on
  // an actual edit, and the old synchronous writer is never dispatched.
  browser.renderList({timers: [{
    timerId: '17', channelId: 'C-1', title: 'Native news',
    day: '2026-11-24', startTime: 2015, endTime: 2030,
    priority: 0, lifetime: 0, enabled: true, aux: '<epgsearch/>'
  }]});
  const auxForm = find(root, item => item.tagName === 'form' &&
    item.elements.timerId && item.elements.timerId.value === '17');
  assert(auxForm, 'native Aux edit form must be present');
  auxForm.elements.manualChannelId.value = 'C-1';
  auxForm.elements.aux.value = '<tvscraper/>';
  auxForm.listeners.submit({preventDefault() {}});
  await tick();
  const auxRequests = httpRequests.slice(5);
  assert.equal(auxRequests[0].url,
    'https://suite.example/api/v1/native-timers/17?backendId=living-room');
  assert.equal(auxRequests[1].url,
    'https://suite.example/api/v1/native-timers/17/modifications?backendId=living-room');
  const auxBody = JSON.parse(auxRequests[1].options.body);
  assert.equal(auxBody.kind, 'update');
  assert.equal(auxBody.expectedAux, '<epgsearch/>');
  assert.equal(auxBody.aux, '<tvscraper/>');
  assert.equal(auxBody.priority, 0);
  assert.equal(auxRequests[1].options.headers['X-CSRF-Token'], 'composition-csrf');
  assert.equal(auxRequests[1].options.headers['If-Match'],
    '"native-timer-binding/1:fixture:2:3"');
  assert.equal(auxRequests[2].url,
    'https://suite.example/api/v1/operations/op_native_f56');
  assert.equal(legacy.length, 1, 'Aux edit may not launch a second legacy writer');
  // Slice 3: a native VDR Timer created outside Suite is not a managed
  // TimerAssignment. Browser -> real client-api -> HTTP must remain usable
  // without inventing a binding ETag or issuing a duplicate native write.
  nativeBindingState = 'adoption_required';
  browser.renderList({timers: [{
    timerId: '17', channelId: 'C-1', title: 'External news',
    day: '2026-11-24', startTime: 2015, endTime: 2030,
    priority: 50, lifetime: 99, enabled: true, aux: '<epgsearch/>'
  }]});
  const externalToggle = find(root, item =>
    item.tagName === 'button' && item.textContent === 'Deaktivieren');
  assert(externalToggle);
  const beforeExternal = httpRequests.length;
  externalToggle.listeners.click();
  await tick();
  const externalCalls = httpRequests.slice(beforeExternal);
  assert.equal(externalCalls.length, 2, 'one detail read and one existing VDR update');
  assert.equal(externalCalls[0].url,
    'https://suite.example/api/v1/native-timers/17?backendId=living-room');
  assert.equal(externalCalls[1].url,
    '/api/vdr/timers/actions/update');
  assert.equal(externalCalls[1].options.method, 'POST');
  assert.equal(externalCalls[1].options.credentials, 'same-origin');
  // Existing private VDR timer client does not auto-attach browser CSRF.
  // Do not invent a stricter legacy transport contract in a Slice-3 test.
  assert.equal(externalCalls[1].options.headers['X-CSRF-Token'], undefined);
  const externalBody = JSON.parse(externalCalls[1].options.body);
  assert.equal(externalBody.backendId, 'living-room');
  assert.equal(externalBody.timerId, '17');
  assert.equal(externalBody.aux, '<epgsearch/>');
  assert.equal(externalBody.active, false);

  nativeBindingState = 'binding_required';
  browser.renderList({timers: [{
    timerId: '17', channelId: 'C-1', title: 'Unbound news',
    day: '2026-11-24', startTime: 2015, endTime: 2030,
    priority: 50, lifetime: 99, enabled: true, aux: ''
  }]});
  const unboundDelete = find(root, item =>
    item.tagName === 'button' && item.textContent === 'Timer löschen');
  assert(unboundDelete);
  const beforeUnbound = httpRequests.length;
  unboundDelete.listeners.click();
  await tick();
  const unboundCalls = httpRequests.slice(beforeUnbound);
  assert.equal(unboundCalls.length, 2, 'unbound delete uses one existing writer');
  assert.equal(unboundCalls[0].url,
    'https://suite.example/api/v1/native-timers/17?backendId=living-room');
  assert.equal(unboundCalls[1].url,
    '/api/vdr/timers/actions/delete');
  assert.deepStrictEqual(JSON.parse(unboundCalls[1].options.body),
    {backendId: 'living-room', timerId: '17'});
  assert.equal(unboundCalls[1].options.headers['X-CSRF-Token'], undefined);
    // The capability advertised by the new daemon switches the same browser
  // to Public-v1 direct actions without ever dispatching legacy as well.
  nativeBindingState = 'adoption_required';
  nativeDirectAvailable = true;
  browser.renderList({timers: [{
    timerId: '17', channelId: 'C-1', title: 'External Public news',
    day: '2026-11-24', startTime: 2015, endTime: 2030,
    priority: 50, lifetime: 99, enabled: true, aux: '<epgsearch/>'
  }]});
  const newExternalToggle = find(root, item =>
    item.tagName === 'button' && item.textContent === 'Deaktivieren');
  assert(newExternalToggle);
  const beforeNativeDirect = httpRequests.length;
  newExternalToggle.listeners.click();
  await tick();
  const newExternalCalls = httpRequests.slice(beforeNativeDirect);
  assert.equal(newExternalCalls.length,2);
  assert.equal(newExternalCalls[1].url,
    'https://suite.example/api/v1/native-timers/17/direct-update?backendId=living-room');
  assert.equal(newExternalCalls[1].options.method,'POST');
  assert.equal(newExternalCalls[1].options.headers['X-CSRF-Token'],'composition-csrf');
  assert.equal(JSON.parse(newExternalCalls[1].options.body).aux,'<epgsearch/>');
  nativeBindingState = 'binding_required';
  browser.renderList({timers: [{
    timerId: '17', channelId: 'C-1', title: 'Unbound Public news',
    day: '2026-11-24', startTime: 2015, endTime: 2030,
    priority: 50, lifetime: 99, enabled: true, aux: ''
  }]});
  const newUnboundDelete = find(root, item =>
    item.tagName === 'button' && item.textContent === 'Timer löschen');
  assert(newUnboundDelete);
  const beforeDirectDelete = httpRequests.length;
  newUnboundDelete.listeners.click();
  await tick();
  const newUnboundCalls = httpRequests.slice(beforeDirectDelete);
  assert.equal(newUnboundCalls.length,2);
  assert.equal(newUnboundCalls[1].url,
    'https://suite.example/api/v1/native-timers/17/direct-delete?backendId=living-room');
  assert.deepStrictEqual(JSON.parse(newUnboundCalls[1].options.body),
    {backendId:'living-room',timerId:'17'});
  assert.equal(newUnboundCalls[1].options.headers['X-CSRF-Token'],'composition-csrf');
  // Known unsupported preview shapes must preserve the previously
  // working editor action, with AUX, before any Public-v1 POST.
  browser.configureContext({
    detailDataElement: root, clientApi,
    getSelectedBackendId() { return 'living-room'; },
    reload() { reloads++; }
  });
  browser.renderList({timers: []});
  const unsupportedForm = find(root, item => item.tagName === 'form');
  assert(unsupportedForm);
  const unsupported = unsupportedForm.elements;
  unsupported.title.value = 'Inactive metadata';
  unsupported.manualChannelId.value = 'S19.2E-1-2-3';
  unsupported.day.value = '2026-11-25';
  unsupported.start.value = '20:15';
  unsupported.stop.value = '20:45';
  unsupported.aux.value = '<epgsearch>plugin:data</epgsearch>';
  unsupported.active.checked = false;
  unsupported.vps.checked = false;
  const beforeUnsupported = calls.length;
  const beforeLegacy = legacy.length;
  unsupportedForm.listeners.submit({preventDefault() {}});
  await tick();
  assert.equal(calls.length, beforeUnsupported);
  assert.equal(legacy.length, beforeLegacy + 1);
  assert.equal(legacy[beforeLegacy].aux, unsupported.aux.value);
  assert.equal(legacy[beforeLegacy].active, false);
  unsupported.active.checked = true;
  unsupported.vps.checked = true;
  unsupportedForm.listeners.submit({preventDefault() {}});
  await tick();
  assert.equal(calls.length, beforeUnsupported);
  assert.equal(legacy.length, beforeLegacy + 2);
  assert.equal(legacy[beforeLegacy + 1].aux, unsupported.aux.value);
  assert.equal(legacy[beforeLegacy + 1].vps, true);
  console.log('Timer browser F5.6 create, toggle and Aux browser/reference composition PASS');
}()).catch(error => { console.error(error); process.exitCode = 1; });
