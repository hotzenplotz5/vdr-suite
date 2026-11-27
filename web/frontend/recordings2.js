(function (global) {
  'use strict';
  const shared = global.VdrSuiteRecordings2Shared;
  const folderArtwork = global.VdrSuiteRecordings2FolderArtwork;
  const browserView = global.VdrSuiteRecordings2BrowserView;
  const refreshRuntime = global.VdrSuiteRecordings2FolderRefresh;
  if (!shared || !browserView || typeof browserView.create !== 'function' ||
      !refreshRuntime || typeof refreshRuntime.create !== 'function') {
    console.error('VDR-Suite Recordings 2 runtime dependencies are unavailable');
    return;
  }
  const state = {
    active: false,
    backendId: '',
    path: '',
    parentPath: '',
    folderId: null,
    parentFolderId: '',
    breadcrumbs: [],
    legacyFolderPath: undefined,
    data: null,
    serverRecordings: [],
    promotedRecordings: [],
    recordings: [],
    serverRecordingCount: 0,
    serverSignature: '',
    selectedRecording: null,
    detailReturn: null, detailHome: null,
    detailReturnLabel: '', detailHomeLabel: '',
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
  function normalizeRecordings(recordings) { return (Array.isArray(recordings) ? recordings : []).map(normalizeRecording); }
  function render() {
    if (!state.active || !view) return;
    if (state.loading) return view.renderLoading();
    if (state.error) return view.renderError();
    if (state.selectedRecording) return view.renderDetail();
    view.renderFolder();
  }
  function installPlaybackPipUi() { if (playbackPipUiBound || typeof document === 'undefined' || typeof document.addEventListener !== 'function') return; playbackPipUiBound = true; const mini = function () { return typeof document.getElementById === 'function' ? document.getElementById('vdr-suite-live-mini-player') : null; }; document.addEventListener('enterpictureinpicture', function (event) { const root = mini(); const video = event && event.target; if (!root || !video || typeof root.contains !== 'function') return; if (!root.contains(video)) return; if (root.dataset) root.dataset.vdrSuitePipSuppressed = 'true'; root.hidden = true; }, true); document.addEventListener('leavepictureinpicture', function (event) { const root = mini(); if (!root || !root.dataset || root.dataset.vdrSuitePipSuppressed !== 'true') return; delete root.dataset.vdrSuitePipSuppressed; const video = event && event.target; if (video && typeof root.contains === 'function' && root.contains(video)) root.hidden = false; }, true); }
  function installPlaybackShell() { const shell = global.VdrSuitePlaybackShell; if (shell && typeof shell.install === 'function') shell.install(); installPlaybackPipUi(); }
  function recordingPlaybackOwnerReady() { return Boolean(global.VdrSuiteRecordingFastPlayback && global.VdrSuiteRecordings2Playback && typeof global.VdrSuiteRecordings2Playback.createPanel === 'function'); }
  function ensurePlaybackRuntime() {
    if (recordingPlaybackOwnerReady()) { installPlaybackShell(); return Promise.resolve(); }
    if (playbackRuntimePromise) return playbackRuntimePromise;
    if (typeof global.loadVdrSuiteDeferredRuntime !== 'function') return Promise.resolve();
    playbackRuntimePromise = global.loadVdrSuiteDeferredRuntime('vdr-suite-session-frontend-sync-runtime',
      '/frontend/api/session-frontend-sync.js', function () { return Boolean(global.VdrSuiteRecordingFastPlayback && global.VdrSuiteLivePlayback); }
    ).then(function () { if (recordingPlaybackOwnerReady()) return null; return global.loadVdrSuiteDeferredRuntime('vdr-suite-recordings2-playback-runtime',
      '/frontend/recordings2-playback.js', function () { return Boolean(global.VdrSuiteRecordings2Playback && typeof global.VdrSuiteRecordings2Playback.createPanel === 'function'); }
    ); }).then(function () {
      if (!recordingPlaybackOwnerReady()) throw new Error('Canonical Recording playback owner ist nicht verfügbar.'); installPlaybackShell(); if (state.active && state.selectedRecording) render();
    }).catch(function (error) { console.error('VDR-Suite Recordings 2 playback runtime failed', error); });
    return playbackRuntimePromise;
  }
  const folderRefresh = refreshRuntime.create({
    getState: function () { return state; },
    fetchClientRecordingFolder: function (options) {
      const api = shared.clientApi();
      return api && typeof api.fetchClientRecordingFolder === 'function'
        ? api.fetchClientRecordingFolder(options)
        : Promise.reject(new Error('Client API für Aufnahmeordner ist nicht verfügbar.'));
    },
    fetchPublicRecordingFolder: function (options) {
      const api = shared.clientApi();
      return api && typeof api.fetchPublicRecordingFolder === 'function'
        ? api.fetchPublicRecordingFolder(options) : null;
    },
    pageSize: shared.PAGE_SIZE,
    normalizePath: shared.normalizePath,
    number: shared.number,
    shared: shared,
    folderArtwork: folderArtwork,
    normalizeRecordings: normalizeRecordings,
    applyFolderData: applyFolderData,
    render: render,
    loadFolder: loadFolder,
    onRecordingMarksChanged: function () { const detail = global.VdrSuiteRecordings2MarksDetail; const target = shared.mountTarget(); const root = target && typeof target.querySelector === 'function' ? target.querySelector('.recordings2-detail') : null; if (detail && typeof detail.notifyExternalMarksChanged === 'function' && root) detail.notifyExternalMarksChanged(root); },
    onRecordingPresentationChanged: function () {
      // An open detail owns playback: update only metadata and its poster,
      // never tear down/recreate the Recording or video session.
      const detail = global.VdrSuiteRecordings2MetadataDetail;
      const target = shared.mountTarget();
      const root = target && typeof target.querySelector === 'function'
        ? target.querySelector('.recordings2-detail') : null;
      if (state.active && state.selectedRecording && root &&
          detail && typeof detail.refresh === 'function')
        detail.refresh(root, state.selectedRecording, state.backendId);
    }
  });
  const {requestFolder, resolveLeaves: resolveSingleRecordingLeaves, stop: stopFolderRefresh,
    schedule: scheduleFolderRefresh, updatePresentedFolderState, forgetRecording} = folderRefresh;
  function applyFolderData(data, append) {
    if (!data || data.recordingFolder !== true) {
      throw new Error('Der Server hat keinen gültigen Aufnahmeordner geliefert.');
    }
    const publicPage = data.publicFolder === true;
    const previousFolders = append && state.data
      ? shared.folderList(state.data).slice()
      : shared.folderList(data).slice();
    if (append && publicPage) {
      if (!state.data || state.data.publicFolder !== true ||
          data.folderId !== state.folderId || data.offset !== state.data.nextOffset)
        throw new Error('Der Aufnahmeordner hat sich geändert. Bitte neu laden.');
      const folderIds = new Set(previousFolders.map(folder => folder.folderId));
      const recordingIds = new Set(state.serverRecordings.map(recording => recording.recordingId));
      if (shared.folderList(data).some(folder=>folderIds.has(folder.folderId)) ||
          shared.recordingList(data).some(recording=>recordingIds.has(recording.recordingId)))
        throw new Error('Der Aufnahmeordner hat sich geändert. Bitte neu laden.');
      previousFolders.push(...shared.folderList(data));
    }
    const incomingRecordings = normalizeRecordings(shared.recordingList(data));
    state.data = append
      ? Object.assign({}, state.data || {}, data, {folders: previousFolders})
      : Object.assign({}, data, {folders: previousFolders});
    state.folderId = publicPage ? data.folderId : null;
    state.parentFolderId = publicPage ? data.parentFolderId : '';
    state.breadcrumbs = publicPage ? data.breadcrumbs : [];
    state.path = publicPage ? '' : shared.normalizePath(shared.first(data, ['path'], state.path));
    state.parentPath = publicPage ? '' : shared.normalizePath(shared.first(data, ['parentPath'], ''));
    state.serverRecordingCount = shared.number(data.recordingCount, incomingRecordings.length);
    state.serverRecordings = append
      ? state.serverRecordings.concat(incomingRecordings)
      : incomingRecordings;
    state.serverSignature = append ? '' : folderRefresh.signature(data);
    if (!append) state.promotedRecordings = [];
    updatePresentedFolderState();
  }
  function clearExternalDetailReturn() { state.detailReturn = null; state.detailReturnLabel = ''; state.detailHome = null; state.detailHomeLabel = ''; }
  function folderTarget() {
    return state.folderId !== null ? {folderId:state.folderId, legacyPath:state.legacyFolderPath} : state.path || '';
  }
  function loadFolder(path) {
    stopFolderRefresh();
    state.active = true;
    state.backendId = shared.selectedBackendId();
    const opaque = path && typeof path === 'object' && typeof path.folderId === 'string';
    state.folderId = opaque ? path.folderId : null;
    state.parentFolderId = '';
    state.breadcrumbs = [];
    state.legacyFolderPath = opaque ? path.legacyPath : undefined;
    state.path = opaque ? '' : shared.normalizePath(path);
    state.parentPath = state.path.split('/').slice(0, -1).join('/');
    state.selectedRecording = null;
    clearExternalDetailReturn();
    state.loading = true;
    state.loadingMore = false;
    state.error = null;
    const sequence = ++state.requestSequence;
    const current = function () {
      return state.active && sequence === state.requestSequence;
    };
    render();
    requestFolder(folderTarget(), 0)
      .then(function (data) {
        if (!current()) return null;
        applyFolderData(data, false);
        return resolveSingleRecordingLeaves(data, current);
      })
      .then(function () {
        if (!current()) return;
        state.loading = false;
        render();
        scheduleFolderRefresh();
      })
      .catch(function (error) {
        if (!current()) return;
        state.loading = false;
        state.error = error;
        render();
        scheduleFolderRefresh();
      });
  }
  function loadMore() {
    if (state.loadingMore || state.loading) return;
    state.loadingMore = true;
    const sequence = ++state.requestSequence;
    render();
    requestFolder(folderTarget(), state.data && state.data.publicFolder === true
      ? state.data.nextOffset : state.serverRecordings.length)
      .then(function (data) {
        if (!state.active || sequence !== state.requestSequence) return;
        applyFolderData(data, true);
        state.loadingMore = false;
        render();
        scheduleFolderRefresh();
      })
      .catch(function (error) {
        if (!state.active || sequence !== state.requestSequence) return;
        state.loadingMore = false;
        state.error = error;
        render();
        scheduleFolderRefresh();
      });
  }
  function selectRecording(recording) {
    const sequence = ++state.requestSequence;
    clearExternalDetailReturn();
    const id = recording && recording.folderCard === true && recording.recordingId;
    if (/^rec_[0-9a-f]{32}$/.test(id || '')) {
      // Only this explicit selection needs the still-private detail bridge.
      // The list remains public and no per-card detail requests are made.
      const api = shared.clientApi();
      if (!api || typeof api.fetchClientRecordingForPublicId !== 'function') {
        state.error = new Error('Aufnahmedetails sind nicht verfügbar.');
        render();
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
          state.selectedRecording = normalizeRecording(Object.assign({}, detail.recording,
            recording.__vdrSuiteOpenHevcShrink ? {__vdrSuiteOpenHevcShrink:true} : {}));
          render();
          scheduleFolderRefresh();
        }).catch(function (error) {
          if (!state.active || sequence !== state.requestSequence) return;
          state.loading = false;
          state.error = error;
          render();
        });
      return;
    }
    state.selectedRecording = normalizeRecording(recording);
    render();
    scheduleFolderRefresh();
  }
  function leaveExternalDetail(callback) { if (view && typeof view.destroy === 'function') view.destroy(); state.requestSequence += 1; state.selectedRecording = null; clearExternalDetailReturn(); if (typeof callback !== 'function') return false; stopFolderRefresh(); state.active = false; callback(); return true; }
  function closeDetail(changed) { const detailReturn = state.detailReturn; if (leaveExternalDetail(typeof detailReturn === 'function' ? function () { detailReturn(changed === true); } : null)) return; render(); scheduleFolderRefresh(0); }
  function goHomeDetail() { const detailHome = state.detailHome; if (leaveExternalDetail(detailHome)) return; leaveExternalDetail(function () { if (typeof global.selectModule === 'function') global.selectModule('overview'); if (typeof global.scrollTo === 'function') global.scrollTo({top: 0, left: 0, behavior: 'auto'}); }); }
  function reload() {
    if (state.selectedRecording && state.detailReturn) {
      render();
      return;
    }
    if (state.selectedRecording) state.selectedRecording = null;
    loadFolder(folderTarget());
  }
  view = browserView.create({
    getState: function () { return state; },
    openFolder: loadFolder, loadMore: loadMore, selectRecording: selectRecording,
    closeDetail: closeDetail, goHome: goHomeDetail, reload: reload,
    completeAction: function () { if (typeof state.detailReturn === 'function') return closeDetail(true); closeDetail(); reload(); },
    completeDelete: function (recording) { forgetRecording(recording); if (typeof state.detailHome === 'function') return goHomeDetail(); closeDetail(true); }
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
      scheduleFolderRefresh(0);
    },
    deactivate: function () {
      stopFolderRefresh();
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
      loadFolder(folderTarget());
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
      state.detailHome = typeof config.onHome === 'function' ? config.onHome : null; state.detailHomeLabel = config.homeLabel || '⌂ Home';
      render();
      if (config.focusNavigation === true && typeof global.setTimeout === 'function') global.setTimeout(function () { const target = shared.mountTarget(); const control = target && typeof target.querySelector === 'function' ? target.querySelector('.recordings2-toolbar button') : null; if (control && typeof control.focus === 'function') control.focus({preventScroll: true}); }, 0);
      scheduleFolderRefresh();
    },
    refreshDetailAddon: function () { if (!state.active || !state.selectedRecording) return; const metadataDetail = global.VdrSuiteRecordings2MetadataDetail; const target = shared.mountTarget(); const root = target && typeof target.querySelector === 'function' ? target.querySelector('.recordings2-detail') : null; if (metadataDetail && typeof metadataDetail.enhance === 'function' && root && root.dataset && root.dataset.recordings2MetadataDetail !== 'true') { metadataDetail.enhance(root, state.selectedRecording, state.backendId); return; } render(); },
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
    if (global.document && typeof global.document.addEventListener === 'function') {
      global.document.addEventListener('visibilitychange', function () {
        if (state.active && !state.selectedRecording && !global.document.hidden) {
          scheduleFolderRefresh(0);
        }
      });
    }
  }
  global.VdrSuiteRecordings2 = moduleApi;
  const boundary = shared.platform();
  if (boundary && typeof boundary.registerModule === 'function' &&
      (!boundary.hasModule || !boundary.hasModule('recordings2'))) {
    boundary.registerModule('recordings2', moduleApi);
  }
  installShellEntry();
  const selectedTab = document.querySelector('[data-module="recordings2"]');
  if (selectedTab && selectedTab.classList.contains('active')) moduleApi.activate();
  ensurePlaybackRuntime();
}(window));
