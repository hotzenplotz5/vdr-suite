/* Read-only, opt-in browser artwork measurement. No URLs or credentials in reports. */
(function (root) {
  'use strict';
  function summarizeResources(entries, baseUrl) {
    const rows = new Map();
    const bounds = [0, 16384, 65536, 262144, 1048576, Infinity];
    const labels = ['zero', '1-16KiB', '16-64KiB', '64-256KiB', '256KiB-1MiB', 'over-1MiB'];
    const knownStatuses = [200, 204, 206, 301, 302, 304, 400, 401, 403, 404, 410, 429, 500, 502, 503, 504];
    const initiators = ['img', 'css', 'fetch', 'xmlhttprequest', 'link', 'script'];
    const base = new URL(baseUrl || 'https://probe.invalid/');
    function category(name) {
      let url;
      try { url = new URL(name, base); } catch (_) { return 'other'; }
      if (url.origin !== base.origin) return 'cross-origin';
      const path = url.pathname.replace(/^\/vdr-suite(?=\/)/, '');
      if (/^\/api\/(?:vdr\/)?recordings\/metadata\/image(?:\/|$)/.test(path)) return 'recording-metadata-image';
      if (/^\/api\/epg\/cache\/metadata\/image(?:\/|$)/.test(path)) return 'epg-metadata-image';
      if (/^\/channel-logos(?:\/|$)/.test(path)) return 'channel-logo';
      if (/^\/recording-artwork(?:\/|$)/.test(path)) return 'recording-artwork';
      if (/^\/api\/epg\/cache(?:\/|$)/.test(path)) return 'epg-cache-api';
      if (/^\/api\/(?:vdr\/)?recordings(?:\/|$)/.test(path)) return 'recordings-api';
      if (/^\/api\/(?:vdr\/)?channels(?:\/|$)/.test(path)) return 'channels-api';
      if (/^\/api\//.test(path)) return 'other-api';
      if (/^\/frontend\//.test(path)) return 'frontend-static';
      if (/\.(?:png|jpe?g|webp|svg|gif|avif)$/i.test(path)) return 'other-image';
      return 'other';
    }
    function bytes(value) {
      const n = Number(value);
      return Number.isFinite(n) && n > 0 ? n : 0;
    }
    (entries || []).forEach(function (entry) {
      const route = category(entry.name);
      const code = Number(entry.responseStatus);
      const status = Number.isInteger(code) && code >= 100 && code <= 599
        ? (knownStatuses.includes(code) ? String(code) : String(Math.floor(code / 100)) + 'xx')
        : 'unavailable';
      const initiator = initiators.includes(entry.initiatorType) ? entry.initiatorType : 'other';
      const key = JSON.stringify([route, status, initiator]);
      let row = rows.get(key);
      if (!row) {
        row = { category: route, status: status, initiator: initiator, count: 0, transferBytes: 0, encodedBytes: 0, decodedBytes: 0, sizeDistribution: Object.fromEntries(labels.map(function (label) { return [label, 0]; })) };
        rows.set(key, row);
      }
      row.count++;
      row.transferBytes += bytes(entry.transferSize);
      row.encodedBytes += bytes(entry.encodedBodySize);
      row.decodedBytes += bytes(entry.decodedBodySize);
      const size = bytes(entry.encodedBodySize);
      const index = size === 0 ? 0 : bounds.findIndex(function (bound, i) { return i > 0 && size <= bound; });
      row.sizeDistribution[labels[index < 0 ? labels.length - 1 : index]]++;
    });
    return {
      scope: 'all-observed-resources',
      observedEntries: (entries || []).length,
      rows: Array.from(rows.values()).sort(function (a, b) { return b.transferBytes - a.transferBytes || a.category.localeCompare(b.category) || a.status.localeCompare(b.status) || a.initiator.localeCompare(b.initiator); }),
      limitations: 'Resource Timing only; not a complete network log. Missing or unavailable statuses do not establish success or failure. Encoded bytes are HTTP body bytes, not decoded image pixels. Cross-origin sizes may be unavailable. No request URLs, IDs or query parameters are exported.'
    };
  }
  function summarize(entries, images, mutations, tasks, baseUrl) {
    const counts = new Map();
    (images || []).forEach(function (image) {
      if (image.url) counts.set(image.url, (counts.get(image.url) || 0) + 1);
    });
    const resources = (entries || []).filter(function (entry) {
      return entry.initiatorType === 'img' || counts.has(entry.name);
    });
    const sum = function (field) {
      return resources.reduce(function (total, entry) { return total + (Number(entry[field]) || 0); }, 0);
    };
    return {
      imageElements: (images || []).length,
      uniqueImageUrls: counts.size,
      duplicateImageElements: Array.from(counts.values()).reduce(function (n, count) { return n + Math.max(0, count - 1); }, 0),
      resourceRequests: resources.length,
      repeatedResourceRequests: resources.length - new Set(resources.map(function (entry) { return entry.name; })).size,
      transferBytes: sum('transferSize'),
      encodedBytes: sum('encodedBodySize'),
      decodedBytes: sum('decodedBodySize'),
      zeroTransferEntries: resources.filter(function (entry) { return entry.transferSize === 0; }).length,
      responseStatusCounts: resources.reduce(function (result, entry) {
        const status = String(entry.responseStatus || 'unavailable');
        result[status] = (result[status] || 0) + 1;
        return result;
      }, {}),
      mutations: mutations || 0,
      longTasks: (tasks || []).length,
      longTaskMs: (tasks || []).reduce(function (n, task) { return n + task.duration; }, 0),
      resourceDiagnostics: summarizeResources(entries, baseUrl)
    };
  }
  function createProbe(env) {
    const win = env || root;
    const doc = win.document;
    const perf = win.performance;
    let active = null;
    const captures = [];
    function images() {
      return Array.from(doc.querySelectorAll('img')).map(function (image) {
        return { url: image.currentSrc || image.src || '', complete: image.complete, width: image.naturalWidth, loading: image.loading };
      });
    }
    function railScroll() {
      return Array.from(doc.querySelectorAll('.media-home-discovery-rail')).map(function (rail) { return rail.scrollLeft; });
    }
    function start(label, options) {
      if (active) throw new Error('Finish the current capture first');
      const buffered = Boolean(options && options.buffered);
      const observers = [];
      const resources = [];
      const tasks = [];
      let mutations = 0;
      const begin = perf.now();
      const supported = { resource: false, longtask: false, mutation: false };
      function observe(callback, type, config) {
        const Constructor = type === 'mutation' ? win.MutationObserver : win.PerformanceObserver;
        if (typeof Constructor !== 'function') return;
        try {
          const observer = new Constructor(callback);
          if (type === 'mutation') observer.observe(doc.documentElement || doc, config);
          else observer.observe(config);
          observers.push({ observer: observer, type: type });
          supported[type] = true;
        } catch (_) { /* unsupported entry type */ }
      }
      observe(function (list) { resources.push.apply(resources, list.getEntries()); }, 'resource', { type: 'resource', buffered: buffered });
      observe(function (list) { tasks.push.apply(tasks, list.getEntries()); }, 'longtask', { type: 'longtask', buffered: buffered });
      observe(function (records) { mutations += records.length; }, 'mutation', { childList: true, attributes: true, characterData: true, subtree: true });
      active = { label: String(label || 'capture'), begin: begin, buffered: buffered, supported: supported, observers: observers, resources: resources, tasks: tasks, getMutations: function () { return mutations; }, addMutations: function (count) { mutations += count; }, initialScroll: railScroll() };
      return 'Capture started';
    }
    function stop() {
      if (!active) throw new Error('No active capture');
      const session = active;
      active = null;
      session.observers.forEach(function (entry) {
        const observer = entry.observer;
        try {
          const pending = typeof observer.takeRecords === 'function' ? observer.takeRecords() : [];
          if (entry.type === 'mutation') session.addMutations(pending.length);
          else if (entry.type === 'resource') session.resources.push.apply(session.resources, pending.filter(function (item) { return item.entryType === 'resource'; }));
          else if (entry.type === 'longtask') session.tasks.push.apply(session.tasks, pending.filter(function (item) { return item.entryType === 'longtask'; }));
        } finally { observer.disconnect(); }
      });
      const current = images();
      const report = {
        label: session.label,
        elapsedMs: Math.round((perf.now() - session.begin) * 10) / 10,
        bufferedStartup: session.buffered,
        observerSupport: session.supported,
        metrics: summarize(session.resources, current, session.getMutations(), session.tasks, win.location.href),
        completeImages: current.filter(function (image) { return image.complete && image.width > 0; }).length,
        lazyImages: current.filter(function (image) { return image.loading === 'lazy'; }).length,
        railScrollBefore: session.initialScroll,
        railScrollAfter: railScroll()
      };
      captures.push(report);
      return report;
    }
    async function inspectHeaders() {
      if (active) throw new Error('Finish the capture before inspecting headers');
      const candidate = Array.from(doc.querySelectorAll('.media-home-discovery-rail img, [data-home-zone="additional-sections"] img, [data-home-zone="primary-rail"] img')).find(function (image) {
        try {
          const url = image.currentSrc || image.src;
          return url && new URL(url, win.location.href).origin === win.location.origin;
        } catch (_) { return false; }
      });
      if (!candidate) throw new Error('No same-origin Home cover is currently rendered');
      const response = await win.fetch(candidate.currentSrc || candidate.src, { method: 'GET', credentials: 'same-origin', cache: 'no-cache', redirect: 'follow' });
      const headers = {};
      ['cache-control', 'etag', 'last-modified', 'expires', 'age', 'vary', 'content-type', 'content-length'].forEach(function (name) {
        const value = response.headers.get(name);
        if (value !== null) headers[name] = value;
      });
      if (response.body && typeof response.body.cancel === 'function') await response.body.cancel();
      return { status: response.status, type: response.type, headers: headers, note: 'One explicit same-origin GET; no URL, credentials or response body included.' };
    }
    function report() { return JSON.parse(JSON.stringify(captures)); }
    return { start: start, stop: stop, report: report, inspectHeaders: inspectHeaders };
  }
  if (typeof module === 'object' && module.exports) module.exports = { summarize: summarize, summarizeResources: summarizeResources, createProbe: createProbe };
  else root.VdrSuiteArtworkProbe = createProbe(root);
})(typeof window !== 'undefined' ? window : globalThis);
