'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.resolve(__dirname, '..', 'recordings2-folder-artwork.js'),
  'utf8'
);

function createElement(tagName) {
  const classes = new Set();
  return {
    tagName: String(tagName || '').toUpperCase(),
    className: '',
    dataset: {},
    style: {},
    attributes: {},
    classList: {
      add(name) { classes.add(name); },
      contains(name) { return classes.has(name); }
    },
    setAttribute(name, value) { this.attributes[name] = String(value); }
  };
}

function createFixture(basePath, includeResolver = true) {
  const appendedStyles = [];
  const resolvedPaths = [];
  const document = {
    getElementById() { return null; },
    createElement,
    head: {
      appendChild(element) { appendedStyles.push(element); }
    }
  };
  const window = {
    VdrSuiteRecordings2Shared: {
      first(value, names, fallback) {
        for (const name of names) {
          if (value && value[name] !== undefined && value[name] !== null) {
            return value[name];
          }
        }
        return fallback;
      },
      decodeDisplayText(value) { return String(value || ''); },
      recordingList(value) { return Array.isArray(value && value.recordings) ? value.recordings : []; },
      folderList(value) { return Array.isArray(value && value.folders) ? value.folders : []; },
      normalizePath(value) { return String(value || '').replace(/^\/+|\/+$/g, ''); },
      number(value, fallback) {
        const parsed = Number(value);
        return Number.isFinite(parsed) ? parsed : fallback;
      }
    }
  };

  if (includeResolver) {
    window.VdrSuitePublicUrl = {
      resolvePath(pathName) {
        const value = String(pathName || '');
        resolvedPaths.push(value);
        assert.ok(value.startsWith('/channel-logos/'));
        if (basePath) assert.ok(!value.startsWith(basePath + basePath + '/'));
        return basePath + value;
      }
    };
  }

  const context = vm.createContext({
    window,
    document,
    console,
    Array,
    Boolean,
    Math,
    Number,
    Object,
    Promise,
    Set,
    String
  });
  vm.runInContext(source, context, {filename: 'recordings2-folder-artwork.js'});

  return {
    api: window.VdrSuiteRecordings2FolderArtwork,
    appendedStyles,
    resolvedPaths,
    cssStyleDeclarationAvailable: typeof context.CSSStyleDeclaration === 'function'
  };
}

function assertArtwork(fixture, name, expectedUrl, expectedPosition) {
  const artwork = fixture.api.create({name});
  assert.ok(artwork, name + ' should use genre artwork');
  assert.strictEqual(artwork.style.backgroundImage, 'url("' + expectedUrl + '")');
  assert.strictEqual(artwork.dataset.genre.length > 0, true);
  if (expectedPosition) {
    assert.strictEqual(artwork.style.backgroundPosition, expectedPosition);
    assert.strictEqual(artwork.classList.contains('is-sprite'), true);
  } else {
    assert.strictEqual(artwork.style.backgroundPosition, undefined);
    assert.strictEqual(artwork.classList.contains('is-sprite'), false);
  }
  return artwork;
}

function assertGenreMapping(api) {
  const expected = [
    ['Horror', 'horror', '0% 0%'],
    ['Katastrophenfilm', 'katastrophenfilm', '50% 0%'],
    ['Fantasy', 'fantasy', '100% 0%'],
    ['Historienfilm', 'historienfilm', '0% 100%'],
    ['Komödie', 'komoedie', '50% 100%'],
    ['Kriegsfilm', 'krieg', '100% 100%'],
    ['Action', 'action', undefined],
    ['Doku', 'doku', undefined],
    ['Drama', 'drama', undefined]
  ];

  expected.forEach(([name, slug, sprite]) => {
    assert.deepStrictEqual(
      JSON.parse(JSON.stringify(api.forFolderName(name))),
      sprite ? {slug, sprite} : {slug}
    );
  });
  assert.strictEqual(api.forFolderName('Ghibli'), null);
  assert.strictEqual(api.create({name: 'Ghibli'}), null);
}

const direct = createFixture('');
assert.strictEqual(direct.cssStyleDeclarationAvailable, false);
assertGenreMapping(direct.api);
assertArtwork(
  direct,
  'Horror',
  '/channel-logos/vdr-suite-brand/recording-genre-sprite.svg',
  '0% 0%'
);
assertArtwork(
  direct,
  'Action',
  '/channel-logos/vdr-suite-brand/recording-genre-action.svg'
);
assert.deepStrictEqual(direct.resolvedPaths, [
  '/channel-logos/vdr-suite-brand/recording-genre-sprite.svg',
  '/channel-logos/vdr-suite-brand/recording-genre-action.svg'
]);

const prefixed = createFixture('/vdr-suite');
assertGenreMapping(prefixed.api);
assertArtwork(
  prefixed,
  'Horror',
  '/vdr-suite/channel-logos/vdr-suite-brand/recording-genre-sprite.svg',
  '0% 0%'
);
assertArtwork(
  prefixed,
  'Action',
  '/vdr-suite/channel-logos/vdr-suite-brand/recording-genre-action.svg'
);
prefixed.resolvedPaths.forEach(value => {
  assert.ok(!value.startsWith('/vdr-suite/'));
});
assert.ok(!prefixed.api.create({name: 'Drama'}).style.backgroundImage.includes('/vdr-suite/vdr-suite/'));

const fallback = createFixture('', false);
assertArtwork(
  fallback,
  'Horror',
  '/channel-logos/vdr-suite-brand/recording-genre-sprite.svg',
  '0% 0%'
);
assertArtwork(
  fallback,
  'Action',
  '/channel-logos/vdr-suite-brand/recording-genre-action.svg'
);

assert.strictEqual(typeof direct.api.normalizeName, 'function');
assert.strictEqual(typeof direct.api.forFolderName, 'function');
assert.strictEqual(typeof direct.api.installStyles, 'function');
assert.strictEqual(typeof direct.api.create, 'function');
assert.strictEqual(typeof direct.api.isSingleRecordingLeaf, 'function');
assert.strictEqual(typeof direct.api.embeddedLeafRecording, 'function');
assert.strictEqual(typeof direct.api.resolveLeaves, 'function');
assert.strictEqual(Object.isFrozen(direct.api), true);

console.log('recordings2 folder artwork public URL tests ok');
