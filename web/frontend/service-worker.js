'use strict';

// PWA.1/PWA.3: public fallback shell and server-revalidated Home previews.
// HTML, composed runtime bundles, API/SSE and writes remain network-owned.
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

// PWA.3: persistent previews are reused only after a fresh, scoped server 304.
const artworkPrefix = 'vdr-suite-pwa-artwork:' + encodeURIComponent(scope.href) + ':';
const artworkName = artworkPrefix + 'v2';
const artworkLimit = 192;
const artworkBytes = 256 * 1024;
let artworkGeneration = 0;
let artworkSessionMutations = 0;
let artworkWrites = Promise.resolve();
const artworkTag = /^"vsr-[0-9a-f]{64}"$/;
const artworkScope = /^vsr-[0-9a-f]{64}$/;
const artworkTypes = new Set(['image/jpeg', 'image/png', 'image/webp']);
const artworkRoutes = new Set(['/api/vdr/recordings/metadata/image', '/api/recordings/metadata/image', '/api/epg/cache/artwork', '/api/epg/cache/metadata/image']);
const artworkParams = new Set(['backend', 'backendNativeId', 'recordingId', 'kind', 'index', 'assignmentRevision', 'channelId', 'eventId', 'variant']);
const installationPath = scope.pathname.slice(0, -'/frontend/'.length);

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
  return serializeArtwork(async () => { await caches.delete(artworkName); });
}

async function artworkCandidates(base) {
  try {
    const cache = await caches.open(artworkName);
    const keys = await cache.keys(base, {ignoreSearch: true});
    const candidates = [];
    for (const key of keys.slice(-4)) {
      const response = await cache.match(key);
      if (!response) continue;
      const tag = response.headers.get('ETag');
      const partition = response.headers.get('X-VDR-Suite-Artwork-Scope');
      const size = Number(response.headers.get('X-VDR-Suite-Artwork-Bytes'));
      const type = response.headers.get('Content-Type');
      if (response.status === 200 && artworkTypes.has(type) && Number.isSafeInteger(size) &&
          size > 0 && size <= artworkBytes && artworkTag.test(tag || '') && artworkScope.test(partition || '') &&
          new URL(key.url).searchParams.get('scope') === partition) candidates.push({response, tag, partition, size, type});
    }
    return candidates;
  } catch (error) { return []; }
}

function artworkFetch(request, tags) {
  const headers = new Headers(request.headers);
  headers.delete('If-None-Match');
  headers.delete('If-Modified-Since');
  if (tags.length) headers.set('If-None-Match', tags.join(', '));
  return fetch(new Request(request, {headers, mode: 'same-origin', cache: 'no-store'}));
}

function storeArtwork(base, response, generation) {
  const tag = response.headers.get('ETag');
  const partition = response.headers.get('X-VDR-Suite-Artwork-Scope');
  const size = Number(response.headers.get('X-VDR-Suite-Artwork-Bytes'));
  const type = response.headers.get('Content-Type');
  if (response.status !== 200 || response.redirected || !artworkTag.test(tag || '') || !artworkScope.test(partition || '') || !artworkTypes.has(type) || !Number.isSafeInteger(size) || size <= 0 || size > artworkBytes || /no-store/i.test(response.headers.get('Cache-Control') || '')) return Promise.resolve();
  const copy = response.clone();
  return serializeArtwork(async () => {
    if (generation !== artworkGeneration || artworkSessionMutations) return;
    const bytes = await copy.arrayBuffer();
    if (bytes.byteLength !== size || generation !== artworkGeneration) return;
    const cache = await caches.open(artworkName);
    const key = base + '?scope=' + partition;
    // Only the image and explicit non-secret metadata are stored; never request headers.
    await cache.delete(key);
    await cache.put(key, new Response(bytes, {headers: {'Content-Type': type, ETag: tag, 'X-VDR-Suite-Artwork-Scope': partition, 'X-VDR-Suite-Artwork-Bytes': String(size), 'Cache-Control': 'private, no-cache', 'X-VDR-Suite-Artwork-Cache': 'revalidated'}}));
    const keys = await cache.keys();
    for (const old of keys.slice(0, Math.max(0, keys.length - artworkLimit))) await cache.delete(old);
  });
}

async function handleArtwork(event) {
  const request = event.request;
  if (artworkSessionMutations) return artworkFetch(request, []);
  const generation = artworkGeneration;
  const base = artworkBase(request);
  const candidates = await artworkCandidates(base);
  let response = await artworkFetch(request, generation === artworkGeneration && !artworkSessionMutations ? candidates.map(c => c.tag) : []);
  if (generation !== artworkGeneration || artworkSessionMutations) return artworkFetch(request, []);
  if (response.status === 304) {
    const match = candidates.find(c => c.tag === response.headers.get('ETag') &&
      c.partition === response.headers.get('X-VDR-Suite-Artwork-Scope') &&
      c.size === Number(response.headers.get('X-VDR-Suite-Artwork-Bytes')) &&
      (!response.headers.get('Content-Type') || c.type === response.headers.get('Content-Type')));
    if (match) return match.response;
    response = await artworkFetch(request, []);
  }
  if (generation !== artworkGeneration || artworkSessionMutations) return response;
  if (response.status === 401) event.waitUntil(clearArtwork());
  else if (response.status === 403 || response.status === 404 || response.status === 410) event.waitUntil(serializeArtwork(async () => {
    if (generation !== artworkGeneration) return;
    const cache = await caches.open(artworkName);
    for (const key of await cache.keys(base, {ignoreSearch: true})) await cache.delete(key);
  }));
  else if (response.status === 200) event.waitUntil(storeArtwork(base, response, generation));
  return response;
}

self.addEventListener('message', event => {
  if (event.data && event.data.type === 'vdr-suite-artwork-clear') event.waitUntil(clearArtwork());
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
  if (request.method !== 'GET' || request.mode !== 'navigate') return;
  const url = new URL(request.url);
  if (url.origin !== scope.origin || url.search ||
      (url.pathname !== scope.pathname && url.pathname !== scope.pathname + 'index.html')) return;
  // Preserve redirects, HTTP errors and all current server authorization decisions.
  // Only a rejected navigation fetch receives the account-neutral offline shell.
  event.respondWith(fetch(request).catch(offlineShell));
});
