'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');

const frontend = path.resolve(__dirname, '../web/frontend');
const bridgeSource = fs.readFileSync(path.join(frontend, 'browser-performance-bridge.js'), 'utf8');
const controllerSource = fs.readFileSync(path.join(frontend, 'browser-performance-diagnostics.js'), 'utf8');
const htmlSource = fs.readFileSync(path.join(frontend, 'browser-performance-diagnostics.html'), 'utf8');

async function main() {
  const listeners = {};
  const sent = [];
  const frame = {
    attributes: { 'data-vdr-suite-diagnostics': 'token' },
    hasAttribute(name) { return name in this.attributes; },
    getAttribute(name) { return this.attributes[name]; }
  };
  const parent = { postMessage(message, origin) { sent.push({ message, origin }); } };
  const probe = { start() {}, stop() { return { metrics: {} }; }, inspectHeaders() { return Promise.resolve({ status: 200, headers: {} }); } };
  let now = 100;
  let loaded = false;
  const resources = [];
  const timers = [];
  class MutationObserverMock {
    constructor(callback) { this.callback = callback; }
    observe() { this.observing = true; }
    disconnect() { this.disconnected = true; }
  }
  const owner = {
    snapshot() { return { loadingChannels: !loaded, loadingPrograms: !loaded, programmeLoadedChannelCount: loaded ? 24 : 0, programmeHasMore: true }; },
    refresh() {
      now = 130;
      resources.push(
        { name: 'https://example.test/vdr-suite/api/vdr/channels?secret=1', initiatorType: 'fetch', startTime: 105, responseEnd: 120, duration: 15, transferSize: 95000, encodedBodySize: 94000 },
        { name: 'https://example.test/vdr-suite/api/epg/cache/window?secret=2', initiatorType: 'fetch', startTime: 121, responseEnd: 129, duration: 8, transferSize: 315000, encodedBodySize: 314000 },
        { name: 'https://example.test/vdr-suite/api/epg/cache/metadata/image?secret=3', initiatorType: 'img', startTime: 122, responseEnd: 128, duration: 6, transferSize: 999999, encodedBodySize: 999000 }
      );
      loaded = true;
      return Promise.resolve();
    }
  };
  const document = {
    documentElement: {},
    querySelectorAll(selector) {
      if (!loaded) return [];
      if (selector.includes('data-home-live-guide="now"')) return new Array(10).fill({});
      if (selector.includes('data-home-live-guide="next"')) return new Array(9).fill({});
      return [];
    }
  };
  const child = {
    frameElement: frame,
    parent,
    VdrSuiteArtworkProbe: probe,
    VdrSuiteHomeLiveHero: owner,
    document,
    performance: { now() { return now; }, getEntriesByType(type) { return type === 'resource' ? resources.slice() : []; } },
    location: { origin: 'https://example.test', href: 'https://example.test/vdr-suite/frontend/browser-performance-home.html' },
    URL,
    MutationObserver: MutationObserverMock,
    requestAnimationFrame(fn) { now += 5; fn(now); },
    setTimeout(fn, ms) { timers.push({ fn, ms }); return timers.length; },
    clearTimeout() {},
    addEventListener(type, fn) { listeners[type] = fn; }
  };
  vm.runInNewContext(bridgeSource, { window: child, Promise, URL, Number, Math, String, Boolean, Array, Object, RegExp, Error });
  assert.equal(sent[0].message.type, 'started');
  listeners.message({ source: parent, origin: child.location.origin, data: { channel: 'vdr-suite-browser-diagnostics', token: 'token', type: 'stop' } });
  listeners.message({ source: parent, origin: child.location.origin, data: { channel: 'vdr-suite-browser-diagnostics', token: 'token', type: 'epg' } });
  await new Promise(resolve => setImmediate(resolve));
  const epgMessage = sent.find(item => item.message.type === 'epg-result');
  assert.ok(epgMessage);
  const report = epgMessage.message.value;
  assert.equal(report.schema, 2);
  assert.equal(report.label, 'home-epg-warm-refresh');
  assert.equal(report.mode, 'warm-refresh');
  assert.equal(report.timedOut, false);
  assert.equal(report.requests.channels.count, 1);
  assert.equal(report.requests.epg.count, 1);
  assert.equal(report.requests.channels.lastResponseEndMs, 20);
  assert.equal(report.requests.epg.lastResponseEndMs, 29);
  assert.equal(report.rails.nowCards, 10);
  assert.equal(report.rails.nextCards, 9);
  assert.ok(report.milestones.railsReadyMs >= 30);
  assert.ok(report.milestones.paintReadyMs >= report.milestones.railsReadyMs);
  assert.ok(!JSON.stringify(report).includes('secret'));

  const coldListeners = {};
  const coldSent = [];
  const coldFrame = {
    attributes: { 'data-vdr-suite-diagnostics': 'cold-token' },
    hasAttribute(name) { return name in this.attributes; },
    getAttribute(name) { return this.attributes[name]; }
  };
  const coldParent = { postMessage(message, origin) { coldSent.push({ message, origin }); } };
  const coldResources = [];
  const coldTimers = [];
  let coldNow = 200;
  let coldLoaded = false;
  let coldObserver = null;
  class ColdMutationObserverMock {
    constructor(callback) { this.callback = callback; coldObserver = this; }
    observe() { this.observing = true; }
    disconnect() { this.disconnected = true; }
  }
  const coldOwner = {
    snapshot() { return { loadingChannels: !coldLoaded, loadingPrograms: !coldLoaded, programmeLoadedChannelCount: coldLoaded ? 24 : 0, programmeHasMore: true }; }
  };
  const coldDocument = {
    documentElement: {},
    querySelectorAll(selector) {
      if (!coldLoaded) return [];
      if (selector.includes('data-home-live-guide="now"')) return new Array(23).fill({});
      if (selector.includes('data-home-live-guide="next"')) return new Array(23).fill({});
      return [];
    }
  };
  const coldChild = {
    frameElement: coldFrame,
    parent: coldParent,
    VdrSuiteArtworkProbe: probe,
    VdrSuiteHomeLiveHero: coldOwner,
    document: coldDocument,
    performance: { now() { return coldNow; }, getEntriesByType(type) { return type === 'resource' ? coldResources.slice() : []; } },
    location: { origin: 'https://example.test', href: 'https://example.test/vdr-suite/frontend/browser-performance-home.html#vdrSuiteEpgCold=1' },
    URL,
    MutationObserver: ColdMutationObserverMock,
    requestAnimationFrame(fn) { coldNow += 5; fn(coldNow); },
    setTimeout(fn, ms) { coldTimers.push({ fn, ms }); return coldTimers.length; },
    clearTimeout() {},
    addEventListener(type, fn) { coldListeners[type] = fn; }
  };
  vm.runInNewContext(bridgeSource, { window: coldChild, Promise, URL, Number, Math, String, Boolean, Array, Object, RegExp, Error });
  assert.equal(coldSent.some(item => item.message.type === 'started'), false);
  assert.ok(coldObserver && coldObserver.observing);
  coldResources.push(
    { name: 'https://example.test/vdr-suite/api/vdr/channels?private=1', initiatorType: 'fetch', startTime: 205, responseEnd: 235, duration: 30, transferSize: 95000, encodedBodySize: 94000 },
    { name: 'https://example.test/vdr-suite/api/epg/cache/window?private=2', initiatorType: 'fetch', startTime: 236, responseEnd: 260, duration: 24, transferSize: 315000, encodedBodySize: 314000 }
  );
  coldNow = 270;
  coldLoaded = true;
  coldObserver.callback([{}, {}]);
  await new Promise(resolve => setImmediate(resolve));
  const coldEpgMessage = coldSent.find(item => item.message.type === 'epg-result');
  assert.ok(coldEpgMessage);
  const coldReport = coldEpgMessage.message.value;
  assert.equal(coldReport.label, 'home-epg-cold-startup');
  assert.equal(coldReport.mode, 'cold-startup');
  assert.equal(coldReport.requests.channels.count, 1);
  assert.equal(coldReport.requests.epg.count, 1);
  assert.equal(coldReport.requests.epg.lastResponseEndMs, 60);
  assert.equal(coldReport.rails.nowCards, 23);
  assert.equal(coldReport.rails.nextCards, 23);
  assert.ok(coldReport.milestones.paintReadyMs >= coldReport.milestones.railsReadyMs);
  assert.ok(!JSON.stringify(coldReport).includes('private'));

  const handlers = {};
  const elements = {};
  function element(id) {
    return elements[id] = { id, disabled: false, textContent: '', attributes: {}, children: [], addEventListener(type, fn) { handlers[id + ':' + type] = fn; }, setAttribute(name, value) { this.attributes[name] = value; }, replaceChildren() { this.children = []; }, append(...nodes) { this.children.push(...nodes); }, click() { this.clicked = true; } };
  }
  for (const id of ['home', 'cold', 'start', 'stop', 'epg-cold', 'epg', 'headers', 'export', 'status', 'metrics', 'history', 'headers-result', 'epg-result']) element(id);
  const outgoing = [];
  const childWindow = { postMessage(data, origin) { outgoing.push({ data, origin }); }, location: { pathname: '/vdr-suite/frontend/browser-performance-home.html' } };
  elements.home.contentWindow = childWindow;
  const doc = { getElementById(id) { return elements[id]; }, createElement() { return { textContent: '', children: [], append(...nodes) { this.children.push(...nodes); }, click() { this.clicked = true; } }; } };
  const root = { document: doc, location: { origin: 'https://example.test' }, addEventListener(type, fn) { handlers['root:' + type] = fn; }, URL: { createObjectURL() { return 'blob:test'; }, revokeObjectURL() {} }, Blob, setTimeout(fn) { fn(); }, Math, Date };
  vm.runInNewContext(controllerSource, { window: root, Blob, Math, Date, JSON, Object, String });
  const token = elements.home.attributes['data-vdr-suite-diagnostics'];
  handlers['root:message']({ source: childWindow, origin: root.location.origin, data: { channel: 'vdr-suite-browser-diagnostics', token, type: 'started', value: 'initial-home' } });
  handlers['stop:click']();
  handlers['root:message']({ source: childWindow, origin: root.location.origin, data: { channel: 'vdr-suite-browser-diagnostics', token, type: 'result', value: { metrics: {} } } });
  handlers['epg:click']();
  assert.equal(outgoing.at(-1).data.type, 'epg');
  assert.equal(elements.epg.disabled, true);
  handlers['root:message']({ source: childWindow, origin: root.location.origin, data: { channel: 'vdr-suite-browser-diagnostics', token, type: 'epg-result', value: report } });
  assert.ok(elements['epg-result'].textContent.includes('home-epg-warm-refresh'));
  assert.equal(elements.epg.disabled, false);

  handlers['epg-cold:click']();
  const coldToken = elements.home.attributes['data-vdr-suite-diagnostics'];
  assert.notEqual(coldToken, token);
  assert.ok(String(elements.home.src).includes('#vdrSuiteEpgCold=1'));
  assert.equal(elements['epg-cold'].disabled, true);
  handlers['root:message']({ source: childWindow, origin: root.location.origin, data: { channel: 'vdr-suite-browser-diagnostics', token: coldToken, type: 'epg-result', value: coldReport } });
  assert.ok(elements['epg-result'].textContent.includes('home-epg-cold-startup'));
  assert.equal(elements['epg-cold'].disabled, false);
  assert.equal(elements.export.disabled, false);

  assert.ok(htmlSource.includes('id="epg-cold"'));
  assert.ok(htmlSource.includes('EPG kalt messen'));
  assert.ok(htmlSource.includes('EPG warm messen'));

  console.log('browser Home EPG cold/warm timing, privacy and controller wiring ok');
}

main().catch(error => { console.error(error); process.exitCode = 1; });
