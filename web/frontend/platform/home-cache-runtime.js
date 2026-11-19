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
  const VALID_SCOPE = /^vsr-[0-9a-f]{64}$/;
  const PROOF_MS = 5000;
  const PREVIEW_MS = 15000;
  const domain = rail => rail === 'continue-watching' ? 'history' : 'recordings';

  function createRuntime(options) {
    const config = options || {};
    const now = config.now || Date.now;
    const active = config.active || (() => {
      const session = global.VdrSuiteBrowserSession;
      return Boolean(session && session.isAuthenticated() && (!global.navigator || global.navigator.onLine !== false));
    });
    const paint = config.paint || paintPreview;
    let backend = '';
    let generation = 0;
    let partition = '';
    const proofs = new Map();
    const payloads = new WeakMap();
    const tickets = new Map();
    let channel = null;
    const makeOwner = config.makeOwner || (opts => global.VdrSuiteHomePersistence.create(opts));
    const owner = makeOwner({authorize: async (scope, rail) => {
      const proof = currentProof(rail);
      return proof && scope.backendId === backend && scope.accountId === proof.partition &&
        scope.permissionRevision === proof.partition ? {...scope, authorized: true} : null;
    }});

    function currentProof(rail) {
      const proof = proofs.get(domain(rail));
      return active() && proof && proof.generation === generation && now() - proof.at <= PROOF_MS ? proof : null;
    }
    function scopeOf(proof) {
      return {accountId: proof.partition, backendId: backend, permissionRevision: proof.partition};
    }
    function reset(purge) {
      generation++;
      partition = '';
      proofs.clear();
      for (const ticket of tickets.values()) ticket.cancel();
      tickets.clear();
      owner.invalidate();
      if (purge) Promise.resolve(owner.clear()).catch(() => {});
    }
    function selectBackend(value) {
      reset(false);
      backend = typeof value === 'string' ? value : '';
    }
    function sessionBoundary() {
      reset(true);
      if (channel) { try { channel.postMessage('session-boundary'); } catch (error) {} }
    }
    function requestStamp(path, requestedBackend) {
      if (!backend || !active() || backend !== requestedBackend) return null;
      const snapshot = path === '/api/backends/' + encodeURIComponent(backend) + '/snapshot';
      if (!snapshot && !Object.values(RAIL_ROUTES).includes(path)) return null;
      return {path, backend, generation, snapshot};
    }
    function observe(stamp, response, payload) {
      if (!stamp || stamp.generation !== generation || stamp.backend !== backend || !active()) return;
      if (!response || !response.ok) {
        if (response && (response.status === 401 || response.status === 403)) reset(true);
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
      for (const [key, value] of scopes) proofs.set(key, {partition: value, generation, at: now()});
      if (payload && typeof payload === 'object') {
        payloads.set(payload, {stamp, partition: next, domains: scopes.map(entry => entry[0])});
      }
      for (const ticket of tickets.values()) ticket.preview();
    }

    function begin(rail, requestedBackend, title, isCurrent, show = true) {
      if (!RAIL_ROUTES[rail] || !backend || backend !== requestedBackend || !active() || typeof isCurrent !== 'function') return null;
      const previous = tickets.get(rail);
      if (previous) previous.cancel();
      const local = generation;
      let done = false, dismissed = !show, accepted = 0, invalid = false;
      let proven = '', attempted = '', remove = null, expiry = null;
      const live = () => !done && local === generation && backend === requestedBackend && active() && isCurrent();
      function dismiss() {
        dismissed = true;
        if (remove) { remove(); remove = null; }
        if (expiry !== null && typeof global.clearTimeout === 'function') global.clearTimeout(expiry);
        expiry = null;
      }
      function cancel() {
        done = true;
        dismiss();
        if (tickets.get(rail) === ticket) tickets.delete(rail);
      }
      async function preview() {
        if (dismissed || !live()) return;
        const proof = currentProof(rail);
        if (!proof || attempted === proof.partition) return;
        attempted = proof.partition;
        try {
          const snapshot = await owner.read(scopeOf(proof), rail);
          const still = currentProof(rail);
          if (dismissed || !live() || !still || still.partition !== proof.partition || !snapshot || !snapshot.items.length) return;
          remove = paint(rail, title, snapshot.items, requestedBackend) || null;
          if (remove && typeof global.setTimeout === 'function') expiry = global.setTimeout(dismiss, PREVIEW_MS);
        } catch (error) { /* Optional storage/presentation. */ }
      }
      function accept(payload) {
        const observed = payload && typeof payload === 'object' ? payloads.get(payload) : null;
        if (!live() || !observed || observed.stamp.generation !== local || observed.stamp.path !== RAIL_ROUTES[rail] ||
            !observed.domains.includes(domain(rail)) || (proven && observed.partition !== proven)) {
          invalid = true;
          return false;
        }
        proven = observed.partition;
        accepted++;
        return true;
      }
      function finish(cards) {
        const proof = currentProof(rail);
        const eligible = live() && !invalid && accepted > 0 && proof && proof.partition === proven && Array.isArray(cards);
        const scope = eligible ? scopeOf(proof) : null;
        cancel();
        if (!eligible) return Promise.resolve(false);
        // These legacy reads do not expose a shared server revision. This is
        // a bounded complete preview window, never an authoritative inventory.
        return owner.write(scope, rail, {complete: true, revision: 'unversioned-preview-v1', items: cards.slice(0, 24)}).catch(() => false);
      }
      const ticket = Object.freeze({preview, accept, dismiss, cancel, finish});
      tickets.set(rail, ticket);
      preview();
      return ticket;
    }
    if (config.bindEvents !== false) {
      try {
        if (typeof global.BroadcastChannel === 'function' && global.VdrSuitePublicUrl) {
          channel = new global.BroadcastChannel('vdr-suite-home:' + global.VdrSuitePublicUrl.basePath);
          channel.onmessage = event => { if (event.data === 'session-boundary') reset(false); };
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
    return Object.freeze({selectBackend, sessionBoundary, requestStamp, observe, begin});
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
    label.textContent = 'Lokale Vorschau – wird aktualisiert';
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
    begin: (...args) => { const r = get(); return r ? r.begin(...args) : null; }
  });
}(window));
