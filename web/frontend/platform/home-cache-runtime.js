(function (global) {
  'use strict';

  const RAIL_ROUTES = Object.freeze({
    'newly-recorded': '/api/vdr/recordings/query',
    'series': '/api/metadata/genres/recordings',
    'genres': '/api/metadata/genres',
    'folders': '/api/vdr/recordings/folder',
    'recent-movies': '/api/vdr/recordings/query',
    'continue-watching': '/api/media/continue-watching'
  });
  const PUBLIC_GENRE_ROUTE = '/api/v1/genres';
  const PUBLIC_RECORDING_ROUTE = '/api/v1/recordings';
  const ALL_RAILS = Object.keys(RAIL_ROUTES);
  const VALID_SCOPE = /^vsr-[0-9a-f]{64}$/;
  const PROOF_MS = 5000;
  const PREVIEW_MS = 15000;
  const STARTUP_PREVIEW_MAX_AGE_MS = 24 * 60 * 60 * 1000;
  const baseRail = rail => typeof rail === 'string' && rail.startsWith('folders:') ? 'folders' : rail;
  const domain = rail => baseRail(rail) === 'continue-watching' ? 'history' : 'recordings';

  function cursor(value) {
    const match = /^sse_([0-9a-f]{32}):(0|[1-9][0-9]*)$/.exec(value || '');
    if (!match || !Number.isSafeInteger(Number(match[2]))) return null;
    return {id: value, epoch: match[1], sequence: Number(match[2])};
  }

  function createRuntime(options) {
    const config = options || {};
    const now = config.now || Date.now;
    const active = config.active || (() => {
      const session = global.VdrSuiteBrowserSession;
      const connection = global.VdrSuiteConnection;
      const state = connection && typeof connection.state === 'function' ? connection.state() : 'ready';
      return Boolean(session && session.isAuthenticated() && (!global.navigator || global.navigator.onLine !== false) &&
        (state === 'ready' || state === 'reconnecting'));
    });
    const paint = config.paint || paintPreview;
    let backend = '';
    let generation = 0;
    let prefetchEpoch = 0;
    let partition = '';
    const proofs = new Map();
    const payloads = new WeakMap();
    const observedHeaders = new WeakMap();
    const tickets = new Map();
    let channel = null;
    const requireCursor = config.requireCursor !== false;
    let currentCursor = null, baseline = null, connected = false;
    // Only the first connection may display a bounded, explicitly stale
    // projection before its cursor is known. Never use this after a gap/error.
    let awaitingInitialCursor = true;
    let storageBarrier = Promise.resolve(true);
    const railVersions = new Map(), minimums = new Map();
    const version = rail => railVersions.get(rail) || 0;
    function advance(rails, at) {
      for (const rail of rails) {
        railVersions.set(rail, version(rail) + 1);
        if (at) minimums.set(rail, at);
        for (const [key, ticket] of tickets)
          if (baseRail(key) === rail) ticket.cancel();
      }
    }
    function discard(target, rails, at) {
      // Serialize cache mutations before new reads/writes use this cursor.
      storageBarrier = storageBarrier.then(() => owner.discard(target, rails, at.id)).catch(() => false);
    }
    function closeCursor() {
      awaitingInitialCursor = false;
      connected = false;
      currentCursor = null;
      baseline = null;
      minimums.clear();
      advance(ALL_RAILS, null);
    }
    function liveEvent(type, event) {
      if (type === 'close') { closeCursor(); return false; }
      if (type === 'error') {
        ++prefetchEpoch;
        awaitingInitialCursor = false;
        connected = false;
        for (const ticket of tickets.values()) ticket.dismiss();
        return false;
      }
      if (type === 'open') { connected = Boolean(currentCursor); return false; }
      if (type !== 'update' && type !== 'resync') return false;
      let data;
      try { data = JSON.parse(event.data); } catch (error) { closeCursor(); return false; }
      const at = cursor(event.lastEventId);
      const sequence = type === 'resync' ? data && data.latestSequenceNumber : data && data.sequenceNumber;
      if (!at || sequence !== at.sequence || (type === 'update' &&
          (!data || !Array.isArray(data.changedDomains) || typeof data.backendId !== 'string' || !data.backendId))) {
        // Legacy/malformed feed: disable persistent reuse rather than guess.
        closeCursor();
        return false;
      }
      const prior = currentCursor;
      awaitingInitialCursor = false;
      if (type === 'update' && prior && at.epoch === prior.epoch && at.sequence <= prior.sequence) return false;
      const gap = type === 'update' && (!prior || at.epoch !== prior.epoch || at.sequence !== prior.sequence + 1);
      currentCursor = at;
      connected = true;
      if (type === 'resync' || gap) {
        const initial = type === 'resync' && data.reason === 'initial' && !prior;
        baseline = at;
        minimums.clear();
        if (!initial && (!prior || prior.id !== at.id || data.reason !== 'initial')) {
          advance(ALL_RAILS, at);
          discard(null, ALL_RAILS, at);
        } else {
          // First connection: equal durable cursor can still use a warm preview.
          discard(null, [], at);
        }
        for (const ticket of tickets.values()) ticket.preview();
        return gap;
      }
      const rails = data.changedDomains.includes('recordings') ? ALL_RAILS :
        data.changedDomains.includes('continueWatching') ? ['continue-watching'] : [];
      if (rails.length) {
        if (backend === data.backendId) advance(rails, at);
        discard(data.backendId, rails, at);
      }
      return false;
    }
    function canPreview(rail, snapshot) {
      if (!requireCursor) return true;
      const saved = cursor(snapshot.revision), minimum = minimums.get(rail) || baseline;
      if (awaitingInitialCursor && !currentCursor) return Boolean(saved);
      return Boolean(connected && currentCursor && minimum && saved &&
        saved.epoch === currentCursor.epoch && saved.sequence >= minimum.sequence && saved.sequence <= currentCursor.sequence);
    }
    const makeOwner = config.makeOwner || (opts => global.VdrSuiteHomePersistence.create(opts));
    const owner = makeOwner({authorize: async (scope, rail) => {
      const proof = currentProof(rail);
      if (!proof || scope.backendId !== backend || scope.accountId !== proof.partition ||
          scope.permissionRevision !== proof.partition) return null;
      const expected = await scope.sessionEpoch;
      if (!Number.isSafeInteger(expected) || expected !== await proof.sessionEpoch ||
          currentProof(rail) !== proof) return null;
      return {...scope, authorized: true};
    }});

    function currentProof(rail) {
      const proof = proofs.get(domain(rail));
      const age = proof && now() - proof.at;
      return active() && proof && proof.generation === generation && age >= 0 && age <= PROOF_MS ? proof : null;
    }
    function scopeOf(proof) {
      return {accountId: proof.partition, backendId: backend, permissionRevision: proof.partition, sessionEpoch: proof.sessionEpoch};
    }
    function reset(purge, notice = 'cache-invalidated') {
      generation++;
      partition = '';
      proofs.clear();
      for (const ticket of tickets.values()) ticket.cancel();
      tickets.clear();
      owner.invalidate();
      if (purge) {
        // Start the durable fence now, rather than behind queued SSE work.
        const clearing = owner.clear();
        storageBarrier = Promise.all([storageBarrier, clearing]).then(results => results[1]).catch(() => false);
        if (channel) { try { channel.postMessage(notice); } catch (error) {} }
        // Grant drift/401/403 also invalidates the optional artwork cache.
        try {
          const controller = global.navigator && global.navigator.serviceWorker && global.navigator.serviceWorker.controller;
          if (controller) controller.postMessage({type: 'vdr-suite-artwork-clear'});
        } catch (error) {}
      }
    }
    function suspend() { reset(false); closeCursor(); }
    function selectBackend(value) {
      reset(false);
      backend = typeof value === 'string' ? value : '';
      baseline = currentCursor;
      minimums.clear();
    }
    function sessionBoundary() {
      reset(true, 'session-boundary');
      closeCursor();
    }
    // Optional short-lived memory prefetch must remain in the same
    // backend/session/grant and domain-change generation.
    function captureRailFence(rail, requestedBackend) {
      if (!active() || !partition || !RAIL_ROUTES[rail] || backend !== requestedBackend)
        return null;
      const local = generation, localPrefetch = prefetchEpoch, railVersion = version(rail), scope = partition;
      return () => active() && generation === local && prefetchEpoch === localPrefetch &&
        version(rail) === railVersion && backend === requestedBackend && partition === scope;
    }
    function requestStamp(path, requestedBackend, view) {
      if (!backend || !active() || backend !== requestedBackend) return null;
      const snapshot = path === '/api/backends/' + encodeURIComponent(backend) + '/snapshot';
      if (!snapshot && path !== PUBLIC_GENRE_ROUTE && path !== PUBLIC_RECORDING_ROUTE && !Object.values(RAIL_ROUTES).includes(path)) return null;
      if (path === PUBLIC_RECORDING_ROUTE && view !== 'new' && view !== 'movies') return null;
      return {path, view, backend, generation, snapshot, cursor: currentCursor && currentCursor.id,
        versions: ALL_RAILS.map(version), sessionEpoch: owner.captureSession()};
    }
    function observe(stamp, response, payload) {
      if (!stamp || stamp.generation !== generation || stamp.backend !== backend || !active()) return;
      // Authorization headers arrive before potentially large JSON bodies.
      // Record the exact response proof once; only parsed payloads can later
      // establish provenance for a durable replacement.
      if (observedHeaders.has(stamp)) {
        const observed = observedHeaders.get(stamp);
        if (observed && observed.partition === partition && payload && typeof payload === 'object')
          payloads.set(payload, observed);
        return;
      }
      observedHeaders.set(stamp, null);
      if (!response || !response.ok || response.redirected) {
        if (response && (response.status === 401 || response.status === 403)) reset(true);
        else if (response && response.redirected && partition) reset(false);
        return;
      }
      if (!response.headers || typeof response.headers.get !== 'function') return;
      if (response.headers.get('X-VDR-Suite-Home-Backend') !== backend) {
        // The ordinary online response still works with an older server.
        // It cannot be admitted to this optional private cache.
        if (partition) reset(true);
        return;
      }
      const scopes = [
        ['recordings', response.headers.get('X-VDR-Suite-Home-Recording-Scope')],
        ['history', response.headers.get('X-VDR-Suite-Home-History-Scope')]
      ].filter(entry => VALID_SCOPE.test(entry[1] || ''));
      const expected = stamp.path === RAIL_ROUTES['continue-watching'] ? 'history' : 'recordings';
      if (!scopes.length || scopes.some(entry => entry[1] !== scopes[0][1]) ||
          (!stamp.snapshot && !scopes.some(entry => entry[0] === expected)) ||
          (stamp.snapshot && Array.from(proofs.keys()).some(key => !scopes.some(entry => entry[0] === key)))) {
        if (partition) reset(true);
        return;
      }
      const next = scopes[0][1];
      if (partition && partition !== next) {
        // Account/grant drift: discard old operations rather than relabel them.
        reset(true);
        return;
      }
      partition = next;
      for (const [key, value] of scopes) proofs.set(key, {partition: value, generation, at: now(), sessionEpoch: stamp.sessionEpoch});
      const observed = {stamp, partition: next, domains: scopes.map(entry => entry[0])};
      observedHeaders.set(stamp, observed);
      if (payload && typeof payload === 'object') payloads.set(payload, observed);
      for (const ticket of tickets.values()) ticket.preview();
    }

    function begin(rail, requestedBackend, title, isCurrent, show = true, folder = null) {
      if (!RAIL_ROUTES[rail] || !backend || backend !== requestedBackend || !active() || typeof isCurrent !== 'function') return null;
      const key = folder ? 'folders:' + folder.path : rail;
      const previous = tickets.get(key);
      if (previous) previous.cancel();
      const local = generation, railVersion = version(rail);
      let done = false, dismissed = !show, accepted = 0, invalid = false;
      let proven = '', acceptedRoute = '', remove = null, expiry = null, displayed = null;
      let attemptedProof = null, attemptedCursor = '', reading = false;
      const acceptedEpochs = [];
      const live = () => !done && local === generation && railVersion === version(rail) && backend === requestedBackend && active() && isCurrent();
      function dismiss() {
        dismissed = true;
        if (remove) { remove(); remove = null; }
        displayed = null;
        if (expiry !== null && typeof global.clearTimeout === 'function') global.clearTimeout(expiry);
        expiry = null;
      }
      function cancel() {
        done = true;
        dismiss();
        if (tickets.get(key) === ticket) tickets.delete(key);
      }
      function previewCursor() {
        if (!requireCursor) return 'unversioned';
        if (awaitingInitialCursor && !currentCursor) return 'startup';
        return connected && currentCursor ? currentCursor.id : '';
      }
      async function preview() {
        if (dismissed || !live()) return;
        // A provisional rail must be removed immediately if the first
        // authoritative cursor shows an epoch/sequence mismatch.
        if (displayed) {
          if (!canPreview(rail, displayed)) dismiss();
          return;
        }
        const proof = currentProof(rail), phase = previewCursor();
        if (!proof || !phase || reading || (attemptedProof === proof && attemptedCursor === phase)) return;
        attemptedProof = proof;
        attemptedCursor = phase;
        reading = true;
        try {
          if (!await storageBarrier || !live() || dismissed) return;
          const snapshot = await owner.read(scopeOf(proof), key,
            phase === 'startup' ? STARTUP_PREVIEW_MAX_AGE_MS : undefined);
          const still = currentProof(rail);
          if (dismissed || !live() || !still || still.partition !== proof.partition || !snapshot || !snapshot.items.length || !canPreview(rail, snapshot)) return;
          const remaining = PROOF_MS - (now() - still.at);
          if (remaining <= 0 || await still.sessionEpoch !== await proof.sessionEpoch || !live() || dismissed || currentProof(rail) !== still || !canPreview(rail, snapshot)) return;
          remove = (folder ? folder.paint(snapshot.items) : paint(rail, title, snapshot.items, requestedBackend)) || null;
          if (remove) {
            displayed = snapshot;
            if (typeof global.setTimeout === 'function') expiry = global.setTimeout(dismiss, Math.min(PREVIEW_MS, remaining));
          }
        } catch (error) { /* Optional storage/presentation. */ }
        finally {
          reading = false;
          // A cursor or proof may arrive while an IndexedDB transaction
          // is in progress. Retry only on new existing evidence, never poll.
          if (!displayed && !dismissed && live() &&
              (currentProof(rail) !== attemptedProof || previewCursor() !== attemptedCursor)) preview();
        }
      }
      function accept(payload) {
        const observed = payload && typeof payload === 'object' ? payloads.get(payload) : null;
        const route = observed && observed.stamp.path;
        const matchesRoute = route === RAIL_ROUTES[rail] || (rail === 'genres' && route === PUBLIC_GENRE_ROUTE) ||
          (route === PUBLIC_RECORDING_ROUTE &&
            ((rail === 'recent-movies' && observed.stamp.view === 'movies') ||
             (rail === 'newly-recorded' && observed.stamp.view === 'new')));
        if ((folder && (!payload || payload.path !== folder.path || payload.homeCardPage !== true ||
            payload.offset !== 0 || payload.cacheReady !== true)) ||
            !live() || !observed || observed.stamp.generation !== local || !matchesRoute || (acceptedRoute && route !== acceptedRoute) ||
            !observed.domains.includes(domain(rail)) ||
            (requireCursor && (!connected || !observed.stamp.cursor || observed.stamp.versions[ALL_RAILS.indexOf(rail)] !== railVersion)) || (proven && observed.partition !== proven)) {
          invalid = true;
          return false;
        }
        proven = observed.partition;
        acceptedRoute = route;
        accepted++;
        acceptedEpochs.push(observed.stamp.sessionEpoch);
        return true;
      }
      async function finish(cards) {
        const proof = currentProof(rail);
        const eligible = live() && !invalid && accepted > 0 && proof && proof.partition === proven && Array.isArray(cards) && (!requireCursor || (connected && currentCursor));
        const scope = eligible ? scopeOf(proof) : null;
        const revision = requireCursor && currentCursor ? currentCursor.id : 'unversioned-preview-v1';
        cancel();
        if (!eligible) return Promise.resolve(false);
        // Cursor is a causal fence, not a server-side resource revision.
        const session = await scope.sessionEpoch;
        const epochs = await Promise.all(acceptedEpochs);
        if (!Number.isSafeInteger(session) || epochs.some(value => value !== session)) return false;
        if (!await storageBarrier || local !== generation || railVersion !== version(rail) ||
            !active() || (requireCursor && !connected)) return false;
        return owner.write(scope, key, {complete: true, revision, items: cards.slice(0, 24)}).catch(() => false);
      }
      const ticket = Object.freeze({preview, accept, dismiss, cancel, finish});
      tickets.set(key, ticket);
      preview();
      return ticket;
    }
    function beginFolder(path, requestedBackend, isCurrent, paintFolder) {
      if (typeof path !== 'string' || !path || path.length > 4096 ||
          /[\u0000-\u001f\u007f]/.test(path) || typeof paintFolder !== 'function') return null;
      return begin('folders', requestedBackend, '', isCurrent, true, {path, paint:paintFolder});
    }
    if (config.bindEvents !== false) {
      try {
        if (typeof global.BroadcastChannel === 'function' && global.VdrSuitePublicUrl) {
          channel = new global.BroadcastChannel('vdr-suite-home:' + global.VdrSuitePublicUrl.basePath);
          channel.onmessage = event => {
            if (event.data === 'session-boundary') { reset(false); closeCursor(); }
            else if (event.data === 'cache-invalidated') reset(false);
          };
        }
      } catch (error) { /* The next server proof remains mandatory. */ }
      if (typeof global.addEventListener === 'function') {
        global.addEventListener('offline', () => reset(false));
        global.addEventListener('pagehide', () => reset(false));
      }
      if (global.document && typeof global.document.addEventListener === 'function') {
        global.document.addEventListener('visibilitychange', () => {
          if (global.document.hidden) reset(false);
        });
      }
    }
    return Object.freeze({selectBackend, sessionBoundary, requestStamp, observe, begin, beginFolder, liveEvent, suspend, captureRailFence});
  }

  // The preview uses the same rail classes but no canonical data attributes,
  // click handlers or playback controls. Actual owners replace it on readback.
  function paintPreview(rail, title, items, backend) {
    const doc = global.document;
    if (!doc || typeof doc.querySelector !== 'function') return null;
    const host = doc.querySelector('[data-home-zone="additional-sections"]');
    if (!host || typeof host.insertBefore !== 'function') return null;
    const target = host.querySelector(rail === 'continue-watching' ? '[data-home-continue-watching]' : '[data-home-discovery-rail="' + rail + '"]');
    if (target && !target.querySelector('.media-home-discovery-state:not(.error)')) return null;
    const node = doc.createElement('section');
    node.className = 'media-home-discovery';
    node.setAttribute('data-home-pwa-preview', rail);
    node.setAttribute('aria-busy', 'true');
    const heading = doc.createElement('div');
    heading.className = 'media-home-section-heading';
    const name = doc.createElement('h3');
    name.textContent = title;
    const label = doc.createElement('small');
    label.textContent = 'Zuletzt geladen – wird aktualisiert';
    heading.append(name, label);
    node.appendChild(heading);
    const row = doc.createElement('div');
    row.className = 'media-home-discovery-rail';
    const compact = rail === 'genres' || rail === 'folders';
    for (const item of items) {
      const card = doc.createElement('div');
      card.className = 'media-home-discovery-card ' + (compact ? (rail === 'genres' ? 'genre' : 'folder') : 'recording');
      if (!compact) {
        const art = doc.createElement('div');
        art.className = 'media-home-discovery-artwork';
        const fallback = item.title.slice(0, 1).toUpperCase() || '▶';
        if (item.posterUrl) {
          const image = doc.createElement('img');
          image.alt = '';
          image.loading = 'lazy';
          image.decoding = 'async';
          const helpers = global.VdrSuiteFrontendHelpers;
          const url = helpers ? helpers.homeArtworkPreviewUrl(item.posterUrl) : item.posterUrl;
          image.src = global.VdrSuitePublicUrl.resolvePath(url);
          image.addEventListener('error', () => { image.remove(); art.textContent = fallback; });
          art.appendChild(image);
        } else art.textContent = fallback;
        card.appendChild(art);
      }
      const copy = doc.createElement('span');
      copy.className = 'media-home-discovery-copy';
      const strong = doc.createElement('strong');
      strong.textContent = item.title;
      const detail = doc.createElement('span');
      detail.textContent = item.subtitle;
      copy.append(strong, detail);
      card.appendChild(copy);
      row.appendChild(card);
    }
    node.appendChild(row);
    host.insertBefore(node, target || null);
    const wasHidden = target && target.hidden;
    if (target) target.hidden = true;
    return () => { node.remove(); if (target) target.hidden = wasHidden; };
  }

  let runtime = null;
  function get() {
    if (!runtime && global.VdrSuiteHomePersistence) runtime = createRuntime();
    return runtime;
  }
  global.VdrSuiteHomeCache = Object.freeze({
    createRuntime,
    selectBackend: backend => { const r = get(); if (r) r.selectBackend(backend); },
    sessionBoundary: () => { const r = get(); if (r) r.sessionBoundary(); },
    requestStamp: (path, backend) => { const r = get(); return r ? r.requestStamp(path, backend) : null; },
    observe: (stamp, response, payload) => { const r = get(); if (r) r.observe(stamp, response, payload); },
    liveEvent: (type, event) => { const r = get(); return r ? r.liveEvent(type, event) : false; },
    suspend: () => { const r = get(); if (r) r.suspend(); },
    begin: (...args) => { const r = get(); return r ? r.begin(...args) : null; },
    beginFolder: (...args) => { const r = get(); return r ? r.beginFolder(...args) : null; },
    captureRailFence: (...args) => { const r = get(); return r ? r.captureRailFence(...args) : null; }
  });
}(window));
