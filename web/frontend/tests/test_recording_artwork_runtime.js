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

function makeElement(tagName) {
  const listeners = {};
  const element = {
    tagName: String(tagName || '').toUpperCase(),
    id: '',
    className: '',
    src: '',
    alt: '',
    loading: '',
    decoding: '',
    referrerPolicy: '',
    textContent: '',
    dataset: {},
    children: [],
    classList: makeClassList(),
    addEventListener(name, callback) {
      listeners[name] = callback;
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
        (child) => child !== this
      );
      this.parentNode = null;
    },
    dispatch(name) {
      if (listeners[name]) {
        listeners[name]();
      }
    }
  };
  return element;
}

const installedStyles = [];
let detailElement = null;

const document = {
  head: {
    appendChild(element) {
      installedStyles.push(element);
      return element;
    }
  },
  createElement: makeElement,
  getElementById(id) {
    return installedStyles.find((element) => element.id === id) || null;
  },
  querySelector(selector) {
    return selector === '.recording-detail' ? detailElement : null;
  }
};

const window = {
  createServerRecordingItem() {
    return makeElement('article');
  },
  renderServerRecordingDetail() {
    detailElement = makeElement('section');
  }
};

const sourcePath = path.resolve(
  __dirname,
  '..',
  'recording-artwork.js'
);
const source = fs.readFileSync(sourcePath, 'utf8');

vm.runInNewContext(source, {
  document,
  window,
  Object,
  String
}, {
  filename: sourcePath
});

const validUrl =
  '/recording-artwork/ferienhaus/' +
  '0123456789abcdef0123456789abcdef';
const validRecording = {
  title: 'Technischer Titel',
  metadata: {
    presentation: {
      title: 'Forrest Gump',
      posterUrl: validUrl
    },
    artwork: {
      preferredUrl: validUrl
    }
  }
};

assert.ok(window.VdrSuiteRecordingArtwork);
assert.strictEqual(
  window.VdrSuiteRecordingArtwork.urlForRecording(validRecording),
  validUrl
);
assert.strictEqual(
  window.VdrSuiteRecordingArtwork.urlForRecording({
    metadata: {presentation: {posterUrl: 'https://example.invalid/poster.jpg'}}
  }),
  ''
);
assert.strictEqual(
  window.VdrSuiteRecordingArtwork.urlForRecording({
    metadata: {presentation: {posterUrl: '/recording-artwork/default/../secret'}}
  }),
  ''
);
assert.strictEqual(installedStyles.length, 1);
assert.ok(
  installedStyles[0].textContent.includes('.recording-artwork-image')
);
assert.ok(
  installedStyles[0].textContent.includes('has-recording-artwork::before')
);

const listItem = window.createServerRecordingItem(validRecording, {});
assert.strictEqual(listItem.children.length, 1);
const listImage = listItem.children[0];
assert.strictEqual(listImage.src, validUrl);
assert.strictEqual(listImage.loading, 'lazy');
assert.strictEqual(listImage.referrerPolicy, 'same-origin');
assert.strictEqual(listImage.alt, 'Poster: Forrest Gump');
assert.ok(!listItem.classList.contains('has-recording-artwork'));
listImage.dispatch('load');
assert.ok(listItem.classList.contains('has-recording-artwork'));
listImage.dispatch('error');
assert.ok(!listItem.classList.contains('has-recording-artwork'));
assert.strictEqual(listItem.children.length, 0);

window.renderServerRecordingDetail(validRecording, {}, {});
assert.ok(detailElement);
assert.strictEqual(detailElement.children.length, 1);
assert.strictEqual(detailElement.children[0].loading, 'eager');
assert.strictEqual(detailElement.children[0].src, validUrl);

const rejectedItem = window.createServerRecordingItem({
  title: 'Rejected',
  metadata: {
    presentation: {
      posterUrl: 'movies/13/poster.jpg'
    }
  }
}, {});
assert.strictEqual(rejectedItem.children.length, 0);

assert.ok(!source.includes('http://'));
assert.ok(!source.includes('https://'));

console.log('test_recording_artwork_runtime passed');
