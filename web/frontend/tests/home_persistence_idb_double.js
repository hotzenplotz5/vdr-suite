'use strict';

// Minimal asynchronous IndexedDB test double for the owner contract; native
// transaction scheduling, upgrade/blocked behavior and quota still need browser tests.
module.exports = function makeIndexedDB() {
  const databases = new Map();
  const later = fn => setImmediate(fn);
  const clone = value => value === undefined ? undefined : structuredClone(value);
  return {
    open(name, version) {
      const request = {};
      later(() => {
        let state = databases.get(name);
        if (state && state.version > version) { request.onerror(); return; }
        const upgrade = !state || state.version < version;
        if (!state) { state = {version: 0, stores: new Map(), connections: new Set()}; databases.set(name, state); }
        if (upgrade) for (const connection of state.connections) if (connection.onversionchange) connection.onversionchange();
        const db = {
          createObjectStore(storeName) {
            state.stores.set(storeName, new Map());
            return {createIndex() {}};
          },
          close() { state.connections.delete(db); },
          transaction() {
            let pending = 0;
            let finished = false;
            const tx = {};
            function complete() {
              later(() => { if (!pending && !finished) { finished = true; if (tx.oncomplete) tx.oncomplete(); } });
            }
            function operation(fn, request = {}) {
              pending++;
              later(() => {
                try { request.result = fn(); if (request.onsuccess) request.onsuccess(); }
                catch (error) { finished = true; if (tx.onerror) tx.onerror(); else throw error; }
                finally { pending--; complete(); }
              });
              return request;
            }
            tx.objectStore = storeName => {
              const rows = state.stores.get(storeName);
              return {
                get: key => operation(() => clone(rows.get(key))),
                put: (value, key = value.key) => operation(() => rows.set(key, clone(value))),
                clear: () => operation(() => rows.clear()),
                count: () => operation(() => rows.size),
                index: () => ({openCursor() {
                  const entries = [...rows].sort((a, b) => a[1].savedAt - b[1].savedAt || a[0].localeCompare(b[0]));
                  let i = 0;
                  const request = {};
                  function next() {
                    return operation(() => {
                      const entry = entries[i++];
                      return entry ? {delete: () => operation(() => rows.delete(entry[0])), continue: next} : null;
                    }, request);
                  }
                  return next();
                }})
              };
            };
            complete();
            return tx;
          }
        };
        state.connections.add(db);
        request.result = db;
        if (upgrade) { state.version = version; if (request.onupgradeneeded) request.onupgradeneeded(); }
        if (request.onsuccess) request.onsuccess();
      });
      return request;
    }
  };
};
