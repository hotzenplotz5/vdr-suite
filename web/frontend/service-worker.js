'use strict';

// PWA.1/PWA.3: public fallback shell and server-revalidated Home previews.
// The installer may embed a verified public static shell; private API/SSE and writes stay network-owned.
const scope = new URL(self.registration.scope);
const cachePrefix = 'vdr-suite-pwa-shell:' + encodeURIComponent(scope.href) + ':';
const cacheName = cachePrefix + 'v2';
const shellKey = new URL('pwa-offline-shell', scope).href;

function publicShell() {
  return new Response(`<!doctype html><html lang="de"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>VDR-Suite – Offline</title>
<style>body{font:1rem system-ui,sans-serif;background:#101827;color:#f1f5f9;margin:12vh auto;padding:1.5rem;max-width:32rem}a{color:#7dd3fc}</style>
<h1>VDR-Suite</h1><p>Keine Verbindung zum Server. / Server unavailable.</p>
<p>Geschützte Inhalte benötigen eine aktuelle Anmeldung. / Protected content requires a current session.</p>
<p><a href="./">Erneut verbinden / Reconnect</a></p>
<script>window.addEventListener("online",function(){window.location.replace("./");},{once:true});</script></html>`, {
    headers: { 'Content-Type': 'text/html; charset=utf-8', 'Cache-Control': 'no-store' }
  });
}

self.addEventListener('install', event => {
  event.waitUntil((async () => {
    try { await (await caches.open(cacheName)).put(shellKey, publicShell()); }
    catch (error) { /* Storage is optional; the same public shell can be generated. */ }
  })());
  // No skipWaiting: existing tabs keep their current worker until normal activation.
});

self.addEventListener('activate', event => {
  event.waitUntil((async () => {
    try {
      for (const name of await caches.keys()) {
        if ((name.startsWith(cachePrefix) && name !== cacheName) ||
            (name.startsWith(artworkPrefix) && name !== artworkName)) await caches.delete(name);
      }
    } catch (error) { /* Never block network access on storage failure. */ }
  })());
  // No clients.claim and no reload of a running application or playback.
});

async function offlineShell() {
  try {
    const cached = await (await caches.open(cacheName)).match(shellKey);
    if (cached) return cached;
  } catch (error) { /* Quota, disabled storage or eviction. */ }
  return publicShell();
}

// PWA.3/PWA.11: scoped recording previews can use fresh server authorization
// from an existing Home read; other artwork retains per-image revalidation.
const artworkPrefix = 'vdr-suite-pwa-artwork:' + encodeURIComponent(scope.href) + ':';
const artworkName = artworkPrefix + 'v2';
const artworkLimit = 1024;
const artworkBytes = 256 * 1024;
let artworkGeneration = 0;
let artworkSessionMutations = 0;
let artworkWrites = Promise.resolve();
let artworkWriteKeys = null;
const artworkTag = /^"vsr-[0-9a-f]{64}"$/;
const artworkScope = /^vsr-[0-9a-f]{64}$/;
const artworkTypes = new Set(['image/jpeg', 'image/png', 'image/webp']);
const artworkRoutes = new Set(['/api/vdr/recordings/metadata/image', '/api/recordings/metadata/image', '/api/epg/cache/artwork', '/api/epg/cache/metadata/image']);
const artworkParams = new Set(['backend', 'backendNativeId', 'recordingId', 'kind', 'index', 'assignmentRevision', 'channelId', 'eventId', 'variant']);
const installationPath = scope.pathname.slice(0, -'/frontend/'.length);
const recordingArtworkRoutes = new Set(['/api/vdr/recordings/metadata/image', '/api/recordings/metadata/image']);
const recordingProofRoutes = new Set(['/api/vdr/recordings/query', '/api/vdr/recordings/folder', '/api/metadata/genres', '/api/metadata/genres/recordings']);
// Match the existing Home authorization proof lifetime. This is not a new
// permission, login lease, or UI gate; a miss simply uses the original path.
const artworkProofMs = 5000;
const artworkFreshMs = 5 * 60 * 1000;
const artworkStaleMs = 24 * 60 * 60 * 1000;
const artworkProofs = new Map();
let artworkProofEpoch = 0, artworkProofSequence = 0;
const artworkRefreshing = new Set();

// Diagnostic counters are deliberately per controlled document and in memory.
// No recording identifiers, URL, account, scope or credential is exported.
const artworkMeasurements = new Map();
function artworkMeasurement(event) {
  if (!event.clientId) return null;
  let value = artworkMeasurements.get(event.clientId);
  if (!value) {
    value = {requests: 0, localHits: 0, revalidatedHits: 0, imageHttpRequests: 0,
      serverImages: 0, serverOther: 0, networkErrors: 0, missingProof: 0,
      expiredProof: 0, otherArtworkRoute: 0, cacheMiss: 0, outdatedEntry: 0,
      backgroundRefreshes: 0, writes: 0, evictions: 0, storageWriteErrors: 0,
      proofReads: 0, acceptedProofs: 0};
    artworkMeasurements.set(event.clientId, value);
    while (artworkMeasurements.size > 128) artworkMeasurements.delete(artworkMeasurements.keys().next().value);
  }
  return value;
}
function countArtwork(event, field) {
  const measurement = artworkMeasurement(event);
  if (measurement) measurement[field]++;
}

function forgetArtworkProofs() {
  artworkProofEpoch++;
  artworkProofs.clear();
}

function proofBackend(request) {
  if (request.method !== 'GET' || request.destination !== '' ||
      !request.headers || !['same-origin', 'include'].includes(request.credentials) ||
      request.headers.has('Authorization') || request.headers.has('Range')) return '';
  const url = new URL(request.url);
  if (url.origin !== scope.origin || !url.pathname.startsWith(installationPath + '/api/')) return '';
  const path = url.pathname.slice(installationPath.length);
  const snapshot = /^\/api\/backends\/([A-Za-z0-9_.-]{1,128})\/snapshot$/.exec(path);
  if (snapshot && !url.search) return snapshot[1];
  if (!recordingProofRoutes.has(path) ||
      (path === '/api/metadata/genres' && url.searchParams.get('scope') !== 'recordings')) return '';
  const backend = url.searchParams.get('backend') || '';
  const ids = url.searchParams.getAll('backendId');
  return /^[A-Za-z0-9_.-]{1,128}$/.test(backend) &&
    url.searchParams.getAll('backend').length === 1 &&
    (ids.length === 0 || (ids.length === 1 && ids[0] === backend)) &&
    !url.search.slice(1).split('&').some(part => part.split('=')[0].includes('%')) ? backend : '';
}

function proofKey(clientId, backend) { return JSON.stringify([clientId, backend]); }

async function observeArtworkProof(event, backend) {
  const epoch = artworkProofEpoch, sequence = ++artworkProofSequence;
  const key = proofKey(event.clientId, backend);
  const started = Date.now();
  countArtwork(event, 'proofReads');
  let response;
  try { response = await fetch(new Request(event.request, {cache: 'no-store'})); }
  catch (error) { forgetArtworkProofs(); throw error; }
  if (epoch !== artworkProofEpoch || artworkSessionMutations) return response;
  const prior = artworkProofs.get(key);
  if (prior && prior.sequence > sequence) return response;
  const partition = response.headers.get('X-VDR-Suite-Home-Recording-Scope');
  if (response.status !== 200 || response.redirected ||
      response.headers.get('X-VDR-Suite-Home-Backend') !== backend ||
      !artworkScope.test(partition || '') ||
      !/no-store/i.test(response.headers.get('Cache-Control') || '')) {
    forgetArtworkProofs();
    if (response.status === 401 || response.status === 403) event.waitUntil(clearArtwork());
    return response;
  }
  if (prior && prior.partition !== partition) forgetArtworkProofs();
  artworkProofs.delete(key);
  artworkProofs.set(key, {partition, sequence, started});
  countArtwork(event, 'acceptedProofs');
  while (artworkProofs.size > 128) artworkProofs.delete(artworkProofs.keys().next().value);
  return response;
}

function currentArtworkProof(event) {
  if (!event.clientId || artworkSessionMutations || (self.navigator && self.navigator.onLine === false)) return null;
  const url = new URL(event.request.url);
  if (!recordingArtworkRoutes.has(url.pathname.slice(installationPath.length))) return null;
  const proof = artworkProofs.get(proofKey(event.clientId, url.searchParams.get('backend')));
  if (!proof) return null;
  const age = Date.now() - proof.started;
  return age >= 0 && age <= artworkProofMs ? proof : null;
}

function artworkRequest(request) {
  if (request.method !== 'GET' || request.destination !== 'image' || request.credentials === 'omit' || request.headers.has('Range') || request.headers.has('Authorization')) return false;
  const url = new URL(request.url);
  if (url.origin !== scope.origin || !artworkRoutes.has(url.pathname.slice(installationPath.length)) || !url.pathname.startsWith(installationPath + '/api/')) return false;
  if (url.searchParams.get('variant') !== 'home' || !url.searchParams.get('backend') || url.href.length > 4096) return false;
  const seen = new Set();
  for (const key of url.searchParams.keys()) {
    if (!artworkParams.has(key) || seen.has(key)) return false;
    seen.add(key);
  }
  return true;
}

function artworkBase(request) {
  const url = new URL(request.url);
  return new URL('pwa-artwork/' + encodeURIComponent(url.pathname + url.search), scope).href;
}

function serializeArtwork(action) {
  const result = artworkWrites.then(action).catch(() => {});
  artworkWrites = result;
  return result;
}

function clearArtwork() {
  artworkGeneration++;
  forgetArtworkProofs();
  return serializeArtwork(async () => {
    artworkWriteKeys = null;
    await caches.delete(artworkName);
  });
}

function validatedArtwork(response, expectedPartition) {
  if (!response) return null;
  const tag = response.headers.get('ETag');
  const partition = response.headers.get('X-VDR-Suite-Artwork-Scope');
  const size = Number(response.headers.get('X-VDR-Suite-Artwork-Bytes'));
  const type = response.headers.get('Content-Type');
  const validatedAt = Number(response.headers.get('X-VDR-Suite-Artwork-Validated-At') || 0);
  if (response.status !== 200 || !artworkTypes.has(type) || !Number.isSafeInteger(size) ||
      size <= 0 || size > artworkBytes || !artworkTag.test(tag || '') || !artworkScope.test(partition || '') ||
      expectedPartition !== partition) return null;
  return {response, tag, partition, size, type, validatedAt};
}

async function scopedArtwork(base, partition) {
  try {
    const cache = await caches.open(artworkName);
    return validatedArtwork(await cache.match(base + '?scope=' + partition), partition);
  } catch (error) { return null; }
}

async function artworkCandidates(base) {
  try {
    const cache = await caches.open(artworkName);
    const keys = await cache.keys(base, {ignoreSearch: true});
    const candidates = [];
    for (const key of keys.slice(-4)) {
      const response = await cache.match(key);
      if (!response) continue;
      const candidate = validatedArtwork(response, new URL(key.url).searchParams.get('scope'));
      if (candidate) candidates.push(candidate);
    }
    return candidates;
  } catch (error) { return []; }
}

function artworkFetch(event, tags) {
  const request = event.request;
  countArtwork(event, 'imageHttpRequests');
  const headers = new Headers(request.headers);
  headers.delete('If-None-Match');
  headers.delete('If-Modified-Since');
  if (tags.length) headers.set('If-None-Match', tags.join(', '));
  return fetch(new Request(request, {headers, mode: 'same-origin', cache: 'no-store'}));
}

function storeArtwork(base, response, generation, event) {
  const tag = response.headers.get('ETag');
  const partition = response.headers.get('X-VDR-Suite-Artwork-Scope');
  const size = Number(response.headers.get('X-VDR-Suite-Artwork-Bytes'));
  const type = response.headers.get('Content-Type');
  if (response.status !== 200 || response.redirected || !artworkTag.test(tag || '') || !artworkScope.test(partition || '') || !artworkTypes.has(type) || !Number.isSafeInteger(size) || size <= 0 || size > artworkBytes || /no-store/i.test(response.headers.get('Cache-Control') || '')) return Promise.resolve();
  const copy = response.clone();
  const validatedAt = Date.now();
  return serializeArtwork(async () => {
    if (generation !== artworkGeneration || artworkSessionMutations) return;
    const bytes = await copy.arrayBuffer();
    if (bytes.byteLength !== size || generation !== artworkGeneration) return;
    try {
      const cache = await caches.open(artworkName);
      const key = base + '?scope=' + partition;
      // Reconstruct insertion order once per worker/storage failure, not
      // once for every poster. All mutations use the existing serial queue.
      if (!artworkWriteKeys) artworkWriteKeys = new Set((await cache.keys()).map(entry => entry.url));
      await cache.delete(key);
      artworkWriteKeys.delete(key);
      // Make room before writing; replacing an existing key does not evict
      // an unrelated poster. Quota failure remains an optional-cache miss.
      while (artworkWriteKeys.size >= artworkLimit) {
        const oldest = artworkWriteKeys.values().next().value;
        const removed = await cache.delete(oldest);
        artworkWriteKeys.delete(oldest);
        if (removed) countArtwork(event, 'evictions');
      }
      // Only the image and explicit non-secret metadata are stored; never request headers.
      await cache.put(key, new Response(bytes, {headers: {'Content-Type': type, ETag: tag, 'X-VDR-Suite-Artwork-Scope': partition, 'X-VDR-Suite-Artwork-Bytes': String(size), 'Cache-Control': 'private, no-cache', 'X-VDR-Suite-Artwork-Cache': 'revalidated', 'X-VDR-Suite-Artwork-Validated-At': String(validatedAt)}}));
      artworkWriteKeys.add(key);
      countArtwork(event, 'writes');
    } catch (error) {
      artworkWriteKeys = null;
      countArtwork(event, 'storageWriteErrors');
    }
  });
}

async function handleArtwork(event) {
  countArtwork(event, 'requests');
  const generation = artworkGeneration;
  const proof = currentArtworkProof(event);
  if (!proof) {
    const url = new URL(event.request.url);
    if (!recordingArtworkRoutes.has(url.pathname.slice(installationPath.length))) countArtwork(event, 'otherArtworkRoute');
    else {
      const previous = artworkProofs.get(proofKey(event.clientId, url.searchParams.get('backend')));
      const age = previous && Date.now() - previous.started;
      countArtwork(event, previous && (age < 0 || age > artworkProofMs) ? 'expiredProof' : 'missingProof');
    }
  }
  if (proof) {
    const base = artworkBase(event.request);
    const candidate = await scopedArtwork(base, proof.partition);
    if (!candidate) countArtwork(event, 'cacheMiss');
    // Recheck after asynchronous storage access; logout or grant drift may
    // have happened while the cache was being read.
    const current = currentArtworkProof(event);
    if (candidate && generation === artworkGeneration && current && current.partition === proof.partition) {
      const age = Date.now() - candidate.validatedAt;
      if (candidate.validatedAt > 0 && age >= 0 && age <= artworkStaleMs) {
        if (age > artworkFreshMs) {
          const key = base + '?scope=' + proof.partition;
          // A stale image can paint now; keep network refresh optional,
          // deduplicated and bounded, never a barrier to visible covers.
          if (artworkRefreshing.size < 4 && !artworkRefreshing.has(key)) {
            artworkRefreshing.add(key);
            countArtwork(event, 'backgroundRefreshes');
            // Independently read the cached candidate for background validation:
            // its body must never share the foreground response's readable stream.
            const refresh = revalidateArtwork(event).then(response => {
              if (response.body) response.body.cancel().catch(() => {});
            }).catch(() => {}).finally(() => artworkRefreshing.delete(key));
            event.waitUntil(refresh);
          }
        }
        const headers = new Headers(candidate.response.headers);
        headers.set('X-VDR-Suite-Artwork-Cache', 'local');
        countArtwork(event, 'localHits');
        return new Response(candidate.response.body, {headers});
      }
      countArtwork(event, 'outdatedEntry');
    }
  }
  return revalidateArtwork(event);
}

async function revalidateArtwork(event) {
  const request = event.request;
  if (artworkSessionMutations) return artworkFetch(event, []);
  const generation = artworkGeneration;
  const base = artworkBase(request);
  const candidates = await artworkCandidates(base);
  let response;
  try {
    response = await artworkFetch(event, generation === artworkGeneration && !artworkSessionMutations ? candidates.map(c => c.tag) : []);
  } catch (error) { forgetArtworkProofs(); countArtwork(event, 'networkErrors'); throw error; }
  if (generation !== artworkGeneration || artworkSessionMutations) return artworkFetch(event, []);
  if (response.status === 304) {
    const match = candidates.find(c => c.tag === response.headers.get('ETag') &&
      c.partition === response.headers.get('X-VDR-Suite-Artwork-Scope') &&
      c.size === Number(response.headers.get('X-VDR-Suite-Artwork-Bytes')) &&
      (!response.headers.get('Content-Type') || c.type === response.headers.get('Content-Type')));
    if (match) {
      countArtwork(event, 'revalidatedHits');
      const age = Date.now() - match.validatedAt;
      if (!match.validatedAt || age < 0 || age > artworkFreshMs) event.waitUntil(storeArtwork(base, match.response, generation, event));
      return match.response;
    }
    response = await artworkFetch(event, []);
  }
  if (generation !== artworkGeneration || artworkSessionMutations) return response;
  countArtwork(event, response.status === 200 ? 'serverImages' : 'serverOther');
  if (response.status === 401) event.waitUntil(clearArtwork());
  else if (response.status === 403 || response.status === 404 || response.status === 410) {
    if (response.status === 403) forgetArtworkProofs();
    event.waitUntil(serializeArtwork(async () => {
    if (generation !== artworkGeneration) return;
    const cache = await caches.open(artworkName);
    for (const key of await cache.keys(base, {ignoreSearch: true})) {
      await cache.delete(key);
      if (artworkWriteKeys) artworkWriteKeys.delete(key.url);
    }
    }));
  }
  else if (response.status === 200) event.waitUntil(storeArtwork(base, response, generation, event));
  return response;
}

self.addEventListener('message', event => {
  if (event.data && event.data.type === 'vdr-suite-artwork-clear') event.waitUntil(clearArtwork());
  if (event.data && event.data.type === 'vdr-suite-artwork-proof-reset') forgetArtworkProofs();
  if (event.data && event.data.type === 'vdr-suite-artwork-measurement' &&
      event.source && event.source.id && event.ports && event.ports[0]) {
    const measurement = artworkMeasurements.get(event.source.id);
    event.ports[0].postMessage({schema: 1, capacity: artworkLimit, maxPreviewBytes: artworkBytes,
      counters: measurement ? {...measurement} : null});
  }
});

function artworkSessionMutation(request) {
  if (request.method !== 'POST') return false;
  const url = new URL(request.url);
  return url.origin === scope.origin && (url.pathname === installationPath + '/api/security/browser-sessions/logout' || url.pathname === installationPath + '/api/security/browser-sessions' ||
    url.pathname === installationPath + '/api/security/first-admin/claim');
}

async function handleArtworkSessionMutation(request) {
  // Fence both ends, including overlapping or failed session mutations;
  // neither an old 304 nor a delayed image write may survive the boundary.
  artworkSessionMutations++;
  const clearing = clearArtwork();
  try { return await fetch(request); }
  finally {
    await clearing;
    await clearArtwork();
    artworkSessionMutations--;
  }
}

self.addEventListener('fetch', event => {
  const request = event.request;
  if (artworkRequest(request)) { event.respondWith(handleArtwork(event)); return; }
  if (artworkSessionMutation(request)) { event.respondWith(handleArtworkSessionMutation(request)); return; }
  const backend = event.clientId && proofBackend(request);
  if (backend) { event.respondWith(observeArtworkProof(event, backend)); return; }
  if (typeof handleLocalShell === 'function' && handleLocalShell(event)) return;
  if (request.method !== 'GET' || request.mode !== 'navigate') return;
  const url = new URL(request.url);
  if (url.origin !== scope.origin || url.search ||
      (url.pathname !== scope.pathname && url.pathname !== scope.pathname + 'index.html')) return;
  // Preserve redirects, HTTP errors and all current server authorization decisions.
  // Only a rejected navigation fetch receives the account-neutral offline shell.
  event.respondWith(fetch(request).catch(offlineShell));
});
