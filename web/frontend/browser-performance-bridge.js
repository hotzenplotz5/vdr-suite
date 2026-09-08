/* Opt-in diagnostics bridge. Loaded before the ordinary frontend bootstrap. */
(function (root) {
  'use strict';
  if (!root.frameElement || !root.frameElement.hasAttribute('data-vdr-suite-diagnostics')) return;
  const parent = root.parent;
  const probe = root.VdrSuiteArtworkProbe;
  if (!probe) return;
  const token = root.frameElement.getAttribute('data-vdr-suite-diagnostics');
  const origin = root.location.origin;
  let active = false;
  function send(type, value) {
    parent.postMessage({ channel: 'vdr-suite-browser-diagnostics', token: token, type: type, value: value }, origin);
  }
  function start(label) {
    if (active) return;
    probe.start(label);
    active = true;
    send('started', label);
  }
  function stop() {
    if (!active) return;
    active = false;
    send('result', probe.stop());
  }
  root.addEventListener('message', function (event) {
    const data = event.data;
    if (event.source !== parent || event.origin !== origin || !data || data.channel !== 'vdr-suite-browser-diagnostics' || data.token !== token) return;
    try {
      if (data.type === 'start') start(data.label || 'navigation');
      else if (data.type === 'stop') stop();
      else if (data.type === 'headers') probe.inspectHeaders().then(function (result) { send('headers', result); }, function (error) { send('error', String(error.message || error)); });
    } catch (error) { send('error', String(error.message || error)); }
  });
  root.addEventListener('pagehide', function () { if (active) stop(); });
  start('initial-home');
})(typeof window !== 'undefined' ? window : globalThis);
