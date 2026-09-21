"use strict";

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const storage = new Map();
const elements = {
  text: {
    dataset: { i18n: 'shell.settings' },
    textContent: ''
  },
  aria: {
    dataset: { i18nAriaLabel: 'shell.openSettings' },
    attributes: {},
    setAttribute(name, value) {
      this.attributes[name] = value;
    }
  }
};

global.window = global;
global.localStorage = {
  getItem(key) {
    return storage.has(key) ? storage.get(key) : null;
  },
  setItem(key, value) {
    storage.set(key, String(value));
  }
};
global.document = {
  readyState: 'complete',
  documentElement: { lang: 'de' },
  querySelectorAll(selector) {
    if (selector === '[data-i18n]') {
      return [elements.text];
    }
    if (selector === '[data-i18n-aria-label]') {
      return [elements.aria];
    }
    return [];
  }
};
global.dispatchEvent = function() {};
global.CustomEvent = function(type, options) {
  this.type = type;
  this.detail = options.detail;
};

[
  'web/frontend/locales/de.js',
  'web/frontend/locales/en.js',
  'web/frontend/platform/i18n.js'
].forEach(fileName => {
  vm.runInThisContext(fs.readFileSync(fileName, 'utf8'), { filename: fileName });
});

assert.strictEqual(window.VdrSuiteI18n.getLocale(), 'de');
assert.strictEqual(window.VdrSuiteI18n.t('shell.settings'), 'Einstellungen');
assert.strictEqual(
  window.VdrSuiteI18n.t('recordings.move.targetReady', { target: 'Ghibli' }),
  'Ziel geprüft – bereit zum Verschieben nach „Ghibli“.'
);
assert.strictEqual(elements.text.textContent, 'Einstellungen');
assert.strictEqual(elements.aria.attributes['aria-label'], 'Einstellungen öffnen');
assert.strictEqual(window.VdrSuiteI18n.setLocale('en-US'), true);
assert.strictEqual(window.VdrSuiteI18n.getLocale(), 'en');
assert.strictEqual(window.VdrSuiteI18n.t('shell.settings'), 'Settings');
assert.strictEqual(
  window.VdrSuiteI18n.t('recordings.move.targetReady', { target: 'Ghibli' }),
  'Target validated — ready to move to “Ghibli”.'
);
assert.strictEqual(storage.get(window.VdrSuiteI18n.storageKey), 'en');
assert.deepStrictEqual(window.VdrSuiteI18n.availableLocales(), ['de', 'en']);
assert.strictEqual(
  window.VdrSuiteI18n.t('missing.key', null, 'Fallback text'),
  'Fallback text'
);

console.log('frontend i18n runtime ok');
