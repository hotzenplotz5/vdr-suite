'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const source = fs.readFileSync(
  path.join(__dirname, '..', 'api', 'recording-time-input-mask.js'),
  'utf8'
);

const listeners = {};
const document = {
  addEventListener(name, callback, capture) {
    if (!listeners[name]) listeners[name] = [];
    listeners[name].push({callback, capture: Boolean(capture)});
  }
};

function dispatch(name, target, extra) {
  const event = Object.assign({target}, extra || {});
  (listeners[name] || []).forEach(entry => entry.callback(event));
}

function input() {
  return {
    tagName: 'INPUT',
    value: '',
    dataset: {},
    parentNode: null,
    getAttribute(name) { return name === 'aria-label' ? 'Direkte Wiedergabezeit' : null; }
  };
}

function button(row) {
  return {
    tagName: 'BUTTON',
    parentNode: row,
    getAttribute(name) {
      return name === 'aria-label' ? 'Zur eingegebenen Wiedergabezeit springen' : null;
    }
  };
}

const window = {document, Object, String};
window.window = window;
const context = vm.createContext({window, document, Object, String});
vm.runInContext(source, context, {filename: 'recording-time-input-mask.js'});

assert.strictEqual(window.__vdrSuiteRecordingTimeInputMaskBound, true);
assert.strictEqual(window.VdrSuiteRecordingTimeInputMask.__test.formatDigits('0135'), '01:35');
assert.strictEqual(window.VdrSuiteRecordingTimeInputMask.__test.formatDigits('013543'), '01:35:43');

const direct = input();
const row = {
  querySelector(selector) {
    return selector === 'input[aria-label="Direkte Wiedergabezeit"]' ? direct : null;
  }
};
direct.parentNode = row;
const jump = button(row);

direct.value = '0';
dispatch('input', direct);
assert.strictEqual(direct.value, '0', 'one/two digit legacy seconds entry remains untouched');

direct.value = '013';
dispatch('input', direct);
assert.strictEqual(direct.value, '01:3');
assert.strictEqual(direct.dataset.vdrSuiteTimeMask, 'true');

direct.value = '01:35';
dispatch('input', direct);
assert.strictEqual(direct.value, '01:35');
dispatch('click', jump);
assert.strictEqual(direct.value, '01:35:00', 'HHMM defaults omitted seconds to 00 before seek');

direct.value = '013543';
direct.dataset = {};
dispatch('input', direct);
assert.strictEqual(direct.value, '01:35:43');
dispatch('keydown', direct, {key: 'Enter'});
assert.strictEqual(direct.value, '01:35:43');

const explicit = input();
explicit.value = '00:10:00';
dispatch('input', explicit);
assert.strictEqual(explicit.value, '00:10:00', 'explicit desktop HH:MM:SS stays unchanged');
assert.strictEqual(explicit.dataset.vdrSuiteTimeMask, undefined);

console.log('phase65d2 mobile Recording time input mask ok');