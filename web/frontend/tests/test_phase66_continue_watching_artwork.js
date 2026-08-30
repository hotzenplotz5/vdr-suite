'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const frontendRoot = path.join(__dirname, '..');
const repositoryRoot = path.join(frontendRoot, '..', '..');
const source = fs.readFileSync(path.join(frontendRoot, 'home-continue-watching.js'), 'utf8');
const apiSource = fs.readFileSync(path.join(repositoryRoot, 'api', 'rest', 'src', 'ContinueWatchingApiRuntime.cpp'), 'utf8');
const identitySource = fs.readFileSync(path.join(repositoryRoot, 'core', 'vdr', 'src', 'VdrRecordingArtworkIdentity.cpp'), 'utf8');

assert(apiSource.includes('#include "VdrRecordingArtworkIdentity.h"'));
assert(apiSource.includes('VdrRecordingArtworkIdentity::preferredArtwork(recording)'));
assert(apiSource.includes('VdrRecordingArtworkIdentity::publicUrl('));
assert(apiSource.includes('\\",\\\"posterUrl\\\":\\\"" << jsonEscape(item.recording.posterUrl)'));
assert(apiSource.includes('jsonEscape(item.recording.backendNativeId)'));
assert(apiSource.includes('truth.backendNativeId = recording.backendNativeId;'));
assert(identitySource.includes('"/recording-artwork/"'));

function element(tagName) {
  const listeners = {};
  return {
    tagName: String(tagName).toUpperCase(),
    className: '',
    textContent: '',
    children: [],
    dataset: {},
    appendChild(child) { this.children.push(child); child.parentNode = this; return child; },
    addEventListener(name, callback) { listeners[name] = callback; },
    remove() {
      if (!this.parentNode) return;
      const index = this.parentNode.children.indexOf(this);
      if (index >= 0) this.parentNode.children.splice(index, 1);
      this.parentNode = null;
    },
    _dispatch(name) { if (listeners[name]) listeners[name](); }
  };
}

const document = {
  readyState: 'loading',
  addEventListener() {},
  querySelector() { return null; },
  createElement: element
};
const context = {
  window: {
    document,
    VdrSuitePublicUrl: {
      resolvePath(value) { return '/vdr-suite' + value; }
    }
  },
  document,
  fetch: async () => ({ok: true, json: async () => ({items: []})}),
  console,
  setTimeout,
  clearTimeout
};
context.window.window = context.window;
context.window.fetch = context.fetch;
vm.createContext(context);
vm.runInContext(source, context);

const api = context.window.VdrSuiteHomeContinueWatching;
assert(api && api._test);

const posterUrl = '/recording-artwork/default/0123456789abcdef0123456789abcdef';
const backendNativeId = '/srv/vdr/video/Ein_unmoralisches_Angebot/2026-08-30.07.00.00-0.rec';
const item = api._test.normalizeItem({
  backendId: 'default',
  recordingId: 'recording-1',
  backendNativeId,
  title: 'Ein unmoralisches Angebot',
  posterUrl,
  resumePositionSeconds: 238,
  durationKnown: true,
  durationSeconds: 7000
});
assert(item);
assert.strictEqual(item.posterUrl, posterUrl);
assert.strictEqual(item.backendNativeId, backendNativeId);

const artwork = api._test.createArtwork(item);
assert.strictEqual(artwork.textContent, '');
assert.strictEqual(artwork.children.length, 1);
const image = artwork.children[0];
assert.strictEqual(image.tagName, 'IMG');
assert.strictEqual(image.src, '/vdr-suite' + posterUrl);
assert.strictEqual(image.alt, 'Poster zu Ein unmoralisches Angebot');
assert.strictEqual(image.loading, 'lazy');

image._dispatch('error');
assert.strictEqual(artwork.children.length, 0);
assert.strictEqual(artwork.textContent, 'E');

const fallback = api._test.createArtwork(api._test.normalizeItem({
  backendId: 'default',
  recordingId: 'recording-2',
  title: 'Fallback',
  resumePositionSeconds: 61,
  durationKnown: false,
  durationSeconds: 0
}));
assert.strictEqual(fallback.children.length, 0);
assert.strictEqual(fallback.textContent, 'F');

console.log('phase66 continue watching artwork contract ok');
