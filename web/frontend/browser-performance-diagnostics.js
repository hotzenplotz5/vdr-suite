/* Browser-only controller for the opt-in diagnostic page. */
(function (root) {
  'use strict';
  const doc = root.document;
  const frame = doc.getElementById('home');
  const status = doc.getElementById('status');
  const metrics = doc.getElementById('metrics');
  const history = doc.getElementById('history');
  const headerResult = doc.getElementById('headers-result');
  const controls = ['cold', 'start', 'stop', 'headers', 'export'].reduce(function (items, name) { items[name] = doc.getElementById(name); return items; }, {});
  const token = String(Math.random()).slice(2) + String(Date.now());
  const channel = 'vdr-suite-browser-diagnostics';
  const captures = [];
  let latestHeaders = null;
  let active = false;
  let ready = false;
  let loading = false;
  function text(message) { status.textContent = message; }
  function buttons() {
    controls.cold.disabled = loading;
    controls.start.disabled = loading || !ready || active;
    controls.stop.disabled = loading || !ready || !active;
    controls.headers.disabled = loading || !ready || active;
    controls.export.disabled = !captures.length && !latestHeaders;
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
    if (event.source !== frame.contentWindow || event.origin !== root.location.origin || !data || data.channel !== channel || data.token !== token) return;
    if (data.type === 'started') { active = true; ready = true; loading = false; text('Messung läuft: ' + data.value); }
    else if (data.type === 'result') { active = false; captures.push(data.value); render(data.value); text('Messung abgeschlossen.'); }
    else if (data.type === 'headers') { latestHeaders = data.value; headerResult.textContent = JSON.stringify(data.value, null, 2); text('Header-Prüfung abgeschlossen.'); }
    else if (data.type === 'error') text('Diagnosefehler: ' + data.value);
    buttons();
  });
  function load() {
    loading = true; ready = false; active = false; buttons();
    text('Home wird mit aktivierter Messung geladen …');
    frame.setAttribute('data-vdr-suite-diagnostics', token);
    frame.src = './?vdr-suite-diagnostics=1';
  }
  frame.addEventListener('load', function () { loading = false; buttons(); });
  controls.cold.addEventListener('click', load);
  controls.start.addEventListener('click', function () { command('start', 'navigation'); });
  controls.stop.addEventListener('click', function () { command('stop'); });
  controls.headers.addEventListener('click', function () { text('Prüfe ein sichtbares Cover …'); command('headers'); });
  controls.export.addEventListener('click', function () {
    const payload = JSON.stringify({ schema: 1, generatedAt: new Date().toISOString(), captures: captures, headers: latestHeaders, limitations: ['Resource Timing may omit cross-origin sizes and HTTP status.', 'Zero transfer does not prove a cache hit.', 'Long tasks before observer initialization may be missing.'] }, null, 2);
    const url = root.URL.createObjectURL(new root.Blob([payload], { type: 'application/json' }));
    const anchor = doc.createElement('a');
    anchor.href = url; anchor.download = 'vdr-suite-browser-diagnostics.json'; anchor.click();
    root.setTimeout(function () { root.URL.revokeObjectURL(url); }, 0);
  });
  buttons();
  load();
})(typeof window !== 'undefined' ? window : globalThis);
