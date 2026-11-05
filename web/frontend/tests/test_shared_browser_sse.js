'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const code = fs.readFileSync(path.join(__dirname, '..', 'api',
  'live-remote-client-api.js'), 'utf8');
const physical = [];
let token = 'a'.repeat(40);
class FakeEventSource {
  constructor(url, options) {
    assert.strictEqual(url, '/api/vdr/live');
    assert.strictEqual(options.withCredentials, true);
    this.listeners = {};
    this.closed = false;
    physical.push(this);
  }
  addEventListener(type, callback) {
    (this.listeners[type] ||= []).push(callback);
  }
  emit(type, event = {data: '{}'}) {
    (this.listeners[type] || []).forEach(fn => fn(event));
    if (type === 'open' && this.onopen) this.onopen(event);
    if (type === 'error' && this.onerror) this.onerror(event);
  }
  close() { this.closed = true; }
}
const window = {
  EventSource: FakeEventSource,
  VdrSuiteClientApi: {requestJson() { return Promise.resolve({}); }},
  fetch: async function (url) {
    if (url === '/api/security/browser-sessions/current') {
      return {ok: true, status: 200, text: async () =>
        JSON.stringify({csrfToken: token, expiresAt: '2099-01-01T00:00:00Z'})};
    }
    if (url === '/api/security/browser-sessions/logout') {
      return {ok: true, status: 204, text: async () => ''};
    }
    throw new Error('unexpected request ' + url);
  },
  setTimeout() { return 1; },
  clearTimeout() {},
  console
};
window.window = window;
vm.runInNewContext(code, {window, console, Promise, Date, setTimeout, clearTimeout});
const client = window.VdrSuiteClientApi;
const session = window.VdrSuiteBrowserSession;

(async function () {
  await session.restore();
  const one = client.createClientLiveUpdateSource();
  const two = client.createClientLiveUpdateSource();
  assert(one && two);
  assert.strictEqual(physical.length, 1, 'rails share a single browser socket');
  let first = 0;
  let second = 0;
  let reconnect = 0;
  one.addEventListener('update', () => first++);
  two.addEventListener('update', () => second++);
  two.onopen = () => reconnect++;
  physical[0].emit('update');
  physical[0].emit('open');
  assert.deepStrictEqual([first, second, reconnect], [1, 1, 1]);

  one.close();
  assert.strictEqual(physical[0].closed, false,
    'closing one rail must not disconnect the other');
  physical[0].emit('update');
  assert.deepStrictEqual([first, second], [1, 2]);

  two.close();
  assert.strictEqual(physical[0].closed, true,
    'last subscriber must release the physical connection');
  const three = client.createClientLiveUpdateSource();
  assert.strictEqual(physical.length, 2);
  let resync = 0;
  three.addEventListener('resync', () => resync++);
  token = 'b'.repeat(40);
  session.clear('session_revoked');
  assert.strictEqual(physical[1].closed, true);
  assert.strictEqual(three.closed, true);
  assert.strictEqual(client.createClientLiveUpdateSource(), null);

  await session.restore();
  const four = client.createClientLiveUpdateSource();
  assert(four);
  assert.strictEqual(physical.length, 3);
  let fourth = 0;
  four.addEventListener('resync', () => fourth++);
  physical[2].emit('resync');
  assert.strictEqual(fourth, 1);
  assert.strictEqual(resync, 0, 'retired actor must not receive new events');
  await session.logout();
  assert(physical[2].closed && four.closed);
  console.log('test_shared_browser_sse passed');
})().catch(error => { console.error(error); process.exitCode = 1; });
