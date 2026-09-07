// Canonical recording-folder reads and background refresh lifecycle.
(function (global) {
  'use strict';
  const INTERVAL_MS = 30000;

  function create(options) {
    const state = options.getState;
    let timer = null;
    let busy = false;

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

    function resolveLeaves(data, guard) {
      const resolver = options.folderArtwork;
      if (!resolver || typeof resolver.resolveLeaves !== 'function') {
        return Promise.resolve();
      }
      return resolver.resolveLeaves(data, requestFolder).then(function (result) {
        if (typeof guard === 'function' && !guard()) return;
        options.applyLeaves(result);
      });
    }

    function stop() {
      if (timer !== null) {
        global.clearTimeout(timer);
        timer = null;
      }
    }

    function schedule(delay) {
      stop();
      if (!state().active || state().selectedRecording) return;
      timer = global.setTimeout(function () {
        timer = null;
        refresh();
      }, delay === undefined ? INTERVAL_MS : delay);
    }

    function refresh() {
      const currentState = state();
      if (!currentState.active) return;
      if (busy || currentState.loading || currentState.loadingMore ||
          currentState.selectedRecording || (global.document && global.document.hidden)) {
        schedule();
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

    return Object.freeze({requestFolder, signature, resolveLeaves, stop, schedule, refresh});
  }

  global.VdrSuiteRecordings2FolderRefresh = Object.freeze({create});
}(window));
