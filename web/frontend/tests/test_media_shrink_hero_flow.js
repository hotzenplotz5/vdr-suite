'use strict';
// Production Hero owner -> read-only Plan -> fenced Stage -> independent Review
// -> explicit Publish. No native paths or implicit retransmissions.
const assert = require('assert');
const fs = require('fs');
const vm = require('vm');
const hero = fs.readFileSync('web/frontend/recordings2-hero-detail.js', 'utf8');
const browser = fs.readFileSync('web/frontend/recordings2-browser-view.js', 'utf8');
const backendId = 'backend-a';
const recordingId = 'rec_' + 'a'.repeat(32);
const reviewId = 'shk_' + 'b'.repeat(32);
const stageOperationId = 'op_' + 'c'.repeat(32);
const publishOperationId = 'op_' + 'd'.repeat(32);
const publishedRecordingId = 'rec_' + 'e'.repeat(32);
const calls = [];
const sources = [];
let stageState = 'dispatching';
let durableState = 'none';
const storage = new Map();
let keys = 0;
let confirmations = 0;
class Node {
  constructor(tag, className, label) {
    this.tagName = tag;
    this.className = className || '';
    this.textContent = label || '';
    this.children = [];
    this.parentNode = null;
    this.events = {};
    this.hidden = false;
    this.disabled = false;
    this.isConnected = true;
    this.dataset = {};
    this.style = {setProperty() {}};
    this.classList = {add: (name) => {this.className += ' ' + name;}};
  }
  appendChild(node) {this.children.push(node); node.parentNode = this; return node;}
  append(...nodes) {for (const n of nodes) this.appendChild(n);}
  replaceChildren(...nodes) {
    this.children = [];
    this.append(...nodes);
  }
  prepend(node) {this.children.unshift(node); node.parentNode = this;}
  insertBefore(node, reference) {
    const at = reference ? this.children.indexOf(reference) : -1;
    if (at < 0) return this.appendChild(node);
    this.children.splice(at, 0, node); node.parentNode = this; return node;
  }
  get firstChild() {return this.children[0] || null;}
  addEventListener(name, handler) {this.events[name] = handler;}
  setAttribute() {}
  click() {if (!this.disabled && this.events.click) this.events.click();}
  querySelector(selector) {
    if (!selector.startsWith('.')) return null;
    const name = selector.slice(1);
    for (const node of this.children) {
      if (node.className.split(/\s+/).includes(name)) return node;
      const nested = node.querySelector(selector);
      if (nested) return nested;
    }
    return null;
  }
  scrollIntoView() {}
}
const api = {
  getMediaCheckCapability() {
    // This fixture tests HEVC, but the production Hero also composes
    // Media Check and must be able to ask about its availability.
    return Promise.resolve({backendId, capability: 'addons.media.check',
      available: false});
  },
  getMediaShrinkCapability() {return Promise.resolve({
    backendId, capability: 'addons.media.shrink', planningAvailable: true,
    available: true, encoders: ['software', 'vaapi', 'qsv', 'nvenc'],
    encoderFallbackToSoftware: true
  });},
  getMediaShrinkRecording(request) {
    calls.push({action: 'findRecording', request});
    if (durableState === 'ready') return Promise.resolve({
      recordingId, backendId, state: 'ready', operationId: stageOperationId, reviewId
    });
    return Promise.resolve({recordingId, backendId, state: durableState});
  },
  submitMediaShrinkPlan(request) {
    calls.push({action: 'plan', request});
    return Promise.resolve({backendId, recordingId, readOnly: true,
      stageAvailable: true, sourceBytes: 1000000, segments: 1});
  },
  submitMediaShrinkStage(request) {
    calls.push({action: 'stage', request});
    return Promise.resolve({backendId, operationId: stageOperationId});
  },
  getMediaShrinkOperation(request) {
    calls.push({action: 'readOperation', request});
    if (request.operationId === stageOperationId)
      return Promise.resolve({backendId, operationId: stageOperationId,
        action: 'stage', state: stageState,
        failureCode: stageState === 'failed_verified' ? 'ffmpeg_failed' : '',
        resultReconciled: stageState === 'succeeded',
        reviewId: stageState === 'succeeded' ? reviewId : ''});
    if (request.operationId === publishOperationId)
      return Promise.resolve({backendId, operationId: publishOperationId,
        action: 'publish', state: 'succeeded', resultReconciled: true,
        publishedRecordingId});
    throw new Error('Unexpected operation');
  },
  getMediaShrinkReview(request) {
    calls.push({action: 'readReview', request});
    return Promise.resolve({backendId, recordingId, reviewId, state: 'staged',
      revision: 1, publishAvailable: true, candidateBytes: 400000,
      sourceBytes: 1000000});
  },
  submitMediaShrinkPublish(request) {
    calls.push({action: 'publish', request});
    return Promise.resolve({backendId, operationId: publishOperationId});
  }
};
const context = {console, document: {
  head: new Node('head'),
  getElementById() {return null;},
  createElement(tag) {return new Node(tag);}
}, location: {origin: 'https://example.invalid'},
  crypto: {randomUUID() {return '00000000-0000-4000-8000-' +
    String(++keys).padStart(12, '0');}},
  confirm(message) {assert(message.includes('Original bleibt erhalten')); ++confirmations; return true;},
  setTimeout(callback, delay) {
    if (delay === 0) return callback(); // Hero opens the action view on navigation.
    throw new Error('Reconciled operations must not poll again');
  },
  sessionStorage: {
    setItem(key, value) {storage.set(key, value);},
    getItem(key) {return storage.get(key) || null;},
    removeItem(key) {storage.delete(key);}
  },
  VdrSuiteBrowserSession: {
    restore() {return Promise.resolve(true);},
    csrfHeaders() {return {'X-CSRF-Token': 'test-csrf'};}
  },
  VdrSuitePublicV1Client: {createClient() {return api;}},
  VdrSuiteClientApi: {
    createClientOperationUpdateSource(operationId) {
      assert(/^op_[0-9a-f]{32}$/.test(operationId));
      const source = {
        operationId, closed: false, listeners: {},
        addEventListener(name, callback) {this.listeners[name] = callback;},
        close() {this.closed = true;}
      };
      sources.push(source);
      return source;
    }
  },
  VdrSuiteRecordings2Shared: {
    node(tag, classes, label) {return new Node(tag, classes, label);},
    text(value) {return value == null ? '' : String(value);},
    first(item, fields, fallback) {
      for (const field of fields) if (item && item[field] != null) return item[field];
      return fallback;
    },
    clientApi() {return null;},
    createPoster() {return new Node('span', 'recordings2-poster');},
    createButton(label, callback) {
      const button = new Node('button', '', label);
      button.addEventListener('click', callback);
      return button;
    },
    recordingTitle(recording) {return recording.title || 'Aufnahme';},
    recordingSubtitle() {return '';},
    recordingSummary() {return '';},
    formatStart() {return '';},
    formatDuration() {return '';},
    formatSize() {return '';},
    folderList() {return [];},
    recordingList() {return [];},
    number(value, fallback) {return typeof value === 'number' ? value : fallback;},
    selectedBackendId() {return backendId;},
    normalizePath(value) {return value || '';},
    decodeDisplayText(value) {return value || '';},
    installStyles() {},
    mountTarget() {return listTarget;}
  }
};
context.window = context;
vm.createContext(context);
vm.runInContext(hero, context);
vm.runInContext(fs.readFileSync('web/frontend/recordings2-browser-cards.js', 'utf8'), context);
vm.runInContext(browser, context);
const listTarget = new Node('section');
const root = new Node('main');
const copy = root.appendChild(new Node('div', 'recordings2-detail-copy'));
const actionHost = root.appendChild(new Node('details', 'recordings2-actions'));
const recording = {recordingId, title: 'Testaufnahme'};
async function flush() {
  for (let i = 0; i < 20; i++) await Promise.resolve();
}
function button(label) {
  const nodes = [];
  function visit(node) {
    if (node.tagName === 'button' && node.textContent === label) nodes.push(node);
    node.children.forEach(visit);
  }
  visit(root);
  assert.strictEqual(nodes.length, 1, 'Expected one button: ' + label);
  return nodes[0];
}
function nodesByTag(tag) {
  const nodes = [];
  function visit(node) {if (node.tagName === tag) nodes.push(node);
    node.children.forEach(visit);}
  visit(root);
  return nodes;
}
(async function run() {
  // Card menu -> existing selection callback -> canonical Hero owner.
  const state = {backendId, path: '', selectedRecording: null,
    data: {recordingCount: 1}, recordings: [recording]};
  let selected = null;
  const view = context.VdrSuiteRecordings2BrowserView.create({
    getState() {return state;},
    openFolder() {}, reload() {},
    selectRecording(value) {selected = value;}
  });
  view.renderFolder();
  const menu = listTarget.querySelector('.recordings2-card-action-menu');
  assert(menu, 'recording card offers one ⋯ action menu');
  const cardAction = menu.querySelector('.recordings2-card-action-panel').children.find(n =>
    n.tagName === 'button' && n.textContent === 'Aufnahme verkleinern (HEVC)');
  assert(cardAction && cardAction.disabled,
    'HEVC card action waits for existing backend capability');
  menu.open = true;
  menu.events.toggle();
  await flush();
  assert.strictEqual(cardAction.disabled, false);
  assert.strictEqual(calls.filter(c => c.action === 'stage' ||
    c.action === 'publish' || c.action === 'plan').length, 0,
    'opening the menu is read-only');
  cardAction.click();
  assert(selected && selected.__vdrSuiteOpenHevcShrink === true);
  assert.strictEqual(selected.recordingId, recordingId);

  // An unavailable plan capability must not expose the same card action.
  const approvedSelection = selected;
  const originalCapability = api.getMediaShrinkCapability;
  api.getMediaShrinkCapability = function () {
    return Promise.resolve({backendId, capability: 'addons.media.shrink',
      planningAvailable: false, available: false});
  };
  view.renderFolder();
  const unavailableMenu = listTarget.querySelector('.recordings2-card-action-menu');
  assert(unavailableMenu);
  unavailableMenu.open = true;
  unavailableMenu.events.toggle();
  await flush();
  const unavailableAction = unavailableMenu.querySelector('.recordings2-card-action-panel').children.find(n =>
    n.tagName === 'button' && n.textContent === 'Aufnahme verkleinern (HEVC)');
  assert(unavailableAction && unavailableAction.disabled,
    'unavailable backend must not enable HEVC card action');
  assert.strictEqual(selected, approvedSelection,
    'read-only unavailable result never navigates or starts a job');
  api.getMediaShrinkCapability = originalCapability;

  context.VdrSuiteRecordings2HeroDetail.enhance(root, selected, backendId, {});
  await flush();
  assert.strictEqual(root.dataset.recordings2HeroMode, 'actions',
    'card selection enters canonical recording-actions mode');
  assert.strictEqual(actionHost.open, true);
  assert.strictEqual(actionHost.querySelector('.recordings2-media-check').hidden,
    false, 'existing HEVC workflow panel is open without starting a mutation');
  const shrink = button('Aufnahme verkleinern (HEVC)');
  assert.strictEqual(shrink.parentNode, actionHost,
    'MT.UX6 HEVC action belongs to the existing recording actions');
  shrink.click();
  assert.strictEqual(actionHost.open, true);
  assert(actionHost.querySelector('.recordings2-media-check'),
    'one existing HEVC plan/stage/publish panel, not a second flow');
  const select = nodesByTag('select').find(n =>
    n.className === 'recordings2-media-shrink-encoder');
  assert(select);
  assert.deepStrictEqual(select.children.map(n => n.value),
    ['software', 'vaapi', 'qsv', 'nvenc']);
  select.value = 'vaapi';
  const inputs = nodesByTag('input');
  assert.strictEqual(inputs.length, 2);
  inputs[0].checked = true;
  inputs[1].checked = true;
  // One explicit click starts the verified Stage and durable auto-publish.
  button('Aufnahme verkleinern').click();
  await flush();
  assert.strictEqual(calls.filter(c => c.action === 'plan').length, 1);
  assert.strictEqual(calls[0].request.recordingId, recordingId);
  const stages = calls.filter(c => c.action === 'stage');
  assert.strictEqual(stages.length, 1);
  assert.strictEqual(stages[0].request.encoder, 'vaapi');
  assert.strictEqual(stages[0].request.maxHeight, 720);
  assert.strictEqual(stages[0].request.nightMode, true);
  assert.strictEqual(stages[0].request.autoPublish, true);
  assert.strictEqual(stages[0].request.headers['X-CSRF-Token'], 'test-csrf');
  assert.strictEqual(stages[0].request.recordingId, recordingId);
  const homeActivity = JSON.parse(storage.get(
    'vdr-suite-hevc-current-v1.' + backendId));
  assert(homeActivity && homeActivity.operationId === stageOperationId &&
    homeActivity.recordingId === recordingId &&
    homeActivity.recording.title === 'Testaufnahme',
    'HEVC activity is available as read-only Home shortcut in this session');
  assert.strictEqual(calls.filter(c => c.action === 'readReview').length, 0,
    'dispatching is not a verified HEVC candidate');
  const stream = sources.find(s => s.operationId === stageOperationId);
  assert(stream && !stream.closed, 'HEVC stage uses actor-owned SSE');
  const status = actionHost.querySelector('.recordings2-hevc-operation-status');
  assert(status && status.textContent.includes('Verkleinerung läuft') &&
    status.title.includes(stageOperationId),
    'UI shows simple status; operation identity remains available in diagnostics');
  let readCount = calls.filter(c => c.action === 'readOperation').length;
  stream.listeners.operation({data: JSON.stringify({
    operationId: 'op_' + 'f'.repeat(32)
  })});
  await flush();
  assert.strictEqual(calls.filter(c => c.action === 'readOperation').length,
    readCount, 'foreign Operation event cannot refresh this HEVC operation');

  stageState = 'outcome_unknown';
  stream.listeners.operation({data: JSON.stringify({operationId: stageOperationId})});
  await flush();
  assert(status.textContent.includes('wird überprüft'));
  assert(status.title.includes('Keine automatische Wiederholung'));
  assert.strictEqual(button('Verkleinerung starten').disabled, true,
    'old uncertainty never removes the one-shot safety fence');
  assert.strictEqual(stream.closed, false,
    'outcome_unknown remains subscribed for later independent readback');

  stream.onopen(); // Initial connect/reconnect must read fresh Core truth.
  await flush();
  readCount = calls.filter(c => c.action === 'readOperation').length;
  stream.listeners.resync();
  await flush();
  assert.strictEqual(calls.filter(c => c.action === 'readOperation').length,
    readCount + 1, 'resync reads once rather than polling');

  stageState = 'succeeded'; // Independent Core-side readback, no second POST.
  stream.listeners.operation({data: JSON.stringify({operationId: stageOperationId})});
  await flush();
  assert.strictEqual(calls.filter(c => c.action === 'readReview').length, 1);
  assert.strictEqual(stream.closed, false,
    'one-click Stage stays on existing SSE until publication readback');
  const publish = button('Als neue Aufnahme hinzufügen');
  assert.strictEqual(publish.hidden, true,
    'one-click Stage never asks for a separate Publish action');
  // The existing Stage SSE carries a Core hint when its durable Publish
  // begins. Its published operation ID is obtained by readback, not guessed.
  api.getMediaShrinkRecording = function () {
    calls.push({action: 'findRecording'});
    return Promise.resolve({backendId, recordingId, state: 'publishing',
      autoPublish: true, publishOperationId});
  };
  api.getMediaShrinkReview = function () {
    calls.push({action: 'readReview'});
    return Promise.resolve({backendId, recordingId, reviewId,
      state: 'publishing', revision: 2,
      publishAvailable: false, candidateBytes: 400000,
      sourceBytes: 1000000});
  };
  stream.listeners.operation({data: JSON.stringify({operationId: stageOperationId})});
  await flush();
  const publishedStream = sources.find(x => x.operationId === publishOperationId);
  assert(publishedStream && !publishedStream.closed,
    'Core auto-publish uses the existing Publish SSE subscription');
  publishedStream.listeners.operation({
    data: JSON.stringify({operationId: publishOperationId})
  });
  await flush();
  assert(status.textContent.includes('HEVC-Aufnahme') ||
         status.textContent.includes('Aufnahme bestätigt'),
    'automatic publication completion reaches the UI without polling');
  assert.strictEqual(confirmations, 0);
  assert.strictEqual(calls.filter(c => c.action === 'stage').length, 1);
  assert.strictEqual(calls.filter(c => c.action === 'publish').length, 0,
    'the durable Core, not the browser, continues publication');
  assert(storage.size >= 2);
  for (const entry of calls) assert(!JSON.stringify(entry).includes('/srv/'));

  // Another page opening the same recording during a temporary capability
  // outage must still show its existing uncertain operation and ID. A new
  // native HEVC command is never submitted from this read-only recovery.
  stageState = 'outcome_unknown';
  storage.delete('vdr-suite-hevc-v1.' + backendId + '.' + recordingId +
    '.publish'); // This recovery fixture has only an old Stage.
  api.getMediaShrinkCapability = function () {
    return Promise.resolve({backendId, capability: 'addons.media.shrink',
      planningAvailable: true, available: false, encoders: ['software']});
  };
  api.submitMediaShrinkPlan = function () {
    return Promise.resolve({backendId, recordingId, readOnly: true,
      stageAvailable: false, sourceBytes: 1000000, segments: 1});
  };
  const restoredRoot = new Node('main');
  restoredRoot.appendChild(new Node('div', 'recordings2-detail-copy'));
  restoredRoot.appendChild(new Node('details', 'recordings2-actions'));
  context.VdrSuiteRecordings2HeroDetail.enhance(
    restoredRoot, recording, backendId, {});
  await flush();
  function restoredButton(label) {
    const visit = function (node) {
      if (node.tagName === 'button' && node.textContent === label) return node;
      for (const child of node.children) {
        const found = visit(child);
        if (found) return found;
      }
      return null;
    };
    return visit(restoredRoot);
  }
  const restoredStatusBeforePlan =
    restoredRoot.querySelector('.recordings2-hevc-operation-status');
  const restoredPlanStatus =
    restoredRoot.querySelector('.recordings2-hevc-plan-status');
  assert(restoredStatusBeforePlan &&
    restoredStatusBeforePlan.textContent.includes('wird überprüft') &&
    restoredStatusBeforePlan.title.includes(stageOperationId),
    'the old stage is restored on detail mount with no manual preflight');
  const planCountBefore = calls.filter(c => c.action === 'plan').length;
  assert.strictEqual(planCountBefore, 1,
    'passive restore must not POST a new Plan request');
  const preview = restoredButton('Aufnahme verkleinern');
  assert(preview);
  preview.click();
  await flush();
  assert(restoredPlanStatus &&
    restoredPlanStatus.textContent.includes('Original:') &&
    restoredStatusBeforePlan.textContent.includes('wird überprüft'),
    'late preflight message cannot overwrite operation result');
  const savedControls =
    restoredRoot.querySelector('.recordings2-media-shrink-controls');
  assert(savedControls && !savedControls.hidden,
    'existing uncertain HEVC remains readable when capability is unavailable');
  assert(restoredButton('Verkleinerung starten').disabled,
    'unavailable capability never authorizes another Stage POST');
  const restoredStatus =
    restoredRoot.querySelector('.recordings2-hevc-operation-status');
  assert(restoredStatus.textContent.includes('wird überprüft') &&
         restoredStatus.title.includes(stageOperationId),
    'read-only recovery shows the restored operation in a simple status');
  assert.strictEqual(calls.filter(c => c.action === 'stage').length, 1,
    'read-only recovery never reissues an old Stage');

  // A genuinely verified no-effect result is distinct from outcome_unknown:
  // it lifts the saved one-shot block but NEVER dispatches automatically.
  api.getMediaShrinkCapability = originalCapability;
  api.submitMediaShrinkPlan = function () {
    return Promise.resolve({backendId, recordingId, readOnly: true,
      stageAvailable: true, sourceBytes: 1000000, segments: 1});
  };
  stageState = 'failed_verified';
  const retryRoot = new Node('main');
  retryRoot.appendChild(new Node('div', 'recordings2-detail-copy'));
  retryRoot.appendChild(new Node('details', 'recordings2-actions'));
  context.VdrSuiteRecordings2HeroDetail.enhance(
    retryRoot, recording, backendId, {});
  await flush();
  const retryButton = function (label) {
    const visit = function (node) {
      if (node.tagName === 'button' && node.textContent === label) return node;
      for (const child of node.children) {
        const found = visit(child);
        if (found) return found;
      }
      return null;
    };
    return visit(retryRoot);
  };
  const failedLabel = retryRoot.querySelector('.recordings2-hevc-operation-status');
  assert(failedLabel.textContent.includes('Verkleinerung fehlgeschlagen') &&
         failedLabel.title.includes(stageOperationId) &&
         failedLabel.textContent.includes('FFmpeg konnte die Aufnahme'),
    'verified failure shows a clear message without technical state labels');
  assert.strictEqual(storage.has('vdr-suite-hevc-v1.' + backendId +
    '.' + recordingId + '.stage'), false,
    'verified terminal stage releases the old session fence');
  assert.strictEqual(calls.filter(c => c.action === 'stage').length, 1,
    'status and SSE readback never perform a new POST');
  retryButton('Aufnahme verkleinern').click();
  await flush();
  const afterRetry = calls.filter(c => c.action === 'stage');
  assert.strictEqual(afterRetry.length, 2,
    'only a NEW user click initiates another HEVC stage');
  assert.notStrictEqual(afterRetry[0].request.idempotencyKey,
    afterRetry[1].request.idempotencyKey,
    'retry must have a new idempotency identity');

  // Reopened on a different phone: no sessionStorage operation reference,
  // but Core has already recovered the HEVC file and review.
  storage.delete('vdr-suite-hevc-v1.' + backendId + '.' + recordingId + '.stage');
  storage.delete('vdr-suite-hevc-v1.' + backendId + '.' + recordingId + '.publish');
  durableState = 'ready';
  api.getMediaShrinkCapability = originalCapability;
  const recoveredRoot = new Node('main');
  recoveredRoot.appendChild(new Node('div', 'recordings2-detail-copy'));
  recoveredRoot.appendChild(new Node('details', 'recordings2-actions'));
  const countBefore = calls.filter(c =>
    ['plan', 'stage', 'publish'].includes(c.action)).length;
  context.VdrSuiteRecordings2HeroDetail.enhance(
    recoveredRoot, recording, backendId, {});
  await flush();
  const recoveredStatus =
    recoveredRoot.querySelector('.recordings2-hevc-operation-status');
  const recoveredControls =
    recoveredRoot.querySelector('.recordings2-media-shrink-controls');
  assert(recoveredStatus && recoveredStatus.textContent.includes('HEVC fertig'),
    'finished candidate is immediately visible without a manual preflight');
  assert(recoveredControls && !recoveredControls.hidden);
  assert(recoveredRoot.querySelector('.recordings2-media-shrink-setup').hidden,
    'encoder settings hidden when finished HEVC recording already exists');
  const recoveredButtons = [];
  (function visit(node) {
    if (node.tagName === 'button') recoveredButtons.push(node);
    node.children.forEach(visit);
  })(recoveredRoot);
  const add = recoveredButtons.find(node =>
    node.textContent === 'Als neue Aufnahme hinzufügen');
  assert(add && !add.hidden,
    'finished HEVC file is offered for explicit addition');
  assert(recoveredButtons.find(node =>
    node.textContent === 'Aufnahme verkleinern').hidden,
    'existing result never asks for a new preflight');
  assert.strictEqual(calls.filter(c =>
    ['plan', 'stage', 'publish'].includes(c.action)).length, countBefore,
    'read-only recovery never starts a second conversion');

  console.log('MEDIA_SHRINK_HERO_ONE_CLICK_AUTO_PUBLISH=PASS');
})().catch(error => {console.error(error); process.exitCode = 1;});
