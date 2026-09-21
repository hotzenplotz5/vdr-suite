/* Opt-in diagnostics bridge, loaded before the ordinary frontend bootstrap. */
(function (root) {
  'use strict';
  const frame = root.frameElement;
  if (!frame || !frame.hasAttribute('data-vdr-suite-diagnostics')) return;
  const parent = root.parent;
  const probe = root.VdrSuiteArtworkProbe;
  const token = frame.getAttribute('data-vdr-suite-diagnostics');
  const origin = root.location.origin;
  const channel = 'vdr-suite-browser-diagnostics';
  if (!probe || !token) return;
  let active = false;
  let checking = false;
  function send(type, value) {
    parent.postMessage({ channel: channel, token: token, type: type, value: value }, origin);
  }
  function start(label, initial) {
    if (active || checking) return;
    probe.start(label, { buffered: Boolean(initial) });
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
    if (event.source !== parent || event.origin !== origin || !data || data.channel !== channel || data.token !== token) return;
    try {
      if (data.type === 'start') start(data.label || 'navigation', false);
      else if (data.type === 'stop') stop();
      else if (data.type === 'headers' && !checking) {
        if (active) throw new Error('Messung zuerst stoppen.');
        checking = true;
        Promise.resolve().then(function () { return probe.inspectHeaders(); }).then(function (result) {
          send('headers', result);
        }, function () {
          send('error', 'Cover-Header konnten nicht geprüft werden.');
        }).then(function () { checking = false; });
      }
    } catch (_) { send('error', 'Diagnoseaktion fehlgeschlagen.'); }
  });
  root.addEventListener('pagehide', function () { if (active) stop(); });
  start('initial-home', true);
})(typeof window !== 'undefined' ? window : globalThis);
