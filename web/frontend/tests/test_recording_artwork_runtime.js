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
  const listeners = {};
  const element = {
    nodeType: 1,
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
    attributes: {},
    children: [],
    parentNode: null,
    isConnected: true,
    classList: makeClassList(),
    setAttribute(name, value) {
      this.attributes[name] = String(value);
    },
    addEventListener(name, callback) {
      if (!listeners[name]) {
        listeners[name] = [];
      }
      listeners[name].push(callback);
    },
    appendChild(child) {
      this.children.push(child);
      child.parentNode = this;
      child.isConnected = true;
      return child;
    },
    replaceChildren(...children) {
      this.children.forEach(child => {
        child.parentNode = null;
        child.isConnected = false;
      });
      this.children = [];
      children.forEach(child => this.appendChild(child));
    },
    remove() {
      if (!this.parentNode) {
        this.isConnected = false;
        return;
      }
      this.parentNode.children = this.parentNode.children.filter(
        child => child !== this
      );
      this.parentNode = null;
      this.isConnected = false;
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
    dispatch(name) {
      (listeners[name] || []).forEach(callback => callback());
    }
  };
  return element;
}

const installedStyles = [];
const body = makeElement('body');
let detailElement = null;
let configuredFolderLoader = null;

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
  querySelector(selector) {
    if (selector === '.recording-detail') {
      return detailElement;
    }
    return body.querySelector(selector);
  },
  querySelectorAll(selector) {
    return body.querySelectorAll(selector);
  }
};

class MutationObserver {
  constructor(callback) {
    this.callback = callback;
  }

  observe() {}
}

const window = {
  configureRecordingBrowserFolderLoader(loader) {
    configuredFolderLoader = loader;
  },
  createServerRecordingItem() {
    return makeElement('article');
  },
  renderServerRecordingDetail() {
    detailElement = makeElement('section');
    detailElement.className = 'recording-detail';
  },
  renderServerRecordingFolder(data) {
    const list = makeElement('section');
    list.className = 'recording-folder-list';

    (data.folders || []).forEach(folder => {
      const item = makeElement('article');
      item.className = 'recording-folder-item';

      const title = makeElement('div');
      title.className = 'list-title';
      title.textContent = folder.name;
      item.appendChild(title);

      const meta = makeElement('div');
      meta.className = 'list-meta';
      meta.textContent = String(folder.recordingCount || 0);
      item.appendChild(meta);

      list.appendChild(item);
    });

    body.replaceChildren(list);
  }
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
  Promise,
  String,
  window
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

async function run() {
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
  const styles = installedStyles[0].textContent;
  assert.ok(styles.includes('.recording-artwork-image'));
  assert.ok(styles.includes('has-recording-artwork::before'));
  assert.ok(styles.includes('@media (hover: hover) and (pointer: fine)'));
  assert.ok(styles.includes('@media (hover: none), (pointer: coarse)'));
  assert.ok(styles.includes('.recording-action-panel > p {'));
  assert.ok(styles.includes('padding: 10.35rem 0.85rem 0.9rem !important;'));
  assert.ok(styles.includes('.recording-folder-item .recording-artwork-image,'));

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

  const folderLoads = [];
  window.configureRecordingBrowserFolderLoader((folderPath, offset) => {
    folderLoads.push([folderPath, offset]);
    return Promise.resolve({
      path: folderPath,
      recordingCount: 1,
      recordings: [validRecording]
    });
  });
  assert.strictEqual(typeof configuredFolderLoader, 'function');

  window.renderServerRecordingFolder({
    folders: [
      {
        name: 'Mission: Impossible',
        path: 'Action/Mission#3A_Impossible',
        recordingCount: 1
      },
      {
        name: 'Action',
        path: 'Action',
        recordingCount: 61
      }
    ]
  });

  await Promise.resolve();
  await Promise.resolve();

  const folderItems = document.querySelectorAll('.recording-folder-item');
  assert.strictEqual(folderItems.length, 2);
  assert.deepStrictEqual(folderLoads, [
    ['Action/Mission#3A_Impossible', 0]
  ]);

  const missionPoster = folderItems[0].querySelector(
    '.recording-artwork-image'
  );
  assert.ok(missionPoster);
  assert.strictEqual(missionPoster.src, validUrl);
  missionPoster.dispatch('load');
  assert.ok(folderItems[0].classList.contains('has-recording-artwork'));

  assert.ok(!source.includes('http://'));
  assert.ok(!source.includes('https://'));

  console.log('test_recording_artwork_runtime passed');
}

run().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
