'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const actionsSource = fs.readFileSync('web/frontend/recordings2-actions.js', 'utf8');

function first(object, keys, fallback) {
  for (const key of keys) {
    if (object && object[key] !== undefined && object[key] !== null && object[key] !== '') {
      return object[key];
    }
  }
  return fallback;
}

function element() {
  return {
    id: '',
    isConnected: true,
    disabled: false,
    handlers: {},
    click() { if (!this.disabled && this.handlers.click) this.handlers.click(); },
    className: '',
    dataset: {},
    style: {},
    children: [],
    classList: {
      values: new Set(),
      add(value) { this.values.add(value); },
      contains(value) { return this.values.has(value); }
    },
    setAttribute() {},
    appendChild(child) { this.children.push(child); return child; },
    append() { this.children.push(...arguments); },
    addEventListener(name, fn) { this.handlers[name] = fn; }
  };
}

const document = {
  head: {appendChild() {}},
  getElementById() { return null; },
  createElement() { return element(); }
};

let publicBasePath = '';
function resolvePublicPath(path) {
  const value = String(path || '');
  if (!publicBasePath) return value;
  if (value === publicBasePath || value.startsWith(publicBasePath + '/')) return value;
  return publicBasePath + (value.startsWith('/') ? value : '/' + value);
}

const shared = {
  first,
  number(value, fallback) {
    const result = Number(value);
    return Number.isFinite(result) ? result : (fallback || 0);
  },
  normalizePath(value) {
    return String(value || '').replace(/^\/+|\/+$/g, '')
      .split('/').map(part => part.trim()).filter(Boolean).join('/');
  },
  decodeDisplayText(value) { return String(value || '').replace(/_/g, ' '); },
  recordingTitle(recording) { return String(first(recording, ['title'], 'Aufnahme')); },
  selectedBackendId() { return 'default'; },
  clientApi() { return null; },
  PAGE_SIZE: 50,
  folderList(data) { return data && Array.isArray(data.folders) ? data.folders : []; },
  recordingList(data) { return data && Array.isArray(data.recordings) ? data.recordings : []; },
  node(tag, cls, text) { return Object.assign(element(), {className: cls, textContent: text}); },
  createButton(text, fn) { const e = Object.assign(element(), {textContent: text}); e.addEventListener('click', fn); return e; }
};

const window = {
  VdrSuiteRecordings2Shared: shared,
  VdrSuitePublicUrl: {resolvePath: resolvePublicPath},
  setTimeout() {},
  confirm() { return true; }
};

const context = vm.createContext({
  window,
  document,
  console,
  Object,
  String,
  Number,
  Array,
  Promise,
  Set
});

[
  'web/frontend/recordings2-folder-artwork.js',
  'web/frontend/recordings2-actions.js'
].forEach(path => {
  vm.runInContext(fs.readFileSync(path, 'utf8'), context, {filename: path});
});

async function main() {
  assert.ok(window.VdrSuiteRecordings2FolderArtwork);
  assert.ok(window.VdrSuiteRecordings2Actions);

  const genre = window.VdrSuiteRecordings2FolderArtwork;
  assert.strictEqual(genre.normalizeName('Science-Fiction'), 'sciencefiction');
  assert.strictEqual(genre.forFolderName('Action').slug, 'action');
  assert.strictEqual(genre.forFolderName('Fantasy').sprite, '100% 0%');
  assert.strictEqual(genre.forFolderName('Unsortiert'), null);

  publicBasePath = '';
  const rootActionArtwork = genre.create({name: 'Action'});
  const rootFantasyArtwork = genre.create({name: 'Fantasy'});
  assert.strictEqual(rootActionArtwork.dataset.genre, 'action');
  assert.strictEqual(
    rootActionArtwork.style.backgroundImage,
    'url("/channel-logos/vdr-suite-brand/recording-genre-action.svg")'
  );
  assert.strictEqual(
    rootFantasyArtwork.style.backgroundImage,
    'url("/channel-logos/vdr-suite-brand/recording-genre-sprite.svg")'
  );
  assert.strictEqual(rootFantasyArtwork.style.backgroundPosition, '100% 0%');

  publicBasePath = '/vdr-suite';
  const prefixedActionArtwork = genre.create({name: 'Action'});
  const prefixedFantasyArtwork = genre.create({name: 'Fantasy'});
  assert.strictEqual(
    prefixedActionArtwork.style.backgroundImage,
    'url("/vdr-suite/channel-logos/vdr-suite-brand/recording-genre-action.svg")'
  );
  assert.strictEqual(
    prefixedFantasyArtwork.style.backgroundImage,
    'url("/vdr-suite/channel-logos/vdr-suite-brand/recording-genre-sprite.svg")'
  );
  assert.ok(!prefixedActionArtwork.style.backgroundImage.includes('/vdr-suite/vdr-suite/'));
  assert.ok(!prefixedFantasyArtwork.style.backgroundImage.includes('/vdr-suite/vdr-suite/'));
  publicBasePath = '';

  const promotedRecording = {
    recordingId: 'default:1340',
    title: 'Tigeren_Club_(1340)',
    backendNativeId: '/srv/vdr/video/Tigeren_Club_(1340)/2026-07-19.05.55.1-0.rec'
  };
  const resolved = await genre.resolveLeaves({
    recordingFolder: true,
    folders: [
      {name: 'Action', path: 'Action', recordingCount: 48},
      {name: 'Tigeren_Club_(1340)', path: 'Tigeren_Club_(1340)', recordingCount: 1}
    ],
    recordings: []
  }, path => {
    assert.strictEqual(path, 'Tigeren_Club_(1340)');
    return Promise.resolve({
      recordingFolder: true,
      folders: [],
      recordings: [promotedRecording],
      recordingCount: 1
    });
  });
  assert.deepStrictEqual(Array.from(resolved.folders).map(folder => folder.name), ['Action']);
  assert.strictEqual(resolved.recordings.length, 1);
  assert.strictEqual(resolved.recordings[0].recordingId, 'default:1340');

  let embeddedLoaderCalls = 0;
  const embeddedRecording = {
    recordingId: 'default:4712',
    title: 'Inline_Leaf',
    backendNativeId: '/srv/vdr/video/Inline_Leaf/2026-07-20.20.15.1-0.rec'
  };
  const embeddedResolved = await genre.resolveLeaves({
    recordingFolder: true,
    folders: [
      {name: 'Action', path: 'Action', recordingCount: 48},
      {
        name: 'Inline_Leaf',
        path: 'Inline_Leaf',
        recordingCount: 1,
        singleRecordingLeaf: true,
        singleRecording: embeddedRecording
      }
    ],
    recordings: []
  }, () => {
    embeddedLoaderCalls += 1;
    return Promise.reject(new Error('inline leaf must not trigger a request'));
  });
  assert.strictEqual(embeddedLoaderCalls, 0);
  assert.deepStrictEqual(
    Array.from(embeddedResolved.folders).map(folder => folder.name),
    ['Action']
  );
  assert.strictEqual(embeddedResolved.recordings.length, 1);
  assert.strictEqual(embeddedResolved.recordings[0].recordingId, 'default:4712');
  assert.strictEqual(genre.embeddedLeafRecording({singleRecordingLeaf: false}), null);
  assert.strictEqual(
    genre.embeddedLeafRecording({
      singleRecordingLeaf: true,
      singleRecording: embeddedRecording
    }).recordingId,
    'default:4712'
  );

  assert(!actionsSource.includes("shared.createButton('Papierkorb prüfen'"));
  assert(!actionsSource.includes('deleteReadback(recording)'));
  assert(actionsSource.includes('const DELETE_QUEUE_BY_BACKEND = new Map();'));
  assert(actionsSource.includes('function waitForDeleteSettlement(recording, sourcePath)'));
  assert(actionsSource.includes('function enqueueDelete(recording, status, button, confirmDelete)'));
  assert(actionsSource.includes("validate(recording, 'DELETE', {}, status, button, isDryRunReady)"));
  assert(actionsSource.includes('return executeDelete(recording, status, button);'));
  assert(actionsSource.includes("Löschen vorgemerkt – wartet auf vorherige Papierkorb-Aktion"));
  assert(actionsSource.includes("typeof config.completeDelete === 'function'"));

  const deleteEditorStart = actionsSource.indexOf('function createDeleteEditor(recording)');
  const deleteEditorEnd = actionsSource.indexOf('function createPanel(recording)', deleteEditorStart);
  const deleteEditorSource = actionsSource.slice(deleteEditorStart, deleteEditorEnd);
  assert.strictEqual(
    (deleteEditorSource.match(/global\.confirm\(/g) || []).length,
    1,
    'delete workflow must ask for exactly one user confirmation'
  );

  const test = window.VdrSuiteRecordings2Actions.__test;
  assert.strictEqual(test.normalizeFolderPath(' Filme\\Archiv '), 'Filme/Archiv');
  assert.strictEqual(test.targetFolderPath('/'), '');
  assert.strictEqual(test.localTitle({title: 'Drama/Tatort'}), 'Tatort');

  const recording = {
    recordingId: 'default:4711',
    title: 'Drama/Tatort',
    path: '/srv/vdr/video/Drama/Tatort/2026-01-01.20.15.1-0.rec',
    backendNativeId: '/srv/vdr/video/Drama/Tatort/2026-01-01.20.15.1-0.rec',
    startTime: 1767294900,
    durationSeconds: 5400
  };
  const expected = test.identity(recording);
  assert.strictEqual(test.candidateMatches(Object.assign({}, recording), expected), true);
  assert.strictEqual(test.candidateMatches(Object.assign({}, recording, {
    backendNativeId: '/srv/vdr/video/Andere/2026-01-01.20.15.1-0.rec'
  }), expected), true);

  const payload = test.actionPayload(recording, 'default', 'RENAME', {
    dryRun: false,
    newName: 'Tatort neu'
  });
  assert.strictEqual(payload.action, 'RENAME');
  assert.strictEqual(payload.backendId, 'default');
  assert.strictEqual(payload.dryRun, false);
  assert.strictEqual(payload.newName, 'Tatort neu');
  assert.strictEqual(payload.backendNativeId, recording.backendNativeId);

  assert.strictEqual(test.isDryRunReady({
    success: false,
    message: 'dry-run backend execution skipped',
    warnings: ['dry-run only'],
    errors: []
  }), true);
  assert.strictEqual(test.isDryRunReady({success: true}), false);
  assert.strictEqual(
    test.actionError({
      errors: ['recording_cut_operation_pending_delete_blocked']
    }, 'fallback'),
    'Original kann während des laufenden Schnitts nicht gelöscht werden.'
  );
  assert.strictEqual(
    test.actionError({
      errors: ['recording_cut_journal_unavailable_delete_blocked']
    }, 'fallback'),
    'Löschen ist gesperrt, weil der Schnittstatus nicht sicher bestätigt werden kann.'
  );


  // Exercise the production panel controls, including delayed safety readback.
  const flush = async () => { for (let i = 0; i < 30; ++i) await Promise.resolve(); };
  const calls = [];
  let release, allow = false, confirmAnswer = true, completed = null;
  const current = {backendId: 'default', path: ''};
  shared.clientApi = () => ({
    fetchClientRecordingActionValidation({payload}) {
      calls.push({kind: 'validate', payload}); return Promise.resolve({valid: true});
    },
    fetchClientRecordingActionExecution({payload}) {
      calls.push({kind: payload.dryRun ? 'preview' : 'delete', payload});
      if (!payload.dryRun) return Promise.resolve({success: true});
      return new Promise(resolve => { release = () => resolve(allow ? {
        success: false, message: 'dry-run backend execution skipped', warnings: ['dry-run only'], errors: []
      } : {success: false, errors: ['recording_cut_active_delete_blocked']}); });
    },
    fetchClientRecordingFolder() { return Promise.resolve({recordings: []}); }
  });
  window.confirm = () => { calls.push({kind: 'confirm'}); return confirmAnswer; };
  const panel = window.VdrSuiteRecordings2Actions.create({
    getState: () => current, completeDelete: value => { completed = value; }
  }).createPanel(recording);
  const deletion = panel.children.at(-1).children.at(-1);
  const body = deletion.children[1], status = body.children[1];
  const [retry, apply] = body.children[2].children;
  assert(apply.disabled);
  deletion.open = true; deletion.handlers.toggle(); await flush();
  assert(apply.disabled); assert(!calls.some(c => c.kind === 'confirm'));
  release(); await flush();
  assert(apply.disabled); assert(status.textContent.includes('laufenden Schnitts'));
  apply.click(); await flush(); assert(!calls.some(c => c.kind === 'delete'));
  allow = true; retry.click(); await flush(); release(); await flush();
  assert(!apply.disabled);
  // A newly blocked cutter must prevent confirmation even after a previous approval.
  allow = false; apply.click(); apply.click(); await flush(); release(); await flush();
  assert(apply.disabled); assert(!calls.some(c => c.kind === 'confirm'));
  allow = true; retry.click(); await flush(); release(); await flush();
  confirmAnswer = false; apply.click(); await flush(); release(); await flush();
  assert.strictEqual(calls.filter(c => c.kind === 'confirm').length, 1);
  assert(!calls.some(c => c.kind === 'delete'));
  retry.click(); await flush(); release(); await flush();
  confirmAnswer = true; apply.click(); await flush();
  // Mutable list IDs and owner removal must not redirect or authorize a late result.
  const sourceNative = recording.backendNativeId;
  recording.backendNativeId = '/different.rec'; recording.recordingId = 'recycled';
  deletion.isConnected = false; release(); await flush();
  assert(!calls.some(c => c.kind === 'delete'));
  deletion.isConnected = true; retry.click(); await flush(); release(); await flush();
  apply.click(); await flush(); release(); await flush();
  assert.strictEqual(calls.filter(c => c.kind === 'delete').length, 1);
  assert.strictEqual(calls.at(-2).kind, 'confirm');
  assert.strictEqual(calls.at(-1).kind, 'delete');
  assert(calls.filter(c => c.payload).every(c => c.payload.backendNativeId === sourceNative && c.payload.recordingId === 'default:4711' && c.payload.backendId === 'default'));
  assert.strictEqual(completed.backendNativeId, sourceNative);
  current.backendId = 'other'; retry.click(); await flush();
  assert.strictEqual(calls.filter(c => c.kind === 'delete').length, 1);

  console.log('recordings2 actions, genre and leaf resolution runtime ok');
}

main().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
