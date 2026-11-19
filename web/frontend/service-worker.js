'use strict';

// PWA.1: only a generated, public fallback shell is persistent. The normal
// HTML, composed runtime bundles, API/SSE, artwork and writes stay network-owned.
const scope = new URL(self.registration.scope);
const cachePrefix = 'vdr-suite-pwa-shell:' + encodeURIComponent(scope.href) + ':';
const cacheName = cachePrefix + 'v1';
const shellKey = new URL('pwa-offline-shell', scope).href;

function publicShell() {
  return new Response(`<!doctype html><html lang="de"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>VDR-Suite – Offline</title>
<style>body{font:1rem system-ui,sans-serif;background:#101827;color:#f1f5f9;margin:12vh auto;padding:1.5rem;max-width:32rem}a{color:#7dd3fc}</style>
<h1>VDR-Suite</h1><p>Keine Verbindung zum Server. / Server unavailable.</p>
<p>Geschützte Inhalte benötigen eine aktuelle Anmeldung. / Protected content requires a current session.</p>
<p><a href="./">Erneut verbinden / Reconnect</a></p></html>`, {
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
        if (name.startsWith(cachePrefix) && name !== cacheName) await caches.delete(name);
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

self.addEventListener('fetch', event => {
  const request = event.request;
  if (request.method !== 'GET' || request.mode !== 'navigate') return;
  const url = new URL(request.url);
  if (url.origin !== scope.origin || url.search ||
      (url.pathname !== scope.pathname && url.pathname !== scope.pathname + 'index.html')) return;
  // Preserve redirects, HTTP errors and all current server authorization decisions.
  // Only a rejected navigation fetch receives the account-neutral offline shell.
  event.respondWith(fetch(request).catch(offlineShell));
});
