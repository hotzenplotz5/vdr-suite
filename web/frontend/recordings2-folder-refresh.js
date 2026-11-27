// Canonical recording-folder reads, projection and background refresh lifecycle.
(function (global) {
  'use strict';
  function create(options) {
    const state = options.getState;
    let timer = null;
    let busy = false;
    let source = null;
    let pending = false;
    let lastSequence = 0;
    let connectionSequence = 0;

    function notifyRecordingMarksChanged() {
      if (typeof options.onRecordingMarksChanged === 'function') {
        options.onRecordingMarksChanged();
      }
    }

    function subscribe() {
      const client = options.shared.clientApi();
      if (source || !client || typeof client.createClientLiveUpdateSource !== 'function') return;
      source = client.createClientLiveUpdateSource();
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
            !Array.isArray(data.changedDomains)) return;
        if (data.changedDomains.includes('recordingMarks')) {
          notifyRecordingMarksChanged();
        }
        if (!data.changedDomains.includes('recordings')) return;
        pending = true;
        if (state().selectedRecording &&
            typeof options.onRecordingPresentationChanged === 'function')
          options.onRecordingPresentationChanged();
        if (!busy) schedule(0);
      });
      source.addEventListener('resync', function () { if (source !== currentSource || !state().active) return; lastSequence = connectionSequence = 0; pending = true; if (state().selectedRecording) notifyRecordingMarksChanged(); if (!busy) schedule(0); });
      source.onerror = function () {
        if (source !== currentSource) return;
        if (connectionSequence > 0 && connectionSequence < lastSequence) {
          lastSequence = connectionSequence;
          pending = true;
          if (state().selectedRecording) notifyRecordingMarksChanged();
          if (!busy) schedule(0);
        }
      };
    }

    function requestFolder(path, offset, limit, backendId) {
      const opaque = path && typeof path === 'object' && typeof path.folderId === 'string';
      const backend = backendId || state().backendId;
      const pageLimit = limit === undefined ? options.pageSize : limit;
      const pageOffset = Math.max(0, options.number(offset, 0));
      if ((opaque || !path) && typeof options.fetchPublicRecordingFolder === 'function') {
        const result = options.fetchPublicRecordingFolder({
          query: {backend, folderId:opaque ? path.folderId : '', limit:pageLimit, offset:pageOffset},
          legacyPath:opaque ? path.legacyPath : '',
          cache:'no-store', credentials:'same-origin'
        });
        if (result) return result;
      }
      if (opaque && path.folderId && typeof path.legacyPath !== 'string')
        return Promise.reject(new Error('Die öffentliche Ordnerschnittstelle ist nicht verfügbar.'));
      return options.fetchClientRecordingFolder({
        query: {
          backend,
          path: options.normalizePath(opaque ? path.legacyPath : path),
          limit: pageLimit,
          offset: pageOffset,
          _: String(Date.now())
        },
        cache: 'no-store',
        credentials: 'same-origin'
      });
    }

    function applyFolderData(data, append) {
      const state = options.getState();
      if (!data || data.recordingFolder !== true) {
        throw new Error('Der Server hat keinen gültigen Aufnahmeordner geliefert.');
      }
      const publicPage = data.publicFolder === true;
      const previousFolders = append && state.data
        ? options.shared.folderList(state.data).slice()
        : options.shared.folderList(data).slice();
      if (append && publicPage) {
        if (!state.data || state.data.publicFolder !== true ||
            data.folderId !== state.folderId || data.offset !== state.data.nextOffset)
          throw new Error('Der Aufnahmeordner hat sich geändert. Bitte neu laden.');
        const folderIds = new Set(previousFolders.map(folder => folder.folderId));
        const recordingIds = new Set(state.serverRecordings.map(recording => recording.recordingId));
        if (options.shared.folderList(data).some(folder=>folderIds.has(folder.folderId)) ||
            options.shared.recordingList(data).some(recording=>recordingIds.has(recording.recordingId)))
          throw new Error('Der Aufnahmeordner hat sich geändert. Bitte neu laden.');
        previousFolders.push(...options.shared.folderList(data));
      }
      const incomingRecordings = options.normalizeRecordings(options.shared.recordingList(data));
      state.data = append
        ? Object.assign({}, state.data || {}, data, {folders: previousFolders})
        : Object.assign({}, data, {folders: previousFolders});
      state.folderId = publicPage ? data.folderId : null;
      state.parentFolderId = publicPage ? data.parentFolderId : '';
      state.breadcrumbs = publicPage ? data.breadcrumbs : [];
      state.path = publicPage ? '' : options.shared.normalizePath(options.shared.first(data, ['path'], state.path));
      state.parentPath = publicPage ? '' : options.shared.normalizePath(options.shared.first(data, ['parentPath'], ''));
      state.serverRecordingCount = options.shared.number(data.recordingCount, incomingRecordings.length);
      state.serverRecordings = append
        ? state.serverRecordings.concat(incomingRecordings)
        : incomingRecordings;
      state.serverSignature = append ? '' : signature(data);
      if (!append) state.promotedRecordings = [];
      updatePresentedFolderState();
    }

    function selectRecording(recording) {
      const state = options.getState();
      const sequence = ++state.requestSequence;
      const id = recording && recording.folderCard === true && recording.recordingId;
      if (/^rec_[0-9a-f]{32}$/.test(id || '')) {
        // Only this explicit selection needs the still-private detail bridge.
        // The list remains public and no per-card detail requests are made.
        const api = options.shared.clientApi();
        if (!api || typeof api.fetchClientRecordingForPublicId !== 'function') {
          state.error = new Error('Aufnahmedetails sind nicht verfügbar.');
          options.render();
          return;
        }
        const backendId = state.backendId;
        state.loadingMore = false;
        state.loading = true;
        api.fetchClientRecordingForPublicId({backendId, recordingId:id})
          .then(function (detail) {
            if (!state.active || sequence !== state.requestSequence || state.backendId !== backendId) return;
            state.loading = false;
            if (!detail || !detail.recording || detail.recording.backendId !== backendId ||
                detail.recording.publicRecordingId !== id)
              throw new Error('Die Aufnahme konnte nicht eindeutig aufgelöst werden.');
            state.selectedRecording = options.normalizeRecording(Object.assign({}, detail.recording,
              recording.__vdrSuiteOpenHevcShrink ? {__vdrSuiteOpenHevcShrink:true} : {}));
            options.render();
            schedule();
          }).catch(function (error) {
            if (!state.active || sequence !== state.requestSequence) return;
            state.loading = false;
            state.error = error;
            options.render();
          });
        return;
      }
      state.selectedRecording = options.normalizeRecording(recording);
      options.render();
      schedule();
    }

    function signature(data) {
      return JSON.stringify([
        data.path, data.folderId, data.breadcrumbs, data.folderCount, data.cardCount,
        data.totalCount, data.recordingCount,
        data.folders, data.recordings
      ]);
    }

    function updatePresentedFolderState() {
      const value = state();
      const folders = options.shared.folderList(value.data);
      value.recordings = value.serverRecordings.concat(value.promotedRecordings);
      value.data = Object.assign({}, value.data || {}, {
        folders: folders,
        folderCount: value.data && value.data.publicFolder === true ? value.data.folderCount : folders.length,
        recordingCount: value.serverRecordingCount + value.promotedRecordings.length,
        returnedCount: value.serverRecordings.length + value.promotedRecordings.length
      });
    }

    function sameRecordingIdentity(candidate, recording) {
      const candidateBackend = String(candidate.backendId || state().backendId || 'default');
      const recordingBackend = String(recording.backendId || state().backendId || 'default');
      if (candidateBackend !== recordingBackend) return false;
      if (typeof options.shared.publicRecordingId === 'function') {
        const a = options.shared.publicRecordingId(candidate), b = options.shared.publicRecordingId(recording);
        if (a && b) return a === b;
      }
      const candidateNative = String(options.shared.first(candidate, ['backendNativeId', 'nativePath', 'path'], ''));
      const recordingNative = String(options.shared.first(recording, ['backendNativeId', 'nativePath', 'path'], ''));
      if (candidateNative || recordingNative) return Boolean(candidateNative && recordingNative && candidateNative === recordingNative);
      const candidateId = String(options.shared.first(candidate, ['recordingId', 'id', 'nativeId'], ''));
      const recordingId = String(options.shared.first(recording, ['recordingId', 'id', 'nativeId'], ''));
      return Boolean(candidateId && recordingId && candidateId === recordingId);
    }

    function forgetRecording(recording) {
      const value = state();
      const beforeServer = value.serverRecordings.length;
      const beforePromoted = value.promotedRecordings.length;
      value.serverRecordings = value.serverRecordings.filter(function (candidate) {
        return !sameRecordingIdentity(candidate, recording);
      });
      value.promotedRecordings = value.promotedRecordings.filter(function (candidate) {
        return !sameRecordingIdentity(candidate, recording);
      });
      const removedServer = beforeServer - value.serverRecordings.length;
      const removedPromoted = beforePromoted - value.promotedRecordings.length;
      if (removedServer > 0) {
        value.serverRecordingCount = Math.max(
          0, value.serverRecordingCount - removedServer
        );
      }
      if (value.data) {
        value.data = Object.assign({}, value.data, {
          recordings: options.shared.recordingList(value.data).filter(function (candidate) {
            return !sameRecordingIdentity(candidate, recording);
          })
        });
      }
      updatePresentedFolderState();
      return removedServer > 0 || removedPromoted > 0;
    }

    function resolveLeaves(data, guard) {
      if (data && data.publicFolder === true) return Promise.resolve();
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
      if (!state().active) return;
      subscribe();
      // SSE hints and explicit lifecycle reads only; no periodic requests.
      if (delay !== undefined) pending = true;
      if (!pending || busy || state().loading || state().loadingMore ||
          state().selectedRecording || (global.document && global.document.hidden)) return;
      timer = global.setTimeout(function () { timer = null; refresh(); }, 0);
    }

    function refresh() {
      const currentState = state();
      if (!currentState.active) return;
      if (busy || currentState.loading || currentState.loadingMore ||
          currentState.selectedRecording || (global.document && global.document.hidden)) {
        pending = true;
        return;
      }
      if (!currentState.data || currentState.error) {
        options.loadFolder(currentState.folderId !== null && currentState.folderId !== undefined
          ? {folderId:currentState.folderId, legacyPath:currentState.legacyFolderPath} : currentState.path || '');
        return;
      }
      const sequence = currentState.requestSequence;
      const backendId = currentState.backendId;
      const path = currentState.path;
      const folderId = currentState.folderId;
      const opaque = currentState.data.publicFolder === true;
      const target = opaque ? {folderId, legacyPath:currentState.legacyFolderPath} : path;
      const limit = Math.max(options.pageSize, opaque ? currentState.data.nextOffset : currentState.serverRecordings.length);
      const current = function () {
        const value = state();
        return value.active && sequence === value.requestSequence &&
          value.backendId === backendId && value.path === path && value.folderId === folderId &&
          !value.loading && !value.loadingMore && !value.selectedRecording &&
          !(global.document && global.document.hidden);
      };
      busy = true;
      pending = false;
      let applied = false;
      requestFolder(target, 0, limit, backendId)
        .then(function (data) {
          if (!current()) return null;
          if (!data || data.recordingFolder !== true ||
              (opaque ? data.publicFolder !== true || data.folderId !== folderId :
                data.publicFolder !== true && options.normalizePath(data.path) !== path)) {
            throw new Error('Der Server hat keinen gültigen Aufnahmeordner geliefert.');
          }
          if (signature(data) === state().serverSignature) return null;
          applyFolderData(data, false);
          applied = true;
          return resolveLeaves(data, current).then(function () {
            if (current()) options.render();
          });
        })
        .catch(function (error) {
          // Keep the usable folder; retry on the next SSE hint or explicit read.
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

    return Object.freeze({requestFolder, signature, applyFolderData, selectRecording, updatePresentedFolderState,
      sameRecordingIdentity, forgetRecording, resolveLeaves, stop, schedule, refresh});
  }

  global.VdrSuiteRecordings2FolderRefresh = Object.freeze({create});
}(window));
