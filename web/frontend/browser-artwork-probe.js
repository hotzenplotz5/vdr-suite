/* Read-only, opt-in browser artwork measurement. No URLs or credentials in reports. */
(function (root) {
  'use strict';
  function publicRecordingArtworkPath(path) {
    return /^\/api\/v1\/recordings\/rec_[0-9a-f]{32}\/artwork\/[0-9a-f]{32}$/.test(path);
  }
  function summarizeResources(entries, baseUrl) {
    const rows = new Map();
    const bounds = [0, 16384, 65536, 262144, 1048576, Infinity];
    const labels = ['zero', '1-16KiB', '16-64KiB', '64-256KiB', '256KiB-1MiB', 'over-1MiB'];
    const knownStatuses = [200, 204, 206, 301, 302, 304, 400, 401, 403, 404, 410, 429, 500, 502, 503, 504];
    const initiators = ['img', 'css', 'fetch', 'xmlhttprequest', 'link', 'script'];
    const base = new URL(baseUrl || 'https://probe.invalid/');
    const recordingRows = new Map();
    const artworkRows = new Map();
    function category(name) {
      let url;
      try { url = new URL(name, base); } catch (_) { return 'other'; }
      if (url.origin !== base.origin) return 'cross-origin';
      const path = url.pathname.replace(/^\/vdr-suite(?=\/)/, '');
      if (/^\/api\/(?:vdr\/)?recordings\/metadata\/image(?:\/|$)/.test(path)) return 'recording-metadata-image';
      if (/^\/api\/epg\/cache\/metadata\/image(?:\/|$)/.test(path)) return 'epg-metadata-image';
      if (/^\/channel-logos(?:\/|$)/.test(path)) return 'channel-logo';
      if (publicRecordingArtworkPath(path) || /^\/recording-artwork(?:\/|$)/.test(path)) return 'recording-artwork';
      if (/^\/api\/backends\/[^/]+\/settings\/series-artwork\/image$/.test(path)) return 'series-artwork';
      if (/^\/api\/epg\/cache(?:\/|$)/.test(path)) return 'epg-cache-api';
      if (/^\/api\/(?:vdr\/|v1\/)?recordings(?:\/|$)/.test(path) ||
          path === '/api/v1/search/recordings' ||
          /^\/api\/v1\/backends\/[^/]+\/recordings\/metadata(?:\/|$)/.test(path)) return 'recordings-api';
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
    function statusOf(entry) {
      const code = Number(entry.responseStatus);
      return Number.isInteger(code) && code >= 100 && code <= 599
        ? (knownStatuses.includes(code) ? String(code) : String(Math.floor(code / 100)) + 'xx')
        : 'unavailable';
    }
    function initiatorOf(entry) {
      return initiators.includes(entry.initiatorType) ? entry.initiatorType : 'other';
    }
    function addRow(map, fields, entry, name) {
      const key = JSON.stringify(fields);
      let row = map.get(key);
      if (!row) {
        row = { category: fields[0], status: fields[1], initiator: fields[2], count: 0, transferBytes: 0, encodedBytes: 0, decodedBytes: 0, sizeDistribution: Object.fromEntries(labels.map(function (label) { return [label, 0]; })) };
        if (fields.length > 3) row.variant = fields[3];
        if (fields.length > 4) row.previewVariant = fields[4];
        if (fields.length > 5) row.revision = fields[5];
        row.__seen = new Set();
        row.repeatedRequests = 0;
        map.set(key, row);
      }
      row.count++;
      row.transferBytes += bytes(entry.transferSize);
      row.encodedBytes += bytes(entry.encodedBodySize);
      row.decodedBytes += bytes(entry.decodedBodySize);
      const size = bytes(entry.encodedBodySize);
      const index = size === 0 ? 0 : bounds.findIndex(function (bound, i) { return i > 0 && size <= bound; });
      row.sizeDistribution[labels[index < 0 ? labels.length - 1 : index]]++;
      if (name && row.__seen.has(name)) row.repeatedRequests++;
      else if (name) row.__seen.add(name);
      // PWA.15.1: aggregate only artwork timing; never export URLs or identities.
      if (map === artworkRows) {
        row.timingMs = row.timingMs || {};
        function addTiming(name, duration) {
          if (!Number.isFinite(duration) || duration < 0) return;
          const stat = row.timingMs[name] || {samples:0,total:0,max:0};
          stat.samples++;
          stat.total = Math.round((stat.total + duration) * 10) / 10;
          stat.max = Math.round(Math.max(stat.max, duration) * 10) / 10;
          row.timingMs[name] = stat;
        }
        addTiming('resourceDuration', entry.duration);
        if (Number.isFinite(entry.requestStart) && entry.requestStart > 0 &&
            Number.isFinite(entry.responseStart) && entry.responseStart >= entry.requestStart)
          addTiming('requestToFirstByte', entry.responseStart - entry.requestStart);
        if (Number.isFinite(entry.responseStart) && entry.responseStart > 0 &&
            Number.isFinite(entry.responseEnd) && entry.responseEnd >= entry.responseStart)
          addTiming('responseBody', entry.responseEnd - entry.responseStart);
        if (Array.isArray(entry.serverTiming)) entry.serverTiming.forEach(function (phase) {
          if (phase.name === 'artwork-read') addTiming('serverArtworkRead', Number(phase.duration));
          if (phase.name === 'artwork-queue') addTiming('serverArtworkQueue', Number(phase.duration));
          if (phase.name === 'artwork-preview') addTiming('serverArtworkPreview', Number(phase.duration));
        });
      }
    }
    function exportedRows(map) {
      return Array.from(map.values()).map(function (row) {
        const result = Object.assign({}, row);
        delete result.__seen;
        return result;
      }).sort(function (a, b) {
        return b.transferBytes - a.transferBytes || a.category.localeCompare(b.category) || a.status.localeCompare(b.status) || a.initiator.localeCompare(b.initiator);
      });
    }
    function recordingCategory(path, url) {
      if (publicRecordingArtworkPath(path)) return null;
      if (path === '/api/v1/recordings') {
        const view = url.searchParams.get('view');
        return view === 'folders' ? 'public-folder-read' : 'public-recording-list';
      }
      if (/^\/api\/v1\/recordings\/rec_[0-9a-f]{32}$/.test(path)) return 'public-recording-detail';
      if (path === '/api/v1/search/recordings') return 'public-recording-search';
      if (/^\/api\/v1\/backends\/[^/]+\/recordings\/metadata(?:\/|$)/.test(path)) return 'public-manual-metadata-workflow';
      if (/^\/api\/v1\/recordings(?:\/|$)/.test(path)) return 'other-public-recording-api';
      const normalized = path.replace(/^\/api\/(?:vdr\/)?/, '/api/');
      if (normalized === '/api/recordings/metadata') return 'metadata-read';
      if (normalized === '/api/recordings/query') return 'recording-query';
      if (normalized === '/api/recordings' || normalized === '/api/recordings/') return 'recording-list';
      if (normalized === '/api/recordings/folder' || normalized === '/api/recordings/folders') return 'folder-read';
      if (normalized === '/api/recordings/genres' || normalized === '/api/recordings/genre') return 'genre-read';
      if (normalized === '/api/recordings/persons/search') return 'person-search';
      if (normalized === '/api/recordings/metadata/image') return null;
      if (/^\/api\/recordings\/artwork(?:\/|$)/.test(normalized)) return null;
      if (/^\/api\/recordings(?:\/|$)/.test(normalized)) return 'other-recording-api';
      if (/^\/api\/backends\/[^/]+\/recordings\/metadata\/(?:manual|search|seasons|episodes|assign|withdraw)$/.test(path)) return 'manual-metadata-workflow';
      return null;
    }
    function artworkVariant(url) {
      const kind = url.searchParams.get('kind');
      const allowed = ['poster', 'banner', 'fanart', 'backdrop', 'thumbnail', 'cover'];
      return kind === null ? 'unspecified' : (allowed.includes(kind) ? kind : 'other');
    }
    function previewVariant(url) {
      const variant = url.searchParams.get('variant');
      if (variant === null) return 'unspecified';
      return variant === 'home' ? 'home' : 'other';
    }
    (entries || []).forEach(function (entry) {
      const route = category(entry.name);
      const status = statusOf(entry);
      const initiator = initiatorOf(entry);
      addRow(rows, [route, status, initiator], entry, entry.name);
      let url;
      try { url = new URL(entry.name, base); } catch (_) { return; }
      if (url.origin !== base.origin) return;
      const path = url.pathname.replace(/^\/vdr-suite(?=\/)/, '');
      const recording = recordingCategory(path, url);
      if (recording) addRow(recordingRows, [recording, status, initiator], entry, entry.name);
      if (route === 'recording-metadata-image' || route === 'recording-artwork' || route === 'epg-metadata-image' || route === 'series-artwork') {
        const revision = url.searchParams.has(route === 'series-artwork' ? 'revision' : 'assignmentRevision') ? 'revisioned' : 'unversioned';
        addRow(
          artworkRows,
          [route, status, initiator, artworkVariant(url), previewVariant(url), revision],
          entry,
          entry.name
        );
      }
    });
    return {
      scope: 'all-observed-resources',
      observedEntries: (entries || []).length,
      rows: exportedRows(rows).map(function (row) {
        delete row.repeatedRequests;
        return row;
      }),
      recordingApiDetails: exportedRows(recordingRows),
      artworkVariantDetails: exportedRows(artworkRows),
      limitations: 'Resource Timing only; not a complete network log. Missing or unavailable statuses do not establish success or failure. Encoded bytes are HTTP body bytes, not decoded image pixels. Cross-origin sizes may be unavailable. Repeated requests mean repeated exact URLs within the observed entries, not necessarily redundant network transfers. Artwork variant reports the kind request parameter; previewVariant reports only whether variant=home is requested. Neither field proves original or resized image dimensions. No request URLs, IDs or query parameters are exported.'
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
  function startupTiming(perf, resources, baseUrl) {
    const result = { basis: 'document navigation start', firstPaintMs: null,
      firstContentfulPaintMs: null, sessionReadyMs: null, backendSelectedMs: null,
      snapshotReadyMs: null, homeControlsReadyMs: null, homeFrameMs: null,
      recordingCountSettledMs: null, homeControlsAfterSessionMs: null,
      homeNavigationReadyMs: null, snapshotRequestedMs: null,
      snapshotResource: null };
    if (!perf || typeof perf.getEntriesByName !== 'function') return result;
    function value(name, type) {
      const entries = perf.getEntriesByName(name, type);
      const start = entries && entries[0] && entries[0].startTime;
      return Number.isFinite(start) && start >= 0 ? Math.round(start * 10) / 10 : null;
    }
    result.firstPaintMs = value('first-paint', 'paint');
    result.firstContentfulPaintMs = value('first-contentful-paint', 'paint');
    for (const [stage, field] of [
      ['session-ready', 'sessionReadyMs'], ['backend-selected', 'backendSelectedMs'],
      ['snapshot-requested', 'snapshotRequestedMs'], ['home-navigation-ready', 'homeNavigationReadyMs'],
      ['snapshot-ready', 'snapshotReadyMs'], ['home-controls-ready', 'homeControlsReadyMs'],
      ['home-frame', 'homeFrameMs'], ['recording-count-settled', 'recordingCountSettledMs']
    ]) result[field] = value('vdr-suite:startup:' + stage, 'mark');
    if (result.sessionReadyMs !== null && result.homeControlsReadyMs !== null &&
        result.homeControlsReadyMs >= result.sessionReadyMs)
      result.homeControlsAfterSessionMs = Math.round((result.homeControlsReadyMs - result.sessionReadyMs) * 10) / 10;
    // Match only the initial same-origin snapshot in its measured interval.
    // The request-to-first-byte duration includes network/server, not pure server time.
    if (baseUrl && result.snapshotRequestedMs !== null && result.snapshotReadyMs !== null) {
      const base = new URL(baseUrl);
      const match = (resources || []).filter(function (entry) {
        try {
          const url = new URL(entry.name, base);
          return url.origin === base.origin &&
            /^\/api\/backends\/[^/]+\/snapshot$/.test(url.pathname.replace(/^\/vdr-suite(?=\/)/, '')) &&
            entry.startTime >= result.snapshotRequestedMs - 0.1 &&
            entry.responseEnd <= result.snapshotReadyMs + 0.1;
        } catch (_) { return false; }
      });
      if (match.length === 1) {
        const entry = match[0];
        const delta = (end, start) => Number.isFinite(end) && Number.isFinite(start) && start > 0 && end >= start
          ? Math.round((end - start) * 10) / 10 : null;
        result.snapshotResource = {
          dispatchToFetchMs: delta(entry.startTime, result.snapshotRequestedMs),
          fetchToRequestMs: delta(entry.requestStart, entry.startTime),
          requestToFirstByteMs: delta(entry.responseStart, entry.requestStart),
          bodyMs: delta(entry.responseEnd, entry.responseStart),
          responseToReadyMs: delta(result.snapshotReadyMs, entry.responseEnd)
        };
      }
    }
    return result;
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
    function visibleArtwork(image) {
      if (!image || typeof image.getBoundingClientRect !== 'function') return false;
      let url;
      try { url = new URL(image.currentSrc || image.src || image.__vdrSuiteDeferredArtworkUrl || '', win.location.href); }
      catch (_) { return false; }
      if (url.origin !== win.location.origin) return false;
      const path = url.pathname.startsWith('/vdr-suite/') ? url.pathname.slice('/vdr-suite'.length) : url.pathname;
      if (!(path.startsWith('/api/vdr/recordings/metadata/image') ||
            path.startsWith('/api/recordings/metadata/image') ||
            path.startsWith('/api/epg/cache/metadata/image') ||
            path.startsWith('/api/epg/cache/artwork') ||
            path.startsWith('/recording-artwork/') || publicRecordingArtworkPath(path) ||
            (path.startsWith('/api/backends/') && path.endsWith('/settings/series-artwork/image'))))
        return false;
      const rect = image.getBoundingClientRect();
      if (!(rect.width > 0 && rect.height > 0 && rect.right > 0 && rect.bottom > 0 &&
            rect.left < win.innerWidth && rect.top < win.innerHeight)) return false;
      // Horizontal rails clip cards that still intersect the document viewport.
      // Do not classify such posters as actually visible first-screen demand.
      const rail = typeof image.closest === 'function'
        ? image.closest('.media-home-discovery-rail, .media-home-series-season-rail, .media-home-live-guide-rail')
        : null;
      if (rail && typeof rail.getBoundingClientRect === 'function') {
        const clip = rail.getBoundingClientRect();
        if (!(rect.right > clip.left && rect.left < clip.right &&
              rect.bottom > clip.top && rect.top < clip.bottom)) return false;
      }
      return true;
    }
    function start(label, options) {
      if (active) throw new Error('Finish the current capture first');
      const buffered = Boolean(options && options.buffered);
      const observers = [];
      const resources = [];
      const tasks = [];
      let mutations = 0;
      const begin = perf.now();
      const round = value => Math.round(value * 10) / 10;
      // A fixed, first-observed viewport cohort survives later scrolling.
      // This is intentionally independent of visibleAtStop, which can be zero
      // after navigation even when the first-screen covers were loaded.
      let viewportImages = null, viewportQueued = false;
      const viewport = {firstVisibleAtMs:null, initialVisible:0,
        alreadyLoadedAtFirstObservation:0, readyAtStop:0, pendingAtStop:0,
        firstReadyObservedMs:null, allReadyObservedMs:null};
      const viewportReady = new Set();
      // All counters are aggregate and use image readiness, not raw URLs.
      const previouslyVisible = new Set(), pendingOnEntry = new Map();
      const previousRailScroll = new Map();
      let hasScrolledRail = false;
      const prefetch = {enteredVisible:0, readyBeforeEntry:0, notReadyOnEntry:0,
        becameReadyAfterEntry:0, pendingAtStop:0, delaySamples:0, delayTotalMs:0,
        delayMaxMs:0, maxPendingImageRequests:0, rightScrollFrames:0,
        leftScrollFrames:0, maxScrollStepPx:0, scrolledEnteredVisible:0,
        scrolledReadyBeforeEntry:0, requestLeadSamples:0,
        requestLeadTotalMs:0, requestLeadMaxMs:0, forwardEntries:0, reverseEntries:0};
      function inspectPrefetch() {
        const now = perf.now();
        const all = Array.from(doc.querySelectorAll('img'));
        const visible = new Set(all.filter(visibleArtwork));
        const pending = all.filter(image => !image.complete &&
          (image.src || image.currentSrc)).length;
        prefetch.maxPendingImageRequests = Math.max(prefetch.maxPendingImageRequests, pending);
        // Sample rail motion before classifying a cover's first visible frame.
        for (const rail of Array.from(doc.querySelectorAll(
          '.media-home-discovery-rail, .media-home-series-season-rail'))) {
          const position = Number(rail.scrollLeft) || 0;
          if (previousRailScroll.has(rail)) {
            const delta = position - previousRailScroll.get(rail);
            if (delta > 0) prefetch.rightScrollFrames++;
            if (delta < 0) prefetch.leftScrollFrames++;
            if (delta) hasScrolledRail = true;
            prefetch.maxScrollStepPx = Math.max(prefetch.maxScrollStepPx, Math.abs(delta));
          }
          previousRailScroll.set(rail, position);
        }
        for (const image of visible) {
          if (previouslyVisible.has(image)) continue;
          prefetch.enteredVisible++;
          if (hasScrolledRail) prefetch.scrolledEnteredVisible++;
          if (image.complete && image.naturalWidth > 0) {
            prefetch.readyBeforeEntry++;
            if (hasScrolledRail) prefetch.scrolledReadyBeforeEntry++;
          } else {
            prefetch.notReadyOnEntry++;
            if (!pendingOnEntry.has(image)) pendingOnEntry.set(image, now);
          }
          const started = image.__vdrSuiteArtworkPrefetchStartedAt;
          if (Number.isFinite(started) && started <= now) {
            const lead = round(now - started);
            prefetch.requestLeadSamples++;
            prefetch.requestLeadTotalMs += lead;
            prefetch.requestLeadMaxMs = Math.max(prefetch.requestLeadMaxMs, lead);
            if (image.__vdrSuiteArtworkPrefetchDirection === 'forward')
              prefetch.forwardEntries++;
            if (image.__vdrSuiteArtworkPrefetchDirection === 'reverse')
              prefetch.reverseEntries++;
          }
        }
        for (const [image, began] of pendingOnEntry) {
          if (!image.complete || !(image.naturalWidth > 0)) continue;
          const delay = round(now - began);
          prefetch.becameReadyAfterEntry++;
          prefetch.delaySamples++;
          prefetch.delayTotalMs += delay;
          prefetch.delayMaxMs = Math.max(prefetch.delayMaxMs, delay);
          pendingOnEntry.delete(image);
        }
        previouslyVisible.clear();
        visible.forEach(image => previouslyVisible.add(image));
        prefetch.pendingAtStop = pendingOnEntry.size;
      }
      function inspectViewport() {
        const now = round(perf.now() - begin);
        if (!viewportImages) {
          const visible = Array.from(doc.querySelectorAll('img')).filter(visibleArtwork);
          if (!visible.length) return;
          viewportImages = visible.slice(0, 24);
          viewport.firstVisibleAtMs = now;
          viewport.initialVisible = viewportImages.length;
          viewport.alreadyLoadedAtFirstObservation = viewportImages.filter(
            image => image.complete && image.naturalWidth > 0).length;
        }
        viewportImages.forEach(function (image) {
          if (!viewportReady.has(image) && image.complete && image.naturalWidth > 0) {
            viewportReady.add(image);
            if (viewport.firstReadyObservedMs === null) viewport.firstReadyObservedMs = now;
          }
        });
        if (viewport.allReadyObservedMs === null &&
            viewportReady.size === viewportImages.length) viewport.allReadyObservedMs = now;
      }
      function queueViewport() {
        if (viewportQueued) return;
        viewportQueued = true;
        function run() {
          viewportQueued = false;
          if (active && active.begin === begin) {
            inspectViewport();
            inspectPrefetch();
          }
        }
        if (typeof win.requestAnimationFrame === 'function') win.requestAnimationFrame(run);
        else run();
      }
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
      observe(function (records) {
        mutations += records.length;
        queueViewport();
      }, 'mutation', { childList: true, attributes: true, characterData: true, subtree: true });
      const imageLoadTimes = [];
      function imageLoaded(event) {
        if (visibleArtwork(event.target))
          imageLoadTimes.push(round(perf.now() - begin));
        // Update the initial cohort without changing its membership after
        // scrolling or opening another rail.
        if (!viewportImages || viewportImages.includes(event.target)) queueViewport();
      }
      function scrollChanged() { queueViewport(); }
      if (typeof doc.addEventListener === 'function') {
        doc.addEventListener('load', imageLoaded, true);
        doc.addEventListener('scroll', scrollChanged, true);
      }
      active = { label: String(label || 'capture'), begin: begin, buffered: buffered, supported: supported, observers: observers, resources: resources, tasks: tasks, imageLoaded: imageLoaded, scrollChanged: scrollChanged, imageLoadTimes: imageLoadTimes,
        finalPrefetch: function () {
          inspectPrefetch();
          return {...prefetch,
            prefetchHitRatePercent: prefetch.enteredVisible
              ? round(100 * prefetch.readyBeforeEntry / prefetch.enteredVisible) : null,
            scrollPrefetchHitRatePercent: prefetch.scrolledEnteredVisible
              ? round(100 * prefetch.scrolledReadyBeforeEntry / prefetch.scrolledEnteredVisible) : null,
            meanReadyDelayMs: prefetch.delaySamples
              ? round(prefetch.delayTotalMs / prefetch.delaySamples) : null,
            meanRequestLeadMs: prefetch.requestLeadSamples
              ? round(prefetch.requestLeadTotalMs / prefetch.requestLeadSamples) : null,
            basis: 'Animation-frame observations. The scroll hit rate excludes entries before the first observed rail movement; both hit rates include browser cache and do not prove network prefetch. Lead is request-start to viewport entry, not ready time. Pending count is incomplete img elements, not active network sockets.'};
        }, getMutations: function () { return mutations; }, addMutations: function (count) { mutations += count; }, initialScroll: railScroll(),
        finalViewport: function () {
          if (viewportImages) inspectViewport();
          viewport.readyAtStop = viewportReady.size;
          viewport.pendingAtStop = viewport.initialVisible - viewportReady.size;
          return {...viewport, basis: 'Fixed first-observed visible image cohort (max 24), independent of later scrolling; times relative to capture start. Ready means image.complete with naturalWidth > 0, not decoded-image paint. Already-cached images are timed at first observation.'};
        }};
      queueViewport();
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
      if (typeof doc.removeEventListener === 'function') {
        doc.removeEventListener('load', session.imageLoaded, true);
        doc.removeEventListener('scroll', session.scrollChanged, true);
      }
      const current = images();
      const visible = Array.from(doc.querySelectorAll('img')).filter(visibleArtwork);
      const loaded = visible.filter(function (image) { return image.complete && image.naturalWidth > 0; });
      const report = {
        label: session.label,
        elapsedMs: Math.round((perf.now() - session.begin) * 10) / 10,
        bufferedStartup: session.buffered,
        observerSupport: session.supported,
        metrics: summarize(session.resources, current, session.getMutations(), session.tasks, win.location.href),
        completeImages: current.filter(function (image) { return image.complete && image.width > 0; }).length,
        lazyImages: current.filter(function (image) { return image.loading === 'lazy'; }).length,
        initialViewportArtwork: session.finalViewport(),
        railArtworkPrefetch: session.finalPrefetch(),
        visibleArtwork: {
          visibleAtStop: visible.length,
          loadedAtStop: loaded.length,
          pendingAtStop: visible.length - loaded.length,
          observedLoadEvents: session.imageLoadTimes.length,
          firstLoadEventMs: session.imageLoadTimes.length ? Math.min.apply(null, session.imageLoadTimes) : null,
          lastLoadEventMs: session.imageLoadTimes.length ? Math.max.apply(null, session.imageLoadTimes) : null,
          basis: 'Since capture start; load is not decoded-image paint. Viewport heuristic can include scroll-clipped cards. Missing load event does not imply a cache miss.'
        },
        ...(session.buffered ? {startupTiming: startupTiming(perf, session.resources, win.location.href)} : {}),
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
  if (typeof module === 'object' && module.exports) module.exports = { summarize: summarize, summarizeResources: summarizeResources, createProbe: createProbe, startupTiming: startupTiming };
  else root.VdrSuiteArtworkProbe = createProbe(root);
})(typeof window !== 'undefined' ? window : globalThis);
