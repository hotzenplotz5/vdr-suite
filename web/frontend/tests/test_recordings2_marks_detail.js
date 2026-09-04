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
const initialPlayback = playbackTimeline();
const playbackControls = initialPlayback.controls;
const timeline = initialPlayback.timeline;
const mount = element('div');
mount.appendChild(detailRoot);
let originalDetailRenders = 0;
let requestedPath = '';
let requestedOptions = null;
let lifecycleListener = null;
let unsubscribeCount = 0;

const playbackOwner = {
  subscribe(listener) {
    lifecycleListener = listener;
    listener({transition: 'snapshot', state: 'idle', transport: 'none'});
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
        if (!detailRoot.querySelector('input[aria-label="Wiedergabeposition"]')) {
          detailRoot.appendChild(playbackControls);
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

  assert.strictEqual(timeline.dataset.nativeMarksVisible, 'true');
  assert.strictEqual(timeline.dataset.nativeMarksCount, '2');
  assert.strictEqual(timeline.dataset.nativeMarksRevision, payload.marksRevision);
  const rail = detailRoot.querySelector('.recordings2-marks-timeline');
  assert.ok(rail, 'read-only marks must decorate the existing canonical playback timeline');
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

  const fallbackPlayback = playbackTimeline();
  const oldControlsIndex = detailRoot.children.indexOf(playbackControls);
  assert.ok(oldControlsIndex >= 0);
  detailRoot.children[oldControlsIndex] = fallbackPlayback.controls;
  playbackControls.parentNode = null;
  fallbackPlayback.controls.parentNode = detailRoot;

  lifecycleListener({
    transition: 'transport-replaced',
    state: 'starting',
    sessionId: null,
    transport: 'hls-compatibility'
  });

  assert.strictEqual(fallbackPlayback.timeline.dataset.nativeMarksVisible, 'true');
  assert.strictEqual(fallbackPlayback.timeline.dataset.nativeMarksCount, '2');
  assert.strictEqual(fallbackPlayback.timeline.dataset.nativeMarksRevision, payload.marksRevision);
  const fallbackRail = detailRoot.querySelector('.recordings2-marks-timeline');
  assert.ok(fallbackRail, 'marks must be rebound after progressive-to-HLS owner replacement');
  assert.strictEqual(fallbackRail.parentNode, fallbackPlayback.controls);
  assert.strictEqual(fallbackPlayback.controls.children[0], fallbackPlayback.timeline);
  assert.strictEqual(fallbackPlayback.controls.children[1], fallbackRail);
  assert.strictEqual(fallbackRail.children.length, 2);
  assert.strictEqual(fallbackRail.children[0].style.left, '25.00000%');
  assert.strictEqual(
    detailRoot.children.filter(child => hasClass(child.className, 'recordings2-marks-detail')).length,
    1,
    'the detailed marks list remains present alongside the replacement timeline markers'
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
  console.log('recordings2 native marks survive canonical playback replacement');
}).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
