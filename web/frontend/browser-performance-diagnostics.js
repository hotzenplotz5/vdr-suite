/* Browser-only controller for the opt-in diagnostic page. */
(function (root) {
  'use strict';
  const doc = root.document;
  const frame = doc.getElementById('home');
  const status = doc.getElementById('status');
  const metrics = doc.getElementById('metrics');
  const history = doc.getElementById('history');
  const headerResult = doc.getElementById('headers-result');
  const epgResult = doc.getElementById('epg-result');
  const controls = ['cold', 'start', 'stop', 'epg-cold', 'epg', 'headers', 'export'].reduce(function (items, name) { items[name] = doc.getElementById(name); return items; }, {});
  const channel = 'vdr-suite-browser-diagnostics';
  const captures = [];
  const epgCaptures = [];
  let latestHeaders = null;
  let active = false;
  let ready = false;
  let loading = false;
  let checking = false;
  let epgChecking = false;
  let token = '';
  let generation = 0;
  let coldReloadPending = false;
  let coldWatchdog = null;
  function text(message) { status.textContent = message; }
  function buttons() {
    controls.cold.disabled = loading || checking || epgChecking;
    controls.start.disabled = loading || checking || epgChecking || !ready || active;
    controls.stop.disabled = loading || checking || epgChecking || !ready || !active;
    if (controls['epg-cold']) controls['epg-cold'].disabled = loading || checking || epgChecking || active;
    if (controls.epg) controls.epg.disabled = loading || checking || epgChecking || !ready || active;
    controls.headers.disabled = loading || checking || epgChecking || !ready || active;
    controls.export.disabled = !captures.length && !epgCaptures.length && !latestHeaders;
  }
  function clearColdWatchdog() {
    if (coldWatchdog !== null && typeof root.clearTimeout === 'function') root.clearTimeout(coldWatchdog);
    coldWatchdog = null;
  }
  function armColdWatchdog() {
    clearColdWatchdog();
    if (typeof root.setTimeout !== 'function') return;
    coldWatchdog = root.setTimeout(function () {
      coldWatchdog = null;
      if (!epgChecking) return;
      coldReloadPending = false;
      epgChecking = false;
      loading = false;
      ready = false;
      text('EPG-Kaltstart-Messung nach 35 Sekunden ohne Ergebnis abgebrochen.');
      buttons();
    }, 35000);
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
    else if (data.type === 'result') { active = false; captures.push(data.value); render(data.value); text('Messung abgeschlossen.'); }
    else if (data.type === 'epg-result') {
      clearColdWatchdog();
      coldReloadPending = false;
      epgChecking = false;
      ready = true;
      loading = false;
      epgCaptures.push(data.value);
      if (epgResult) epgResult.textContent = JSON.stringify(data.value, null, 2);
      text(data.value && data.value.mode === 'cold-startup' ? 'EPG-Kaltstart-Messung abgeschlossen.' : 'EPG-Warmmessung abgeschlossen.');
    }
    else if (data.type === 'headers') { checking = false; latestHeaders = data.value; headerResult.textContent = JSON.stringify(data.value, null, 2); text('Header-Prüfung abgeschlossen.'); }
    else if (data.type === 'error') {
      clearColdWatchdog();
      coldReloadPending = false;
      checking = false;
      epgChecking = false;
      loading = false;
      text('Diagnosefehler: ' + data.value);
    }
    buttons();
  });
  function load(mode) {
    const coldEpg = mode === 'epg-cold';
    if (loading || checking || epgChecking) return;
    clearColdWatchdog();
    coldReloadPending = false;
    loading = true; ready = false; active = false; checking = false; epgChecking = coldEpg;
    generation += 1;
    token = String(Math.random()).slice(2) + String(Date.now()) + String(generation);
    buttons();
    text(coldEpg ? 'Home wird für die EPG-Kaltstart-Messung neu geladen …' : 'Home wird mit aktivierter Messung geladen …');
    frame.setAttribute('data-vdr-suite-diagnostics', token);
    if (coldEpg) {
      coldReloadPending = true;
      armColdWatchdog();
      frame.src = 'about:blank';
      return;
    }
    frame.src = 'browser-performance-home.html';
  }
  frame.addEventListener('load', function () {
    let location = null;
    try { location = frame.contentWindow && frame.contentWindow.location ? frame.contentWindow.location : null; } catch (_) { location = null; }
    if (coldReloadPending && location && location.href === 'about:blank') {
      coldReloadPending = false;
      frame.src = 'browser-performance-home.html#vdrSuiteEpgCold=1';
      return;
    }
    if (location && location.pathname.endsWith('/browser-performance-home.html')) {
      loading = false;
      buttons();
    }
  });
  controls.cold.addEventListener('click', function () { load('general'); });
  controls.start.addEventListener('click', function () { command('start', 'navigation'); });
  controls.stop.addEventListener('click', function () { command('stop'); });
  if (controls['epg-cold']) controls['epg-cold'].addEventListener('click', function () { load('epg-cold'); });
  if (controls.epg) controls.epg.addEventListener('click', function () { epgChecking = true; buttons(); text('EPG-Home wird warm neu projiziert und gemessen …'); command('epg'); });
  controls.headers.addEventListener('click', function () { checking = true; buttons(); text('Prüfe ein sichtbares Cover …'); command('headers'); });
  controls.export.addEventListener('click', function () {
    const payload = JSON.stringify({ schema: 2, generatedAt: new Date().toISOString(), captures: captures, epgMeasurements: epgCaptures, headers: latestHeaders, limitations: ['Resource Timing may omit cross-origin sizes and HTTP status.', 'Zero transfer does not prove a cache hit.', 'Long tasks and mutations before observer initialization may be missing.', 'Reload does not clear the browser cache.'] }, null, 2);
    const url = root.URL.createObjectURL(new root.Blob([payload], { type: 'application/json' }));
    const anchor = doc.createElement('a');
    anchor.href = url; anchor.download = 'vdr-suite-browser-diagnostics.json'; anchor.click();
    root.setTimeout(function () { root.URL.revokeObjectURL(url); }, 0);
  });
  buttons();
  load('general');
})(typeof window !== 'undefined' ? window : globalThis);
