'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

function hasClass(value, className) {
  return String(value || '').split(/\s+/).filter(Boolean).includes(className);
}

function element(tag) {
  const value = {
    tagName: String(tag || '').toUpperCase(), id: '', className: '', dataset: {},
    children: [], textContent: '', attributes: {}, style: {}, parentNode: null,
    title: '', classList: {add() {}, remove() {}}, disabled: false, value: '', listeners: {},
    addEventListener(name, fn) { this.listeners[name] = fn; },
    click() { if (!this.disabled && this.listeners.click) this.listeners.click(); },
    removeChild(child) { this.children = this.children.filter(value => value !== child); child.parentNode = null; },
    setAttribute(name, attributeValue) { this.attributes[name] = String(attributeValue); },
    appendChild(child) { if (child) child.parentNode = this; this.children.push(child); return child; },
    replaceChildren() {
      this.children.forEach(child => { if (child && child.parentNode === this) child.parentNode = null; });
      this.children = Array.from(arguments);
      this.children.forEach(child => { if (child) child.parentNode = this; });
    },
    insertAdjacentElement(position, child) {
      assert.strictEqual(position, 'afterend'); assert.ok(this.parentNode);
      const index = this.parentNode.children.indexOf(this); assert.ok(index >= 0);
      child.parentNode = this.parentNode; this.parentNode.children.splice(index + 1, 0, child); return child;
    },
    querySelector(selector) {
      if (selector === '.recordings2-detail' && hasClass(this.className, 'recordings2-detail')) return this;
      if (selector === '.recordings2-marks-detail' && hasClass(this.className, 'recordings2-marks-detail')) return this;
      if (selector === '.recordings2-marks-timeline' && hasClass(this.className, 'recordings2-marks-timeline')) return this;
      if (selector === 'input[aria-label="Wiedergabeposition"]' &&
          this.tagName === 'INPUT' && this.attributes['aria-label'] === 'Wiedergabeposition') return this;
      for (const child of this.children) {
        if (child && typeof child.querySelector === 'function') {
          const found = child.querySelector(selector); if (found) return found;
        }
      }
      return null;
    },
    querySelectorAll(selector) {
      const result = [];
      function collect(current) {
        if (!current) return;
        if (selector === 'input[aria-label="Wiedergabeposition"]' &&
            current.tagName === 'INPUT' && current.attributes['aria-label'] === 'Wiedergabeposition') result.push(current);
        (current.children || []).forEach(collect);
      }
      collect(this); return result;
    }
  };
  return value;
}

function allText(root) {
  let result = root && root.textContent ? String(root.textContent) : '';
  (root && root.children || []).forEach(child => { result += ' ' + allText(child); });
  return result;
}

async function flush() { for (let i = 0; i < 16; ++i) await Promise.resolve(); }

async function run() {
  let root;
  const mount = element('div');
  let playbackCreations = 0;
  const listeners = new Set();
  let snapshot = {transition: 'snapshot', sessionId: null, state: 'idle'};
  let position = 32;
  const seeks = [];
  const owner = {
    element: element('div'), snapshot: () => snapshot, position: () => position,
    seekAbsolute(value) { seeks.push(value); return Promise.resolve(); },
    subscribe(fn) { listeners.add(fn); fn(snapshot); return () => listeners.delete(fn); }
  };
  function publish(value) { snapshot = value; [...listeners].forEach(fn => fn(value)); }

  const initial = {
    backendId: 'default', recordingId: '7', availability: 'available',
    framesPerSecond: 25, inUse: false, marksRevision: 'a'.repeat(32), sequenceCount: 1,
    marks: [
      {positionFrame: 250, positionSeconds: 10, timecode: '0:00:10.00'},
      {positionFrame: 500, positionSeconds: 20, timecode: '0:00:20.00'}
    ]
  };
  let native = initial;
  let mode = 'queued';
  let previewReady = true;
  let revisionCounter = 11;
  const requests = [];
  const appliedOperations = new Set();
  const timers = new Map();
  let nextTimerId = 1;

  function revision() { const value = revisionCounter.toString(16).padStart(32, '0'); revisionCounter += 1; return value; }
  function markForFrame(positionFrame) {
    const seconds = positionFrame / 25;
    return {positionFrame, positionSeconds: seconds, timecode: 'frame-' + String(positionFrame)};
  }
  function applyMutation(body) {
    if (!body || !body.kind || appliedOperations.has(body.operationId)) return;
    let marks = native.marks.map(mark => Object.assign({}, mark));
    if (body.kind === 'add') marks.push(markForFrame(body.targetFrame));
    else if (body.kind === 'delete') marks = marks.filter(mark => mark.positionFrame !== body.sourceFrame);
    else if (body.kind === 'move') marks = marks.map(mark => mark.positionFrame === body.sourceFrame ? markForFrame(body.targetFrame) : mark);
    else if (body.kind === 'reset') marks = [];
    marks.sort((left, right) => left.positionFrame - right.positionFrame);
    appliedOperations.add(body.operationId);
    native = Object.assign({}, native, {
      marks, sequenceCount: Math.floor((marks.length + 1) / 2), marksRevision: revision()
    });
  }
  async function fireNextTimer() {
    const entry = timers.entries().next(); assert(!entry.done, 'expected scheduled verification');
    const [id, fn] = entry.value; timers.delete(id); fn(); await flush();
  }

  const document = {head: element('head'), createElement: element, getElementById() { return null; }};
  const window = {
    document, crypto: require('crypto').webcrypto,
    setTimeout(fn) { const id = nextTimerId++; timers.set(id, fn); return id; },
    clearTimeout(id) { timers.delete(id); },
    VdrSuiteBrowserSession: {csrfHeaders: () => ({'X-CSRF-Token': 'test-only'})},
    VdrSuiteRecordings2Shared: {
      mountTarget: () => mount, installStyles() {}, selectedBackendId: () => 'default',
      node(tag, className, label) { const n = element(tag); n.className = className; n.textContent = label || ''; return n; },
      createButton(label, action) { const n = element('button'); n.textContent = label; n.addEventListener('click', action || (() => {})); return n; },
      createPoster: () => element('img'), provider: () => ({}),
      text: value => String(value || ''), recordingTitle: () => 'Testaufnahme',
      recordingSubtitle: () => '', recordingSummary: () => '',
      formatStart: () => '', formatDuration: () => '', formatSize: () => '',
      first(value, keys, fallback) { for (const key of keys) if (value && value[key] !== undefined) return value[key]; return fallback; },
      number: (value, fallback) => Number(value) || fallback
    },
    VdrSuiteRecordingPlaybackRestartChoice: {install() {}},
    VdrSuiteRecordings2Playback: {createPanel() {
      playbackCreations += 1;
      const start = element('button'); start.textContent = 'Start im Playback-Owner';
      start.addEventListener('click', () => publish({transition: 'session-started', sessionId: 'one', state: 'playing'}));
      owner.element.appendChild(start); return owner;
    }},
    VdrSuiteClientApi: {requestJson(path, config) {
      const body = config.body && JSON.parse(config.body);
      requests.push({path, config, body});
      if (!body) return Promise.resolve(path.endsWith('/cut')
        ? Object.assign({}, native, {ready: previewReady, editedDestinationExists: !previewReady}) : native);
      if (mode === 'lost') return Promise.reject(new Error('connection lost'));
      if (mode === 'conflict') return Promise.reject(new Error('recording_marks_revision_conflict'));
      if (mode === 'rejected') return Promise.reject(new Error('recording_marks_modify_rejected'));
      if (mode === 'queued-applied') {
        applyMutation(body);
        return Promise.resolve({accepted: true, operationId: body.operationId, verification: 'readback_required'});
      }
      if (mode === 'verified') {
        applyMutation(body);
        return Promise.resolve({accepted: true, operationId: body.operationId,
          verification: 'verified', canonicalMarksRevision: native.marksRevision});
      }
      return Promise.resolve({accepted: true, operationId: body.operationId, verification: 'readback_required'});
    }}
  };

  const context = vm.createContext({window, document, console, Promise, Uint32Array});
  for (const path of [
    'web/frontend/recordings2-browser-view.js',
    'web/frontend/recordings2-marks-editor.js',
    'web/frontend/recordings2-marks-detail.js'
  ]) vm.runInContext(fs.readFileSync(path, 'utf8'), context, {filename: path});

  const view = window.VdrSuiteRecordings2BrowserView.create({getState: () => ({
    selectedRecording: {id: '7', title: 'Testaufnahme'}, backendId: 'default'
  })});
  view.renderDetail(); await flush();
  root = mount.querySelector('.recordings2-detail');
  assert(root && root.__vdrSuiteRecordingPlaybackOwner === owner);
  assert.strictEqual(playbackCreations, 1);

  function findButton(label) {
    let found;
    function visit(value) {
      if (value.tagName === 'BUTTON' && value.textContent === label) found = value;
      value.children.forEach(visit);
    }
    visit(root); return found;
  }
  function button(label) { const found = findButton(label); assert(found, label); return found; }
  function posts() { return requests.filter(request => request.body); }

  assert(button('Marke setzen').disabled);
  button('Start im Playback-Owner').click();
  assert(!button('Marke setzen').disabled);

  button('0:00:10.00').click(); await flush();
  assert.deepStrictEqual(seeks, [10]);
  assert(allText(root).includes('Ausgewählt: 0:00:10.00 · Frame 250'));
  assert(button('Vorherige Marke').disabled);
  assert(!button('Nächste Marke').disabled);
  button('Nächste Marke').click(); await flush();
  assert.deepStrictEqual(seeks, [10, 20]);
  assert(allText(root).includes('Ausgewählt: 0:00:20.00 · Frame 500'));
  assert(!button('Vorherige Marke').disabled);
  assert(button('Nächste Marke').disabled);
  button('Vorherige Marke').click(); await flush();
  assert.deepStrictEqual(seeks, [10, 20, 10]);

  position = 32;
  mode = 'queued-applied';
  button('Marke setzen').click(); await flush();
  const addOperation = posts().at(-1);
  assert.strictEqual(addOperation.body.kind, 'add');
  assert.strictEqual(addOperation.body.targetFrame, 800);
  assert.strictEqual(addOperation.body.expectedMarksRevision, initial.marksRevision);
  assert.strictEqual(addOperation.config.headers['X-CSRF-Token'], 'test-only');
  assert(!JSON.stringify(addOperation.body).includes('/srv/'));
  assert(allText(root).includes('Frame 800'));
  assert(button('Marke setzen').disabled);
  assert(findButton('Auftrag prüfen'));
  mode = 'verified';
  await fireNextTimer();
  assert.deepStrictEqual(posts().at(-1).body, addOperation.body);
  assert(!findButton('Auftrag prüfen'));
  assert(!button('Marke setzen').disabled);

  button('0:00:10.00').click(); await flush();
  position = 12;
  mode = 'verified';
  const beforeMove = posts().length;
  button('Auswahl hierher verschieben').click(); await flush();
  assert.strictEqual(posts().length, beforeMove + 1);
  const moveOperation = posts().at(-1);
  assert.strictEqual(moveOperation.body.kind, 'move');
  assert.strictEqual(moveOperation.body.sourceFrame, 250);
  assert.strictEqual(moveOperation.body.targetFrame, 300);
  assert(allText(root).includes('Frame 300'));
  assert(allText(root).includes('Ausgewählt: frame-300 · Frame 300'));

  const beforeDelete = posts().length;
  button('Auswahl löschen').click(); await flush();
  assert.strictEqual(posts().length, beforeDelete + 1);
  assert.strictEqual(posts().at(-1).body.kind, 'delete');
  assert.strictEqual(posts().at(-1).body.sourceFrame, 300);
  assert(!allText(root).includes('Frame 300'));
  assert(allText(root).includes('Keine Schnittmarke ausgewählt.'));

  mode = 'queued-applied';
  button('Alle Marken entfernen').click();
  assert(allText(root).includes('Alle nativen Schnittmarken'));
  button('Bestätigen').click(); await flush();
  const resetOperation = posts().at(-1);
  assert.strictEqual(resetOperation.body.kind, 'reset');
  assert(allText(root).includes('Keine nativen Schnittmarken vorhanden.'));
  assert(button('Alle Marken entfernen').disabled);
  mode = 'verified';
  await fireNextTimer();
  assert.deepStrictEqual(posts().at(-1).body, resetOperation.body);
  assert(allText(root).includes('Keine nativen Schnittmarken vorhanden.'));

  native = Object.assign({}, native, {
    marksRevision: revision(), sequenceCount: 1,
    marks: [
      {positionFrame: 250, positionSeconds: 10, timecode: '0:00:10.00'},
      {positionFrame: 500, positionSeconds: 20, timecode: '0:00:20.00'}
    ]
  });
  button('Neu laden').click(); await flush();
  assert(allText(root).includes('Frame 250'));
  assert(allText(root).includes('Frame 500'));
  assert(!allText(root).includes('Frame 800'));

  button('0:00:10.00').click(); await flush();
  mode = 'rejected';
  const marksBeforeRejectedDelete = native.marks.map(mark => mark.positionFrame);
  button('Auswahl löschen').click(); await flush();
  assert(allText(root).includes('VDR hat die Änderung abgelehnt'));
  assert.deepStrictEqual(native.marks.map(mark => mark.positionFrame), marksBeforeRejectedDelete);
  assert(!findButton('Auftrag prüfen'));
  assert(!button('Auswahl löschen').disabled);

  mode = 'conflict';
  button('Auswahl löschen').click(); await flush();
  assert(allText(root).includes('inzwischen geändert'));
  assert(!findButton('Auftrag prüfen'));

  position = 45;
  mode = 'lost';
  button('Marke setzen').click(); await flush();
  const lost = posts().at(-1).body;
  assert.strictEqual(lost.targetFrame, 1125);
  assert(button('Marke setzen').disabled);
  assert(findButton('Auftrag prüfen'));
  mode = 'verified';
  await fireNextTimer();
  assert.deepStrictEqual(posts().at(-1).body, lost);
  assert(!findButton('Auftrag prüfen'));

  const editor = root.__vdrSuiteMarksEditor;
  publish({transition: 'session-replaced', sessionId: 'two', state: 'playing'});
  assert.strictEqual(root.__vdrSuiteMarksEditor, editor);
  assert.strictEqual(playbackCreations, 1, 'editing never creates another playback owner');

  mode = 'verified';
  const beforeCut = posts().length;
  button('Schneiden …').click(); await flush();
  assert.strictEqual(posts().length, beforeCut, 'preview never starts a cut');
  assert(allText(root).includes('Original'));
  button('Abbrechen').click();
  assert.strictEqual(posts().length, beforeCut, 'cancel never starts a cut');
  previewReady = false;
  button('Schneiden …').click(); await flush();
  assert(allText(root).includes('existiert bereits'));
  assert.strictEqual(posts().length, beforeCut);
  previewReady = true;
  button('Schneiden …').click(); await flush();
  button('Bestätigen').click(); await flush();
  assert.strictEqual(posts().length, beforeCut + 1, 'explicit confirmation starts exactly once');
  assert.strictEqual(posts().at(-1).path, '/api/vdr/recordings/cut');

  native = Object.assign({}, native, {inUse: true});
  button('Neu laden').click(); await flush();
  assert(button('Schneiden …').disabled);
  assert(button('Marke setzen').disabled);
  button('0:00:10.00').click(); await flush();
  assert(button('Auswahl löschen').disabled);

  publish({transition: 'destroyed', state: 'destroyed'});
  assert.strictEqual(listeners.size, 0);
  assert.strictEqual(timers.size, 0);
  console.log('native marks editor add/delete/reset/move/navigation/readback and failure-state hardening ok');
}

run().catch(error => { console.error(error); process.exitCode = 1; });
