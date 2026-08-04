'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

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
    addEventListener() {}
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
  node() { return element(); },
  createButton() { return element(); }
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

  console.log('recordings2 actions, genre and leaf resolution runtime ok');
}

main().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
