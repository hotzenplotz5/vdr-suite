'use strict';

// F5.6 browser form/card -> Public-v1 native binding -> durable Operation.
// Legacy remains available only for contract gaps (unbound, aux, recording).
const assert = require('assert').strict;
const fs = require('fs');
const path = require('path');
const vm = require('vm');

class Element {
  constructor(tag) {
    this.tagName = tag;
    this.children = [];
    this.listeners = {};
    this.classList = {add() {}};
    this.value = '';
    this.checked = false;
    this.disabled = false;
    this.textContent = '';
  }
  appendChild(child) { this.children.push(child); return child; }
  replaceChildren(...children) { this.children = children; }
  addEventListener(name, callback) { this.listeners[name] = callback; }
  setAttribute() {}
  get elements() {
    const elements = {};
    walk(this, node => {
      if (node.name && !elements[node.name]) elements[node.name] = node;
    });
    return elements;
  }
  querySelectorAll(selector) {
    assert.equal(selector, 'input[name="weekday"]:checked');
    const nodes = [];
    walk(this, node => {
      if (node.name === 'weekday' && node.checked) nodes.push(node);
    });
    return nodes;
  }
}
function walk(node, visit) {
  visit(node);
  node.children.forEach(child => walk(child, visit));
}
function find(root, predicate) {
  let result;
  walk(root, node => { if (!result && predicate(node)) result = node; });
  return result;
}
const tick = () => new Promise(resolve => setImmediate(resolve));
const root = new Element('main');
const publicReads = [];
const publicWrites = [];
const operationReads = [];
const legacy = [];
const sources = [];
let bindingState = 'verified';
let opState = 'accepted';
let failWrite = false;
let denyWrite = false;
let denyDetail = false;
let reloads = 0;
let id = 0;
const clientApi = {
  fetchPublicNativeTimerDetail(input) {
    publicReads.push(input);
    if (denyDetail)
      return Promise.reject(Object.assign(new Error('v1 view denied'), {status: 403}));
    return Promise.resolve({
      status: 200,
      etag: bindingState === 'verified' ? '"native-timer-binding/1:fixture:2:3"' : null,
      data: {bindingState}
    });
  },
  fetchPublicNativeTimerModify(input) {
    publicWrites.push({action: 'modify', input});
    if (failWrite) return Promise.reject(new Error('unknown transport outcome'));
    if (denyWrite)
      return Promise.reject(Object.assign(new Error('v1 admin denied'), {status: 403}));
    return Promise.resolve({status: 202, data: {
      operation: {operationId: 'op_native_' + (++id), state: 'accepted'}
    }});
  },
  fetchPublicNativeTimerDelete(input) {
    publicWrites.push({action: 'delete', input});
    return Promise.resolve({status: 202, data: {
      operation: {operationId: 'op_native_' + (++id), state: 'accepted'}
    }});
  },
  fetchPublicNativeTimerOperation(input) {
    operationReads.push(input);
    return Promise.resolve({status: 200, data: {
      operationId: input.operationId, state: opState
    }});
  },
  createClientOperationUpdateSource(operationId) {
    const source = {
      operationId, listeners: {}, closed: false,
      addEventListener(name, callback) { this.listeners[name] = callback; },
      emit(name) {
        if (this.listeners[name])
          this.listeners[name]({data: JSON.stringify({operationId})});
      },
      close() { this.closed = true; }
    };
    sources.push(source);
    return source;
  },
  fetchClientTimerUpdateAction(input) {
    legacy.push({action: 'update', payload: input.payload});
    return Promise.resolve({success: true});
  },
  fetchClientTimerDeleteAction(input) {
    legacy.push({action: 'delete', payload: input.payload});
    return Promise.resolve({success: true});
  }
};
const win = {
  VdrSuiteClientApi: clientApi,
  VdrSuitePlatform: {hasModule() {return false;}, registerModule() {}},
  crypto: {getRandomValues(bytes) {bytes.fill(3); return bytes;}},
  confirm() {return true;},
  setTimeout(callback) {callback();}
};
const document = {
  head: {appendChild() {}},
  createElement(tag) {return new Element(tag);},
  getElementById() {return null;}
};
const file = path.join(__dirname, '..', 'modules', 'timers.js');
vm.runInNewContext(fs.readFileSync(file, 'utf8'), {
  window: win, document, Date, Array, Boolean, Math, Number,
  Object, Promise, Set, String, WeakMap, Uint8Array
}, {filename: file});
const browser = win.VdrSuiteTimerBrowser;
browser.configureContext({
  detailDataElement: root, clientApi,
  getSelectedBackendId() {return 'default';},
  reload() {reloads++;}
});
const nativeTimer = {
  timerId: '17', channelId: 'C-1', title: 'News',
  day: '2026-11-24', startTime: 2015, endTime: 2030,
  priority: 0, lifetime: 0, enabled: true, aux: ''
};
function render(timer = nativeTimer) {browser.renderList({timers: [timer]});}
function button(label) {
  const value = find(root, node => node.tagName === 'button' && node.textContent === label);
  assert(value, 'missing button ' + label);
  return value;
}
function editor() {
  const form = find(root, node => node.tagName === 'form' &&
    node.elements.timerId && node.elements.timerId.value === '17');
  assert(form, 'update form missing');
  return form;
}
(async () => {
  render();
  const toggle = button('Deaktivieren');
  toggle.listeners.click();
  await tick();
  assert.equal(publicReads.length, 1);
  assert.equal(publicWrites[0].action, 'modify');
  assert.deepEqual(JSON.parse(JSON.stringify(publicWrites[0].input.payload)),
    {kind: 'toggle', enabled: false});
  assert.equal(publicWrites[0].input.ifMatch,
    '"native-timer-binding/1:fixture:2:3"');
  assert(publicWrites[0].input.idempotencyKey.startsWith('manual-web-'));
  assert.equal(legacy.length, 0);
  assert.equal(reloads, 0, '202 is not success');
  opState = 'succeeded';
  sources[0].emit('operation');
  await tick();
  assert.equal(reloads, 1, 'verified SSE readback reloads list');

  opState = 'accepted';
  render();
  const form = editor();
  form.elements.title.value = 'Updated news';
  form.elements.manualChannelId.value = 'C-1';
  form.elements.day.value = '2026-11-24';
  form.elements.start.value = '20:15';
  form.elements.stop.value = '20:30';
  form.elements.priority.value = '0';
  form.elements.lifetime.value = '0';
  form.listeners.submit({preventDefault() {}});
  await tick();
  assert.equal(publicWrites[1].action, 'modify');
  const specification = publicWrites[1].input.payload;
  assert.equal(specification.kind, 'update');
  assert.equal(specification.title, 'Updated news');
  assert.equal(specification.startTime, '2015');
  assert.equal(specification.priority, 0);
  assert.equal(specification.lifetime, 0);
  assert.equal(specification.enabled, true);
  assert.equal(Object.keys(specification).length, 12);

  // A repeating managed Timer with no opaque aux is a complete native
  // UPDATE too; it must not silently revert to the legacy endpoint.
  render();
  const recurring = editor();
  recurring.elements.title.value = 'Weekly news';
  recurring.elements.manualChannelId.value = 'C-1';
  find(recurring, node => node.name === 'weekday').checked = true;
  recurring.listeners.submit({preventDefault() {}});
  await tick();
  assert.equal(publicWrites[2].action, 'modify');
  assert.equal(publicWrites[2].input.payload.kind, 'update');
  assert.equal(publicWrites[2].input.payload.weekdays, 'M------');
  assert.equal(publicWrites[2].input.payload.day, '');
  assert.equal(legacy.length, 0);

  render();
  button('Timer löschen').listeners.click();
  await tick();
  assert.equal(publicWrites[3].action, 'delete');
  assert.equal(publicWrites[3].input.payload, undefined);
  assert.equal(legacy.length, 0);

  // An unbound Timer has no public ETag, but its old editor still functions.
  bindingState = 'binding_required';
  render();
  button('Deaktivieren').listeners.click();
  await tick();
  assert.equal(legacy.length, 1);
  assert.equal(legacy[0].action, 'update');
  assert.equal(publicWrites.length, 4);

  // Native MODIFY preserves the current VDR Aux() verbatim. An unchanged
  // opaque aux value is not a reason to bypass the managed Public-v1 writer.
  bindingState = 'verified';
  render({...nativeTimer, aux: '<epgsearch/>'});
  const unchangedAux = editor();
  unchangedAux.elements.manualChannelId.value = 'C-1';
  unchangedAux.listeners.submit({preventDefault() {}});
  await tick();
  assert.equal(publicWrites.length, 5);
  assert.equal(publicWrites[4].action, 'modify');
  assert.equal(publicWrites[4].input.payload.aux, undefined);
  assert.equal(legacy.length, 1);

  // Clearing or adding aux is a real edit, not representable by Public v1.
  render({...nativeTimer, aux: '<epgsearch/>'});
  const clearAux = editor();
  clearAux.elements.manualChannelId.value = 'C-1';
  clearAux.elements.aux.value = '';
  clearAux.listeners.submit({preventDefault() {}});
  await tick();
  assert.equal(legacy.length, 2);
  assert.equal(legacy[1].payload.aux, '');
  assert.equal(publicWrites.length, 5);

  render();
  const addAux = editor();
  addAux.elements.manualChannelId.value = 'C-1';
  addAux.elements.aux.value = '<tvscraper/>';
  addAux.listeners.submit({preventDefault() {}});
  await tick();
  assert.equal(legacy.length, 3);
  assert.equal(legacy[2].payload.aux, '<tvscraper/>');
  assert.equal(publicWrites.length, 5);

  // Stopping a current recording keeps the existing authorized action.
  render({...nativeTimer, recording: true});
  button('Aufnahme stoppen').listeners.click();
  await tick();
  assert.equal(legacy.length, 4);
  assert.equal(legacy[3].action, 'delete');
  assert.deepEqual(JSON.parse(JSON.stringify(legacy[3].payload)),
    {backendId: 'default', timerId: '17'});

  // A potentially dispatched public write never falls through to legacy.
  render();
  failWrite = true;
  const pending = button('Deaktivieren');
  pending.listeners.click();
  await tick();
  const firstKey = publicWrites.at(-1).input.idempotencyKey;
  pending.listeners.click();
  await tick();
  assert.equal(publicWrites.at(-1).input.idempotencyKey, firstKey);
  assert.equal(legacy.length, 4);

  // Public native v1 is administrator-only in the current SecurityHttpGate.
  // A definitive 403 must not revoke existing independently checked Human
  // Timer rights on the established browser action route.
  failWrite = false;
  denyWrite = true;
  render();
  button('Deaktivieren').listeners.click();
  await tick();
  assert.equal(legacy.length, 5);
  assert.equal(legacy[4].action, 'update');
  denyWrite = false;
  denyDetail = true;
  const previousWrites = publicWrites.length;
  render();
  button('Deaktivieren').listeners.click();
  await tick();
  assert.equal(publicWrites.length, previousWrites);
  assert.equal(legacy.length, 6);
  assert.equal(legacy[5].action, 'update');

  console.log('Timer browser Public-v1 native actions + legacy parity PASS');
})().catch(error => {console.error(error); process.exitCode = 1;});
