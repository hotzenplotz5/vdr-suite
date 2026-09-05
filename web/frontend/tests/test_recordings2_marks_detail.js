'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

function hasClass(value, className) {
  return String(value || '').split(/\s+/).filter(Boolean).includes(className);
}

function element(tag) {
  const value = {
    tagName: String(tag || '').toUpperCase(),
    id: '',
    className: '',
    dataset: {},
    children: [],
    textContent: '',
    attributes: {},
    style: {},
    parentNode: null,
    title: '',
    setAttribute(name, attributeValue) { this.attributes[name] = String(attributeValue); },
    appendChild(child) {
      if (child) child.parentNode = this;
      this.children.push(child);
      return child;
    },
    replaceChildren() {
      this.children.forEach(child => {
        if (child && child.parentNode === this) child.parentNode = null;
      });
      this.children = Array.from(arguments);
      this.children.forEach(child => {
        if (child) child.parentNode = this;
      });
    },
    insertAdjacentElement(position, child) {
      assert.strictEqual(position, 'afterend');
      assert.ok(this.parentNode);
      const index = this.parentNode.children.indexOf(this);
      assert.ok(index >= 0);
      child.parentNode = this.parentNode;
      this.parentNode.children.splice(index + 1, 0, child);
      return child;
    },
    querySelector(selector) {
      if (selector === '.recordings2-detail' && hasClass(this.className, 'recordings2-detail')) return this;
      if (selector === '.recordings2-marks-detail' && hasClass(this.className, 'recordings2-marks-detail')) return this;
      if (selector === '.recordings2-marks-timeline' && hasClass(this.className, 'recordings2-marks-timeline')) return this;
      if (selector === 'input[aria-label="Wiedergabeposition"]' &&
          this.tagName === 'INPUT' && this.attributes['aria-label'] === 'Wiedergabeposition') return this;
      for (const child of this.children) {
        if (child && typeof child.querySelector === 'function') {
          const found = child.querySelector(selector);
          if (found) return found;
        }
      }
      return null;
    },
    querySelectorAll(selector) {
      const result = [];
      function collect(current) {
        if (!current) return;
        if (selector === 'input[aria-label="Wiedergabeposition"]' &&
            current.tagName === 'INPUT' && current.attributes['aria-label'] === 'Wiedergabeposition') {
          result.push(current);
        }
        (current.children || []).forEach(collect);
      }
      collect(this);
      return result;
    }
  };
  return value;
}

function allText(root) {
  let result = root && root.textContent ? String(root.textContent) : '';
  (root && root.children || []).forEach(child => { result += ' ' + allText(child); });
  return result;
}

function playbackTimeline() {
  const controls = element('div');
  controls.className = 'recordings2-playback-controls';
  const timeline = element('input');
  timeline.type = 'range';
  timeline.min = '0';
  timeline.max = '0';
  timeline.setAttribute('aria-label', 'Wiedergabeposition');
  controls.appendChild(timeline);
  return {controls, timeline};
}

const styles = [];
const document = {
  head: { appendChild(value) { styles.push(value); } },
  createElement: element,
  getElementById(id) { return styles.find(style => style.id === id) || null; }
};

const detailRoot = element('section');
detailRoot.className = 'recordings2 recordings2-detail';

// Model the production Recording compatibility owner: its stable owner-level
// timeline follows a replaceable transport host that itself contains another
// timeline. A root.querySelector() therefore finds the wrong transport-local
// timeline first and loses native marks when MediaSession startup replaces it.
const initialTransportPlayback = playbackTimeline();
const transportHost = element('div');
transportHost.className = 'recordings2-recording-fallback-transport';
transportHost.appendChild(initialTransportPlayback.controls);
const stablePlayback = playbackTimeline();
const playbackControls = stablePlayback.controls;
const timeline = stablePlayback.timeline;
const playbackOwnerElement = element('div');
playbackOwnerElement.className = 'recordings2-recording-fallback-shell';
playbackOwnerElement.appendChild(transportHost);
playbackOwnerElement.appendChild(playbackControls);

const mount = element('div');
mount.appendChild(detailRoot);
let originalDetailRenders = 0;
let requestedPath = '';
let requestedOptions = null;
let lifecycleListener = null;
let unsubscribeCount = 0;

const playbackOwner = {
  element: playbackOwnerElement,
  subscribe(listener) {
    lifecycleListener = listener;
    listener({transition: 'snapshot', state: 'idle', transport: 'hls-compatibility'});
    return function () { unsubscribeCount += 1; };
  }
};

const shared = {
  mountTarget() { return mount; }
};

const originalBrowserOwner = {
  create() {
    return {
      renderLoading() {},
      renderError() {},
      renderFolder() {},
      renderDetail() {
        originalDetailRenders += 1;
        detailRoot.__vdrSuiteRecordingPlaybackOwner = playbackOwner;
        if (playbackOwnerElement.parentNode !== detailRoot) {
          detailRoot.appendChild(playbackOwnerElement);
        }
      },
      destroy() {}
    };
  },
  createRecordingCard() {}
};

const payload = {
  backendId: 'default',
  recordingId: '7',
  availability: 'available',
  state: 'present',
  framesPerSecond: 25,
  inUse: true,
  inUseFlags: 2,
  marksFilePresent: true,
  sequenceCount: 1,
  marksRevision: '0123456789abcdef0123456789abcdef',
  marks: [
    {positionFrame: 250, timecode: '0:00:10.00', positionSeconds: 10, comment: 'begin'},
    {positionFrame: 500, timecode: '0:00:20.00', positionSeconds: 20, comment: 'end'}
  ]
};

const window = {
  document,
  VdrSuiteRecordings2Shared: shared,
  VdrSuiteRecordings2BrowserView: originalBrowserOwner,
  VdrSuiteClientApi: {
    requestJson(path, options) {
      requestedPath = path;
      requestedOptions = options;
      return Promise.resolve(payload);
    }
  }
};

const context = vm.createContext({
  window,
  document,
  console,
  Promise,
  Number,
  String,
  Object,
  Array,
  Math
});

vm.runInContext(
  fs.readFileSync('web/frontend/recordings2-marks-timeline.js', 'utf8'),
  context,
  {filename: 'web/frontend/recordings2-marks-timeline.js'}
);
vm.runInContext(
  fs.readFileSync('web/frontend/recordings2-marks-detail.js', 'utf8'),
  context,
  {filename: 'web/frontend/recordings2-marks-detail.js'}
);

const api = window.VdrSuiteRecordings2MarksDetail;
assert.ok(api);
assert.ok(window.VdrSuiteRecordings2MarksTimeline);
assert.notStrictEqual(window.VdrSuiteRecordings2BrowserView, originalBrowserOwner);

const recording = {
  id: '7',
  backendId: 'default',
  backendNativeId: '/srv/vdr/video/Secret/2026-09-04.07.00.1-0.rec',
  path: 'Secret',
  durationSeconds: 40
};

api.fetchMarks(recording, 'default').then(result => {
  assert.strictEqual(result, payload);
  assert.strictEqual(requestedPath, '/api/vdr/recordings/marks');
  assert.deepStrictEqual(
    JSON.parse(JSON.stringify(requestedOptions.query)),
    {backend: 'default', recordingId: '7'}
  );
  assert.strictEqual(requestedOptions.cache, 'no-store');
  assert.strictEqual(requestedOptions.credentials, 'same-origin');
  assert.strictEqual(JSON.stringify(requestedOptions).includes('backendNativeId'), false);
  assert.strictEqual(JSON.stringify(requestedOptions).includes('/srv/vdr'), false);

  const view = window.VdrSuiteRecordings2BrowserView.create({
    getState() {
      return {selectedRecording: recording, backendId: 'default'};
    }
  });
  view.renderDetail();
  return Promise.resolve();
}).then(() => Promise.resolve()).then(() => {
  assert.strictEqual(originalDetailRenders, 1);
  assert.strictEqual(detailRoot.dataset.recordings2MarksDetail, 'true');
  const rendered = allText(detailRoot);
  assert.ok(rendered.includes('Schnitt / Schnittmarken'));
  assert.ok(rendered.includes('2 Schnittmarken'));
  assert.ok(rendered.includes('1 Schnittbereich'));
  assert.ok(rendered.includes('0:00:10.00'));
  assert.ok(rendered.includes('Frame 250'));
  assert.ok(rendered.includes('Aufnahme wird aktuell von VDR verwendet'));
  assert.strictEqual(rendered.includes('/srv/vdr'), false);

  const marksPanel = detailRoot.querySelector('.recordings2-marks-detail');
  assert.ok(marksPanel);
  assert.strictEqual(
    marksPanel.dataset.marksRevision,
    '0123456789abcdef0123456789abcdef'
  );

  assert.strictEqual(initialTransportPlayback.timeline.dataset.nativeMarksVisible, undefined);
  assert.strictEqual(timeline.dataset.nativeMarksVisible, 'true');
  assert.strictEqual(timeline.dataset.nativeMarksCount, '2');
  assert.strictEqual(timeline.dataset.nativeMarksRevision, payload.marksRevision);
  const rail = detailRoot.querySelector('.recordings2-marks-timeline');
  assert.ok(rail, 'read-only marks must decorate the stable compatibility-owner timeline');
  assert.strictEqual(rail.parentNode, playbackControls);
  assert.strictEqual(playbackControls.children[0], timeline);
  assert.strictEqual(playbackControls.children[1], rail);
  assert.strictEqual(rail.dataset.durationSeconds, '40');
  assert.strictEqual(
    rail.attributes['aria-label'],
    '2 native Schnittmarken auf der Wiedergabe-Zeitlinie'
  );
  assert.strictEqual(rail.children.length, 2);
  assert.strictEqual(rail.children[0].style.left, '25.00000%');
  assert.strictEqual(rail.children[1].style.left, '50.00000%');
  assert.strictEqual(rail.children[0].dataset.positionFrame, '250');
  assert.strictEqual(rail.children[0].title, '0:00:10.00 · begin');
  assert.strictEqual(typeof lifecycleListener, 'function');

  lifecycleListener({
    transition: 'start-requested',
    state: 'starting',
    sessionId: null,
    transport: 'hls-compatibility'
  });
  assert.strictEqual(
    detailRoot.querySelector('.recordings2-marks-timeline'),
    rail,
    'MediaSession preparation must not move marks into the replaceable transport'
  );
  assert.strictEqual(rail.parentNode, playbackControls);

  const replacementTransportPlayback = playbackTimeline();
  transportHost.replaceChildren(replacementTransportPlayback.controls);
  lifecycleListener({
    transition: 'transport-replaced',
    state: 'starting',
    sessionId: null,
    transport: 'hls-compatibility'
  });

  assert.strictEqual(replacementTransportPlayback.timeline.dataset.nativeMarksVisible, undefined);
  assert.strictEqual(timeline.dataset.nativeMarksVisible, 'true');
  assert.strictEqual(timeline.dataset.nativeMarksCount, '2');
  assert.strictEqual(timeline.dataset.nativeMarksRevision, payload.marksRevision);
  const fallbackRail = detailRoot.querySelector('.recordings2-marks-timeline');
  assert.strictEqual(fallbackRail, rail, 'stable marks rail must survive compatibility transport replacement');
  assert.strictEqual(fallbackRail.parentNode, playbackControls);
  assert.strictEqual(playbackControls.children[0], timeline);
  assert.strictEqual(playbackControls.children[1], fallbackRail);
  assert.strictEqual(fallbackRail.children.length, 2);
  assert.strictEqual(fallbackRail.children[0].style.left, '25.00000%');
  assert.strictEqual(
    detailRoot.children.filter(child => hasClass(child.className, 'recordings2-marks-detail')).length,
    1,
    'the detailed marks list remains present alongside the stable timeline markers'
  );

  assert.strictEqual(
    api.errorText(new Error('recording_marks_capability_unavailable')),
    'Native Schnittmarken werden von diesem VDR derzeit nicht angeboten.'
  );
  assert.strictEqual(
    api.errorText(new Error('recording_native_state_stale')),
    'Der Aufnahmestand hat sich geändert. Bitte die Aufnahmedetails neu laden.'
  );

  lifecycleListener({transition: 'destroyed', state: 'destroyed', transport: 'none'});
  assert.strictEqual(unsubscribeCount, 1);
  console.log('recordings2 native marks survive compatibility MediaSession preparation');
}).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
