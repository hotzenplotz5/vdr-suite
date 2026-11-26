'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const appSource = fs.readFileSync(path.resolve(__dirname, '..', 'app.js'), 'utf8');
const start = appSource.indexOf('function appendSettingsBuildIdentityCard(');
const end = appSource.indexOf('function renderSettingsView(', start);
assert(start >= 0 && end > start, 'real Settings build identity renderer must exist');
const realRenderer = appSource.slice(start, end);

class Element {
  constructor(tag) {
    this.tagName = String(tag).toUpperCase();
    this.children = [];
    this.className = '';
    this.textContent = '';
    this.dataset = {};
    this.attributes = {};
    this.listeners = {};
    this.hidden = false;
  }
  replaceChildren(...items) { this.children = items; }
  setAttribute(key, value) { this.attributes[key] = value; }
  addEventListener(key, fn) { this.listeners[key] = fn; }
  appendChild(child) {
    this.children.push(child);
    return child;
  }
}

const document = {createElement(tag) { return new Element(tag); }};
function addText(element, value) {
  element.textContent = String(value);
  return element;
}
function appendSettingsLine(parent, label, value) {
  const row = document.createElement('div');
  row.appendChild(addText(document.createElement('span'), label));
  const displayed = addText(document.createElement('strong'), value);
  row.appendChild(displayed);
  parent.appendChild(row);
  return displayed;
}

const window = {};
const mount = new Element('section');
const render = new Function('window', 'document', 'addText', 'appendSettingsLine', 'Promise',
  realRenderer + '\nreturn appendSettingsBuildIdentityCard;')(
  window, document, addText, appendSettingsLine, Promise
);

function values() {
  const card = mount.children[mount.children.length - 1];
  assert(card && card.className.includes('settings-build-identity-card'));
  return card.children.filter(child => child.children.length === 2)
    .map(child => [child.children[0].textContent, child.children[1].textContent]);
}

(async function() {
  let requests = 0;
  window.VdrSuitePublicV1Client = {
    createClient(options) {
      assert.strictEqual(options.credentials, 'same-origin');
      return {getApiRoot(requestOptions) {
        requests += 1;
        assert.strictEqual(requestOptions.cache, 'no-store');
        assert.strictEqual(requestOptions.credentials, 'same-origin');
        return Promise.resolve({apiVersion:'v1',serverVersion:'git-ad2b20cdc873'});
      }};
    }
  };
  await render(mount);
  assert.deepStrictEqual(values(), [
    ['Server-Build','git-ad2b20cdc873'], ['Public API','v1']
  ]);
  assert.strictEqual(requests, 1, 'exactly one public root request');

  window.VdrSuitePublicV1Client = {
    createClient() {
      return {getApiRoot() {return Promise.resolve({
        apiVersion:'v1',serverVersion:'0.14.1-MLD'
      });}};
    }
  };
  await render(mount);
  assert.deepStrictEqual(values(), [
    ['Server-Build','0.14.1-MLD'], ['Public API','v1']
  ], 'package string must not be fabricated as a Git SHA');

  window.VdrSuitePublicV1Client = {
    createClient() {
      return {getApiRoot() {return Promise.reject(new Error('internal host detail'));}};
    }
  };
  await render(mount);
  assert.deepStrictEqual(values(), [
    ['Server-Build','Nicht verfügbar'], ['Public API','Nicht verfügbar']
  ], 'API failure should be local to the information card');
  assert(!JSON.stringify(values()).includes('internal host detail'));

  window.VdrSuitePublicV1Client = null;
  await render(mount);
  assert.deepStrictEqual(values(), [
    ['Server-Build','Nicht verfügbar'], ['Public API','Nicht verfügbar']
  ], 'missing reference client should not throw or disable Settings');

  window.VdrSuitePublicV1Client = {
    createClient() { throw new Error('client unavailable'); }
  };
  await render(mount);
  assert.strictEqual(values()[0][1], 'Nicht verfügbar');

  assert(appSource.includes('appendSettingsBuildIdentityCard(panel);'),
    'Settings must mount real build identity card');
  assert(!realRenderer.includes('git rev-parse'));
  assert(!realRenderer.includes('pluginVersion'));
  // Exercise the actual production Settings composition and category buttons.
  const viewStart = appSource.indexOf('let settingsCategory =');
  const viewEnd = appSource.indexOf('// A compact shortcut', viewStart);
  assert(viewStart >= 0 && viewEnd > viewStart);
  const detail = new Element('main');
  const mounted = [];
  for (const key of ['DeviceAdmin', 'AccountAdmin', 'Addon']) {
    window['VdrSuite' + key + 'Settings'] = {render(parent) { mounted.push([key, parent]); }};
  }
  const view = new Function('window', 'document', 'detailDataElement', 'addText',
    'frontendTranslate', 'appendSettingsLanguageCard', 'appendSettingsBuildIdentityCard',
    'appendSettingsLine', 'settingsBoolean', 'selectedBackend', 'selectedBackendId',
    'selectedEpgBackendId', 'moduleLabels', 'selectedModule', 'epgTimeAxisMode',
    'epgChannelOffset', 'epgTimeWindowPageOffset', 'firstValue',
    appSource.slice(viewStart, viewEnd) + '\nreturn renderSettingsView;')(
    window, document, detail, addText, (_, fallback) => fallback, () => {}, () => {},
    appendSettingsLine, String, {}, 'default', () => 'default', {}, 'settings',
    'vertical', 0, 0, () => '-');
  view(null);
  const [nav, content] = detail.children[0].children;
  const section = id => content.children.find(x => x.dataset.settingsCategory === id);
  assert(!section('general').hidden && section('devices').hidden);
  nav.children.find(x => x.textContent === 'Geräte').listeners.click();
  assert(section('general').hidden && !section('devices').hidden);
  assert.strictEqual(mounted.length, 3, 'navigation does not remount owners');
  assert.strictEqual(mounted.find(x => x[0] === 'DeviceAdmin')[1], section('devices'));
  assert.strictEqual(mounted.find(x => x[0] === 'AccountAdmin')[1], section('accounts'));
  assert.strictEqual(mounted.find(x => x[0] === 'Addon')[1], section('addons'));
  console.log('test_settings_build_identity passed');
})().catch(function(error) {
  console.error(error);
  process.exitCode = 1;
});
