'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');

const frontend = path.resolve(__dirname, '../web/frontend');
const bridgeSource = fs.readFileSync(path.join(frontend, 'browser-performance-bridge.js'), 'utf8');
const controllerSource = fs.readFileSync(path.join(frontend, 'browser-performance-diagnostics.js'), 'utf8');
const htmlSource = fs.readFileSync(path.join(frontend, 'browser-performance-diagnostics.html'), 'utf8');
const heroSource = fs.readFileSync(path.join(frontend, 'home-live-hero.js'), 'utf8');
const countKeys = ['eventsBeforeNow', 'eventsCurrent', 'eventsFuture', 'channelsWithCurrent', 'channelsWithAnyFuture', 'channelsWithFrontendNext'];
const expectedCounts = { eventsBeforeNow: 2, eventsCurrent: 2, eventsFuture: 3, channelsWithCurrent: 1, channelsWithAnyFuture: 2, channelsWithFrontendNext: 1 };

function loadedHero() {
  const now = 1800000000;
  const root = {};
  let clockReads = 0;
  class Clock extends Date {
    static now() { clockReads += 1; return now * 1000; }
  }
  vm.runInNewContext(heroSource, { window: root, Date: Clock });
  const hero = root.VdrSuiteHomeLiveHero;
  assert.deepEqual(JSON.parse(JSON.stringify(hero.epgDiagnostics())), Object.fromEntries(countKeys.map(key => [key, 0])));
  const channels = [{ id: 'private-channel-a', number: 1 }, { id: 'private-channel-b', number: 2 }, { id: 'private-channel-b', number: 2 }];
  const events = [
    { channelId: channels[0].id, startTime: now - 200, endTime: now - 100 },
    { channelId: channels[0].id, startTime: now - 100, endTime: now },
    { channelId: channels[0].id, startTime: now, durationSeconds: 100 },
    { channel_id: channels[0].id, beginTime: (now - 20) * 1000, stopTime: (now + 80) * 1000 },
    // Future event overlaps the current one: Home must still select no Next.
    { channel: channels[0].id, start: new Date((now + 50) * 1000).toISOString(), duration: 20 },
    { channelId: channels[1].id, startTime: now + 10, durationSeconds: 30 },
    { channelId: channels[1].id, startTime: now + 60, durationSeconds: 30 },
    { channelId: channels[1].id, startTime: 'invalid' }
  ].map((event, index) => Object.freeze({ ...event, eventId: 'private-event-' + index, title: 'private-title', artworkUrl: 'https://private.test/?password=private-credential' }));
  hero.__test.applyChannels(channels);
  hero.__test.applyPrograms(events);
  const before = JSON.stringify(hero.snapshot());
  const originalEvents = JSON.stringify(events);
  clockReads = 0;
  assert.deepEqual(JSON.parse(JSON.stringify(hero.epgDiagnostics())), expectedCounts);
  assert.equal(clockReads, 1, 'all counts share one captured now');
  assert.equal(JSON.stringify(hero.snapshot()), before, 'diagnostics must not mutate owner state');
  assert.equal(JSON.stringify(events), originalEvents, 'loaded events must remain unchanged');
  assert.equal(hero.__test.nextEventForChannel(channels[0], events, now), null);
  assert.equal(hero.__test.nextEventForChannel(channels[1], events, now), events[5]);
  // Replacing/appending data must be reflected immediately; fallback currents
  // affect frontend Next selection but are not counted as loaded current events.
  const fallback = { id: 'private-fallback', currentEvent: { startTime: now - 10, endTime: now + 100 } };
  hero.__test.applyChannels([fallback]);
  hero.__test.applyPrograms([{ channelId: fallback.id, startTime: now + 50, duration: 10 }]);
  assert.deepEqual(JSON.parse(JSON.stringify(hero.epgDiagnostics())), {
    eventsBeforeNow: 0, eventsCurrent: 0, eventsFuture: 1,
    channelsWithCurrent: 0, channelsWithAnyFuture: 1, channelsWithFrontendNext: 0
  });
  hero.__test.applyPrograms([{ channelId: fallback.id, startTime: now + 100, duration: 10 }], true);
  assert.equal(hero.epgDiagnostics().channelsWithFrontendNext, 1);
  const manyChannels = Array.from({ length: 48 }, (_, index) => ({ id: 'private-many-' + index }));
  hero.__test.applyChannels(manyChannels);
  hero.__test.applyPrograms(manyChannels.map(channel => ({ channelId: channel.id, startTime: now + 1, duration: 10 })));
  assert.equal(hero.epgDiagnostics().channelsWithFrontendNext, 48, 'counts are not capped by rendered rails');
  hero.__test.applyChannels(channels);
  hero.__test.applyPrograms(events);
  return hero;
}

async function main() {
  const realHero = loadedHero();
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
  let nextCardCount = 9;
  const resources = [];
  const timers = [];
  class MutationObserverMock {
    constructor(callback) { this.callback = callback; }
    observe() { this.observing = true; }
    disconnect() { this.disconnected = true; }
  }
  const owner = {
    epgDiagnostics() { return { ...realHero.epgDiagnostics(), title: 'private-title', channelId: 'private-channel-a', url: 'https://private.test/?password=private-credential' }; },
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
      if (selector.includes('data-home-live-guide="next"')) return new Array(nextCardCount).fill({});
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
  assert.deepEqual(JSON.parse(JSON.stringify(report.events)), expectedCounts);
  assert.deepEqual(Object.keys(report.events), countKeys);
  assert.ok(!JSON.stringify(report).includes('private-'));
  assert.ok(report.milestones.paintReadyMs >= report.milestones.railsReadyMs);
  assert.ok(!JSON.stringify(report).includes('secret'));

  // The original zero-Next symptom must still export aggregates on timeout.
  nextCardCount = 0;
  listeners.message({ source: parent, origin: child.location.origin, data: { channel: 'vdr-suite-browser-diagnostics', token: 'token', type: 'epg' } });
  await new Promise(resolve => setImmediate(resolve));
  timers.filter(timer => timer.ms === 30000).at(-1).fn();
  await new Promise(resolve => setImmediate(resolve));
  const timeoutReport = sent.filter(item => item.message.type === 'epg-result').at(-1).message.value;
  assert.equal(timeoutReport.timedOut, true);
  assert.equal(timeoutReport.rails.nextCards, 0);
  assert.deepEqual(JSON.parse(JSON.stringify(timeoutReport.events)), expectedCounts);

  owner.refresh = () => Promise.reject(new Error('https://private.test/?password=private-credential private-title private-event'));
  owner.epgDiagnostics = () => ({ eventsBeforeNow: -1, eventsCurrent: NaN, eventsFuture: 'private-title', channelsWithCurrent: Infinity, channelsWithAnyFuture: 1.5 });
  listeners.message({ source: parent, origin: child.location.origin, data: { channel: 'vdr-suite-browser-diagnostics', token: 'token', type: 'epg' } });
  await new Promise(resolve => setImmediate(resolve));
  const errorReport = sent.filter(item => item.message.type === 'epg-result').at(-1).message.value;
  assert.equal(errorReport.refreshError, 'Home EPG refresh failed');
  assert.ok(!JSON.stringify(errorReport).includes('private'));
  assert.deepEqual(JSON.parse(JSON.stringify(errorReport.events)), Object.fromEntries(countKeys.map(key => [key, null])));

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
    epgDiagnostics: realHero.epgDiagnostics,
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
  assert.deepEqual(JSON.parse(JSON.stringify(coldReport.events)), expectedCounts);
  assert.ok(!JSON.stringify(coldReport).includes('private'));

  const handlers = {};
  const elements = {};
  function element(id) {
    return elements[id] = {
      id, disabled: false, textContent: '', attributes: {}, children: [], src: '',
      addEventListener(type, fn) { handlers[id + ':' + type] = fn; },
      setAttribute(name, value) { this.attributes[name] = value; },
      replaceChildren() { this.children = []; },
      append(...nodes) { this.children.push(...nodes); },
      click() { this.clicked = true; }
    };
  }
  for (const id of ['home', 'cold', 'start', 'stop', 'epg-cold', 'epg', 'headers', 'export', 'status', 'metrics', 'history', 'headers-result', 'epg-result']) element(id);
  const outgoing = [];
  const childWindow = {
    postMessage(data, origin) { outgoing.push({ data, origin }); },
    location: { href: 'https://example.test/vdr-suite/frontend/browser-performance-home.html', pathname: '/vdr-suite/frontend/browser-performance-home.html' }
  };
  elements.home.contentWindow = childWindow;
  const doc = {
    getElementById(id) { return elements[id]; },
    createElement() { return { textContent: '', children: [], append(...nodes) { this.children.push(...nodes); }, click() { this.clicked = true; } }; }
  };
  const controllerTimers = [];
  let exportedBlob;
  const root = {
    document: doc,
    location: { origin: 'https://example.test' },
    addEventListener(type, fn) { handlers['root:' + type] = fn; },
    URL: { createObjectURL(blob) { exportedBlob = blob; return 'blob:test'; }, revokeObjectURL() {} },
    Blob,
    setTimeout(fn, ms) { controllerTimers.push({ fn, ms, cleared: false }); return controllerTimers.length; },
    clearTimeout(id) { if (controllerTimers[id - 1]) controllerTimers[id - 1].cleared = true; },
    Math,
    Date
  };
  vm.runInNewContext(controllerSource, { window: root, Blob, Math, Date, JSON, Object, String });
  const token = elements.home.attributes['data-vdr-suite-diagnostics'];
  handlers['root:message']({ source: childWindow, origin: root.location.origin, data: { channel: 'vdr-suite-browser-diagnostics', token, type: 'started', value: 'initial-home' } });
  handlers['stop:click']();
  handlers['root:message']({ source: childWindow, origin: root.location.origin, data: { channel: 'vdr-suite-browser-diagnostics', token, type: 'result', value: { metrics: {} } } });
  handlers['epg:click']();
  assert.equal(outgoing.at(-1).data.type, 'epg');
  handlers['root:message']({ source: childWindow, origin: root.location.origin, data: { channel: 'vdr-suite-browser-diagnostics', token, type: 'epg-result', value: report } });
  assert.ok(elements['epg-result'].textContent.includes('home-epg-warm-refresh'));

  handlers['epg-cold:click']();
  const coldToken = elements.home.attributes['data-vdr-suite-diagnostics'];
  assert.notEqual(coldToken, token);
  assert.equal(elements.home.src, 'about:blank');
  assert.equal(controllerTimers.at(-1).ms, 35000);
  assert.equal(elements['epg-cold'].disabled, true);

  childWindow.location.href = 'about:blank';
  childWindow.location.pathname = 'blank';
  handlers['home:load']();
  assert.equal(elements.home.src, 'browser-performance-home.html#vdrSuiteEpgCold=1');

  childWindow.location.href = 'https://example.test/vdr-suite/frontend/browser-performance-home.html#vdrSuiteEpgCold=1';
  childWindow.location.pathname = '/vdr-suite/frontend/browser-performance-home.html';
  handlers['home:load']();
  handlers['root:message']({ source: childWindow, origin: root.location.origin, data: { channel: 'vdr-suite-browser-diagnostics', token: coldToken, type: 'epg-result', value: coldReport } });
  assert.ok(elements['epg-result'].textContent.includes('home-epg-cold-startup'));
  assert.equal(elements['epg-cold'].disabled, false);
  assert.equal(elements.export.disabled, false);
  assert.equal(controllerTimers.some(timer => timer.ms === 35000 && timer.cleared), true);
  handlers['export:click']();
  const exportedText = await exportedBlob.text();
  const exported = JSON.parse(exportedText);
  assert.deepEqual(exported.epgMeasurements.map(item => item.events), [expectedCounts, expectedCounts]);
  for (const privateValue of ['private-channel', 'private-event', 'private-title', 'private.test', 'password=', 'private-credential', 'secret=']) {
    assert.ok(!exportedText.includes(privateValue), 'export must omit ' + privateValue);
  }

  assert.ok(htmlSource.includes('id="epg-cold"'));
  assert.ok(htmlSource.includes('EPG kalt messen'));
  assert.ok(htmlSource.includes('EPG warm messen'));

  console.log('browser Home EPG cold/warm timing, forced reload, watchdog, privacy and controller wiring ok');
}

main().catch(error => { console.error(error); process.exitCode = 1; });
