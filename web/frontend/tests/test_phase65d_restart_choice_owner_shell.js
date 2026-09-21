'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'recording-playback-restart-choice.js'),
  'utf8'
);

function descendants(root) {
  const result = [];
  (function walk(node) {
    if (!node) return;
    result.push(node);
    (node.children || []).forEach(walk);
  }(root));
  return result;
}

function element(tagName) {
  const listeners = {};
  const value = {
    tagName: String(tagName || '').toUpperCase(),
    children: [],
    style: {},
    className: '',
    textContent: '',
    hidden: false,
    disabled: false,
    parentNode: null,
    nextSibling: null,
    appendChild(child) {
      if (child.parentNode && typeof child.parentNode.removeChild === 'function') {
        child.parentNode.removeChild(child);
      }
      child.parentNode = this;
      this.children.push(child);
      this.children.forEach((entry, index, entries) => {
        entry.nextSibling = entries[index + 1] || null;
      });
      return child;
    },
    insertBefore(child, reference) {
      if (reference && reference.parentNode !== this) {
        throw new Error('NotFoundError: reference node is not a child of this owner shell');
      }
      if (!reference) return this.appendChild(child);
      const index = this.children.indexOf(reference);
      child.parentNode = this;
      this.children.splice(index, 0, child);
      this.children.forEach((entry, childIndex, entries) => {
        entry.nextSibling = entries[childIndex + 1] || null;
      });
      return child;
    },
    removeChild(child) {
      const index = this.children.indexOf(child);
      if (index >= 0) this.children.splice(index, 1);
      child.parentNode = null;
      return child;
    },
    setAttribute(name, val) { this[name] = String(val); },
    addEventListener(name, callback, options) {
      if (!listeners[name]) listeners[name] = [];
      listeners[name].push({
        callback,
        capture: options === true || Boolean(options && options.capture === true)
      });
    },
    querySelector(selector) {
      const all = descendants(this);
      if (selector === 'button.recordings2-primary') {
        return all.find(node => node.tagName === 'BUTTON' && node.className === 'recordings2-primary') || null;
      }
      if (selector === 'button[aria-label="Wiedergabe stoppen"]') {
        return all.find(node => node.tagName === 'BUTTON' && node['aria-label'] === 'Wiedergabe stoppen') || null;
      }
      if (selector === 'input[aria-label="Wiedergabeposition"]') {
        return all.find(node => node.tagName === 'INPUT' && node['aria-label'] === 'Wiedergabeposition') || null;
      }
      if (selector === '.recordings2-playback-status') {
        return all.find(node => String(node.className || '').split(/\s+/).includes('recordings2-playback-status')) || null;
      }
      return null;
    },
    click() {
      const event = {
        defaultPrevented: false,
        immediateStopped: false,
        preventDefault() { this.defaultPrevented = true; },
        stopImmediatePropagation() { this.immediateStopped = true; }
      };
      const entries = listeners.click || [];
      for (const entry of entries.filter(value => value.capture)) {
        entry.callback(event);
        if (event.immediateStopped) return event;
      }
      for (const entry of entries.filter(value => !value.capture)) {
        entry.callback(event);
        if (event.immediateStopped) return event;
      }
      return event;
    }
  };
  return value;
}

function flush(count = 8) {
  let result = Promise.resolve();
  for (let index = 0; index < count; index += 1) result = result.then(() => Promise.resolve());
  return result;
}

(async function () {
  const outerOwner = element('div');
  outerOwner.className = 'recordings2-volume-owner-shell';
  const trackOwner = element('div');
  trackOwner.className = 'recordings2-track-owner-shell';
  const transport = element('section');
  transport.className = 'recordings2-recording-fast-playback';

  const startButton = element('button');
  startButton.className = 'recordings2-primary';
  const stopButton = element('button');
  stopButton.setAttribute('aria-label', 'Wiedergabe stoppen');
  const timeline = element('input');
  timeline.setAttribute('aria-label', 'Wiedergabeposition');
  timeline.disabled = false;
  const status = element('p');
  status.className = 'recordings2-playback-status';

  transport.appendChild(startButton);
  transport.appendChild(stopButton);
  transport.appendChild(timeline);
  transport.appendChild(status);
  trackOwner.appendChild(transport);
  outerOwner.appendChild(trackOwner);

  assert.strictEqual(startButton.nextSibling, stopButton);
  assert.notStrictEqual(stopButton.parentNode, outerOwner, 'reference node must be nested below stable owner');

  let state = 'playing';
  let stopCalls = 0;
  let startCalls = 0;
  const seekCalls = [];
  const playback = {
    element: outerOwner,
    state() { return state; },
    position() { return 137; },
    stop() {
      stopCalls += 1;
      state = 'stopped';
      return Promise.resolve(true);
    },
    start() {
      startCalls += 1;
      state = 'playing';
      return Promise.resolve('fresh-progressive-session');
    },
    seekAbsolute(position) {
      seekCalls.push(position);
      return Promise.resolve(true);
    }
  };

  const document = {createElement: element};
  const window = {document, console, Promise, Object, Number, String, Math};
  window.window = window;
  const context = vm.createContext({window, document, console, Promise, Object, Number, String, Math});
  vm.runInContext(source, context, {filename: 'recording-playback-restart-choice.js'});

  const installed = window.VdrSuiteRecordingPlaybackRestartChoice.install(playback);
  assert.ok(installed, 'restart choice must install on the stable outer owner shell');
  assert.strictEqual(
    installed.choices.parentNode,
    outerOwner,
    'restart choice must stay outside nested replaceable transport DOM'
  );

  stopButton.click();
  await flush();

  assert.strictEqual(stopCalls, 1, 'user Stop must invoke the playback owner once');
  assert.strictEqual(installed.choices.hidden, false, 'successful Stop must expose restart choices');
  assert.strictEqual(
    installed.resumeButton.hidden,
    false,
    'Progressive seek timeline must expose resume even without owner resume()/canResume() helpers'
  );
  assert.strictEqual(installed.resumeButton.textContent, '▶ Wiedergabe ab 00:02:17 fortsetzen');
  assert.strictEqual(status.textContent, 'Wiedergabe gestoppt · Wiedergabe ab 00:02:17 fortsetzen?');

  installed.resumeButton.click();
  await flush();

  assert.strictEqual(startCalls, 1, 'Progressive resume must create exactly one fresh MediaSession');
  assert.deepStrictEqual(
    seekCalls,
    [137],
    'Progressive resume must seek the fresh session to the captured stop position'
  );

  console.log('phase65d restart choice stable Progressive owner shell ok');
}()).catch(error => {
  console.error(error);
  process.exitCode = 1;
});
