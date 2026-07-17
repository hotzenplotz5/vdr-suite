'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

function makeClassList() {
  const values = new Set();
  return {
    add(value) {
      values.add(value);
    },
    remove(value) {
      values.delete(value);
    },
    contains(value) {
      return values.has(value);
    }
  };
}

function elementHasClass(element, className) {
  return element.classList.contains(className) ||
    String(element.className || '').split(/\s+/).includes(className);
}

function makeElement(tagName) {
  const element = {
    nodeType: 1,
    tagName: String(tagName || '').toUpperCase(),
    id: '',
    className: '',
    textContent: '',
    dataset: {},
    attributes: {},
    children: [],
    parentNode: null,
    classList: makeClassList(),
    setAttribute(name, value) {
      this.attributes[name] = String(value);
    },
    appendChild(child) {
      this.children.push(child);
      child.parentNode = this;
      return child;
    },
    remove() {
      if (!this.parentNode) {
        return;
      }
      this.parentNode.children = this.parentNode.children.filter(
        child => child !== this
      );
      this.parentNode = null;
    },
    matches(selector) {
      return selector.startsWith('.') &&
        elementHasClass(this, selector.slice(1));
    },
    closest(selector) {
      let cursor = this;
      while (cursor) {
        if (cursor.matches && cursor.matches(selector)) {
          return cursor;
        }
        cursor = cursor.parentNode;
      }
      return null;
    },
    querySelector(selector) {
      return this.querySelectorAll(selector)[0] || null;
    },
    querySelectorAll(selector) {
      const result = [];
      const visit = node => {
        node.children.forEach(child => {
          if (child.matches && child.matches(selector)) {
            result.push(child);
          }
          visit(child);
        });
      };
      visit(this);
      return result;
    },
    addEventListener() {}
  };
  return element;
}

const installedStyles = [];
const body = makeElement('body');
let observedTarget = null;
let observedOptions = null;

class MutationObserver {
  constructor(callback) {
    this.callback = callback;
  }

  observe(target, options) {
    observedTarget = target;
    observedOptions = options;
  }
}

const document = {
  body,
  head: {
    appendChild(element) {
      installedStyles.push(element);
      return element;
    }
  },
  createElement: makeElement,
  getElementById(id) {
    return installedStyles.find(element => element.id === id) || null;
  },
  querySelector() {
    return null;
  },
  querySelectorAll(selector) {
    return body.querySelectorAll(selector);
  }
};

const window = {
  createServerRecordingItem() {
    return makeElement('article');
  },
  renderServerRecordingDetail() {}
};

const sourcePath = path.resolve(
  __dirname,
  '..',
  'recording-artwork.js'
);
const source = fs.readFileSync(sourcePath, 'utf8');

vm.runInNewContext(source, {
  Array,
  document,
  MutationObserver,
  Object,
  String,
  window
}, {
  filename: sourcePath
});

assert.ok(window.VdrSuiteRecordingGenreArtwork);
assert.strictEqual(
  window.VdrSuiteRecordingGenreArtwork.normalizeName('Komödie'),
  'komodie'
);
assert.strictEqual(
  window.VdrSuiteRecordingGenreArtwork.forFolderName('Horror').slug,
  'horror'
);
assert.strictEqual(
  window.VdrSuiteRecordingGenreArtwork.forFolderName('Katastrophe').slug,
  'katastrophenfilm'
);
assert.strictEqual(
  window.VdrSuiteRecordingGenreArtwork.forFolderName('Historienfilm').slug,
  'historienfilm'
);
assert.strictEqual(
  window.VdrSuiteRecordingGenreArtwork.forFolderName('Mission: Impossible'),
  null
);

const genreFolder = makeElement('article');
genreFolder.classList.add('recording-folder-item');
genreFolder.classList.add('has-recording-artwork');
genreFolder.dataset.recordingArtworkAttached = 'true';

const title = makeElement('div');
title.className = 'list-title';
title.textContent = 'Komödie';
genreFolder.appendChild(title);

const competingPoster = makeElement('img');
competingPoster.className = 'recording-artwork-image';
genreFolder.appendChild(competingPoster);

assert.strictEqual(
  window.VdrSuiteRecordingGenreArtwork.attach(genreFolder),
  true
);
assert.ok(!genreFolder.classList.contains('has-recording-artwork'));
assert.ok(genreFolder.classList.contains('has-recording-genre-artwork'));
assert.strictEqual(genreFolder.dataset.recordingArtworkAttached, 'false');
assert.strictEqual(
  genreFolder.querySelectorAll('.recording-artwork-image').length,
  0
);

const genreArtwork = genreFolder.querySelector(
  '.recording-genre-artwork-image'
);
assert.ok(genreArtwork);
assert.ok(
  String(genreArtwork.className).includes(
    'recording-genre-artwork-komoedie'
  )
);
assert.strictEqual(genreArtwork.attributes['aria-hidden'], 'true');

const normalFolder = makeElement('article');
normalFolder.classList.add('recording-folder-item');
const normalTitle = makeElement('div');
normalTitle.className = 'list-title';
normalTitle.textContent = 'Mission: Impossible';
normalFolder.appendChild(normalTitle);
assert.strictEqual(
  window.VdrSuiteRecordingGenreArtwork.attach(normalFolder),
  false
);
assert.strictEqual(normalFolder.children.length, 1);

assert.strictEqual(installedStyles.length, 1);
const styles = installedStyles[0].textContent;
assert.ok(styles.includes('recording-genre-sprite.svg'));
assert.ok(styles.includes('background-size: 300% 200%;'));
assert.ok(styles.includes('@media (min-width: 72rem)'));
assert.ok(styles.includes('width: 12rem;'));
assert.ok(styles.includes('height: 18rem;'));
assert.ok(styles.includes('@media (max-width: 760px)'));
assert.ok(styles.includes('width: 5.8rem;'));
assert.ok(styles.includes('height: 8.35rem;'));

assert.strictEqual(observedTarget, body);
assert.strictEqual(observedOptions.childList, true);
assert.strictEqual(observedOptions.subtree, true);
assert.ok(!source.includes('http://'));
assert.ok(!source.includes('https://'));

console.log('test_recording_genre_artwork_runtime passed');
