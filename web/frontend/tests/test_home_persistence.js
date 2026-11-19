'use strict';

// This contract is also runnable against native IndexedDB in a browser by
// loading platform/home-persistence.js and invoking runHomePersistenceContract.
async function runHomePersistenceContract(global) {
  const check = (ok, message) => { if (!ok) throw Error(message); };
  const scope = {accountId: 'account-a', backendId: 'backend-a', permissionRevision: 'grant-1'};
  const card = {id: 'rec-1', title: 'Test', subtitle: 'Episode', csrfToken: 'must-not-store', mediaUrl: '/secret'};
  const snapshot = {complete: true, revision: 'r1', items: [card], password: 'must-not-store'};
  let authority = {...scope, authorized: true};
  const create = () => global.VdrSuiteHomePersistence.create({authorize: async () => authority});
  const first = create();
  const second = create();
  if (!global.indexedDB) {
    check(await first.read(scope, 'series') === null, 'missing IndexedDB read');
    check(await first.write(scope, 'series', snapshot) === false, 'missing IndexedDB write');
    return 'fallback PASS';
  }
  await first.clear();
  check(await first.write(scope, 'series', snapshot), 'initial write');
  const read = await second.read(scope, 'series');
  check(read.items[0].title === 'Test' && !('csrfToken' in read.items[0]) && !('mediaUrl' in read.items[0]) && !('password' in read), 'projection allowlist');
  check(await first.write(scope, 'series', {...snapshot, complete: false}) === false, 'partial snapshot rejected');
  check(await first.write(scope, 'series', {...snapshot, items: [card, card]}) === false, 'duplicate IDs rejected');
  check(await first.write(scope, 'unknown', snapshot) === false, 'unknown rail rejected');
  for (const other of [{...scope, accountId: 'account-b'}, {...scope, backendId: 'backend-b'}, {...scope, permissionRevision: 'grant-2'}]) {
    check(await second.read(other, 'series') === null, 'different identity denied');
    authority = {...other, authorized: true};
    check(await second.read(other, 'series') === null, 'different scope isolated');
    authority = {...scope, authorized: true};
  }
  authority = null;
  check(await second.read(scope, 'series') === null, 'offline or expired session');
  authority = {...scope, authorized: true};
  let release, entered;
  const waiting = new Promise(resolve => { entered = resolve; });
  const delayed = global.VdrSuiteHomePersistence.create({authorize: () => { entered(); return new Promise(resolve => { release = resolve; }); }});
  const pending = delayed.write(scope, 'series', snapshot);
  await waiting;
  check(await first.clear(), 'logout purge');
  release(authority);
  check(await pending === false, 'concurrent prior write fenced');
  check(await second.read(scope, 'series') === null, 'logout purge visible to other owner');
  check(await first.write(scope, 'series', snapshot), 'newly authorized write');
  const key = JSON.stringify([JSON.stringify([scope.accountId, scope.backendId, scope.permissionRevision]), 'series']);
  const name = 'vdr-suite-home:' + encodeURIComponent(global.VdrSuitePublicUrl.basePath);
  const db = await new Promise((resolve, reject) => {
    const r = global.indexedDB.open(name, 1);
    r.onsuccess = () => resolve(r.result); r.onerror = () => reject(r.error);
  });
  await new Promise((resolve, reject) => {
    const tx = db.transaction('snapshots', 'readwrite');
    tx.objectStore('snapshots').put({key, schema: 999, savedAt: Date.now(), value: snapshot});
    tx.oncomplete = resolve; tx.onerror = reject;
  });
  db.close();
  check(await second.read(scope, 'series') === null, 'corrupt schema ignored');
  await first.clear();
  for (let i = 0; i < 33; i++) {
    const selected = {...scope, backendId: 'backend-' + i};
    authority = {...selected, authorized: true};
    check(await first.write(selected, 'series', snapshot), 'bounded write ' + i);
  }
  authority = {...scope, backendId: 'backend-0', authorized: true};
  check(await first.read(authority, 'series') === null, 'oldest evicted');
  await first.clear();
  const upgraded = await new Promise((resolve, reject) => {
    const r = global.indexedDB.open(name, 2);
    r.onupgradeneeded = () => {}; r.onsuccess = () => resolve(r.result); r.onerror = () => reject(r.error);
  });
  upgraded.close();
  check(await first.read(scope, 'series') === null, 'newer schema falls back');
  return 'native IndexedDB contract PASS';
}

if (typeof module !== 'undefined' && module.exports) {
  const fs = require('node:fs');
  const path = require('node:path');
  const vm = require('node:vm');
  const source = fs.readFileSync(path.join(__dirname, '../platform/home-persistence.js'), 'utf8');
  const window = {VdrSuitePublicUrl: {basePath: '/test'}};
  vm.runInNewContext(source, {window});
  (async () => {
    console.log(await runHomePersistenceContract(window));
    window.indexedDB = require('./home_persistence_idb_double.js')();
    await runHomePersistenceContract(window);
    console.log('IndexedDB asynchronous test-double contract PASS');
    window.indexedDB = {open() { throw Error('storage denied'); }};
    const owner = window.VdrSuiteHomePersistence.create({authorize: async () => { throw Error('must not call'); }});
    if (await owner.read({accountId:'a', backendId:'b', permissionRevision:'g'}, 'series') !== null) throw Error('storage failure');
    console.log('IndexedDB denied-storage fallback PASS');
  })().catch(error => { console.error(error); process.exitCode = 1; });
} else {
  window.runHomePersistenceContract = runHomePersistenceContract;
}
