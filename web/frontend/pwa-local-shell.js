// Embedded by the runtime installer together with its exact served-asset manifest.
// Only anonymously fetched, byte-verified public shell resources are persisted.
const staticPrefix = 'vdr-suite-pwa-static:' + encodeURIComponent(scope.href) + ':';
const staticName = staticPrefix + localShellRelease.id;
const staticReady = new URL('pwa-static-ready', scope).href;
const staticAssets = new Map(Object.entries(localShellRelease.assets).map(([name, spec]) =>
  [new URL(name, scope).href, spec]));
const staticIndex = new URL('index.html', scope).href;
const canonicalShell = url => url === scope.href || url === staticIndex;
let staticCachePromise;

async function precacheLocalShell() {
  try {
    if (!staticAssets.has(staticIndex)) return;
    const cache = await caches.open(staticName);
    // A failed or interrupted install must never advertise a partial release.
    await cache.delete(staticReady);
    const entries = [...staticAssets];
    let next = 0;
    async function load() {
      while (next < entries.length) {
        const [url, spec] = entries[next++];
        const response = await fetch(new Request(url, {credentials: 'omit', cache: 'no-store', redirect: 'error'}));
        if (response.status !== 200 || response.redirected ||
            (response.headers.get('Content-Type') || '').split(';')[0].trim().toLowerCase() !== spec.type ||
            /(?:private|no-store)/i.test(response.headers.get('Cache-Control') || '') ||
            /(?:cookie|authorization|\*)/i.test(response.headers.get('Vary') || '')) throw Error('public shell unavailable');
        const bytes = await response.clone().arrayBuffer();
        const digest = new Uint8Array(await crypto.subtle.digest('SHA-256', bytes));
        const hex = Array.from(digest, byte => byte.toString(16).padStart(2, '0')).join('');
        if (hex !== spec.sha256) throw Error('shell release changed');
        await cache.put(url, response);
      }
    }
    const results = await Promise.allSettled(Array.from({length: 4}, load));
    if (results.some(result => result.status !== 'fulfilled')) throw Error('incomplete shell release');
    await cache.put(staticReady, new Response(localShellRelease.id));
  } catch (error) {
    try { await caches.delete(staticName); } catch (_) { /* Storage is optional. */ }
  }
}

self.addEventListener('install', event => event.waitUntil(precacheLocalShell()));
self.addEventListener('activate', event => event.waitUntil((async () => {
  try {
    for (const name of await caches.keys()) {
      if (name.startsWith(staticPrefix) && name !== staticName) await caches.delete(name);
    }
  } catch (_) { /* Keep ordinary network operation. */ }
})()));

function completeStaticCache() {
  if (!staticCachePromise) staticCachePromise = (async () => {
    try {
      const cache = await caches.open(staticName);
      const keys = new Set((await cache.keys()).map(key => key.url));
      if (keys.has(staticReady) && [...staticAssets.keys()].every(url => keys.has(url))) return cache;
    } catch (_) { /* Storage unavailable. */ }
    return null;
  })();
  return staticCachePromise;
}

async function localShellResponse(event, navigation) {
  const request = event.request;
  try {
    // Diagnostics, legacy routes and other controlled documents remain network-owned.
    const client = navigation ? null : await self.clients.get(event.clientId);
    if (navigation || (client && canonicalShell(client.url))) {
      const cache = await completeStaticCache();
      if (cache) {
        // Only one verified representation exists; Accept-Encoding may differ.
        const response = await cache.match(navigation ? staticIndex : request.url, {ignoreVary: true});
        if (response) return response;
        staticCachePromise = Promise.resolve(null);
      }
    }
  } catch (_) { staticCachePromise = Promise.resolve(null); }
  return navigation ? fetch(request).catch(offlineShell) : fetch(request);
}

function handleLocalShell(event) {
  const request = event.request;
  if (request.method !== 'GET') return false;
  const navigation = request.mode === 'navigate' && canonicalShell(request.url);
  const asset = staticAssets.has(request.url) && request.url !== staticIndex &&
    ['script', 'style', 'worker'].includes(request.destination);
  if (!navigation && !asset) return false;
  event.respondWith(localShellResponse(event, navigation));
  return true;
}
