// Canonical recording-folder reads, projection and background refresh lifecycle.
(function (global) {
  'use strict';
  const INTERVAL_MS = 30000;

  function create(options) {
    const state = options.getState;
    let timer = null;
    let busy = false;
    let source = null;
    let pending = false;
    let lastSequence = 0;
    let connectionSequence = 0;

    function subscribe() {
      if (source || typeof options.createClientLiveUpdateSource !== 'function') return;
      source = options.createClientLiveUpdateSource();
      if (!source) return;
      const currentSource = source;
      source.onopen = function () { connectionSequence = 0; };
      source.addEventListener('update', function (event) {
        if (source !== currentSource || !state().active) return;
        let data;
        try { data = JSON.parse(event.data); } catch (_) { return; }
        const sequence = Number(data && data.sequenceNumber);
        if (!Number.isSafeInteger(sequence) || sequence < 1) return;
        connectionSequence = Math.max(connectionSequence, sequence);
        if (sequence <= lastSequence) return;
        lastSequence = sequence;
        if (String(data.backendId || 'default') !== state().backendId ||
            !Array.isArray(data.changedDomains) ||
            !data.changedDomains.includes('recordings')) return;
        pending = true;
        if (!busy) schedule(0);
      });
      source.onerror = function () {
        // The existing endpoint replays a finite feed on reconnect. A daemon
        // restart can reset sequence numbers; refresh once and accept its epoch.
        if (source !== currentSource) return;
        if (connectionSequence < lastSequence) {
          lastSequence = connectionSequence;
          pending = true;
          if (!busy) schedule(0);
        }
      };
    }

    function requestFolder(path, offset, limit, backendId) {
      return options.fetchClientRecordingFolder({
        query: {
          backend: backendId || state().backendId,
          path: options.normalizePath(path),
          limit: limit === undefined ? options.pageSize : limit,
          offset: Math.max(0, options.number(offset, 0)),
          _: String(Date.now())
        },
        cache: 'no-store',
        credentials: 'same-origin'
      });
    }

    function signature(data) {
      return JSON.stringify([
        data.path, data.totalCount, data.recordingCount,
        data.folders, data.recordings
      ]);
    }

    function updatePresentedFolderState() {
      const value = state();
      const folders = options.shared.folderList(value.data);
      value.recordings = value.serverRecordings.concat(value.promotedRecordings);
      value.data = Object.assign({}, value.data || {}, {
        folders: folders,
        folderCount: folders.length,
        recordingCount: value.serverRecordingCount + value.promotedRecordings.length,
        returnedCount: value.serverRecordings.length + value.promotedRecordings.length
      });
    }

    function resolveLeaves(data, guard) {
      const resolver = options.folderArtwork;
      if (!resolver || typeof resolver.resolveLeaves !== 'function') {
        return Promise.resolve();
      }
      return resolver.resolveLeaves(data, requestFolder).then(function (result) {
        if (typeof guard === 'function' && !guard()) return;
        const value = state();
        value.promotedRecordings = result && Array.isArray(result.recordings)
          ? options.normalizeRecordings(result.recordings) : [];
        value.data = Object.assign({}, value.data || {}, {
          folders: result && Array.isArray(result.folders)
            ? result.folders.slice() : options.shared.folderList(value.data).slice()
        });
        updatePresentedFolderState();
      });
    }

    function cancelTimer() {
      if (timer !== null) {
        global.clearTimeout(timer);
        timer = null;
      }
    }

    function stop() {
      cancelTimer();
      if (source) { source.close(); source = null; }
      lastSequence = 0;
      connectionSequence = 0;
      pending = false;
    }

    function schedule(delay) {
      cancelTimer();
      if (!state().active || state().selectedRecording) return;
      subscribe();
      timer = global.setTimeout(function () {
        timer = null;
        refresh();
      }, delay === undefined ? (pending ? 0 : INTERVAL_MS) : delay);
    }

    function refresh() {
      const currentState = state();
      if (!currentState.active) return;
      if (busy || currentState.loading || currentState.loadingMore ||
          currentState.selectedRecording || (global.document && global.document.hidden)) {
        schedule(INTERVAL_MS);
        return;
      }
      if (!currentState.data || currentState.error) {
        options.loadFolder(currentState.path || '');
        return;
      }
      const sequence = currentState.requestSequence;
      const backendId = currentState.backendId;
      const path = currentState.path;
      const limit = Math.max(options.pageSize, currentState.serverRecordings.length);
      const current = function () {
        const value = state();
        return value.active && sequence === value.requestSequence &&
          value.backendId === backendId && value.path === path &&
          !value.loading && !value.loadingMore && !value.selectedRecording &&
          !(global.document && global.document.hidden);
      };
      busy = true;
      pending = false;
      let applied = false;
      requestFolder(path, 0, limit, backendId)
        .then(function (data) {
          if (!current()) return null;
          if (!data || data.recordingFolder !== true ||
              options.normalizePath(data.path) !== path) {
            throw new Error('Der Server hat keinen gültigen Aufnahmeordner geliefert.');
          }
          if (signature(data) === state().serverSignature) return null;
          options.applyFolderData(data, false);
          applied = true;
          return resolveLeaves(data, current).then(function () {
            if (current()) options.render();
          });
        })
        .catch(function (error) {
          // Keep the usable folder and retry failed leaf enrichment on the next poll.
          if (current() && applied) {
            state().serverSignature = '';
            options.render();
          }
          if (current() && global.console && typeof global.console.warn === 'function') {
            global.console.warn('VDR-Suite recording folder refresh failed', error);
          }
        })
        .finally(function () {
          busy = false;
          if (state().active) schedule();
        });
    }

    return Object.freeze({requestFolder, signature, updatePresentedFolderState,
      resolveLeaves, stop, schedule, refresh});
  }

  global.VdrSuiteRecordings2FolderRefresh = Object.freeze({create});
}(window));
