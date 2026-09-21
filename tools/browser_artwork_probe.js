/* Read-only, opt-in browser artwork measurement. Paste into DevTools on the authenticated frontend. */
(function (root) {
  'use strict';
  function summarize(entries, images, mutations, tasks) {
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
      longTaskMs: (tasks || []).reduce(function (n, task) { return n + task.duration; }, 0)
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
    function start(label) {
      if (active) throw new Error('Finish the current capture first');
      const observers = [];
      const resources = [];
      const tasks = [];
      let mutations = 0;
      const begin = perf.now();
      function observe(callback, type, options) {
        const Constructor = type === 'mutation' ? win.MutationObserver : win.PerformanceObserver;
        if (typeof Constructor !== 'function') return;
        try {
          const observer = new Constructor(callback);
          if (type === 'mutation') observer.observe(doc.documentElement || doc, options);
          else observer.observe(options);
          observers.push({ observer: observer, type: type });
        } catch (_) { /* unsupported entry type */ }
      }
      observe(function (list) { resources.push.apply(resources, list.getEntries()); }, 'resource', { type: 'resource', buffered: false });
      observe(function (list) { tasks.push.apply(tasks, list.getEntries()); }, 'longtask', { type: 'longtask', buffered: false });
      observe(function (records) { mutations += records.length; }, 'mutation', { childList: true, attributes: true, characterData: true, subtree: true });
      active = { label: String(label || 'capture'), begin: begin, observers: observers, resources: resources, tasks: tasks, getMutations: function () { return mutations; }, addMutations: function (count) { mutations += count; }, initialImages: images(), initialScroll: Array.from(doc.querySelectorAll('.media-home-discovery-rail')).map(function (rail) { return rail.scrollLeft; }) };
      return 'Capture started. Perform one browser action, then call stop().';
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
        } finally {
          observer.disconnect();
        }
      });
      const current = images();
      const report = {
        label: session.label,
        elapsedMs: Math.round((perf.now() - session.begin) * 10) / 10,
        metrics: summarize(session.resources, current, session.getMutations(), session.tasks),
        completeImages: current.filter(function (image) { return image.complete && image.width > 0; }).length,
        lazyImages: current.filter(function (image) { return image.loading === 'lazy'; }).length,
        railScrollBefore: session.initialScroll,
        railScrollAfter: Array.from(doc.querySelectorAll('.media-home-discovery-rail')).map(function (rail) { return rail.scrollLeft; })
      };
      captures.push(report);
      if (win.console && win.console.table) win.console.table(report.metrics);
      return report;
    }
    async function inspectHeaders() {
      if (active) throw new Error('Finish the capture before inspecting headers');
      const candidate = images().find(function (image) {
        try { return image.url && new URL(image.url, win.location.href).origin === win.location.origin; } catch (_) { return false; }
      });
      if (!candidate) throw new Error('No same-origin image is currently rendered');
      const response = await win.fetch(candidate.url, { method: 'GET', credentials: 'same-origin', cache: 'no-cache', redirect: 'follow' });
      const headers = {};
      ['cache-control', 'etag', 'last-modified', 'expires', 'age', 'vary', 'content-type', 'content-length'].forEach(function (name) {
        const value = response.headers.get(name);
        if (value !== null) headers[name] = value;
      });
      if (response.body && typeof response.body.cancel === 'function') await response.body.cancel();
      return { status: response.status, type: response.type, headers: headers, note: 'One explicit same-origin GET; no URL, credentials or response body included. Inspect the Network panel for proxy and 304 details.' };
    }
    function report() { return JSON.parse(JSON.stringify(captures)); }
    return { start: start, stop: stop, report: report, inspectHeaders: inspectHeaders };
  }
  if (typeof module === 'object' && module.exports) module.exports = { summarize: summarize, createProbe: createProbe };
  else root.VdrSuiteArtworkProbe = createProbe(root);
})(typeof window !== 'undefined' ? window : globalThis);
