// Recordings2 folder transport, page application and interaction-owned detail reads.
// The SSE lifecycle remains in recordings2-folder-refresh.js.
(function (global) {
  'use strict';
  function create(options, lifecycle) {
    const state = options.getState;
    const {signature, updatePresentedFolderState, schedule} = lifecycle;

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
      ++state.requestSequence;
      // Public folder cards already own the canonical identity and initial
      // presentation. The existing detail metadata owner reads API v1 on
      // demand; no private native-detail projection is needed before opening.
      if (recording && recording.folderCard === true &&
          (!/^rec_[0-9a-f]{32}$/.test(recording.recordingId || '') ||
           recording.backendId !== state.backendId)) {
        state.error = new Error('Die Aufnahme konnte nicht eindeutig zugeordnet werden.');
        options.render();
        return;
      }
      state.loadingMore = false;
      state.loading = false;
      state.selectedRecording = options.normalizeRecording(recording);
      options.render();
      schedule();
    }

    return Object.freeze({requestFolder, applyFolderData, selectRecording});
  }
  global.VdrSuiteRecordings2FolderPage = Object.freeze({create});
}(window));
