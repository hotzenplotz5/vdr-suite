'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const SOURCE = 'web/frontend/recordings2.js';
const CUT = '/srv/vdr/video/%Brisant/2026-09-07.17.16.1-0.rec';
const ORIGINAL = '/srv/vdr/video/Brisant/2026-09-07.17.13.1-0.rec';

function deferred() {
  let resolve;
  let reject;
  const promise = new Promise((yes, no) => { resolve = yes; reject = no; });
  return {promise, resolve, reject};
}
async function flush() {
  for (let i = 0; i < 8; i += 1) await Promise.resolve();
}

function makeHarness() {
  let now = 0;
  let nextTimer = 1;
  let backend = 'default';
  let owner;
  let failLeaves = false;
  const timers = new Map();
  const listeners = {};
  const requests = [];
  const renders = [];
  const modules = new Map();
  const target = {classList: {remove() {}}, querySelector() { return null; }};
  const tab = {
    classList: {contains() { return true; }, toggle() {}},
    addEventListener() {}
  };
  const document = {
    hidden: false,
    querySelector(selector) { return selector === '[data-module="recordings2"]' ? tab : null; },
    querySelectorAll() { return []; },
    getElementById() { return null; },
    addEventListener(name, callback) { (listeners[name] ||= []).push(callback); }
  };
  const shared = {
    PAGE_SIZE: 2,
    selectedBackendId() { return backend; },
    clientApi() { return {fetchClientRecordingFolder(options) {
      const pending = deferred();
      requests.push({options, pending});
      return pending.promise;
    }}; },
    normalizePath(path) { return String(path || '').replace(/^\/+|\/+$/g, ''); },
    number(value, fallback) { const n = Number(value); return Number.isFinite(n) ? n : fallback; },
    first(object, keys, fallback) {
      for (const key of keys) if (object && object[key] !== undefined) return object[key];
      return fallback;
    },
    folderList(data) { return data && Array.isArray(data.folders) ? data.folders : []; },
    recordingList(data) { return data && Array.isArray(data.recordings) ? data.recordings : []; },
    recordingPathTitle(recording) { return recording && recording.title || ''; },
    platform() { return {
      registerModule(name, api) { modules.set(name, api); },
      hasModule(name) { return modules.has(name); }
    }; },
    mountTarget() { return target; }
  };
  for (const name of ['decodeDisplayText', 'recordingNativeTitle', 'recordingMetadataTitle',
    'recordingTitle', 'recordingSubtitle', 'recordingSummary', 'recordingPosterUrl',
    'formatDuration', 'formatSize']) shared[name] = value => String(value || '');
  const browserView = {create(options) {
    owner = options;
    return {
      renderLoading() { renders.push('loading'); },
      renderError() { renders.push('error'); },
      renderFolder() { renders.push('folder'); },
      renderDetail() { renders.push('detail'); },
      destroy() { renders.push('destroy'); }
    };
  }};
  const window = {
    document,
    setTimeout(callback, delay) {
      const id = nextTimer++;
      timers.set(id, {at: now + (delay || 0), callback});
      return id;
    },
    clearTimeout(id) { timers.delete(id); },
    VdrSuiteRecordings2Shared: shared,
    VdrSuiteRecordings2FolderArtwork: {
      resolveLeaves(data) {
        if (failLeaves) { failLeaves = false; return Promise.reject(new Error('leaf unavailable')); }
        return Promise.resolve({folders: data.folders, recordings: []});
      }
    },
    VdrSuiteRecordings2BrowserView: browserView
  };
  const context = vm.createContext({window, document, console, Date, Promise, Object,
    String, Number, Math, Array, JSON, Map, Set});
  vm.runInContext(fs.readFileSync(SOURCE, 'utf8'), context, {filename: SOURCE});
  const api = window.VdrSuiteRecordings2;
  assert.strictEqual(modules.get('recordings2'), api);
  async function advance(milliseconds) {
    now += milliseconds;
    const due = [...timers.entries()].filter(([, timer]) => timer.at <= now)
      .sort((a, b) => a[1].at - b[1].at);
    for (const [id, timer] of due) {
      if (!timers.has(id)) continue;
      timers.delete(id);
      timer.callback();
      await flush();
    }
  }
  function page(path, recordings, folders, count) {
    return {
      recordingFolder: true, backendId: backend, path,
      parentPath: path.split('/').slice(0, -1).join('/'),
      folders: folders || [], recordings: recordings || [],
      recordingCount: count === undefined ? (recordings || []).length : count,
      totalCount: 1002
    };
  }
  async function respond(index, data) {
    requests[index].pending.resolve(data);
    await flush();
  }
  return {api, owner: () => owner, requests, renders, timers, document, listeners,
    advance, page, respond, failNextLeaves() { failLeaves = true; },
    setBackend(value) { backend = value; }};
}

async function run() {
  const h = makeHarness();
  const {api, requests, renders} = h;
  const original = {id: '1000', title: 'Brisant', backendNativeId: ORIGINAL};
  const edited = {id: '1001', title: '%Brisant', backendNativeId: CUT};
  const root = h.page('', [], [{name: 'Brisant', path: 'Brisant', recordingCount: 1}], 0);
  const updated = h.page('', [], [
    {name: '%Brisant', path: '%Brisant', recordingCount: 1},
    {name: 'Brisant', path: 'Brisant', recordingCount: 1}
  ], 0);

  api.activate();
  assert.strictEqual(requests.length, 1);
  assert.strictEqual(requests[0].options.cache, 'no-store');
  await h.respond(0, root);
  assert.strictEqual(h.timers.size, 1);
  const initialRenders = renders.length;

  // An unchanged folder is not rebuilt or made to flash a loading screen.
  await h.advance(30000);
  assert.strictEqual(requests.length, 2);
  await h.respond(1, root);
  assert.strictEqual(renders.length, initialRenders);

  // A native cut arriving in the canonical cache appears without manual refresh.
  await h.advance(30000);
  await h.respond(2, updated);
  assert.strictEqual(h.owner().getState().data.folders[0].path, '%Brisant');
  assert.strictEqual(renders.at(-1), 'folder');
  assert.strictEqual(h.timers.size, 1);

  // An already-active owner checks immediately on reactivation.
  api.activate();
  await h.advance(0);
  assert.strictEqual(requests.length, 4);
  await h.respond(3, updated);

  // Preserve the number of already loaded rows across an automatic refresh.
  api.openFolder('%Brisant');
  await h.respond(4, h.page('%Brisant', [edited, original], [], 3));
  h.owner().loadMore();
  await h.respond(5, h.page('%Brisant', [{id: '1003', title: 'Extra'}], [], 3));
  await h.advance(30000);
  assert.strictEqual(requests[6].options.query.limit, 3);
  await h.respond(6, h.page('%Brisant', [edited, original, {id: '1003'}], [], 3));
  assert.strictEqual(h.owner().getState().serverRecordings.length, 3);

  // Selection and playback ownership must not be destroyed by background work.
  h.owner().selectRecording(edited);
  const detail = h.owner().getState().selectedRecording;
  const detailRenders = renders.length;
  await h.advance(90000);
  assert.strictEqual(requests.length, 7);
  assert.strictEqual(h.owner().getState().selectedRecording, detail);
  assert.strictEqual(renders.length, detailRenders);
  h.owner().closeDetail();
  await h.advance(0);
  await h.respond(7, h.page('%Brisant', [edited], [], 1));

  // A request from an old folder must never overwrite a newer navigation.
  await h.advance(30000);
  assert.strictEqual(requests.length, 9);
  h.owner().openFolder('Brisant');
  assert.strictEqual(requests.length, 10);
  await h.respond(9, h.page('Brisant', [original]));
  await h.respond(8, h.page('%Brisant', [edited]));
  assert.strictEqual(h.owner().getState().path, 'Brisant');

  // Hidden tabs do not poll; visibility restoration checks the current folder.
  h.document.hidden = true;
  await h.advance(30000);
  assert.strictEqual(requests.length, 10);
  h.document.hidden = false;
  h.listeners.visibilitychange[0]();
  await h.advance(0);
  await h.respond(10, h.page('Brisant', [original]));

  // Failed background requests keep the usable view and are retried.
  await h.advance(30000);
  const beforeFailure = renders.length;
  requests[11].pending.reject(new Error('temporary outage'));
  await flush();
  assert.strictEqual(renders.length, beforeFailure);
  assert.strictEqual(h.owner().getState().error, null);
  await h.advance(30000);
  h.failNextLeaves();
  await h.respond(12, h.page('Brisant', [original, edited]));
  assert.strictEqual(h.owner().getState().serverSignature, '');
  assert.strictEqual(renders.at(-1), 'folder');
  await h.advance(30000);
  await h.respond(13, h.page('Brisant', [original, edited]));
  assert.notStrictEqual(h.owner().getState().serverSignature, '');

  // A backend switch uses the selected backend and discards the old owner.
  h.setBackend('other');
  api.activate();
  assert.strictEqual(requests[14].options.query.backend, 'other');
  await h.respond(14, h.page('', []));
  api.deactivate();
  assert.strictEqual(h.timers.size, 0);
  await h.advance(90000);
  assert.strictEqual(requests.length, 15);
  assert.strictEqual(h.owner().getState().active, false);
  console.log('recordings2 automatic folder refresh lifecycle ok');
}

run().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
