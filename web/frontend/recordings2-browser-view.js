// DOM owner for the Recordings 2 folder and basic recording-detail views.
(function (global) {
  'use strict';

  const shared = global.VdrSuiteRecordings2Shared;
  const cards = global.VdrSuiteRecordings2Cards;
  const folderArtwork = global.VdrSuiteRecordings2FolderArtwork;
  const actions = global.VdrSuiteRecordings2Actions; let restartChoiceRuntimePromise = null;
  if (!shared || !cards) {
    console.error('VDR-Suite Recordings 2 shared runtime is unavailable');
    return;
  }

  function ensureRestartChoiceRuntime() { const ready = global.VdrSuiteRecordingPlaybackRestartChoice; if (ready && typeof ready.install === 'function') return Promise.resolve(ready); if (restartChoiceRuntimePromise) return restartChoiceRuntimePromise; if (typeof global.loadVdrSuiteDeferredRuntime !== 'function') return Promise.reject(new Error('Recording restart choice runtime loader is unavailable.')); restartChoiceRuntimePromise = global.loadVdrSuiteDeferredRuntime('vdr-suite-recording-playback-restart-choice-runtime', '/frontend/recording-playback-restart-choice.js', function () { return Boolean(global.VdrSuiteRecordingPlaybackRestartChoice && typeof global.VdrSuiteRecordingPlaybackRestartChoice.install === 'function'); }).then(function () { return global.VdrSuiteRecordingPlaybackRestartChoice; }); return restartChoiceRuntimePromise; }
  if (typeof global.loadVdrSuiteDeferredRuntime === 'function') ensureRestartChoiceRuntime().catch(function (error) { console.error('VDR-Suite Recording restart choice preload failed', error); });

  function create(options) {
    const getState = options.getState;
    const actionView = actions && typeof actions.create === 'function'
      ? actions.create({
          getState: getState,
          closeDetail: options.closeDetail,
          reload: options.reload,
          completeDelete: options.completeDelete
        })
      : null;
    let activePlayback = null; let detailStartRevealed = false;

    // Keep only a few detached, previously authorized folder DOM trees. This is
    // a view reuse optimization, not another data or artwork cache authority.
    const MAX_CACHED_FOLDERS = 3;
    const MAX_CACHED_RECORDINGS = 200;
    const folderViews = new Map();
    let mountedFolderView = null;
    let cachedBackend = null;

    function folderViewKey(value) {
      return String(value.backendId || 'default') + '\u0000' + shared.normalizePath(value.path);
    }

    function recordingIdentity(recording) {
      const backend = String(recording.backendId || state().backendId || 'default');
      const native = String(shared.first(recording, ['backendNativeId', 'nativePath', 'path'], ''));
      if (native) return backend + '\u0000native:' + native;
      const id = String(shared.first(recording, ['recordingId', 'id', 'nativeId'], ''));
      return id ? backend + '\u0000id:' + id : '';
    }

    function folderViewSignature(value) {
      // Include the rendered projection: new recordings, artwork URLs, metadata,
      // pagination, folder totals and cache status must all invalidate the view.
      return JSON.stringify([value.data, value.recordings, value.loadingMore]);
    }

    function rememberMountedFolder(target) {
      const previous = mountedFolderView;
      mountedFolderView = null;
      if (!previous || previous.recordingCount > MAX_CACHED_RECORDINGS ||
          (typeof target.contains === 'function' && !target.contains(previous.root))) return;
      previous.scrollTop = Number(target.scrollTop) || 0;
      previous.scrollY = Number(global.scrollY) || 0;
      folderViews.delete(previous.key);
      folderViews.set(previous.key, previous);
      while (folderViews.size > MAX_CACHED_FOLDERS) {
        folderViews.delete(folderViews.keys().next().value);
      }
    }

    function restoreFolderScroll(target, previous) {
      if (!previous) return;
      if (typeof target.scrollTop === 'number') target.scrollTop = previous.scrollTop || 0;
      if (previous.scrollY > 0 && typeof global.scrollTo === 'function') {
        global.scrollTo({top: previous.scrollY, left: Number(global.scrollX) || 0, behavior: 'auto'});
      }
    }

    function reusePoster(previousPosters, recording) {
      const identity = recordingIdentity(recording);
      if (!identity || !previousPosters) return null;
      const previous = previousPosters.get(identity);
      previousPosters.delete(identity);
      const url = shared.publicPath(shared.recordingPosterUrl(recording));
      if (!previous || !url || previous.url !== url) return null;
      const image = previous.node && typeof previous.node.querySelector === 'function'
        ? previous.node.querySelector('img') : null;
      if (!image) return null;
      image.alt = 'Poster zu ' + shared.recordingTitle(recording);
      previous.node.dataset.variant = String(shared.recordingPlaceholderVariant(recording));
      return previous.node;
    }


    function destroyPlayback() {
      const playback = activePlayback;
      activePlayback = null;
      if (!playback) return;
      // Navigation out of Playback is a user Stop, not just DOM teardown:
      // publish the final checkpoint while the MediaSession and position
      // are still available. destroy() then releases the local stream now.
      if (typeof playback.stop === 'function') {
        try {
          const stopped = playback.stop();
          if (stopped && typeof stopped.catch === 'function') {
            stopped.catch(function (error) {
              console.error('Recording playback Stop on navigation failed', error);
            });
          }
        } catch (error) {
          console.error('Recording playback Stop on navigation failed', error);
        }
      }
      if (typeof playback.destroy === 'function') playback.destroy();
    }

    function state() {
      return getState();
    }

    // Invalidate detached private DOM when the canonical browser session
    // becomes unauthenticated; authorization remains with existing API reads.
    const session = global.VdrSuiteBrowserSession;
    if (session && typeof session.subscribe === 'function') {
      session.subscribe(function (snapshot) {
        if (!snapshot || snapshot.authenticated !== true) {
          folderViews.clear();
          mountedFolderView = null;
          cachedBackend = null;
        }
      });
    }

    function createBreadcrumbs() {
      const currentState = state();
      const breadcrumbs = document.createElement('nav');
      breadcrumbs.className = 'recordings2-breadcrumbs';
      breadcrumbs.setAttribute('aria-label', 'Aufnahmeordner');
      breadcrumbs.appendChild(shared.createButton('Aufnahmen', function () { options.openFolder(''); }));
      const parts = shared.normalizePath(currentState.path).split('/').filter(Boolean);
      let current = '';
      parts.forEach(function (part) {
        current = current ? current + '/' + part : part;
        const target = current;
        const separator = shared.node('span', 'recordings2-separator', '›');
        breadcrumbs.appendChild(separator);
        breadcrumbs.appendChild(shared.createButton(
          shared.decodeDisplayText(part), function () { options.openFolder(target); }));
      });
      return breadcrumbs;
    }

    function createHeader() {
      const currentState = state();
      const header = document.createElement('header');
      header.className = 'recordings2-header';
      const heading = document.createElement('div');
      heading.className = 'recordings2-heading';
      const copy = document.createElement('div');
      copy.className = 'recordings2-heading-copy';
      copy.appendChild(shared.node('h3', '',
        currentState.selectedRecording ? 'Aufnahmedetails' : 'Aufnahmen'));
      copy.appendChild(shared.node('p', '',
        'Backend ' + (currentState.backendId || shared.selectedBackendId())));
      heading.appendChild(copy);
      header.appendChild(heading);

      const toolbar = document.createElement('div'); toolbar.className = 'recordings2-toolbar';
      if (currentState.selectedRecording) {
        toolbar.appendChild(shared.createButton(
          currentState.detailReturnLabel || '← Zum Ordner', options.closeDetail, 'recordings2-primary'));
        if (typeof options.goHome === 'function') toolbar.appendChild(shared.createButton(currentState.detailHomeLabel || '⌂ Home', options.goHome, 'recordings2-primary recordings2-home'));
      } else if (currentState.path) {
        toolbar.appendChild(shared.createButton('← Zurück',
          function () { options.openFolder(currentState.parentPath || ''); }, 'recordings2-primary'));
      }
      toolbar.appendChild(shared.createButton('Neu laden', options.reload));
      header.appendChild(toolbar);
      if (!currentState.selectedRecording) header.appendChild(createBreadcrumbs());
      return header;
    }

    function createStatus(title, message, error, technical) {
      const box = document.createElement('section');
      box.className = 'recordings2-status' + (error ? ' error' : '');
      box.setAttribute('role', error ? 'alert' : 'status');
      box.setAttribute('aria-live', 'polite');
      box.appendChild(shared.node('strong', '', title));
      if (message) box.appendChild(shared.node('p', '', message));
      if (technical) box.appendChild(shared.node('p', 'recordings2-status-detail', technical));
      return box;
    }

    function prepareTarget() {
      const target = shared.mountTarget();
      if (!target) return null;
      const backend = String(state().backendId || 'default');
      if (backend !== cachedBackend) {
        folderViews.clear();
        mountedFolderView = null;
        cachedBackend = backend;
      } else {
        rememberMountedFolder(target);
      }
      destroyPlayback();
      shared.installStyles();
      cards.installStyles();
      target.classList.add('recordings2-mount');
      target.replaceChildren();
      return target;
    }

    function createRoot(className) {
      const root = document.createElement('section');
      root.className = className || 'recordings2';
      root.appendChild(createHeader());
      return root;
    }

    function renderLoading() { detailStartRevealed = false;
      const currentState = state();
      // A same-folder reload keeps its visible posters until the server replies.
      if (mountedFolderView && cachedBackend === String(currentState.backendId || 'default') &&
          mountedFolderView.key === folderViewKey(currentState)) return;
      const target = prepareTarget();
      if (!target) return;
      const root = createRoot('recordings2');
      const status = createStatus('Aufnahmeordner wird geladen …',
        currentState.path
          ? 'Ordner: ' + shared.decodeDisplayText(currentState.path)
          : 'Lade den Hauptordner.',
        false, '/api/vdr/recordings/folder · Backend ' + currentState.backendId);
      const progress = document.createElement('progress');
      progress.className = 'recordings2-progress';
      progress.setAttribute('aria-label', 'Aufnahmeordner wird geladen');
      status.appendChild(progress);
      root.appendChild(status);
      target.appendChild(root);
    }

    function renderError() { detailStartRevealed = false;
      const currentState = state();
      const target = prepareTarget();
      if (!target) return;
      // A rejected folder read must not leave an authorization-unknown
      // view available for a later reuse.
      folderViews.clear();
      mountedFolderView = null;
      const root = createRoot('recordings2');
      const message = currentState.error && currentState.error.message
        ? currentState.error.message
        : String(currentState.error || 'Unbekannter Fehler');
      const status = createStatus('Aufnahmeordner konnte nicht geladen werden',
        message, true, 'Backend: ' + currentState.backendId +
          ' · Pfad: ' + (currentState.path || '<Hauptordner>') +
          ' · Endpunkt: /api/vdr/recordings/folder');
      status.appendChild(shared.createButton('Erneut versuchen', options.reload, 'recordings2-primary'));
      root.appendChild(status);
      target.appendChild(root);
    }

    function createMetric(value, label) {
      const metric = document.createElement('div');
      metric.className = 'recordings2-metric';
      metric.appendChild(shared.node('strong', '', String(value)));
      metric.appendChild(shared.node('span', '', label));
      return metric;
    }

    function createFolderCard(folder) {
      const button = document.createElement('button');
      button.type = 'button';
      button.className = 'recordings2-folder';
      const artwork = folderArtwork && typeof folderArtwork.create === 'function'
        ? folderArtwork.create(folder)
        : null;
      const icon = artwork || shared.node('span', 'recordings2-folder-icon', '📁');
      if (artwork) button.classList.add('has-genre-artwork');
      const copy = document.createElement('span');
      copy.className = 'recordings2-folder-copy';
      copy.appendChild(shared.node('span', 'recordings2-folder-name',
        shared.decodeDisplayText(shared.first(folder, ['name'], 'Ordner'))));
      copy.appendChild(shared.node('span', 'recordings2-folder-meta',
        String(shared.number(folder.recordingCount, 0)) + ' Aufnahme(n) · antippen zum Öffnen'));
      button.append(icon, copy, shared.node('span', 'recordings2-chevron', '›'));
      button.addEventListener('click', function () {
        options.openFolder(shared.normalizePath(shared.first(folder, ['path'], '')));
      });
      return button;
    }

    function createSection(title, count, className) {
      const section = document.createElement('section');
      section.className = 'recordings2-section';
      const heading = document.createElement('div');
      heading.className = 'recordings2-section-title';
      heading.appendChild(shared.node('h4', '', title));
      heading.appendChild(shared.node('span', 'recordings2-count', String(count)));
      section.appendChild(heading);
      const list = document.createElement('div');
      list.className = className;
      section.appendChild(list);
      return {section: section, list: list};
    }

    function renderFolder() { detailStartRevealed = false;
      const currentState = state();
      const key = folderViewKey(currentState);
      const signature = folderViewSignature(currentState);
      const target = prepareTarget();
      if (!target) return;
      const previous = folderViews.get(key);
      folderViews.delete(key);
      if (previous && previous.signature === signature) {
        target.appendChild(previous.root);
        mountedFolderView = previous;
        restoreFolderScroll(target, previous);
        return;
      }
      const oldPosters = previous ? new Map(previous.posters) : null;
      const posters = new Map();
      const root = createRoot('recordings2');
      const data = currentState.data || {};
      const folders = shared.folderList(data);
      const recordings = currentState.recordings;
      const summary = document.createElement('section');
      summary.className = 'recordings2-summary';
      summary.appendChild(createMetric(
        shared.number(data.totalCount, folders.length + recordings.length),
        'Gesamt'
      ));
      summary.appendChild(createMetric(shared.number(data.folderCount, folders.length), 'Ordner'));
      summary.appendChild(createMetric(shared.number(data.recordingCount, recordings.length), 'Aufnahmen'));
      root.appendChild(summary);

      if (shared.text(data.cacheState) && data.cacheReady === false) {
        root.appendChild(createStatus(
          'Recording-Cache wird vorbereitet',
          'Die bereits verfügbaren Einträge werden angezeigt. Ein erneutes Laden aktualisiert den Stand.',
          false,
          'Cache-Zustand: ' + shared.text(data.cacheState)
        ));
      }

      if (folders.length) {
        const folderSection = createSection('Ordner', folders.length, 'recordings2-folder-list');
        folders.forEach(function (folder) {
          folderSection.list.appendChild(createFolderCard(folder));
        });
        root.appendChild(folderSection.section);
      }

      if (recordings.length) {
        const recordingSection = createSection(
          'Aufnahmen',
          shared.number(data.recordingCount, recordings.length),
          'recordings2-recording-list'
        );
        recordings.forEach(function (recording) {
          recordingSection.list.appendChild(cards.withActions(recording, {
            getState: getState, selectRecording: options.selectRecording,
            createPoster: function (entry) {
              const poster = reusePoster(oldPosters, entry) || shared.createPoster(entry);
              const identity = recordingIdentity(entry);
              if (identity) posters.set(identity, {
                node: poster, url: shared.publicPath(shared.recordingPosterUrl(entry))
              });
              return poster;
            }
          }));
        });
        root.appendChild(recordingSection.section);
      }

      if (!folders.length && !recordings.length) {
        root.appendChild(shared.node('section', 'recordings2-empty', 'Dieser Aufnahmeordner ist leer.'));
      }

      const returned = shared.number(data.returnedCount, shared.recordingList(data).length);
      const totalRecordings = shared.number(data.recordingCount, recordings.length);
      if (recordings.length < totalRecordings && returned > 0) {
        const more = shared.createButton(
          currentState.loadingMore
            ? 'Weitere Aufnahmen werden geladen …'
            : 'Weitere Aufnahmen laden',
          options.loadMore,
          'recordings2-more'
        );
        more.disabled = currentState.loadingMore;
        root.appendChild(more);
      }
      target.appendChild(root);
      mountedFolderView = {
        key: key, signature: signature, root: root, posters: posters,
        recordingCount: recordings.length, scrollTop: 0, scrollY: 0
      };
      restoreFolderScroll(target, previous);
    }

    function detailField(label, value) {
      const field = document.createElement('div');
      field.className = 'recordings2-detail-field';
      field.appendChild(shared.node('span', '', label));
      field.appendChild(shared.node('strong', '', shared.text(value) || '–'));
      return field;
    }

    function renderDetail() {
      const currentState = state();
      const target = prepareTarget();
      if (!target || !currentState.selectedRecording) return;
      const root = createRoot('recordings2 recordings2-detail');
      const recording = currentState.selectedRecording;
      const hero = document.createElement('article');
      hero.className = 'recordings2-detail-hero';
      hero.appendChild(shared.createPoster(recording, 'recordings2-poster recordings2-detail-poster'));
      const copy = document.createElement('div');
      copy.className = 'recordings2-detail-copy';
      copy.appendChild(shared.node('h3', '', shared.recordingTitle(recording)));
      const subtitle = shared.recordingSubtitle(recording);
      if (subtitle) copy.appendChild(shared.node('p', 'recordings2-subtitle', subtitle));
      copy.appendChild(shared.node(
        'p',
        'recordings2-detail-description',
        shared.recordingSummary(recording) || 'Keine Beschreibung vorhanden.'
      ));
      hero.appendChild(copy);
      const recordingPanel = document.createElement('section');
      recordingPanel.className = 'recordings2-metadata-panel recordings2-hero-recording-panel';
      recordingPanel.appendChild(hero);

      const provider = shared.provider(recording);
      const details = document.createElement('section');
      details.className = 'recordings2-detail-grid';
      details.appendChild(detailField('Aufgenommen', shared.formatStart(shared.first(recording, ['startTime', 'start'], ''))));
      details.appendChild(detailField('Dauer', shared.formatDuration(shared.first(recording, ['durationSeconds', 'duration'], 0))));
      details.appendChild(detailField('Größe', shared.formatSize(shared.first(recording, ['sizeMb'], 0))));
      details.appendChild(detailField('Ordner', shared.first(recording, ['path'], currentState.path || 'Hauptordner')));
      details.appendChild(detailField('Genre', shared.first(provider, ['genreText'], '')));
      details.appendChild(detailField('Veröffentlichung', shared.first(provider, ['releaseDate'], '')));
      details.appendChild(detailField(
        'Bewertung',
        shared.number(shared.first(provider, ['rating'], 0), 0) > 0
          ? String(shared.first(provider, ['rating'], 0))
          : ''
      ));
      details.appendChild(detailField('Metadatenquelle', shared.first(provider, ['source'], 'VDR')));
      recordingPanel.appendChild(details);
      root.appendChild(recordingPanel);

      const playback = global.VdrSuiteRecordings2Playback; if (playback && typeof playback.createPanel === 'function') {
        activePlayback = playback.createPanel(recording, currentState.backendId);
        if (activePlayback && activePlayback.element) {
          root.__vdrSuiteRecordingPlaybackOwner = activePlayback;
          root.appendChild(activePlayback.element);
          const ownedPlayback = activePlayback;
          root.__vdrSuiteRecordingPlaybackExit = function () {
            if (activePlayback !== ownedPlayback) return false;
            // prepareTarget() destroys the current playback and builds a
            // fresh owner, rather than leaving a hidden stream or dead controls.
            renderDetail();
            return true;
          };
          ensureRestartChoiceRuntime().then(function (helper) { if (activePlayback === ownedPlayback && helper && typeof helper.install === 'function') helper.install(ownedPlayback); }).catch(function (error) { console.error('VDR-Suite Recording restart choice runtime failed', error); });
        }
      }

      if (actionView && typeof actionView.createPanel === 'function') {
        root.appendChild(actionView.createPanel(recording));
      }

      target.appendChild(root); if (!detailStartRevealed && typeof root.scrollIntoView === 'function') { detailStartRevealed = true; root.scrollIntoView({block: 'start', behavior: 'auto'}); }

      const metadataDetail = global.VdrSuiteRecordings2MetadataDetail;
      if (metadataDetail && typeof metadataDetail.enhance === 'function') {
        metadataDetail.enhance(root, recording, currentState.backendId);
      } else {
        // Metadata is a deferred addon. The first frame is already the Hero,
        // even when the addon or TVScraper has not arrived.
        const heroDetail = global.VdrSuiteRecordings2HeroDetail;
        if (heroDetail && typeof heroDetail.enhance === 'function')
          heroDetail.enhance(root, recording, currentState.backendId);
      }
    }

    return Object.freeze({
      renderLoading: renderLoading,
      renderError: renderError,
      renderFolder: renderFolder,
      renderDetail: renderDetail,
      destroy: function () {
        destroyPlayback();
        folderViews.clear();
        mountedFolderView = null;
        cachedBackend = null;
        detailStartRevealed = false;
      }
    });
  }

  global.VdrSuiteRecordings2BrowserView = Object.freeze({create: create, createRecordingCard: cards.createRecordingCard});
}(window));
