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
  let controllerAtStart = null;
  let controllerChanges = 0;
  let listeningToController = null;
  function serviceWorker() {
    return root.navigator && root.navigator.serviceWorker || null;
  }
  function onControllerChange() {
    if (active) controllerChanges++;
  }
  function stopControllerTracking() {
    if (listeningToController && typeof listeningToController.removeEventListener === 'function')
      listeningToController.removeEventListener('controllerchange', onControllerChange);
    listeningToController = null;
  }
  function send(type, value) {
    parent.postMessage({ channel: channel, token: token, type: type, value: value }, origin);
  }
  function start(label, initial) {
    if (active || checking) return;
    probe.start(label, { buffered: Boolean(initial) });
    const sw = serviceWorker();
    controllerAtStart = sw ? Boolean(sw.controller) : null;
    controllerChanges = 0;
    if (sw && typeof sw.addEventListener === 'function') {
      sw.addEventListener('controllerchange', onControllerChange);
      listeningToController = sw;
    }
    active = true;
    send('started', label);
  }
  function readArtworkMeasurement() {
    const worker = root.navigator && root.navigator.serviceWorker && root.navigator.serviceWorker.controller;
    if (!worker || typeof root.MessageChannel !== 'function') return null;
    return new Promise(function (resolve) {
      const channel = new root.MessageChannel();
      let timer;
      function finish(value) {
        if (timer !== undefined) root.clearTimeout(timer);
        channel.port1.close();
        resolve(value);
      }
      channel.port1.onmessage = function (event) { finish(event.data || null); };
      timer = root.setTimeout(function () { finish(null); }, 900);
      try { worker.postMessage({type:'vdr-suite-artwork-measurement'}, [channel.port2]); }
      catch (_) { finish(null); }
    });
  }
  // Console output belongs only to this opt-in diagnostic iframe.
  // One aggregate object per completed capture, never per-image logging.
  function logArtworkSummary(report) {
    if (!root.console || typeof root.console.info !== 'function') return;
    const metrics = report.metrics || {};
    const worker = metrics.artworkWorker && metrics.artworkWorker.counters || {};
    const rails = report.railArtworkPrefetch || {};
    const rows = metrics.resourceDiagnostics && metrics.resourceDiagnostics.rows || [];
    const phase = function (name) {
      const samples = Number(worker[name + 'Samples']) || 0;
      return {samples: samples, meanMs: samples
        ? Math.round(Number(worker[name + 'TotalMs']) / samples * 10) / 10 : null,
      maxMs: samples ? Number(worker[name + 'MaxMs']) : null};
    };
    root.console.info('[VDR-Suite Artwork] Diagnose (nur nach Stop)', {
      homeFrameMs: report.startupTiming && report.startupTiming.homeFrameMs || null,
      scrollReadyPercent: rails.scrollPrefetchHitRatePercent ?? null,
      afterEntryMeanMs: rails.meanReadyDelayMs ?? null,
      afterEntryMaxMs: rails.delayMaxMs ?? null,
      image404: rows.filter(row => row.initiator === 'img' && row.status === '404')
        .reduce((total, row) => total + (Number(row.count) || 0), 0),
      missingProofBeforeRead: worker.missingProofBeforeAcceptedRead ?? null,
      localCacheHits: worker.localHits ?? null,
      cacheMisses: worker.cacheMiss ?? null,
      parallelImageHttpPeak: worker.maxImageHttpInFlight ?? null,
      firstProofGap: phase('firstProofGap'),
      proofHttp: phase('proofHttp'),
      cacheLookup: phase('cacheLookup'),
      candidateLookup: phase('candidateLookup'),
      foregroundHttp: phase('foregroundHttp'),
      foregroundPriorityUnknown: worker.foregroundPriorityUnknown ?? null,
      snapshotProofReads: worker.snapshotProofReads ?? null,
      snapshotProofAccepted: worker.snapshotProofAccepted ?? null,
      otherProofReads: worker.otherProofReads ?? null,
      otherProofAccepted: worker.otherProofAccepted ?? null,
      proofReadsAtFirstImage: worker.proofReadsAtFirstImage ?? null,
      acceptedProofsAtFirstImage: worker.acceptedProofsAtFirstImage ?? null,
      firstImageWorkerUptimeMs: worker.firstImageWorkerUptimeMs ?? null,
      firstProofWorkerUptimeMs: worker.firstProofWorkerUptimeMs ?? null,
      firstAcceptedProofWorkerUptimeMs: worker.firstAcceptedProofWorkerUptimeMs ?? null,
      workerUptimeMs: (metrics.artworkWorker && metrics.artworkWorker.workerUptimeMs) ?? null,
      controllerLifecycle: report.serviceWorkerLifecycle || null
    });
  }
  function stop(includeCache) {
    if (!active) return;
    active = false;
    const report = probe.stop();
    const sw = serviceWorker();
    report.serviceWorkerLifecycle = {
      controlledAtStart: controllerAtStart,
      controlledAtStop: sw ? Boolean(sw.controller) : null,
      controllerChanges: controllerChanges
    };
    stopControllerTracking();
    const measurement = includeCache !== false && readArtworkMeasurement();
    if (!measurement) {
      report.metrics.artworkWorker = {available:false};
      logArtworkSummary(report);
      send('result', report);
      return;
    }
    checking = true;
    measurement.then(function (value) {
      report.metrics.artworkWorker = value && value.schema === 1
        ? {available:true, ...value, interval:'controlled document since worker start; cumulative'}
        : {available:false};
      logArtworkSummary(report);
      send('result', report);
    }).finally(function () { checking = false; });
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
  root.addEventListener('pagehide', function () { if (active) stop(false); });
  start('initial-home', true);
})(typeof window !== 'undefined' ? window : globalThis);
