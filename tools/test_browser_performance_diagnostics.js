'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const vm = require('node:vm');
const { spawnSync } = require('node:child_process');
const frontend = path.resolve(__dirname, '../web/frontend');
const read = name => fs.readFileSync(path.join(frontend, name), 'utf8');
const run = (name, env) => vm.runInNewContext(read(name), env, { filename: name });
const { createProbe } = require('../web/frontend/browser-artwork-probe');

async function main() {
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'vdr-suite-diagnostics-'));
  try {
    const output = path.join(tmp, 'browser-performance-home.html');
    const build = spawnSync(process.execPath, [path.join(__dirname, 'build_browser_performance_diagnostics.js'), output], { encoding: 'utf8' });
    assert.equal(build.status, 0, build.stderr);
    const html = fs.readFileSync(output, 'utf8');
    assert.ok(html.includes('browser-artwork-probe.js'));
    assert.ok(html.includes('browser-performance-bridge.js'));
    assert.ok(html.indexOf('browser-performance-bridge.js') < html.indexOf('platform/bootstrap.js'));
    assert.ok(html.includes('../frontend/app.js'));
    assert.ok(html.includes('../frontend/platform/public-url.js'));
    assert.equal((html.match(/browser-performance-bridge.js/g) || []).length, 1);
    assert.ok(!html.includes('src="../frontend/browser-performance-bridge.js"'));
    assert.ok(!html.includes('vdr-suite-diagnostics=1'));
    assert.ok(!read('index.html').includes('browser-performance-bridge.js'));
    const page = read('browser-performance-diagnostics.html');
    for (const id of ['home', 'cold', 'start', 'stop', 'headers', 'export', 'status', 'metrics', 'history', 'headers-result']) assert.ok(page.includes('id="' + id + '"'));

    const listeners = {};
    const sent = [];
    const frame = { attributes: {}, hasAttribute(name) { return name in this.attributes; }, getAttribute(name) { return this.attributes[name]; } };
    frame.attributes['data-vdr-suite-diagnostics'] = 'test-token';
    const parent = { postMessage(message, origin) { sent.push({ message, origin }); } };
    const calls = [];
    const probe = {
      start(label, options) { calls.push(['start', label, options]); },
      stop() { calls.push(['stop']); return { label: 'initial-home', metrics: { resourceRequests: 2 } }; },
      inspectHeaders() { calls.push(['headers']); return Promise.resolve({ status: 200, headers: {} }); }
    };
    const child = { frameElement: frame, parent, VdrSuiteArtworkProbe: probe, location: { origin: 'https://example.test' }, addEventListener(type, fn) { listeners[type] = fn; }, Promise };
    run('browser-performance-bridge.js', { window: child, Promise });
    assert.equal(calls[0][1], 'initial-home');
    assert.equal(calls[0][2].buffered, true);
    assert.equal(sent[0].message.type, 'started');
    const message = (type, extra) => listeners.message({ source: parent, origin: 'https://example.test', data: Object.assign({ channel: 'vdr-suite-browser-diagnostics', token: 'test-token', type }, extra) });
    listeners.message({ source: {}, origin: 'https://example.test', data: { channel: 'vdr-suite-browser-diagnostics', token: 'test-token', type: 'stop' } });
    assert.equal(calls.length, 1);
    message('stop');
    assert.equal(sent.at(-1).message.type, 'result');
    message('start', { label: 'navigation' });
    assert.equal(calls.at(-1)[2].buffered, false);
    message('stop');
    message('headers');
    await new Promise(resolve => setImmediate(resolve));
    assert.equal(sent.at(-1).message.type, 'headers');
    assert.equal(calls.at(-1)[0], 'headers');

    const handlers = {};
    const elements = {};
    function element(id) {
      return elements[id] = { id, disabled: false, textContent: '', attributes: {}, children: [], addEventListener(type, fn) { handlers[id + ':' + type] = fn; }, setAttribute(name, value) { this.attributes[name] = value; }, replaceChildren() { this.children = []; }, append(...nodes) { this.children.push(...nodes); }, click() { this.clicked = true; } };
    }
    for (const id of ['home', 'cold', 'start', 'stop', 'headers', 'export', 'status', 'metrics', 'history', 'headers-result']) element(id);
    const outgoing = [];
    const childWindow = { postMessage(data, origin) { outgoing.push({ data, origin }); }, location: { pathname: '/vdr-suite/frontend/browser-performance-home.html' } };
    elements.home.contentWindow = childWindow;
    const document = { getElementById(id) { return elements[id]; }, createElement() { return { textContent: '', children: [], append(...nodes) { this.children.push(...nodes); }, click() { this.clicked = true; } }; } };
    const root = { document, location: { origin: 'https://example.test' }, addEventListener(type, fn) { handlers['root:' + type] = fn; }, URL: { createObjectURL() { return 'blob:test'; }, revokeObjectURL() {} }, Blob, setTimeout(fn) { fn(); }, Math, Date };
    run('browser-performance-diagnostics.js', { window: root, Blob, Math, Date, JSON, Object, String });
    assert.equal(elements.home.src, 'browser-performance-home.html');
    const token = elements.home.attributes['data-vdr-suite-diagnostics'];
    function incoming(type, value, override) {
      handlers['root:message']({ source: childWindow, origin: 'https://example.test', data: Object.assign({ channel: 'vdr-suite-browser-diagnostics', token, type, value }, override) });
    }
    incoming('started', 'initial-home');
    assert.equal(elements.stop.disabled, false);
    handlers['stop:click']();
    assert.equal(outgoing.at(-1).data.type, 'stop');
    incoming('result', { label: 'initial-home', metrics: { resourceRequests: 2 } });
    assert.equal(elements.start.disabled, false);
    assert.equal(elements.export.disabled, false);
    incoming('result', { label: 'forged', metrics: {} }, { token: 'wrong' });
    assert.ok(!elements.history.textContent.includes('forged'));
    handlers['headers:click']();
    assert.equal(outgoing.at(-1).data.type, 'headers');
    incoming('headers', { status: 200, headers: { 'cache-control': 'private' } });
    assert.equal(elements.headers.disabled, false);
    handlers['cold:click']();
    assert.equal(elements.home.src, 'browser-performance-home.html');
    assert.notEqual(elements.home.attributes['data-vdr-suite-diagnostics'], token);
    incoming('result', { label: 'stale', metrics: {} });
    assert.ok(!elements.history.textContent.includes('stale'));

    let now = 0;
    const observers = [];
    class Observer {
      constructor(callback) { this.callback = callback; this.pending = []; observers.push(this); }
      observe(options) { this.options = options; }
      takeRecords() { const records = this.pending; this.pending = []; return records; }
      disconnect() { this.disconnected = true; }
    }
    const cover = { currentSrc: 'https://example.test/api/metadata/cover?id=secret', src: '', complete: true, naturalWidth: 100, loading: 'lazy' };
    const env = { document: { querySelectorAll(selector) { return selector === 'img' ? [cover] : selector === '.media-home-discovery-rail' ? [{ scrollLeft: 30 }] : selector.includes(' img') ? [cover] : []; } }, performance: { now() { return now; } }, PerformanceObserver: Observer, MutationObserver: Observer, location: { href: 'https://example.test/frontend/', origin: 'https://example.test' }, URL, fetch: async () => { throw new Error('Unexpected network request'); } };
    const measurement = createProbe(env);
    measurement.start('initial-home', { buffered: true });
    assert.equal(observers[0].options.buffered, true);
    assert.equal(observers[1].options.buffered, true);
    observers[0].pending.push({ entryType: 'resource', name: cover.currentSrc, initiatorType: 'img', transferSize: 120 });
    observers[1].pending.push({ entryType: 'longtask', duration: 70 });
    observers[2].pending.push({});
    now = 80;
    const result = measurement.stop();
    assert.equal(result.metrics.resourceRequests, 1);
    assert.equal(result.metrics.longTasks, 1);
    assert.equal(result.metrics.mutations, 1);
    assert.equal(result.bufferedStartup, true);
    assert.ok(observers.every(observer => observer.disconnected));
    assert.ok(!JSON.stringify(result).includes('secret'));
    const requests = [];
    env.fetch = async function (url, options) { requests.push({ url, options }); return { status: 304, type: 'basic', headers: { get(name) { return name === 'etag' ? '"abc"' : null; } }, body: { cancel: async function () {} } }; };
    const headers = await measurement.inspectHeaders();
    assert.equal(requests.length, 1);
    assert.equal(requests[0].url, cover.currentSrc);
    assert.equal(requests[0].options.cache, 'no-cache');
    assert.equal(headers.status, 304);
    assert.ok(!JSON.stringify(headers).includes('secret'));
    assert.ok(!JSON.stringify(headers).includes('url:'));
    console.log('browser diagnostics composition, lifecycle, buffered observers and privacy ok');
  } finally { fs.rmSync(tmp, { recursive: true, force: true }); }
}
main().catch(error => { console.error(error); process.exitCode = 1; });
