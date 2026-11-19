(function (global) {
  'use strict';

  const SCHEMA = 1;
  const MAX_ITEMS = 500;
  const MAX_BYTES = 256 * 1024;
  const MAX_SNAPSHOTS = 32;
  const RAILS = new Set(['newly-recorded', 'series', 'genres', 'folders', 'recent-movies', 'continue-watching', 'epg', 'live']);
  const text = (value, max) => typeof value === 'string' && value.length > 0 && value.length <= max;
  const id = value => text(value, 256) && !/[\u0000-\u001f\u007f]/.test(value);

  function cursor(value) {
    const match = /^sse_([0-9a-f]{32}):(0|[1-9][0-9]*)$/.exec(value || '');
    if (!match || !Number.isSafeInteger(Number(match[2]))) return null;
    return {epoch: match[1], sequence: Number(match[2])};
  }

  function atLeast(value, minimum) {
    const a = cursor(value), b = cursor(minimum);
    return Boolean(a && b && a.epoch === b.epoch && a.sequence >= b.sequence);
  }

  function scopeKey(scope) {
    if (!scope || !id(scope.accountId) || !id(scope.backendId) || !id(scope.permissionRevision)) return null;
    return JSON.stringify([scope.accountId, scope.backendId, scope.permissionRevision]);
  }

  // Only existing, backend-scoped canonical preview addresses may be persisted.
  // Never keep session-bearing, external or arbitrary URLs.
  function poster(value, backend) {
    if (!text(value, 4096) || !value.startsWith('/') || value.startsWith('//') ||
        typeof global.URL !== 'function') return '';
    try {
      const url = new global.URL(value, 'https://vdr-suite.invalid');
      if (url.origin !== 'https://vdr-suite.invalid' || url.hash || url.username || url.password) return '';
      const routes = ['/api/vdr/recordings/metadata/image', '/api/recordings/metadata/image'];
      const direct = url.pathname.match(/^\/recording-artwork\/([^/]+)\/[0-9a-fA-F]{32}$/);
      if (!routes.includes(url.pathname) && !direct) return '';
      if (direct ? decodeURIComponent(direct[1]) !== backend : url.searchParams.get('backend') !== backend) return '';
      const allowed = direct ? ['variant'] : ['backend', 'backendNativeId', 'recordingId', 'kind', 'index', 'assignmentRevision', 'variant'];
      const seen = new Set();
      for (const key of url.searchParams.keys()) {
        if (!allowed.includes(key) || seen.has(key)) return '';
        seen.add(key);
      }
      if (seen.has('variant') && url.searchParams.get('variant') !== 'home') return '';
      return url.pathname + url.search;
    } catch (error) { return ''; }
  }

  // A deliberately small card projection; never persist arbitrary API objects,
  // credentials, playback URLs, artwork responses or partially loaded snapshots.
  function projection(value, backend) {
    if (!value || value.complete !== true || !id(value.revision) || !Array.isArray(value.items) || value.items.length > MAX_ITEMS) return null;
    const ids = new Set();
    const items = [];
    for (const item of value.items) {
      if (!item || !id(item.id) || ids.has(item.id) || !text(item.title, 1024) ||
          (item.subtitle !== undefined && (typeof item.subtitle !== 'string' || item.subtitle.length > 1024))) return null;
      ids.add(item.id);
      const card = {id: item.id, title: item.title, subtitle: item.subtitle || ''};
      const url = poster(item.posterUrl, backend);
      if (url) card.posterUrl = url;
      items.push(card);
    }
    const result = {complete: true, revision: value.revision, items};
    return JSON.stringify(result).length * 2 <= MAX_BYTES ? result : null;
  }

  function create(options) {
    const authorize = options && options.authorize;
    const publicUrl = global.VdrSuitePublicUrl;
    const name = publicUrl && typeof publicUrl.basePath === 'string'
      ? 'vdr-suite-home:' + encodeURIComponent(publicUrl.basePath) : null;
    let dbPromise = null;
    let generation = 0;

    function database() {
      if (!name) return Promise.resolve(null);
      if (!dbPromise) dbPromise = new Promise(resolve => {
        try {
          const request = global.indexedDB.open(name, SCHEMA);
          let abandoned = false;
          request.onupgradeneeded = () => {
            const db = request.result;
            db.createObjectStore('snapshots', {keyPath: 'key'}).createIndex('savedAt', 'savedAt');
            db.createObjectStore('meta');
          };
          request.onblocked = () => { abandoned = true; resolve(null); };
          request.onerror = () => resolve(null);
          request.onsuccess = () => {
            const db = request.result;
            if (abandoned) { db.close(); return; }
            db.onversionchange = () => { db.close(); dbPromise = null; generation++; };
            resolve(db);
          };
        } catch (error) { resolve(null); }
      }).then(db => { if (!db) dbPromise = null; return db; });
      return dbPromise;
    }

    async function transaction(mode, action, fallback) {
      try {
        const db = await database();
        if (!db) return fallback;
        return await new Promise(resolve => {
          const tx = db.transaction(['snapshots', 'meta'], mode);
          let result = fallback;
          tx.oncomplete = () => resolve(result);
          tx.onabort = tx.onerror = () => resolve(fallback);
          const set = value => { result = value; };
          action(tx.objectStore('snapshots'), tx.objectStore('meta'), set);
        });
      } catch (error) { return fallback; }
    }

    function epoch() {
      return transaction('readonly', (_, meta, set) => {
        const r = meta.get('epoch');
        r.onsuccess = () => set(r.result === undefined ? 0 : r.result);
      }, null);
    }

    // Separate session epoch: SSE invalidation must not invalidate a fresh
    // authorization proof for an unrelated rail. Capture before network reads.
    function captureSession() {
      return transaction('readonly', (_, meta, set) => {
        const r = meta.get('sessionEpoch');
        r.onsuccess = () => set(r.result === undefined ? 0 : r.result);
      }, null);
    }

    async function access(scope, rail, action, fallback) {
      const key = scopeKey(scope);
      if (!key || !RAILS.has(rail) || typeof authorize !== 'function') return fallback;
      const local = generation;
      const fenced = scope.sessionEpoch !== undefined;
      const expectedSession = fenced ? await scope.sessionEpoch : null;
      if (fenced && (!Number.isSafeInteger(expectedSession) || expectedSession < 0)) return fallback;
      const before = await epoch();
      if (!Number.isSafeInteger(before) || before < 0 || local !== generation) return fallback;
      // The caller must obtain current server authorization for this exact rail;
      // a remembered login boolean or offline token is not sufficient evidence.
      let proof;
      try { proof = await authorize(Object.freeze({...scope}), rail); } catch (error) { return fallback; }
      if (!proof || proof.authorized !== true || scopeKey(proof) !== key || local !== generation) return fallback;
      const result = await transaction('readwrite', (store, meta, set) => {
        const r = meta.get('epoch');
        r.onsuccess = () => {
          if ((r.result === undefined ? 0 : r.result) !== before || local !== generation) return;
          if (!fenced) { action(store, JSON.stringify([key, rail]), set, meta); return; }
          const session = meta.get('sessionEpoch');
          session.onsuccess = () => {
            if ((session.result === undefined ? 0 : session.result) !== expectedSession || local !== generation) return;
            action(store, JSON.stringify([key, rail]), set, meta);
          };
        };
      }, fallback);
      if (fenced && await captureSession() !== expectedSession) return fallback;
      return local === generation ? result : fallback;
    }

    function read(scope, rail) {
      return access(scope, rail, (store, key, set) => {
        const request = store.get(key);
        request.onsuccess = () => {
          const row = request.result;
          if (!row || row.schema !== SCHEMA || row.key !== key || !Number.isFinite(row.savedAt)) return;
          set(projection(row.value, scope.backendId));
        };
      }, null);
    }

    function write(scope, rail, value) {
      const safe = projection(value, scope && scope.backendId);
      if (!safe) return Promise.resolve(false);
      return access(scope, rail, (store, key, set, meta) => {
        const floor = meta.get('liveFloor');
        floor.onsuccess = () => {
          if (floor.result && !atLeast(safe.revision, floor.result)) return;
          const previous = store.get(key);
          previous.onsuccess = () => {
            const old = previous.result && previous.result.value && previous.result.value.revision;
            if (cursor(old) && atLeast(old, safe.revision) && old !== safe.revision) return;
            persist();
          };
        };
        function persist() {
          store.put({key, schema: SCHEMA, savedAt: Date.now(), value: safe});
          const count = store.count();
          count.onsuccess = () => {
            let excess = count.result - MAX_SNAPSHOTS;
            if (excess <= 0) { set(true); return; }
            const oldest = store.index('savedAt').openCursor();
            oldest.onsuccess = () => {
              const cursor = oldest.result;
              if (!cursor || excess <= 0) { set(true); return; }
              cursor.delete();
              excess--;
              cursor.continue();
            };
          };
        }
      }, false);
    }

    // Synchronous local fence for backend/account transitions. Logout additionally
    // advances a shared transaction epoch and purges this installation only.
    function invalidate() { generation++; }
    function clear() {
      invalidate();
      return transaction('readwrite', (store, meta, set) => {
        const r = meta.get('epoch');
        r.onsuccess = () => {
          const old = Number.isSafeInteger(r.result) && r.result >= 0 ? r.result : 0;
          meta.put(old + 1, 'epoch');
          const session = meta.get('sessionEpoch');
          session.onsuccess = () => {
            const previous = Number.isSafeInteger(session.result) && session.result >= 0 ? session.result : 0;
            meta.put(previous + 1, 'sessionEpoch');
            store.clear();
            set(true);
          };
        };
      }, false);
    }

    // One bounded transaction for the shared write fence and affected rows.
    // A newer tab's rows must not be removed by replay of an older event.
    function discard(backend, rails, revision) {
      if (!cursor(revision) || !Array.isArray(rails) || rails.some(rail => !RAILS.has(rail))) return Promise.resolve(false);
      invalidate();
      return transaction('readwrite', (store, meta, set) => {
        const floor = meta.get('liveFloor');
        floor.onsuccess = () => {
          const previous = cursor(floor.result), next = cursor(revision);
          if (atLeast(floor.result, revision) && !rails.length) { set(true); return; }
          if (previous && previous.epoch !== next.epoch) {
            const retired = meta.get('retiredLiveEpochs');
            retired.onsuccess = () => {
              const epochs = Array.isArray(retired.result) ? retired.result : [];
              if (epochs.includes(next.epoch)) return;
              meta.put([...epochs, previous.epoch].slice(-8), 'retiredLiveEpochs');
              proceed();
            };
          } else proceed();
          function proceed() {
            if (!atLeast(floor.result, revision)) meta.put(revision, 'liveFloor');
            const epochRequest = meta.get('epoch');
            epochRequest.onsuccess = () => {
              const old = Number.isSafeInteger(epochRequest.result) ? epochRequest.result : 0;
              meta.put(old + 1, 'epoch');
              if (!rails.length) { set(true); return; }
              const request = store.index('savedAt').openCursor();
              request.onsuccess = () => {
                const row = request.result;
                if (!row) { set(true); return; }
                try {
                  const [key, rail] = JSON.parse(row.value.key);
                  const [, rowBackend] = JSON.parse(key);
                  if ((!backend || rowBackend === backend) && rails.includes(rail) &&
                      !atLeast(row.value.value && row.value.value.revision, revision)) row.delete();
                } catch (error) { row.delete(); }
                row.continue();
              };
            };
          }
        };
      }, false);
    }

    return Object.freeze({read, write, invalidate, clear, discard, captureSession});
  }

  const api = Object.freeze({create});
  global.VdrSuiteHomePersistence = api;
  if (global.VdrSuitePlatform && typeof global.VdrSuitePlatform.registerModule === 'function') {
    global.VdrSuitePlatform.registerModule('home-persistence', api);
  }
}(window));
