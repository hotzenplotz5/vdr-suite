'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

// Exercise the actual shipped Home/nav code, not a duplicate implementation.
// No network, installed service or VDR files are accessed.
const source = fs.readFileSync(path.join(__dirname, '../app.js'), 'utf8');
const first = source.indexOf("let selectedModule = 'overview';");
const last = source.indexOf("let homeResumeBackendId = '';", first);
assert(first >= 0 && last > first);
const gate = source.slice(first, last);

function el(tag) {
  return {
    tag, children: [], attrs: {}, handlers: {}, hidden: false,
    disabled: false, title: '', textContent: '',
    appendChild(child) { this.children.push(child); child.parentNode = this; return child; },
    remove() {
      if (this.parentNode)
        this.parentNode.children = this.parentNode.children.filter(x => x !== this);
    },
    setAttribute(name, value) { this.attrs[name] = value; },
    addEventListener(name, handler) { this.handlers[name] = handler; },
    querySelector(selector) {
      return selector === '[data-home-media-tools]'
        ? this.children.find(x => 'data-home-media-tools' in x.attrs) || null
        : null;
    }
  };
}
const host = el('div');
const tab = el('button');
const recordingsTab = el('button');
const nav = el('nav');
nav.children = [recordingsTab, tab];
recordingsTab.parentNode = nav;
tab.parentNode = nav;
recordingsTab.nextElementSibling = tab;
nav.querySelector = () => recordingsTab;
nav.insertBefore = () => {};
let capability = false;
let adminStatus = {installationState: 'present', desiredEnabled: false};
const moduleApi = {
  checkCapability() { return Promise.resolve(capability); },
  deactivate() {}, clearSession() {}
};
const document = {
  getElementById() { return nav; },
  createElement: el,
  querySelector(selector) {
    if (selector === '[data-home-zone="additional-sections"]') return host;
    if (selector.includes('module-tab[data-module="media-import"]')) return tab;
    return null;
  }
};
const ctx = {
  document, window: {
    VdrSuiteMediaImport: moduleApi,
    VdrSuiteAddonSettings: {
      readActivationState() { return Promise.resolve(adminStatus); }
    }
  },
  Promise, console, frontendPlatformModule() { return moduleApi; }
};
vm.createContext(ctx);
vm.runInContext([
  "let selectedBackendId = 'default';",
  'let browserSessionFrontendActive = true;',
  "let clicked = '';",
  'function selectModule(name) { selectedModule = name; clicked = name; }',
  gate,
  "function goHome() { selectedModule = 'overview'; renderMediaToolsHomeShortcut(); }",
  'function readClicked() { return clicked; }',
  'function logout() { browserSessionFrontendActive = false; resetMediaImportVisibility(true); }'
].join('\n'), ctx);

async function test() {
  // Configured and installed != enabled; navigation remains usable, while
  // no Home shortcut advertises an inactive add-on.
  assert.equal(await ctx.refreshMediaImportVisibility('default'), true);
  assert.equal(tab.hidden, false);
  assert.equal(host.children.length, 0);
  // An administrator's enabled intent remains visible even while Agent
  // capability is temporarily absent.
  adminStatus = {installationState: 'present', desiredEnabled: true};
  assert.equal(await ctx.refreshMediaImportVisibility('default'), true);
  assert.equal(host.children.length, 1);
  capability = true;
  assert.equal(await ctx.refreshMediaImportVisibility('default'), true);
  assert.equal(host.children.length, 1);
  const open = host.children[0].children[0];
  assert.equal(open.textContent, 'Media Tools');
  assert.equal(await ctx.refreshMediaImportVisibility('default'), true);
  assert.equal(host.children.length, 1, 'refresh must not duplicate Home tile');
  open.handlers.click();
  assert.equal(ctx.readClicked(), 'media-import',
    'Home tile must open existing module, not a second importer');
  ctx.goHome();
  assert.equal(host.children.length, 1);
  capability = false;
  assert.equal(await ctx.refreshMediaImportVisibility('default'), true);
  assert.equal(host.children.length, 1,
    'temporary Agent capability loss must not erase existing Home shortcut');
  adminStatus = {installationState: 'absent', desiredEnabled: false};
  assert.equal(await ctx.refreshMediaImportVisibility('default'), false);
  assert.equal(host.children.length, 0);
  assert.equal(tab.hidden, true, 'confirmed missing files hide navigation');
  adminStatus = {installationState: 'present', desiredEnabled: true};
  assert.equal(await ctx.refreshMediaImportVisibility('default'), true);
  assert.equal(host.children.length, 1, 'installed and active is restored');
  ctx.logout();
  assert.equal(host.children.length, 0);
  assert.equal(await ctx.refreshMediaImportVisibility('default'), false);
  console.log('MEDIA_TOOLS_HOME_SHORTCUT=PASS');
}
test().catch(e => { console.error(e); process.exitCode = 1; });
