// Independent mobile-first recording browser runtime and state owner.
(function (global) {
  'use strict';

  const shared = global.VdrSuiteRecordings2Shared;
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
    recordings: [],
    selectedRecording: null,
    loading: false,
    loadingMore: false,
    error: null,
    requestSequence: 0
  };

  let view;

  function render() {
    if (!state.active || !view) return;
    if (state.loading) return view.renderLoading();
    if (state.error) return view.renderError();
    if (state.selectedRecording) return view.renderDetail();
    view.renderFolder();
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

  function applyFolderData(data, append) {
    if (!data || data.recordingFolder !== true) {
      throw new Error('Der Server hat keinen gültigen Aufnahmeordner geliefert.');
    }
    state.data = append ? Object.assign({}, state.data || {}, data) : data;
    state.path = shared.normalizePath(shared.first(data, ['path'], state.path));
    state.parentPath = shared.normalizePath(shared.first(data, ['parentPath'], ''));
    state.recordings = append
      ? state.recordings.concat(shared.recordingList(data))
      : shared.recordingList(data).slice();
  }

  function loadFolder(path) {
    state.active = true;
    state.backendId = shared.selectedBackendId();
    state.path = shared.normalizePath(path);
    state.parentPath = state.path.split('/').slice(0, -1).join('/');
    state.selectedRecording = null;
    state.loading = true;
    state.loadingMore = false;
    state.error = null;
    const sequence = ++state.requestSequence;
    render();
    requestFolder(state.path, 0)
      .then(function (data) {
        if (!state.active || sequence !== state.requestSequence) return;
        applyFolderData(data, false);
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
    requestFolder(state.path, state.recordings.length)
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
    state.selectedRecording = recording;
    render();
  }

  function closeDetail() {
    state.selectedRecording = null;
    render();
  }

  function reload() {
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
      state.active = false;
      state.requestSequence += 1;
      state.selectedRecording = null;
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
    refreshDetailAddon: function () {
      if (state.active && state.selectedRecording) render();
    },
    __test: Object.freeze({
      normalizePath: shared.normalizePath,
      decodeDisplayText: shared.decodeDisplayText,
      recordingTitle: shared.recordingTitle,
      recordingSubtitle: shared.recordingSubtitle,
      recordingSummary: shared.recordingSummary,
      recordingPosterUrl: shared.recordingPosterUrl,
      formatDuration: shared.formatDuration,
      formatSize: shared.formatSize,
      applyFolderData: applyFolderData
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
}(window));
