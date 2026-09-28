'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const marksEditorSource = fs.readFileSync(
  'web/frontend/recordings2-marks-editor.js',
  'utf8'
);
const heroDetailSource = fs.readFileSync(
  'web/frontend/recordings2-hero-detail.js',
  'utf8'
);
const cutLayer = marksEditorSource.match(
  /\.recordings2-cut-state\{[^}]*top:([0-9.]+)rem;z-index:(\d+)/
);
const heroBackLayer = heroDetailSource.match(
  /\.recordings2-hero-mode-back\{[^}]*z-index:(\d+)/
);
assert(cutLayer, 'cut-state sticky offset/layer contract missing');
assert(heroBackLayer, 'Hero mode back layer contract missing');
assert(Number(cutLayer[1]) >= 4.5,
  'running cut state must stay below the fixed Home/Back navigation');
assert(Number(cutLayer[2]) < Number(heroBackLayer[1]),
  'running cut state must never cover the fixed Home/Back navigation');

function hasClass(value, className) {
  return String(value || '').split(/\s+/).filter(Boolean).includes(className);
}

function element(tag) {
  const value = {
    tagName: String(tag || '').toUpperCase(), id: '', className: '', dataset: {},
    children: [], textContent: '', attributes: {}, style: {}, parentNode: null,
    title: '', classList: {
      add(name) { if (!hasClass(value.className, name)) value.className = (value.className + ' ' + name).trim(); },
      remove(name) { value.className = value.className.split(/\s+/).filter(item => item && item !== name).join(' '); }
    }, disabled: false, value: '', listeners: {},
    focus() { this.focused = true; },
    scrollIntoView() { this.scrolledIntoView = true; },
    insertBefore(child, before) { child.parentNode = this; const index = this.children.indexOf(before); this.children.splice(index < 0 ? 0 : index, 0, child); return child; },
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
      if (selector.startsWith('.') && hasClass(this.className, selector.slice(1))) return this;
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
        if (selector.startsWith('.') && hasClass(current.className, selector.slice(1))) result.push(current);
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

function deferred() {
  let resolve;
  let reject;
  const promise = new Promise((yes, no) => { resolve = yes; reject = no; });
  return {promise, resolve, reject};
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
  let cutStateOverride = {};
  let openedCutVariant = null;
  let openedCutVariantOptions = null;
  let revisionCounter = 11;
  let deferredMutation = null;
  let deferredRead = null;
  const requests = [];
  const appliedOperations = new Set();
  const timers = new Map();
  let nextTimerId = 1;
  let timelineInteraction = null;

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
    VdrSuiteRecordings2HeroDetail: {enhance(root) { return root; }},
    VdrSuiteRecordings2MarksTimeline: {
      render(root, recording, payload, interaction) {
        if (interaction) timelineInteraction = interaction;
        return true;
      },
      bind() { return true; }
    },
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
    VdrSuiteRecordings2: {
      openRecording(recording, options) { openedCutVariant = recording; openedCutVariantOptions = options || null; }
    },
    VdrSuiteRecordings2Playback: {createPanel() {
      playbackCreations += 1;
      const start = element('button'); start.textContent = 'Start im Playback-Owner';
      start.addEventListener('click', () => publish({transition: 'session-started', sessionId: 'one', state: 'playing'}));
      owner.element.appendChild(start); return owner;
    }},
    VdrSuiteClientApi: {requestJson(path, config) {
      const body = config.body && JSON.parse(config.body);
      requests.push({path, config, body});
      if (!body && deferredRead && deferredRead.path === path) return deferredRead.promise;
      if (!body) return Promise.resolve(path.endsWith('/cut')
        ? Object.assign({}, native, {
            availability: 'available',
            ready: previewReady,
            editedDestinationExists: !previewReady,
            editedRecordingFound: false,
            handlerUsage: 0,
            operationState: 'none',
            operationPending: false,
            operationVerified: false
          }, cutStateOverride, {marksRevision: native.marksRevision}) : native);
      if (deferredMutation) return deferredMutation.promise;
      if (mode === 'lease') return Promise.reject(new Error('active_agent_lease_required'));
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
    'web/frontend/recordings2-marks-detail.js',
    'web/frontend/recordings2-hero-visibility.js'
  ]) vm.runInContext(fs.readFileSync(path, 'utf8'), context, {filename: path});

  const view = window.VdrSuiteRecordings2BrowserView.create({getState: () => ({
    selectedRecording: {id: '7', title: 'Testaufnahme'}, backendId: 'default'
  })});
  view.renderDetail(); await flush();
  root = mount.querySelector('.recordings2-detail');
  assert(root && root.__vdrSuiteRecordingPlaybackOwner === owner);
  assert.strictEqual(playbackCreations, 1);
  const editor = root.__vdrSuiteMarksEditor;
  assert(editor && typeof editor.notifyExternalMarksChanged === 'function');

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
  function markReads() { return requests.filter(request => !request.body && request.path === '/api/vdr/recordings/marks'); }

  assert(button('Marke setzen').disabled);
  button('Start im Playback-Owner').click();
  assert(!button('Marke setzen').disabled);

  assert.ok(timelineInteraction);
  assert.strictEqual(typeof timelineInteraction.onSelect, 'function');
  assert.strictEqual(typeof timelineInteraction.onMove, 'function');
  timelineInteraction.onSelect(initial.marks[0]); await flush();
  assert.deepStrictEqual(seeks, [10]);
  assert.strictEqual(timelineInteraction.selectedFrame, 250);
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
  assert(!findButton('Auftrag prüfen'));
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

  const directMark = native.marks.find(mark => mark.positionFrame === 300);
  const beforeDirectMove = posts().length;
  timelineInteraction.onMove(directMark, 14);
  assert.deepStrictEqual(
    JSON.parse(JSON.stringify(timelineInteraction.previewMove)),
    {sourceFrame: 300, positionSeconds: 14}
  );
  await flush();
  assert.strictEqual(posts().length, beforeDirectMove + 1);
  const directMove = posts().at(-1);
  assert.strictEqual(directMove.body.kind, 'move');
  assert.strictEqual(directMove.body.sourceFrame, 300);
  assert.strictEqual(directMove.body.targetFrame, 350);
  assert(allText(root).includes('Ausgewählt: frame-350 · Frame 350'));

  const beforeDelete = posts().length;
  button('Auswahl löschen').click(); await flush();
  assert.strictEqual(posts().length, beforeDelete + 1);
  assert.strictEqual(posts().at(-1).body.kind, 'delete');
  assert.strictEqual(posts().at(-1).body.sourceFrame, 350);
  assert(!allText(root).includes('Frame 350'));
  assert(allText(root).includes('Keine Schnittmarke ausgewählt.'));

  mode = 'queued-applied';
  button('Alle Marken entfernen').click();
  assert(!findButton('Bestätigen'));
  await flush();
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

  mode = 'lease';
  button('Marke setzen').click(); await flush();
  assert(allText(root).includes('native Bearbeitung ist derzeit nicht verfügbar'));
  assert(!button('Marke setzen').disabled);
  assert.strictEqual(timers.size, 0);

  position = 45;
  mode = 'lost';
  button('Marke setzen').click(); await flush();
  const lost = posts().at(-1).body;
  for (let i = 0; i < 18; ++i) await fireNextTimer();
  assert.deepStrictEqual(posts().at(-1).body, lost);
  assert.strictEqual(timers.size, 1);
  assert.strictEqual(lost.targetFrame, 1125);
  assert(button('Marke setzen').disabled);
  assert(!findButton('Auftrag prüfen'));
  mode = 'verified';
  await fireNextTimer();
  assert.deepStrictEqual(posts().at(-1).body, lost);
  assert(!findButton('Auftrag prüfen'));

  // External marks hints must not replace, confirm or duplicate an own pending mutation.
  position = 52;
  mode = 'queued';
  const readsBeforeExternalBurst = markReads().length;
  deferredMutation = deferred();
  button('Marke setzen').click(); await flush();
  const externalPendingOperation = posts().at(-1).body;
  assert.strictEqual(externalPendingOperation.targetFrame, 1300);
  assert(button('Marke setzen').disabled);

  native = Object.assign({}, native, {
    marksRevision: revision(),
    marks: native.marks.concat([markForFrame(1500)]).sort((left, right) => left.positionFrame - right.positionFrame)
  });
  editor.notifyExternalMarksChanged();
  editor.notifyExternalMarksChanged();
  editor.notifyExternalMarksChanged();
  await flush();
  assert.strictEqual(markReads().length, readsBeforeExternalBurst, 'external burst waits for busy mutation');

  const pendingMutationReply = deferredMutation;
  deferredMutation = null;
  pendingMutationReply.resolve({
    accepted: true,
    operationId: externalPendingOperation.operationId,
    verification: 'readback_required'
  });
  await flush();
  assert.strictEqual(
    markReads().length,
    readsBeforeExternalBurst + 2,
    'mutation readback plus one coalesced external canonical refresh'
  );
  assert(allText(root).includes('Frame 1500'));
  assert(allText(root).includes('bestehenden Auftrags läuft unverändert weiter'));
  assert(button('Marke setzen').disabled, 'external refresh never confirms the pending operation');
  assert.deepStrictEqual(posts().at(-1).body, externalPendingOperation);

  mode = 'verified';
  await fireNextTimer();
  assert.deepStrictEqual(posts().at(-1).body, externalPendingOperation, 'verification reuses exact operation identity and body');
  assert(!button('Marke setzen').disabled);

  publish({transition: 'session-replaced', sessionId: 'two', state: 'playing'});
  assert.strictEqual(root.__vdrSuiteMarksEditor, editor);
  assert.strictEqual(playbackCreations, 1, 'editing never creates another playback owner');

  mode = 'verified';
  const beforeCut = posts().length;
  const cutReadsBeforePreview = requests.filter(request => !request.body && request.path === '/api/vdr/recordings/cut').length;
  deferredRead = Object.assign({path: '/api/vdr/recordings/cut'}, deferred());
  button('Schneiden …').click(); await flush();
  assert.strictEqual(
    requests.filter(request => !request.body && request.path === '/api/vdr/recordings/cut').length,
    cutReadsBeforePreview,
    'cut preview must not synchronously depend on cut-state telemetry'
  );
  assert.strictEqual(posts().length, beforeCut, 'preview never starts a cut');
  assert(findButton('Bestätigen'), 'cut confirmation must appear immediately from canonical marks state');
  assert(button('Bestätigen').focused, 'explicit cut confirmation must receive focus');
  assert(button('Bestätigen').parentNode.scrolledIntoView, 'confirmation must be brought into view');
  assert(!button('Neu laden').disabled, 'opening cut confirmation must never strand the editor in busy state');
  assert(allText(root).includes('Original'));
  button('Abbrechen').click();
  assert.strictEqual(posts().length, beforeCut, 'cancel never starts a cut');

  button('Schneiden …').click(); await flush();
  mode = 'queued';
  cutStateOverride = {
    ready: false,
    editedDestinationExists: true,
    editedRecordingFound: false,
    handlerUsage: 4,
    operationState: 'accepted',
    operationPending: true,
    operationVerified: false
  };
  deferredMutation = deferred();
  button('Bestätigen').click();
  const cutView = root.children.find(child => hasClass(child.className, 'recordings2-cut-state'));
  assert(cutView, 'cut progress belongs to the detail owner, outside the hidden playback/marks subtree');
  const progress = cutView.children.find(child => child.tagName === 'PROGRESS');
  assert(progress, 'indeterminate progress is visible before the POST or completion readback resolves');
  assert.strictEqual(progress.attributes.value, undefined);
  for (const mode of ['playback', 'detail', 'actions', 'metadata']) {
    root.dataset.recordings2HeroMode = mode;
    window.VdrSuiteRecordings2HeroDetail.enhance(root);
    assert(!cutView.hidden, 'cut state survives the production visibility owner in ' + mode);
    assert.strictEqual(root.querySelector('.recordings2-marks-detail').hidden, mode !== 'playback');
  }
  publish({transition: 'session-replaced', sessionId: 'three', state: 'playing'});
  assert.strictEqual(cutView.parentNode, root);
  deferredMutation.resolve({accepted: true, operationId: posts().at(-1).body.operationId, verification: 'readback_required'});
  deferredMutation = null;
  await flush();
  assert.strictEqual(posts().length, beforeCut + 1, 'explicit confirmation starts exactly once');
  assert.strictEqual(posts().at(-1).path, '/api/vdr/recordings/cut');
  assert(!button('Neu laden').disabled, 'stalled cut-state telemetry must not keep the editor busy after dispatch');
  assert(allText(root).includes('Schnitt läuft'));
  assert(allText(root).includes('keinen verlässlichen Prozentwert'));
  assert(
    allText(root).includes('automatisch überwacht'),
    'accepted native cut must remain visibly monitored'
  );
  deferredRead = null;

  const editedRecording = {
    id: 'cut-7',
    recordingId: 'cut-7',
    backendId: 'default',
    title: 'Testaufnahme (geschnitten)',
    backendNativeId: '/srv/vdr/video/%Testaufnahme/cut.rec'
  };
  cutStateOverride = {
    ready: false,
    editedDestinationExists: true,
    editedRecordingFound: true,
    handlerUsage: 0,
    operationState: 'verified',
    operationPending: false,
    operationVerified: true,
    editedRecordingKey: 'b'.repeat(32),
    editedRecording
  };
  cutStateOverride.handlerUsage = 36;
  await fireNextTimer();
  assert(allText(root).includes('Schnitt läuft'), 'existing result and legacy verified journal must not hide a running cutter');
  assert(!allText(root).includes('Schnittfassung öffnen'));
  cutStateOverride.handlerUsage = 0;
  await fireNextTimer();
  assert(allText(root).includes('Schnittfassung'));
  assert(button('Schnittfassung öffnen'));
  button('Schnittfassung öffnen').click();
  assert.strictEqual(openedCutVariant, editedRecording);
  assert.strictEqual(openedCutVariantOptions.backLabel, '← Zurück zur Originalfassung');
  assert.strictEqual(typeof openedCutVariantOptions.onClose, 'function');
  openedCutVariant = null;
  openedCutVariantOptions.onClose();
  assert.strictEqual(openedCutVariant.id, '7');
  assert(!allText(root).includes('Original löschen'));

  mode = 'verified';
  for (let attempt = 0; attempt < 3 && !allText(root).includes('Native geschnittene Ausgabe bestätigt'); ++attempt) await fireNextTimer();
  assert.deepStrictEqual(posts().at(-1).body.operationId, posts()[beforeCut].body.operationId);
  assert(allText(root).includes('Native geschnittene Ausgabe bestätigt'));
  assert(!allText(root).includes('100 %'), 'native cutter progress must never be fabricated');

  native = Object.assign({}, native, {inUse: true});
  button('Neu laden').click(); await flush();
  assert(button('Schneiden …').disabled);
  assert(button('Marke setzen').disabled);
  button('0:00:10.00').click(); await flush();
  assert(button('Auswahl löschen').disabled);

  publish({transition: 'destroyed', state: 'destroyed'});
  assert.strictEqual(listeners.size, 0);
  assert.strictEqual(timers.size, 0);
  const postsBeforeReload = posts().length;
  snapshot = {transition: 'snapshot', state: 'idle', sessionId: null};
  cutStateOverride = {operationState: 'accepted', operationPending: true, operationVerified: false, editedRecordingFound: false};
  view.renderDetail(); await flush();
  root = mount.querySelector('.recordings2-detail');
  window.VdrSuiteRecordings2HeroDetail.enhance(root);
  const restored = root.children.find(child => hasClass(child.className, 'recordings2-cut-state'));
  assert(restored && !restored.hidden && allText(restored).includes('Schnitt läuft'), 'reload recovers running state even in detail mode');
  assert.strictEqual(posts().length, postsBeforeReload, 'reload must not submit another mutation');
  assert(requests.filter(request => request.path.endsWith('/cut')).every(request =>
    (request.body || request.config.query).recordingId === '7'), 'cut and variant navigation keep the source identity');
  assert(posts().every(request => request.path === '/api/vdr/recordings/cut' || request.path === '/api/vdr/recordings/marks'), 'cut never invokes Delete/Trash');
  root.__vdrSuiteMarksEditor.destroy();
  console.log('native marks editor add/delete/reset/move/navigation/readback/external-sync and failure-state hardening ok');
}

run().catch(error => { console.error(error); process.exitCode = 1; });
