(function (global) {
  'use strict';
  const shared = global.VdrSuiteRecordings2Shared;
  const folderArtwork = global.VdrSuiteRecordings2FolderArtwork;
  const browserView = global.VdrSuiteRecordings2BrowserView;
  if (!shared || !browserView || typeof browserView.create !== 'function') {
    console.error('VDR-Suite Recordings 2 runtime dependencies are unavailable');
    return;
  }
  const state = {
    active: false,
    backendId: '',
    path: '',
    parentPath: '',
    data: null,
    serverRecordings: [],
    promotedRecordings: [],
    recordings: [],
    serverRecordingCount: 0,
    selectedRecording: null,
    detailReturn: null,
    detailReturnLabel: '',
    loading: false,
    loadingMore: false,
    error: null,
    requestSequence: 0
  };
  let view; let playbackRuntimePromise = null; let playbackPipUiBound = false;
  function normalizeRecording(recording) {
    if (!recording || typeof recording !== 'object') return recording;
    const title = typeof shared.recordingPathTitle === 'function'
      ? shared.recordingPathTitle(recording)
      : '';
    return title ? Object.assign({}, recording, {title: title}) : recording;
  }
  function normalizeRecordings(recordings) {
    return (Array.isArray(recordings) ? recordings : []).map(normalizeRecording);
  }
  function render() {
    if (!state.active || !view) return;
    if (state.loading) return view.renderLoading();
    if (state.error) return view.renderError();
    if (state.selectedRecording) return view.renderDetail();
    view.renderFolder();
  }
  function installPlaybackPipUi() { if (playbackPipUiBound || typeof document === 'undefined' || typeof document.addEventListener !== 'function') return; playbackPipUiBound = true; const mini = function () { return typeof document.getElementById === 'function' ? document.getElementById('vdr-suite-live-mini-player') : null; }; document.addEventListener('enterpictureinpicture', function (event) { const root = mini(); const video = event && event.target; if (!root || !video || typeof root.contains !== 'function' || !root.contains(video)) return; if (root.dataset) root.dataset.vdrSuitePipSuppressed = 'true'; root.hidden = true; }, true); document.addEventListener('leavepictureinpicture', function (event) { const root = mini(); if (!root || !root.dataset || root.dataset.vdrSuitePipSuppressed !== 'true') return; delete root.dataset.vdrSuitePipSuppressed; const video = event && event.target; if (video && typeof root.contains === 'function' && root.contains(video)) root.hidden = false; }, true); }
  function installPlaybackShell() { const shell = global.VdrSuitePlaybackShell; if (shell && typeof shell.install === 'function') shell.install(); installPlaybackPipUi(); }
  function ensurePlaybackRuntime() {
    if (global.VdrSuiteRecordings2Playback && typeof global.VdrSuiteRecordings2Playback.createPanel === 'function') { installPlaybackShell(); return Promise.resolve(); }
    if (playbackRuntimePromise) return playbackRuntimePromise;
    if (typeof global.loadVdrSuiteDeferredRuntime !== 'function') return Promise.resolve();
    playbackRuntimePromise = global.loadVdrSuiteDeferredRuntime('vdr-suite-session-frontend-sync-runtime',
      '/frontend/api/session-frontend-sync.js',
      function () { return Boolean(global.VdrSuiteRecordingFastPlayback && global.VdrSuiteLivePlayback); }
    ).then(function () { return global.loadVdrSuiteDeferredRuntime('vdr-suite-recordings2-playback-runtime',
      '/frontend/recordings2-playback.js',
      function () { return Boolean(global.VdrSuiteRecordings2Playback && typeof global.VdrSuiteRecordings2Playback.createPanel === 'function'); }
    ); }).then(function () {
      installPlaybackShell(); if (state.active && state.selectedRecording) render();
    }).catch(function (error) { console.error('VDR-Suite Recordings 2 playback runtime failed', error); });
    return playbackRuntimePromise;
  }
  function requestFolder(path, offset) {
    const api = shared.clientApi();
    if (!api || typeof api.fetchClientRecordingFolder !== 'function') {
      return Promise.reject(new Error('Client API für Aufnahmeordner ist nicht verfügbar.'));
    }
    return api.fetchClientRecordingFolder({
      query: {
        backend: state.backendId,
        path: shared.normalizePath(path),
        limit: shared.PAGE_SIZE,
        offset: Math.max(0, shared.number(offset, 0)),
        _: String(Date.now())
      },
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }
  function updatePresentedFolderState() {
    const folders = shared.folderList(state.data);
    state.recordings = state.serverRecordings.concat(state.promotedRecordings);
    state.data = Object.assign({}, state.data || {}, {
      folders: folders,
      folderCount: folders.length,
      recordingCount: state.serverRecordingCount + state.promotedRecordings.length,
      returnedCount: state.serverRecordings.length + state.promotedRecordings.length
    });
  }
  function applyFolderData(data, append) {
    if (!data || data.recordingFolder !== true) {
      throw new Error('Der Server hat keinen gültigen Aufnahmeordner geliefert.');
    }
    const previousFolders = append && state.data
      ? shared.folderList(state.data).slice()
      : shared.folderList(data).slice();
    const incomingRecordings = normalizeRecordings(shared.recordingList(data));
    state.data = append
      ? Object.assign({}, state.data || {}, data, {folders: previousFolders})
      : Object.assign({}, data, {folders: previousFolders});
    state.path = shared.normalizePath(shared.first(data, ['path'], state.path));
    state.parentPath = shared.normalizePath(shared.first(data, ['parentPath'], ''));
    state.serverRecordingCount = shared.number(data.recordingCount, incomingRecordings.length);
    state.serverRecordings = append
      ? state.serverRecordings.concat(incomingRecordings)
      : incomingRecordings;
    if (!append) state.promotedRecordings = [];
    updatePresentedFolderState();
  }
  function resolveSingleRecordingLeaves(data) {
    if (!folderArtwork || typeof folderArtwork.resolveLeaves !== 'function') {
      return Promise.resolve();
    }
    return folderArtwork.resolveLeaves(data, requestFolder).then(function (result) {
      state.promotedRecordings = result && Array.isArray(result.recordings)
        ? normalizeRecordings(result.recordings)
        : [];
      state.data = Object.assign({}, state.data || {}, {
        folders: result && Array.isArray(result.folders)
          ? result.folders.slice()
          : shared.folderList(state.data).slice()
      });
      updatePresentedFolderState();
    });
  }
  function clearExternalDetailReturn() {
    state.detailReturn = null;
    state.detailReturnLabel = '';
  }
  function loadFolder(path) {
    state.active = true;
    state.backendId = shared.selectedBackendId();
    state.path = shared.normalizePath(path);
    state.parentPath = state.path.split('/').slice(0, -1).join('/');
    state.selectedRecording = null;
    clearExternalDetailReturn();
    state.loading = true;
    state.loadingMore = false;
    state.error = null;
    const sequence = ++state.requestSequence;
    render();
    requestFolder(state.path, 0)
      .then(function (data) {
        if (!state.active || sequence !== state.requestSequence) return null;
        applyFolderData(data, false);
        return resolveSingleRecordingLeaves(data);
      })
      .then(function () {
        if (!state.active || sequence !== state.requestSequence) return;
        state.loading = false;
        render();
      })
      .catch(function (error) {
        if (!state.active || sequence !== state.requestSequence) return;
        state.loading = false;
        state.error = error;
        render();
      });
  }
  function loadMore() {
    if (state.loadingMore || state.loading) return;
    state.loadingMore = true;
    const sequence = ++state.requestSequence;
    render();
    requestFolder(state.path, state.serverRecordings.length)
      .then(function (data) {
        if (!state.active || sequence !== state.requestSequence) return;
        applyFolderData(data, true);
        state.loadingMore = false;
        render();
      })
      .catch(function (error) {
        if (!state.active || sequence !== state.requestSequence) return;
        state.loadingMore = false;
        state.error = error;
        render();
      });
  }
  function selectRecording(recording) {
    clearExternalDetailReturn();
    state.selectedRecording = normalizeRecording(recording);
    render();
  }
  function closeDetail() {
    const detailReturn = state.detailReturn;
    if (view && typeof view.destroy === 'function') view.destroy();
    state.selectedRecording = null;
    clearExternalDetailReturn();
    if (typeof detailReturn === 'function') {
      state.active = false;
      detailReturn();
      return;
    }
    render();
  }
  function reload() {
    if (state.selectedRecording && state.detailReturn) {
      render();
      return;
    }
    if (state.selectedRecording) state.selectedRecording = null;
    loadFolder(state.path || '');
  }
  view = browserView.create({
    getState: function () { return state; },
    openFolder: loadFolder,
    loadMore: loadMore,
    selectRecording: selectRecording,
    closeDetail: closeDetail,
    reload: reload
  });
  const moduleApi = Object.freeze({
    activate: function () {
      const backend = shared.selectedBackendId();
      if (!state.active || state.backendId !== backend || !state.data) {
        state.backendId = backend;
        loadFolder('');
        return;
      }
      state.active = true;
      render();
    },
    deactivate: function () {
      if (view && typeof view.destroy === 'function') view.destroy();
      state.active = false;
      state.requestSequence += 1;
      state.selectedRecording = null;
      clearExternalDetailReturn();
      const target = shared.mountTarget();
      if (target) target.classList.remove('recordings2-mount');
    },
    refresh: function () {
      state.active = true;
      loadFolder(state.path || '');
    },
    openFolder: function (path) {
      loadFolder(path || '');
    },
    openRecording: function (recording, options) {
      const config = options && typeof options === 'object' ? options : {};
      state.requestSequence += 1;
      state.active = true;
      state.backendId = String(
        (recording && recording.backendId) || config.backendId || shared.selectedBackendId()
      );
      state.loading = false;
      state.loadingMore = false;
      state.error = null;
      state.selectedRecording = normalizeRecording(recording);
      state.detailReturn = typeof config.onClose === 'function' ? config.onClose : null;
      state.detailReturnLabel = config.backLabel || '← Zurück zum Genre';
      render();
    },
    refreshDetailAddon: function () {
      if (state.active && state.selectedRecording) render();
    },
    __test: Object.freeze({
      normalizePath: shared.normalizePath,
      decodeDisplayText: shared.decodeDisplayText,
      recordingPathTitle: shared.recordingPathTitle,
      recordingNativeTitle: shared.recordingNativeTitle,
      recordingMetadataTitle: shared.recordingMetadataTitle,
      recordingTitle: shared.recordingTitle,
      recordingSubtitle: shared.recordingSubtitle,
      recordingSummary: shared.recordingSummary,
      recordingPosterUrl: shared.recordingPosterUrl,
      formatDuration: shared.formatDuration,
      formatSize: shared.formatSize,
      normalizeRecording: normalizeRecording,
      applyFolderData: applyFolderData,
      resolveSingleRecordingLeaves: resolveSingleRecordingLeaves,
      ensurePlaybackRuntime: ensurePlaybackRuntime
    })
  });
  function ensureNavigationTab() {
    let tab = document.querySelector('[data-module="recordings2"]');
    if (tab) return tab;
    const navigation = document.getElementById('module-nav');
    if (!navigation) return null;
    tab = document.createElement('button');
    tab.type = 'button';
    tab.className = 'module-tab';
    tab.dataset.module = 'recordings2';
    tab.textContent = 'Recordings 2';
    tab.setAttribute('aria-label', 'Recordings 2 öffnen');
    const legacy = navigation.querySelector('[data-module="recordings"]');
    if (legacy && legacy.nextSibling) navigation.insertBefore(tab, legacy.nextSibling);
    else navigation.appendChild(tab);
    return tab;
  }
  function installShellEntry() {
    const tab = ensureNavigationTab();
    if (!tab) return;
    tab.addEventListener('click', function () {
      document.querySelectorAll('.module-tab').forEach(function (button) {
        button.classList.toggle('active', button === tab);
      });
      const channels2 = global.VdrSuiteChannels2;
      if (channels2 && typeof channels2.deactivate === 'function') channels2.deactivate();
      global.setTimeout(function () { moduleApi.activate(); }, 0);
    });
    document.querySelectorAll('.module-tab').forEach(function (button) {
      if (button === tab) return;
      button.addEventListener('click', function () { moduleApi.deactivate(); });
    });
    const refresh = document.getElementById('refresh-detail');
    if (refresh) {
      refresh.addEventListener('click', function (event) {
        if (!tab.classList.contains('active')) return;
        event.preventDefault();
        event.stopImmediatePropagation();
        moduleApi.refresh();
      }, true);
    }
    document.addEventListener('click', function (event) {
      const backend = event.target && typeof event.target.closest === 'function'
        ? event.target.closest('.backend-card')
        : null;
      if (backend) moduleApi.deactivate();
    }, true);
  }
  global.VdrSuiteRecordings2 = moduleApi;
  const boundary = shared.platform();
  if (boundary && typeof boundary.registerModule === 'function' &&
      (!boundary.hasModule || !boundary.hasModule('recordings2'))) {
    boundary.registerModule('recordings2', moduleApi);
  }
  installShellEntry();
  ensurePlaybackRuntime();
}(window));