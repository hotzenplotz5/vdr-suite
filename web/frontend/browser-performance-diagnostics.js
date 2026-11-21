/* Browser-only controller for the opt-in diagnostic page. */
(function (root) {
  'use strict';
  const doc = root.document;
  const frame = doc.getElementById('home');
  const status = doc.getElementById('status');
  const metrics = doc.getElementById('metrics');
  const history = doc.getElementById('history');
  const headerResult = doc.getElementById('headers-result');
  const controls = ['cold', 'clean', 'start', 'stop', 'headers', 'export'].reduce(function (items, name) { items[name] = doc.getElementById(name); return items; }, {});
  const channel = 'vdr-suite-browser-diagnostics';
  const captures = [];
  let latestHeaders = null;
  let active = false;
  let ready = false;
  let loading = false;
  let checking = false;
  let token = '';
  let generation = 0;
  let cachePreparation = {pwaArtwork: 'not-cleared', httpCache: 'not-cleared'};
  function text(message) { status.textContent = message; }
  function buttons() {
    controls.cold.disabled = loading || checking;
    controls.clean.disabled = loading || checking;
    controls.start.disabled = loading || checking || !ready || active;
    controls.stop.disabled = loading || checking || !ready || !active;
    controls.headers.disabled = loading || checking || !ready || active;
    controls.export.disabled = !captures.length && !latestHeaders;
  }
  // The worker already owns session-fenced artwork deletion. Wait for its
  // acknowledgment before opening the measured Home document; never delete
  // browser HTTP cache, other origins, recordings or unrelated Cache Storage.
  function clearPwaArtwork() {
    const service = root.navigator && root.navigator.serviceWorker;
    if (!service || typeof root.MessageChannel !== 'function') return Promise.resolve('unavailable');
    return new Promise(function (resolve) {
      const channel = new root.MessageChannel();
      let finished = false, cancelActivationWatch = null;
      function finish(status) {
        if (finished) return;
        finished = true;
        if (cancelActivationWatch) cancelActivationWatch();
        root.clearTimeout(timer);
        channel.port1.close();
        resolve(status);
      }
      function send(worker) {
        if (finished) return;
        if (!worker) { finish('unavailable'); return; }
        try { worker.postMessage({type:'vdr-suite-artwork-clear'}, [channel.port2]); }
        catch (_) { finish('failed'); }
      }
      function useRegistration(registration) {
        if (finished) return;
        // skipWaiting does not claim already-open tabs. Their controller can
        // be the old worker, which clears the cache but has no ACK protocol.
        // Prefer the current registration's active worker after any update.
        const next = registration && (registration.installing || registration.waiting);
        if (next && next !== registration.active &&
            next.state !== 'activated' && next.state !== 'redundant' &&
            typeof next.addEventListener === 'function') {
          const changed = function () {
            if (next.state !== 'activated' && next.state !== 'redundant') return;
            if (cancelActivationWatch) cancelActivationWatch();
            send(registration.active || service.controller);
          };
          next.addEventListener('statechange', changed);
          cancelActivationWatch = function () {
            next.removeEventListener('statechange', changed);
            cancelActivationWatch = null;
          };
          changed();
          return;
        }
        send(registration && registration.active || service.controller);
      }
      channel.port1.onmessage = function (event) {
        const value = event.data;
        finish(value && value.schema === 1 && value.cleared === true ? 'cleared' : 'failed');
      };
      // Image writes may still be draining. This is a diagnostic timeout,
      // not a deadline or cancellation of the Service Worker clear itself.
      const timer = root.setTimeout(function () { finish('timeout'); }, 15000);
      if (typeof service.getRegistration !== 'function') {
        send(service.controller);
        return;
      }
      Promise.resolve().then(function () {
        return service.getRegistration(root.location.href);
      }).then(function (registration) {
        if (!registration || typeof registration.update !== 'function') return registration;
        return Promise.resolve(registration.update()).then(function () {
          return registration;
        }, function () {
          return registration;
        });
      }).then(useRegistration).catch(function () {
        send(service.controller);
      });
    });
  }

  function command(type, label) {
    if (!ready || !frame.contentWindow) return;
    frame.contentWindow.postMessage({ channel: channel, token: token, type: type, label: label }, root.location.origin);
  }
  function render(report) {
    metrics.replaceChildren();
    const table = doc.createElement('table');
    const body = doc.createElement('tbody');
    Object.keys(report.metrics).forEach(function (key) {
      const row = doc.createElement('tr');
      const name = doc.createElement('td');
      const value = doc.createElement('td');
      name.textContent = key;
      value.textContent = JSON.stringify(report.metrics[key]);
      row.append(name, value);
      body.append(row);
    });
    table.append(body);
    metrics.append(table);
    history.textContent = JSON.stringify(captures, null, 2);
  }
  root.addEventListener('message', function (event) {
    const data = event.data;
    if (event.source !== frame.contentWindow || event.origin !== root.location.origin || !data || data.channel !== channel || data.token !== token || data.generation !== undefined && data.generation !== generation) return;
    if (data.type === 'started') { active = true; ready = true; loading = false; text('Messung läuft: ' + data.value); }
    else if (data.type === 'result') {
      active = false;
      data.value.cachePreparation = {...cachePreparation};
      captures.push(data.value);
      render(data.value);
      text('Messung abgeschlossen (' + cachePreparation.pwaArtwork + ').');
    }
    else if (data.type === 'headers') { checking = false; latestHeaders = data.value; headerResult.textContent = JSON.stringify(data.value, null, 2); text('Header-Prüfung abgeschlossen.'); }
    else if (data.type === 'error') { checking = false; text('Diagnosefehler: ' + data.value); }
    buttons();
  });
  function load(clearArtwork) {
    if (loading || checking) return;
    loading = true; ready = false; active = false; checking = false;
    generation += 1;
    const currentGeneration = generation;
    token = String(Math.random()).slice(2) + String(Date.now()) + String(generation);
    cachePreparation = {pwaArtwork: clearArtwork ? 'pending' : 'not-cleared',
      httpCache: 'not-cleared'};
    buttons();
    function openMeasuredHome() {
      if (currentGeneration !== generation) return;
      text('Home wird mit aktivierter Messung geladen …');
      frame.setAttribute('data-vdr-suite-diagnostics', token);
      frame.src = 'browser-performance-home.html';
    }
    if (!clearArtwork) { openMeasuredHome(); return; }
    // Detach the previous measured document so it cannot start new image
    // requests while the PWA artwork cache is being cleared.
    frame.src = 'about:blank';
    text('PWA-Artwork-Cache wird vor der Messung geleert …');
    clearPwaArtwork().then(function (result) {
      if (currentGeneration !== generation) return;
      cachePreparation.pwaArtwork = result;
      openMeasuredHome();
    });
  }
  frame.addEventListener('load', function () {
    if (frame.contentWindow && frame.contentWindow.location && frame.contentWindow.location.pathname.endsWith('/browser-performance-home.html')) {
      loading = false;
      buttons();
    }
  });
  controls.cold.addEventListener('click', function () { load(false); });
  controls.clean.addEventListener('click', function () { load(true); });
  controls.start.addEventListener('click', function () {
    cachePreparation = {pwaArtwork:'not-cleared', httpCache:'not-cleared'};
    command('start', 'navigation');
  });
  controls.stop.addEventListener('click', function () { command('stop'); });
  controls.headers.addEventListener('click', function () { checking = true; buttons(); text('Prüfe ein sichtbares Cover …'); command('headers'); });
  controls.export.addEventListener('click', function () {
    const payload = JSON.stringify({ schema: 1, generatedAt: new Date().toISOString(), captures: captures, headers: latestHeaders, limitations: ['Resource Timing may omit cross-origin sizes and HTTP status.', 'Zero transfer does not prove a cache hit.', 'Long tasks and mutations before observer initialization may be missing.', 'PWA artwork clear only affects the installation-scoped Service Worker Cache Storage; browser HTTP and memory caches remain unchanged.', 'A failed or unavailable clear is exported as such, not called a cold cache.'] }, null, 2);
    const url = root.URL.createObjectURL(new root.Blob([payload], { type: 'application/json' }));
    const anchor = doc.createElement('a');
    anchor.href = url; anchor.download = 'vdr-suite-browser-diagnostics.json'; anchor.click();
    root.setTimeout(function () { root.URL.revokeObjectURL(url); }, 0);
  });
  buttons();
  load(false);
})(typeof window !== 'undefined' ? window : globalThis);
