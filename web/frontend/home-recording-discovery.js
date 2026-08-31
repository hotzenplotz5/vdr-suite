(function (global) {
  'use strict';

  if (!global || global.VdrSuiteHomeRecordingDiscovery) return;

  const doc = global.document;
  const NEW_LIMIT = 12;
  const GENRE_LIMIT = 12;
  const SERIES_LIMIT = 12;
  const FOLDER_LIMIT = 12;
  const state = {
    generation: 0,
    loadedBackendId: '',
    observer: null,
    armed: false
  };

  function text(value) {
    return value === undefined || value === null ? '' : String(value).trim();
  }

  function list(value, key) {
    if (Array.isArray(value)) return value;
    if (value && Array.isArray(value[key])) return value[key];
    if (value && Array.isArray(value.items)) return value.items;
    return [];
  }

  function platform() {
    return global.VdrSuitePlatform || null;
  }

  function clientApi() {
    const owner = platform();
    if (owner && typeof owner.getClientApi === 'function') {
      const client = owner.getClientApi();
      if (client) return client;
    }
    return global.VdrSuiteClientApi || null;
  }

  function selectedBackendId() {
    const owner = platform();
    if (owner && typeof owner.getSelectedBackendId === 'function') {
      return text(owner.getSelectedBackendId()) || 'default';
    }
    const selected = doc && doc.querySelector
      ? doc.querySelector('#backends .backend-card.selected, #backends [aria-selected="true"]')
      : null;
    return text(selected && selected.dataset && selected.dataset.backendId) || 'default';
  }

  function homeIsActive() {
    const owner = platform();
    if (owner && typeof owner.getSelectedModule === 'function') {
      return text(owner.getSelectedModule()) === 'overview';
    }
    const active = doc && doc.querySelector
      ? doc.querySelector('.module-tab.active[data-module="overview"]')
      : null;
    return Boolean(active);
  }

  function host() {
    return doc && doc.querySelector
      ? doc.querySelector('[data-home-zone="additional-sections"]')
      : null;
  }

  function publicPath(path) {
    const value = text(path);
    const resolver = global.VdrSuitePublicUrl;
    return value && resolver && typeof resolver.resolvePath === 'function'
      ? resolver.resolvePath(value)
      : value;
  }

  function metadata(recording) {
    return recording && recording.metadata && typeof recording.metadata === 'object'
      ? recording.metadata
      : {};
  }

  function presentation(recording) {
    const value = metadata(recording).presentation;
    return value && typeof value === 'object' ? value : {};
  }

  function provider(recording) {
    const value = metadata(recording).provider;
    return value && typeof value === 'object' ? value : {};
  }

  function recordingId(recording) {
    return text(recording && (recording.recordingId || recording.id));
  }

  function recordingBackendId(recording, fallback) {
    return text(recording && recording.backendId) || text(fallback);
  }

  function recordingTitle(recording) {
    return text(
      presentation(recording).title ||
      provider(recording).seriesTitle ||
      provider(recording).title ||
      (recording && recording.title)
    ) || 'Aufnahme';
  }

  function recordingSubtitle(recording) {
    return text(
      presentation(recording).subtitle ||
      provider(recording).episodeTitle
    );
  }

  function recordingPosterUrl(recording) {
    const meta = metadata(recording);
    const artwork = meta.artwork && typeof meta.artwork === 'object' ? meta.artwork : {};
    return text(presentation(recording).posterUrl || artwork.preferredUrl);
  }

  function canonicalRecordings(payload, backendId) {
    return list(payload, 'recordings').filter(function (recording) {
      const id = recordingId(recording);
      const itemBackend = recordingBackendId(recording, backendId);
      return Boolean(id && itemBackend && itemBackend === backendId);
    });
  }

  function canonicalGenres(payload) {
    return list(payload, 'genres').filter(function (entry) {
      return Boolean(entry && text(entry.id) && Number(entry.count || 0) > 0);
    });
  }

  function canonicalFolders(payload) {
    return list(payload, 'folders').filter(function (entry) {
      return Boolean(entry && text(entry.path || entry.folderPath || entry.name));
    });
  }

  function sectionFor(key) {
    const target = host();
    if (!target) return null;
    let section = target.querySelector
      ? target.querySelector('[data-home-discovery-rail="' + key + '"]')
      : null;
    if (!section) {
      section = doc.createElement('section');
      section.className = 'media-home-discovery';
      section.setAttribute('data-home-discovery-rail', key);
      target.appendChild(section);
    }
    return section;
  }

  function clearRail(key) {
    const target = host();
    const section = target && target.querySelector
      ? target.querySelector('[data-home-discovery-rail="' + key + '"]')
      : null;
    if (section && typeof section.remove === 'function') section.remove();
  }

  function renderState(key, title, message, error) {
    const section = sectionFor(key);
    if (!section) return false;
    section.replaceChildren();
    const heading = doc.createElement('div');
    heading.className = 'media-home-section-heading';
    const name = doc.createElement('h3');
    name.textContent = title;
    heading.appendChild(name);
    section.appendChild(heading);
    const status = doc.createElement('div');
    status.className = 'media-home-discovery-state' + (error ? ' error' : '');
    status.textContent = message;
    section.appendChild(status);
    return true;
  }

  function createArtwork(recording) {
    const artwork = doc.createElement('div');
    artwork.className = 'media-home-discovery-artwork';
    const fallback = recordingTitle(recording).slice(0, 1).toUpperCase() || '▶';
    const url = recordingPosterUrl(recording);
    if (!url) {
      artwork.textContent = fallback;
      return artwork;
    }
    const image = doc.createElement('img');
    image.src = publicPath(url);
    image.alt = 'Poster zu ' + recordingTitle(recording);
    image.loading = 'lazy';
    image.addEventListener('error', function () {
      image.remove();
      artwork.textContent = fallback;
    });
    artwork.appendChild(image);
    return artwork;
  }

  function releasePreview(reason) {
    const preview = global.VdrSuiteHomeLivePreview;
    if (preview && typeof preview.cancel === 'function') {
      preview.cancel(reason || 'Recording Discovery geöffnet');
    }
  }

  function ensureRecordings2() {
    if (global.VdrSuiteRecordings2 &&
        typeof global.VdrSuiteRecordings2.openRecording === 'function' &&
        typeof global.VdrSuiteRecordings2.openFolder === 'function') {
      return Promise.resolve(true);
    }
    const runtimes = global.VdrSuiteDeferredFrontendRuntimes;
    if (!runtimes || typeof runtimes.loadRecordings2 !== 'function') {
      return Promise.resolve(false);
    }
    return Promise.resolve(runtimes.loadRecordings2()).then(function () {
      return Boolean(global.VdrSuiteRecordings2);
    }).catch(function () {
      return false;
    });
  }

  function ensureGenres() {
    const runtimes = global.VdrSuiteDeferredFrontendRuntimes;
    if (global.VdrSuiteGenres &&
        typeof global.VdrSuiteGenres.openRecordingGenre === 'function') {
      return Promise.resolve(true);
    }
    if (!runtimes || typeof runtimes.loadGenres !== 'function') {
      return Promise.resolve(false);
    }
    return Promise.resolve(runtimes.loadGenres()).then(function () {
      return Boolean(global.VdrSuiteGenres &&
        typeof global.VdrSuiteGenres.openRecordingGenre === 'function');
    }).catch(function () {
      return false;
    });
  }

  function selectShellModule(moduleName) {
    if (typeof global.selectModule !== 'function') return false;
    global.selectModule(moduleName);
    return true;
  }

  function returnHome() {
    selectShellModule('overview');
    if (typeof global.setTimeout === 'function') global.setTimeout(refresh, 0);
  }

  function openRecording(recording, backendId) {
    const id = recordingId(recording);
    if (!id) return Promise.resolve(false);
    const scopedBackend = recordingBackendId(recording, backendId);
    if (!scopedBackend) return Promise.resolve(false);
    releasePreview('Recording Discovery Aufnahme geöffnet');
    return ensureRecordings2().then(function (ready) {
      if (!ready || !selectShellModule('recordings2')) return false;
      global.VdrSuiteRecordings2.openRecording(recording, {
        backendId: scopedBackend,
        backLabel: '← Zurück zu Home',
        onClose: returnHome
      });
      return true;
    });
  }

  function openFolder(folder, backendId) {
    const path = text(folder && (folder.path || folder.folderPath || folder.name));
    if (!path) return Promise.resolve(false);
    releasePreview('Recording Discovery Ordner geöffnet');
    return ensureRecordings2().then(function (ready) {
      if (!ready || !selectShellModule('recordings2')) return false;
      global.VdrSuiteRecordings2.openFolder(path);
      return true;
    });
  }

  function openGenre(entry, backendId) {
    const id = text(entry && entry.id);
    if (!id || !backendId) return Promise.resolve(false);
    releasePreview('Recording Discovery Genre geöffnet');
    return ensureGenres().then(function (ready) {
      if (!ready || !selectShellModule('genres')) return false;
      return Promise.resolve(global.VdrSuiteGenres.openRecordingGenre(entry, {
        backendId: backendId
      })).then(function () { return true; });
    });
  }

  function renderRecordingRail(key, title, recordings, backendId) {
    if (!recordings.length) {
      clearRail(key);
      return true;
    }
    const section = sectionFor(key);
    if (!section) return false;
    section.replaceChildren();
    const heading = doc.createElement('div');
    heading.className = 'media-home-section-heading';
    const name = doc.createElement('h3');
    name.textContent = title;
    heading.appendChild(name);
    section.appendChild(heading);
    const rail = doc.createElement('div');
    rail.className = 'media-home-discovery-rail';
    recordings.forEach(function (recording) {
      const card = doc.createElement('button');
      card.type = 'button';
      card.className = 'media-home-discovery-card recording';
      card.dataset.recordingId = recordingId(recording);
      card.dataset.backendId = recordingBackendId(recording, backendId);
      card.appendChild(createArtwork(recording));
      const copy = doc.createElement('span');
      copy.className = 'media-home-discovery-copy';
      const name = doc.createElement('strong');
      name.textContent = recordingTitle(recording);
      copy.appendChild(name);
      const subtitle = recordingSubtitle(recording);
      if (subtitle) {
        const detail = doc.createElement('span');
        detail.textContent = subtitle;
        copy.appendChild(detail);
      }
      card.appendChild(copy);
      card.addEventListener('click', function () {
        openRecording(recording, backendId);
      });
      rail.appendChild(card);
    });
    section.appendChild(rail);
    return true;
  }

  function renderGenreRail(entries, backendId) {
    if (!entries.length) {
      clearRail('genres');
      return true;
    }
    const section = sectionFor('genres');
    if (!section) return false;
    section.replaceChildren();
    const heading = doc.createElement('div');
    heading.className = 'media-home-section-heading';
    const name = doc.createElement('h3');
    name.textContent = 'Genres';
    heading.appendChild(name);
    section.appendChild(heading);
    const rail = doc.createElement('div');
    rail.className = 'media-home-discovery-rail genres';
    entries.forEach(function (entry) {
      const card = doc.createElement('button');
      card.type = 'button';
      card.className = 'media-home-discovery-card genre';
      card.dataset.genreId = text(entry.id);
      const copy = doc.createElement('span');
      copy.className = 'media-home-discovery-copy';
      const label = doc.createElement('strong');
      label.textContent = text(entry.label || entry.labelDe || entry.id);
      const count = doc.createElement('span');
      count.textContent = String(Number(entry.count || 0)) + ' Aufnahmen';
      copy.append(label, count);
      card.appendChild(copy);
      card.addEventListener('click', function () {
        openGenre(entry, backendId);
      });
      rail.appendChild(card);
    });
    section.appendChild(rail);
    return true;
  }

  function renderFolderRail(entries, backendId) {
    if (!entries.length) {
      clearRail('folders');
      return true;
    }
    const section = sectionFor('folders');
    if (!section) return false;
    section.replaceChildren();
    const heading = doc.createElement('div');
    heading.className = 'media-home-section-heading';
    const name = doc.createElement('h3');
    name.textContent = 'Aufnahmeordner';
    heading.appendChild(name);
    section.appendChild(heading);
    const rail = doc.createElement('div');
    rail.className = 'media-home-discovery-rail folders';
    entries.forEach(function (entry) {
      const path = text(entry.path || entry.folderPath || entry.name);
      const card = doc.createElement('button');
      card.type = 'button';
      card.className = 'media-home-discovery-card folder';
      card.dataset.folderPath = path;
      const copy = doc.createElement('span');
      copy.className = 'media-home-discovery-copy';
      const label = doc.createElement('strong');
      label.textContent = text(entry.name || entry.title) || path.split('/').filter(Boolean).pop() || path;
      const count = doc.createElement('span');
      const total = Number(entry.totalCount || entry.count || entry.recordingCount || 0);
      count.textContent = total > 0 ? String(total) + ' Aufnahmen' : path;
      copy.append(label, count);
      card.appendChild(copy);
      card.addEventListener('click', function () {
        openFolder(entry, backendId);
      });
      rail.appendChild(card);
    });
    section.appendChild(rail);
    return true;
  }

  function current(generation, backendId) {
    return generation === state.generation &&
      backendId === selectedBackendId() &&
      homeIsActive();
  }

  function loadNewly(client, backendId, generation) {
    renderState('newly-recorded', 'Neu aufgenommen', 'Aufnahmen werden geladen …', false);
    return Promise.resolve(client.fetchClientRecordings({
      query: {
        backend: backendId,
        sort: 'startTime',
        order: 'desc',
        limit: NEW_LIMIT,
        offset: 0
      },
      cache: 'no-store',
      credentials: 'same-origin'
    })).then(function (payload) {
      if (!current(generation, backendId)) return false;
      return renderRecordingRail(
        'newly-recorded',
        'Neu aufgenommen',
        canonicalRecordings(payload, backendId).slice(0, NEW_LIMIT),
        backendId
      );
    }).catch(function () {
      if (!current(generation, backendId)) return false;
      return renderState(
        'newly-recorded',
        'Neu aufgenommen',
        'Neu aufgenommene Inhalte sind vorübergehend nicht verfügbar.',
        true
      );
    });
  }

  function loadSeries(client, backendId, generation, genreEntries) {
    const seriesGenre = genreEntries.find(function (entry) {
      return text(entry.id).toLowerCase() === 'series';
    });
    if (!seriesGenre) {
      clearRail('series');
      return Promise.resolve(false);
    }
    renderState('series', 'Serien', 'Serienaufnahmen werden geladen …', false);
    return Promise.resolve(client.fetchClientGenreRecordings({
      backendId: backendId,
      genreId: text(seriesGenre.id),
      limit: SERIES_LIMIT,
      offset: 0,
      cache: 'no-store',
      credentials: 'same-origin'
    })).then(function (payload) {
      if (!current(generation, backendId)) return false;
      const recordings = canonicalRecordings(payload, backendId);
      if (!recordings.length) {
        clearRail('series');
        return false;
      }
      return renderRecordingRail(
        'series',
        'Serien',
        recordings.slice(0, SERIES_LIMIT),
        backendId
      );
    }).catch(function () {
      if (!current(generation, backendId)) return false;
      return renderState(
        'series',
        'Serien',
        'Serienaufnahmen sind vorübergehend nicht verfügbar.',
        true
      );
    });
  }

  function loadGenres(client, backendId, generation) {
    renderState('genres', 'Genres', 'Genres werden geladen …', false);
    return Promise.resolve(client.fetchClientGenres({
      backendId: backendId,
      scope: 'recordings',
      locale: 'de',
      cache: 'no-store',
      credentials: 'same-origin'
    })).then(function (payload) {
      if (!current(generation, backendId)) return false;
      const entries = canonicalGenres(payload);
      renderGenreRail(entries.slice(0, GENRE_LIMIT), backendId);
      return loadSeries(client, backendId, generation, entries);
    }).catch(function () {
      if (!current(generation, backendId)) return false;
      clearRail('series');
      return renderState(
        'genres',
        'Genres',
        'Genres sind vorübergehend nicht verfügbar.',
        true
      );
    });
  }

  function loadFolders(client, backendId, generation) {
    renderState('folders', 'Aufnahmeordner', 'Aufnahmeordner werden geladen …', false);
    return Promise.resolve(client.fetchClientRecordingFolder({
      backendId: backendId,
      query: {
        path: '',
        limit: FOLDER_LIMIT,
        offset: 0
      },
      cache: 'no-store',
      credentials: 'same-origin'
    })).then(function (payload) {
      if (!current(generation, backendId)) return false;
      return renderFolderRail(
        canonicalFolders(payload).slice(0, FOLDER_LIMIT),
        backendId
      );
    }).catch(function () {
      if (!current(generation, backendId)) return false;
      return renderState(
        'folders',
        'Aufnahmeordner',
        'Aufnahmeordner sind vorübergehend nicht verfügbar.',
        true
      );
    });
  }

  function refresh() {
    if (!homeIsActive()) return Promise.resolve(false);
    const client = clientApi();
    const backendId = selectedBackendId();
    if (!client ||
        typeof client.fetchClientRecordings !== 'function' ||
        typeof client.fetchClientGenres !== 'function' ||
        typeof client.fetchClientGenreRecordings !== 'function' ||
        typeof client.fetchClientRecordingFolder !== 'function') {
      return Promise.resolve(false);
    }
    const generation = ++state.generation;
    state.loadedBackendId = backendId;
    return Promise.allSettled([
      loadNewly(client, backendId, generation),
      loadGenres(client, backendId, generation),
      loadFolders(client, backendId, generation)
    ]).then(function () { return true; });
  }

  function armLazyLoad() {
    if (state.armed) return true;
    const target = host();
    if (!target) return false;
    state.armed = true;
    if (typeof global.IntersectionObserver === 'function') {
      state.observer = new global.IntersectionObserver(function (entries) {
        if (!entries.some(function (entry) { return entry && entry.isIntersecting; })) return;
        state.observer.disconnect();
        state.observer = null;
        refresh();
      }, {rootMargin: '320px 0px'});
      state.observer.observe(target);
      return true;
    }
    global.setTimeout(refresh, 0);
    return true;
  }

  function scheduleForHome() {
    if (!homeIsActive()) return;
    const backendId = selectedBackendId();
    if (state.loadedBackendId && state.loadedBackendId !== backendId) {
      state.generation += 1;
      state.loadedBackendId = '';
    }
    if (state.observer) {
      state.observer.disconnect();
      state.observer = null;
    }
    state.armed = false;
    armLazyLoad();
  }

  function installStyles() {
    if (!doc || !doc.head || doc.getElementById('vdr-suite-recording-discovery-style')) return;
    const style = doc.createElement('style');
    style.id = 'vdr-suite-recording-discovery-style';
    style.textContent =
      '.media-home-discovery{min-width:0;padding-bottom:.3rem}' +
      '.media-home-discovery-rail{display:grid;grid-auto-flow:column;grid-auto-columns:minmax(11rem,15rem);gap:.8rem;overflow-x:auto;padding:0 1rem 1.1rem;scroll-snap-type:x proximity;overscroll-behavior-inline:contain}' +
      '.media-home-discovery-card{scroll-snap-align:start;min-width:0;border:1px solid rgba(148,163,184,.2);border-radius:.95rem;background:rgba(15,23,42,.76);color:#e2e8f0;text-align:left;overflow:hidden;padding:0;cursor:pointer}' +
      '.media-home-discovery-card:focus-visible{outline:3px solid rgba(125,211,252,.86);outline-offset:2px}' +
      '.media-home-discovery-artwork{display:grid;place-items:center;width:100%;aspect-ratio:2/3;background:linear-gradient(135deg,#1e293b,#334155);font-size:2rem;font-weight:800}' +
      '.media-home-discovery-artwork img{display:block;width:100%;height:100%;object-fit:cover}' +
      '.media-home-discovery-copy{display:grid;gap:.25rem;padding:.7rem}.media-home-discovery-copy strong{color:#f8fafc}.media-home-discovery-copy span{color:#94a3b8;font-size:.8rem}' +
      '.media-home-discovery-card.genre,.media-home-discovery-card.folder{min-height:7rem;padding:.35rem;background:linear-gradient(145deg,rgba(30,41,59,.88),rgba(2,6,23,.94))}' +
      '.media-home-discovery-state{margin:0 1rem 1rem;padding:1rem;border:1px solid rgba(148,163,184,.16);border-radius:.9rem;color:#94a3b8;background:rgba(15,23,42,.5)}' +
      '.media-home-discovery-state.error{border-color:rgba(239,68,68,.48);color:#fecaca}' +
      '@media(max-width:46rem){.media-home-discovery-rail{grid-auto-columns:minmax(42vw,11rem);padding:0 .78rem 1rem}.media-home-discovery-state{margin:0 .78rem 1rem}}';
    doc.head.appendChild(style);
  }

  function install() {
    if (!doc) return false;
    installStyles();
    armLazyLoad();
    if (typeof doc.addEventListener === 'function') {
      doc.addEventListener('click', function (event) {
        const target = event && event.target;
        if (target && typeof target.closest === 'function' &&
            target.closest('[data-brand-module="overview"], .module-tab[data-module="overview"], #backends')) {
          global.setTimeout(scheduleForHome, 0);
        }
      });
    }
    return true;
  }

  global.VdrSuiteHomeRecordingDiscovery = Object.freeze({
    install: install,
    refresh: refresh,
    _test: Object.freeze({
      selectedBackendId: selectedBackendId,
      canonicalRecordings: canonicalRecordings,
      canonicalGenres: canonicalGenres,
      canonicalFolders: canonicalFolders,
      recordingPosterUrl: recordingPosterUrl,
      openRecording: openRecording,
      openFolder: openFolder,
      openGenre: openGenre,
      loadNewly: loadNewly,
      loadGenres: loadGenres,
      loadFolders: loadFolders,
      armLazyLoad: armLazyLoad
    })
  });

  if (doc) {
    if (doc.readyState === 'loading') {
      doc.addEventListener('DOMContentLoaded', install, {once: true});
    } else {
      install();
    }
  }
}(window));