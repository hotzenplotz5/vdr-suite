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
  let epgChecking = false;
  function send(type, value) {
    parent.postMessage({ channel: channel, token: token, type: type, value: value }, origin);
  }
  function start(label, initial) {
    if (active || checking || epgChecking) return;
    probe.start(label, { buffered: Boolean(initial) });
    active = true;
    send('started', label);
  }
  function stop() {
    if (!active) return;
    active = false;
    send('result', probe.stop());
  }
  function relativeMs(value, begin) {
    const number = Number(value);
    return Number.isFinite(number) ? Math.max(0, Math.round((number - begin) * 10) / 10) : null;
  }
  function summarizeRequest(entries, begin) {
    const values = (entries || []).filter(Boolean);
    return {
      count: values.length,
      firstStartMs: values.length ? Math.min.apply(null, values.map(function (entry) { return relativeMs(entry.startTime, begin); })) : null,
      lastResponseEndMs: values.length ? Math.max.apply(null, values.map(function (entry) { return relativeMs(entry.responseEnd, begin); })) : null,
      totalDurationMs: Math.round(values.reduce(function (total, entry) { return total + (Number(entry.duration) || 0); }, 0) * 10) / 10,
      transferBytes: values.reduce(function (total, entry) { return total + (Number(entry.transferSize) || 0); }, 0),
      encodedBytes: values.reduce(function (total, entry) { return total + (Number(entry.encodedBodySize) || 0); }, 0)
    };
  }
  function homeEpgResources(begin) {
    const perf = root.performance;
    const entries = perf && typeof perf.getEntriesByType === 'function' ? perf.getEntriesByType('resource') : [];
    const channels = [];
    const epg = [];
    (entries || []).forEach(function (entry) {
      if (!entry || Number(entry.startTime) < begin || entry.initiatorType !== 'fetch') return;
      let url;
      try { url = new root.URL(entry.name, root.location.href); } catch (_) { return; }
      if (url.origin !== root.location.origin) return;
      const path = url.pathname.replace(/^\/vdr-suite(?=\/)/, '');
      if (/^\/api\/(?:vdr\/)?channels(?:\/|$)/.test(path)) channels.push(entry);
      else if (/^\/api\/epg\/cache(?:\/|$)/.test(path) && !/^\/api\/epg\/cache\/metadata\/image(?:\/|$)/.test(path)) epg.push(entry);
    });
    return { channels: summarizeRequest(channels, begin), epg: summarizeRequest(epg, begin) };
  }
  function railCardCount(kind) {
    if (!root.document || typeof root.document.querySelectorAll !== 'function') return 0;
    return root.document.querySelectorAll('[data-home-live-guide="' + kind + '"] .media-home-live-guide-card').length;
  }
  function coldEpgStartupRequested() {
    try {
      const url = new root.URL(root.location.href);
      return url.searchParams.get('vdrSuiteEpgCold') === '1' || url.hash === '#vdrSuiteEpgCold=1';
    } catch (_) { return false; }
  }
  function measureHomeEpg(mode) {
    const perf = root.performance;
    const startup = mode === 'startup';
    const ownerAtStart = root.VdrSuiteHomeLiveHero;
    if (!perf || typeof perf.now !== 'function') return Promise.reject(new Error('Performance API unavailable'));
    if (!startup && (!ownerAtStart || typeof ownerAtStart.refresh !== 'function' || typeof ownerAtStart.snapshot !== 'function')) {
      return Promise.reject(new Error('Home EPG owner unavailable'));
    }
    const begin = perf.now();
    const timeoutMs = 30000;
    const milestones = { nowRailDomMs: null, nextRailDomMs: null, railsReadyMs: null, paintReadyMs: null };
    let mutationCount = 0;
    let observer = null;
    let timer = null;
    let finished = false;
    function inspect() {
      const nowCards = railCardCount('now');
      const nextCards = railCardCount('next');
      const elapsed = relativeMs(perf.now(), begin);
      if (nowCards > 0 && milestones.nowRailDomMs === null) milestones.nowRailDomMs = elapsed;
      if (nextCards > 0 && milestones.nextRailDomMs === null) milestones.nextRailDomMs = elapsed;
      const owner = root.VdrSuiteHomeLiveHero;
      let snapshot = {};
      try { snapshot = owner && typeof owner.snapshot === 'function' ? owner.snapshot() || {} : {}; } catch (_) { snapshot = {}; }
      const ready = Boolean(owner) && snapshot.loadingChannels === false && snapshot.loadingPrograms === false && Number(snapshot.programmeLoadedChannelCount) > 0;
      if (ready && milestones.railsReadyMs === null) milestones.railsReadyMs = elapsed;
      return { ready: ready, nowCards: nowCards, nextCards: nextCards, snapshot: snapshot };
    }
    function afterPaint(callback) {
      const raf = typeof root.requestAnimationFrame === 'function' ? root.requestAnimationFrame.bind(root) : function (fn) { return root.setTimeout(fn, 0); };
      raf(function () { raf(callback); });
    }
    function cleanup() {
      if (timer !== null && typeof root.clearTimeout === 'function') root.clearTimeout(timer);
      timer = null;
      if (observer) observer.disconnect();
      observer = null;
    }
    function buildReport(timedOut, refreshError) {
      const state = inspect();
      const elapsedMs = relativeMs(perf.now(), begin);
      return {
        schema: 2,
        label: startup ? 'home-epg-cold-startup' : 'home-epg-warm-refresh',
        mode: startup ? 'cold-startup' : 'warm-refresh',
        elapsedMs: elapsedMs,
        timedOut: Boolean(timedOut),
        refreshError: refreshError ? String(refreshError.message || refreshError) : '',
        milestones: milestones,
        requests: homeEpgResources(begin),
        rails: {
          nowCards: state.nowCards,
          nextCards: state.nextCards,
          programmeLoadedChannelCount: Number(state.snapshot.programmeLoadedChannelCount) || 0,
          programmeHasMore: Boolean(state.snapshot.programmeHasMore)
        },
        mutations: mutationCount,
        limitations: [
          startup
            ? 'Request timings are Resource Timing values for same-origin Channel/EPG fetches started after the diagnostic bridge began during this Home startup.'
            : 'Request timings are Resource Timing values for same-origin Channel/EPG fetches started by this explicit warm Home refresh.',
          'Rail DOM milestones mean cards exist in the document; paintReadyMs is after two animation frames and is not a pixel-level paint measurement.',
          'A cold-startup diagnostic reloads the Home document but does not clear the browser HTTP cache.',
          'No URLs, channel IDs, event IDs, titles or credentials are exported.'
        ]
      };
    }
    return new Promise(function (resolve) {
      function finish(timedOut, refreshError) {
        if (finished) return;
        const state = inspect();
        if (!timedOut && !refreshError && (!state.ready || state.nowCards === 0 || state.nextCards === 0)) return;
        finished = true;
        cleanup();
        if (timedOut || refreshError) {
          resolve(buildReport(timedOut, refreshError));
          return;
        }
        afterPaint(function () {
          milestones.paintReadyMs = relativeMs(perf.now(), begin);
          resolve(buildReport(false, null));
        });
      }
      if (typeof root.MutationObserver === 'function' && root.document && root.document.documentElement) {
        try {
          observer = new root.MutationObserver(function (records) {
            mutationCount += records.length;
            inspect();
            finish(false, null);
          });
          observer.observe(root.document.documentElement, { childList: true, subtree: true });
        } catch (_) { observer = null; }
      }
      timer = root.setTimeout(function () { finish(true, null); }, timeoutMs);
      if (startup) {
        inspect();
        finish(false, null);
        return;
      }
      Promise.resolve().then(function () { return ownerAtStart.refresh(); }).then(function () {
        inspect();
        finish(false, null);
      }, function (error) {
        finish(false, error || new Error('Home EPG refresh failed'));
      });
    });
  }
  root.addEventListener('message', function (event) {
    const data = event.data;
    if (event.source !== parent || event.origin !== origin || !data || data.channel !== channel || data.token !== token) return;
    try {
      if (data.type === 'start') start(data.label || 'navigation', false);
      else if (data.type === 'stop') stop();
      else if (data.type === 'epg' && !epgChecking) {
        if (active || checking) throw new Error('Messung zuerst stoppen.');
        epgChecking = true;
        measureHomeEpg('refresh').then(function (result) {
          send('epg-result', result);
        }, function () {
          send('error', 'EPG-Home-Messung konnte nicht gestartet werden.');
        }).then(function () { epgChecking = false; });
      }
      else if (data.type === 'headers' && !checking) {
        if (active || epgChecking) throw new Error('Messung zuerst stoppen.');
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
  if (coldEpgStartupRequested()) {
    epgChecking = true;
    measureHomeEpg('startup').then(function (result) {
      send('epg-result', result);
    }, function () {
      send('error', 'EPG-Kaltstart-Messung konnte nicht gestartet werden.');
    }).then(function () { epgChecking = false; });
  } else {
    start('initial-home', true);
  }
})(typeof window !== 'undefined' ? window : globalThis);
