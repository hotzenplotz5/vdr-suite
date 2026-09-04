'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

function element(tag) {
  return {
    tagName: String(tag || '').toUpperCase(),
    id: '',
    className: '',
    dataset: {},
    children: [],
    textContent: '',
    attributes: {},
    setAttribute(name, value) { this.attributes[name] = String(value); },
    appendChild(child) { this.children.push(child); return child; },
    replaceChildren() { this.children = Array.from(arguments); },
    querySelector(selector) {
      if (selector === '.recordings2-detail' && this.className.indexOf('recordings2-detail') !== -1) return this;
      for (const child of this.children) {
        if (child && typeof child.querySelector === 'function') {
          const found = child.querySelector(selector);
          if (found) return found;
        }
      }
      return null;
    }
  };
}

function allText(root) {
  let result = root && root.textContent ? String(root.textContent) : '';
  (root && root.children || []).forEach(child => { result += ' ' + allText(child); });
  return result;
}

const styles = [];
const document = {
  head: { appendChild(value) { styles.push(value); } },
  createElement: element,
  getElementById(id) { return styles.find(style => style.id === id) || null; }
};

const detailRoot = element('section');
detailRoot.className = 'recordings2 recordings2-detail';
const mount = element('div');
mount.appendChild(detailRoot);
let originalDetailRenders = 0;
let requestedPath = '';
let requestedOptions = null;

const shared = {
  mountTarget() { return mount; }
};

const originalBrowserOwner = {
  create() {
    return {
      renderLoading() {},
      renderError() {},
      renderFolder() {},
      renderDetail() { originalDetailRenders += 1; },
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
  fs.readFileSync('web/frontend/recordings2-marks-detail.js', 'utf8'),
  context,
  {filename: 'web/frontend/recordings2-marks-detail.js'}
);

const api = window.VdrSuiteRecordings2MarksDetail;
assert.ok(api);
assert.notStrictEqual(window.VdrSuiteRecordings2BrowserView, originalBrowserOwner);

const recording = {
  id: '7',
  backendId: 'default',
  backendNativeId: '/srv/vdr/video/Secret/2026-09-04.07.00.1-0.rec',
  path: 'Secret'
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
  assert.strictEqual(
    detailRoot.children[0].dataset.marksRevision,
    '0123456789abcdef0123456789abcdef'
  );

  assert.strictEqual(
    api.errorText(new Error('recording_marks_capability_unavailable')),
    'Native Schnittmarken werden von diesem VDR derzeit nicht angeboten.'
  );
  assert.strictEqual(
    api.errorText(new Error('recording_native_state_stale')),
    'Der Aufnahmestand hat sich geändert. Bitte die Aufnahmedetails neu laden.'
  );
  console.log('recordings2 native marks read detail ok');
}).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
