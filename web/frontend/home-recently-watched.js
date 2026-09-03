(function (global) {
  'use strict';

  if (!global || global.VdrSuiteHomeRecentlyWatched) return;

  const doc = global.document;
  const LIMIT = 12;
  const state = {generation: 0, placementObserver: null, moduleObserver: null};

  function text(value) {
    return value === undefined || value === null ? '' : String(value).trim();
  }
  function selectedBackendId() {
    const platform = global.VdrSuitePlatform;
    if (platform && typeof platform.getSelectedBackendId === 'function') {
      return text(platform.getSelectedBackendId()) || 'default';
    }
    return 'default';
  }
  function homeIsActive() {
    const platform = global.VdrSuitePlatform;
    if (platform && typeof platform.getSelectedModule === 'function') {
      return text(platform.getSelectedModule()) === 'overview';
    }
    return true;
  }
  function host() {
    return doc && doc.querySelector
      ? doc.querySelector('[data-home-zone="additional-sections"]')
      : null;
  }
  function csrfHeaders() {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.csrfHeaders !== 'function') return {};
    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object' ? headers : {};
  }
  function post(body) {
    const fetcher = global.fetch || fetch;
    return fetcher('/api/media/recently-watched', {
      method: 'POST',
      credentials: 'same-origin',
      cache: 'no-store',
      headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
      body: JSON.stringify(body)
    }).then(function (response) {
      if (!response || !response.ok) throw new Error('recently watched unavailable');
      return response.json();
    });
  }
  function normalizeItem(item, backendId) {
    if (!item || typeof item !== 'object') return null;
    const recording = item.recording && typeof item.recording === 'object'
      ? item.recording
      : null;
    const recordingId = text(item.recordingId || (recording && (recording.recordingId || recording.id)));
    const scopedBackend = text(item.backendId || (recording && recording.backendId));
    if (!recording || !recordingId || scopedBackend !== backendId) return null;
    return {
      backendId: scopedBackend,
      recordingId: recordingId,
      recording: recording,
      posterUrl: text(item.posterUrl),
      positionKnown: item.positionKnown === true,
      positionSeconds: Math.max(0, Math.floor(Number(item.positionSeconds) || 0)),
      completionKnown: item.completionKnown === true,
      completed: item.completed === true,
      resumeRelevanceKnown: item.resumeRelevanceKnown === true,
      resumeRelevant: item.resumeRelevant === true,
      sourceEvidence: text(item.sourceEvidence),
      lastActivityAt: text(item.lastActivityAt)
    };
  }
  function recordingMetadata(recording) {
    return recording && recording.metadata && typeof recording.metadata === 'object'
      ? recording.metadata : {};
  }
  function presentation(recording) {
    const value = recordingMetadata(recording).presentation;
    return value && typeof value === 'object' ? value : {};
  }
  function title(item) {
    return text(presentation(item.recording).title || item.recording.title) || 'Aufnahme';
  }
  function subtitle(item) {
    return text(presentation(item.recording).subtitle);
  }
  function posterUrl(item) {
    const metadata = recordingMetadata(item.recording);
    const artwork = metadata.artwork && typeof metadata.artwork === 'object' ? metadata.artwork : {};
    return text(presentation(item.recording).posterUrl || artwork.preferredUrl || item.posterUrl);
  }
  function publicPath(path) {
    const resolver = global.VdrSuitePublicUrl;
    return path && resolver && typeof resolver.resolvePath === 'function'
      ? resolver.resolvePath(path) : path;
  }
  function formatTime(seconds) {
    const value = Math.max(0, Math.floor(Number(seconds) || 0));
    const hours = Math.floor(value / 3600);
    const minutes = Math.floor((value % 3600) / 60);
    const rest = value % 60;
    return hours > 0
      ? hours + ':' + String(minutes).padStart(2, '0') + ':' + String(rest).padStart(2, '0')
      : minutes + ':' + String(rest).padStart(2, '0');
  }
  function activityLabel(item) {
    if (item.completionKnown && item.completed) return 'Angesehen';
    if (item.resumeRelevanceKnown && item.resumeRelevant) {
      return item.positionKnown ? 'Fortsetzbar · ' + formatTime(item.positionSeconds) : 'Fortsetzbar';
    }
    if (item.completionKnown) return 'Teilweise angesehen';
    return 'Zuletzt angesehen';
  }
  function section() {
    const target = host();
    if (!target) return null;
    let value = target.querySelector
      ? target.querySelector('[data-home-recently-watched]') : null;
    if (!value) {
      value = doc.createElement('section');
      value.className = 'media-home-discovery media-home-recently-watched';
      value.setAttribute('data-home-recently-watched', 'true');
      target.appendChild(value);
    } else if (target.lastElementChild !== value) {
      target.appendChild(value);
    }
    return value;
  }
  function clear() {
    const target = host();
    const value = target && target.querySelector
      ? target.querySelector('[data-home-recently-watched]') : null;
    if (value && typeof value.remove === 'function') value.remove();
  }
  function createArtwork(item) {
    const artwork = doc.createElement('div');
    artwork.className = 'media-home-discovery-artwork';
    const fallback = title(item).slice(0, 1).toUpperCase() || '▶';
    const url = posterUrl(item);
    if (!url) {
      artwork.textContent = fallback;
      return artwork;
    }
    const image = doc.createElement('img');
    image.src = publicPath(url);
    image.alt = 'Poster zu ' + title(item);
    image.loading = 'lazy';
    image.addEventListener('error', function () {
      image.remove();
      artwork.textContent = fallback;
    });
    artwork.appendChild(image);
    return artwork;
  }
  function ensureRecordings2() {
    if (global.VdrSuiteRecordings2 && typeof global.VdrSuiteRecordings2.openRecording === 'function') {
      return Promise.resolve(true);
    }
    const runtimes = global.VdrSuiteDeferredFrontendRuntimes;
    if (!runtimes || typeof runtimes.loadRecordings2 !== 'function') return Promise.resolve(false);
    return Promise.resolve(runtimes.loadRecordings2()).then(function () {
      return Boolean(global.VdrSuiteRecordings2 && typeof global.VdrSuiteRecordings2.openRecording === 'function');
    }).catch(function () { return false; });
  }
  function releasePreview() {
    const preview = global.VdrSuiteHomeLivePreview;
    if (preview && typeof preview.cancel === 'function') preview.cancel('Recently Watched geöffnet');
  }
  function returnHome() {
    if (typeof global.selectModule === 'function') global.selectModule('overview');
    if (typeof global.setTimeout === 'function') global.setTimeout(refresh, 0);
  }
  function openItem(item) {
    releasePreview();
    return ensureRecordings2().then(function (ready) {
      if (!ready || typeof global.selectModule !== 'function') return false;
      global.selectModule('recordings2');
      global.VdrSuiteRecordings2.openRecording(item.recording, {
        backendId: item.backendId,
        backLabel: '← Zurück zu Home',
        onClose: returnHome
      });
      return true;
    });
  }
  function render(items) {
    if (!items.length) {
      clear();
      return true;
    }
    const target = section();
    if (!target) return false;
    target.replaceChildren();
    const heading = doc.createElement('div');
    heading.className = 'media-home-section-heading';
    const name = doc.createElement('h3');
    name.textContent = 'Zuletzt angesehen';
    heading.appendChild(name);
    target.appendChild(heading);
    const rail = doc.createElement('div');
    rail.className = 'media-home-discovery-rail';
    items.forEach(function (item) {
      const card = doc.createElement('button');
      card.type = 'button';
      card.className = 'media-home-discovery-card recording recently-watched';
      card.dataset.recordingId = item.recordingId;
      card.dataset.backendId = item.backendId;
      card.dataset.historyEvidence = item.sourceEvidence;
      card.appendChild(createArtwork(item));
      const copy = doc.createElement('span');
      copy.className = 'media-home-discovery-copy';
      const label = doc.createElement('strong');
      label.textContent = title(item);
      copy.appendChild(label);
      const detailText = subtitle(item);
      if (detailText) {
        const detail = doc.createElement('span');
        detail.textContent = detailText;
        copy.appendChild(detail);
      }
      const activity = doc.createElement('span');
      activity.textContent = activityLabel(item);
      copy.appendChild(activity);
      card.appendChild(copy);
      card.addEventListener('click', function () { openItem(item); });
      rail.appendChild(card);
    });
    target.appendChild(rail);
    const owner = host();
    if (owner && owner.lastElementChild !== target) owner.appendChild(target);
    return true;
  }
  function refresh() {
    if (!homeIsActive()) return Promise.resolve(false);
    const backendId = selectedBackendId();
    const generation = ++state.generation;
    return post({operation: 'list', backendId: backendId}).then(function (payload) {
      if (generation !== state.generation || backendId !== selectedBackendId() || !homeIsActive()) return false;
      const raw = payload && Array.isArray(payload.items) ? payload.items : [];
      const items = raw.map(function (item) { return normalizeItem(item, backendId); }).filter(Boolean).slice(0, LIMIT);
      return render(items);
    }).catch(function () {
      if (generation === state.generation) clear();
      return false;
    });
  }
  function installPlacementObserver() {
    const target = host();
    if (!target || state.placementObserver || typeof global.MutationObserver !== 'function') return;
    state.placementObserver = new global.MutationObserver(function () {
      const current = target.querySelector ? target.querySelector('[data-home-recently-watched]') : null;
      if (current && target.lastElementChild !== current) target.appendChild(current);
    });
    state.placementObserver.observe(target, {childList: true});
  }
  function installModuleObserver() {
    if (!doc || state.moduleObserver || typeof global.MutationObserver !== 'function' || !doc.querySelector) return;
    const navigation = doc.querySelector('#module-nav');
    if (!navigation) return;

    // The canonical shell owns module state by toggling the active module-tab.
    // Observe that state only as a refresh trigger. Viewing-history truth still
    // comes exclusively from the server-side actor-scoped history projection.
    state.moduleObserver = new global.MutationObserver(function (mutations) {
      const enteredHome = Array.prototype.some.call(mutations || [], function (mutation) {
        const target = mutation && mutation.target;
        return Boolean(
          target &&
          target.dataset &&
          target.dataset.module === 'overview' &&
          target.classList &&
          typeof target.classList.contains === 'function' &&
          target.classList.contains('active')
        );
      });
      if (enteredHome && typeof global.setTimeout === 'function') global.setTimeout(refresh, 0);
    });
    state.moduleObserver.observe(navigation, {
      subtree: true,
      attributes: true,
      attributeFilter: ['class']
    });
  }
  function install() {
    if (!doc) return false;
    installPlacementObserver();
    installModuleObserver();
    if (typeof doc.addEventListener === 'function') {
      doc.addEventListener('click', function (event) {
        const target = event && event.target;
        if (target && typeof target.closest === 'function' &&
            target.closest('[data-brand-module="overview"], .module-tab[data-module="overview"], #backends')) {
          global.setTimeout(refresh, 0);
        }
      });
    }
    if (typeof global.setTimeout === 'function') global.setTimeout(refresh, 0);
    return true;
  }

  global.VdrSuiteHomeRecentlyWatched = Object.freeze({
    install: install,
    refresh: refresh,
    _test: Object.freeze({
      normalizeItem: normalizeItem,
      activityLabel: activityLabel,
      openItem: openItem,
      post: post,
      installModuleObserver: installModuleObserver
    })
  });

  if (doc) {
    if (doc.readyState === 'loading') doc.addEventListener('DOMContentLoaded', install, {once: true});
    else install();
  }
}(window));

// Bounded Phase-66 Recording Discovery follow-up. This projection deliberately
// shares the existing discovery bundle and its canonical Recording helpers; it
// does not introduce another metadata, artwork, identity or playback owner.
(function (global) {
  'use strict';

  if (!global || global.VdrSuiteHomeRecentMovies) return;

  const doc = global.document;
  const TITLE = 'Filme der letzten 5 Jahre';
  const LIMIT = 12;
  const PAGE_LIMIT = 100;
  const WARM_RETURN_MS = 60000;
  const HOME_RAIL_NEAR_END_EVENT = 'vdr-suite-home-rail-near-end';
  const state = {
    generation: 0,
    placementObserver: null,
    moduleObserver: null,
    movies: [],
    backendId: '',
    completedAt: 0,
    loadingBackendId: '',
    loadingPromise: null,
    visibleLimit: LIMIT,
    nearEndBound: false
  };

  function text(value) {
    return value === undefined || value === null ? '' : String(value).trim();
  }

  function selectedBackendId() {
    const platform = global.VdrSuitePlatform;
    if (platform && typeof platform.getSelectedBackendId === 'function') {
      return text(platform.getSelectedBackendId()) || 'default';
    }
    return 'default';
  }

  function homeIsActive() {
    const platform = global.VdrSuitePlatform;
    if (platform && typeof platform.getSelectedModule === 'function') {
      return text(platform.getSelectedModule()) === 'overview';
    }
    return true;
  }

  function clientApi() {
    const platform = global.VdrSuitePlatform;
    if (platform && typeof platform.getClientApi === 'function') {
      const client = platform.getClientApi();
      if (client) return client;
    }
    return global.VdrSuiteClientApi || null;
  }

  function discoveryTestApi() {
    const owner = global.VdrSuiteHomeRecordingDiscovery;
    return owner && owner._test ? owner._test : null;
  }

  function host() {
    return doc && typeof doc.querySelector === 'function'
      ? doc.querySelector('[data-home-zone="additional-sections"]')
      : null;
  }

  function provider(recording) {
    const metadata = recording && recording.metadata && typeof recording.metadata === 'object'
      ? recording.metadata
      : {};
    const value = metadata.provider;
    return value && typeof value === 'object' ? value : {};
  }

  function releaseYear(recording) {
    const value = text(provider(recording).releaseDate);
    const match = value.match(/^(\d{4})(?:-(\d{2})-(\d{2}))?$/);
    if (!match) return 0;
    const year = Number(match[1]);
    if (!Number.isInteger(year) || year < 1000 || year > 9999) return 0;
    if (!match[2]) return year;
    const month = Number(match[2]);
    const day = Number(match[3]);
    const date = new Date(Date.UTC(year, month - 1, day));
    if (date.getUTCFullYear() !== year ||
        date.getUTCMonth() !== month - 1 ||
        date.getUTCDate() !== day) return 0;
    return year;
  }

  function recentMovie(recording, currentYear) {
    if (text(provider(recording).contentKind) !== 'movie') return false;
    const year = releaseYear(recording);
    const nowYear = Number(currentYear);
    return Number.isInteger(nowYear) &&
      year >= nowYear - 4 &&
      year <= nowYear;
  }

  function recordingId(recording) {
    return text(recording && (recording.recordingId || recording.id));
  }

  function recordingTitle(recording) {
    const metadata = recording && recording.metadata && typeof recording.metadata === 'object'
      ? recording.metadata
      : {};
    const presentation = metadata.presentation && typeof metadata.presentation === 'object'
      ? metadata.presentation
      : {};
    return text(presentation.title || provider(recording).title || (recording && recording.title)) || 'Aufnahme';
  }

  function releaseDate(recording) {
    return text(provider(recording).releaseDate);
  }

  function recordingStartTime(recording) {
    return text(recording && recording.startTime);
  }

  function sortMovies(recordings) {
    return (recordings || []).slice().sort(function (left, right) {
      const leftDate = releaseDate(left);
      const rightDate = releaseDate(right);
      if (leftDate !== rightDate) return rightDate.localeCompare(leftDate, 'de');
      const leftStart = recordingStartTime(left);
      const rightStart = recordingStartTime(right);
      if (leftStart !== rightStart) return rightStart.localeCompare(leftStart, 'de');
      const leftTitle = recordingTitle(left);
      const rightTitle = recordingTitle(right);
      if (leftTitle !== rightTitle) return leftTitle.localeCompare(rightTitle, 'de');
      return recordingId(left).localeCompare(recordingId(right), 'de');
    });
  }

  function list(value, key) {
    if (Array.isArray(value)) return value;
    if (value && Array.isArray(value[key])) return value[key];
    if (value && Array.isArray(value.items)) return value.items;
    return [];
  }

  function pageTotal(payload, fallback) {
    const values = [payload && payload.totalCount, payload && payload.total];
    for (let index = 0; index < values.length; index += 1) {
      const candidate = Number(values[index]);
      if (Number.isFinite(candidate) && candidate >= 0) return candidate;
    }
    return fallback;
  }

  function fetchAllRecordings(client, backendId, generation) {
    const owner = discoveryTestApi();
    if (!owner || typeof owner.canonicalRecordings !== 'function') {
      return Promise.resolve([]);
    }
    const recordings = [];
    const seen = new Set();

    function requestPage(offset) {
      return Promise.resolve(client.fetchClientRecordings({
        query: {
          backend: backendId,
          limit: PAGE_LIMIT,
          offset: offset
        },
        cache: 'no-store',
        credentials: 'same-origin'
      })).then(function (payload) {
        if (generation !== state.generation ||
            backendId !== selectedBackendId() ||
            !homeIsActive()) return [];
        const rawPage = list(payload, 'recordings');
        owner.canonicalRecordings(payload, backendId).forEach(function (recording) {
          const id = recordingId(recording);
          if (!id || seen.has(id)) return;
          seen.add(id);
          recordings.push(recording);
        });
        const nextOffset = offset + rawPage.length;
        const total = pageTotal(payload, nextOffset);
        if (!rawPage.length || nextOffset >= total || nextOffset <= offset) return recordings;
        return requestPage(nextOffset);
      });
    }

    return requestPage(0);
  }

  function section() {
    const target = host();
    if (!target) return null;
    let value = target.querySelector
      ? target.querySelector('[data-home-discovery-rail="recent-movies"]')
      : null;
    if (!value) {
      value = doc.createElement('section');
      value.className = 'media-home-discovery media-home-recent-movies';
      value.setAttribute('data-home-discovery-rail', 'recent-movies');
      target.appendChild(value);
    }
    return value;
  }

  function clear() {
    const target = host();
    const value = target && target.querySelector
      ? target.querySelector('[data-home-discovery-rail="recent-movies"]')
      : null;
    if (value && typeof value.remove === 'function') value.remove();
  }

  function positionBeforeSeries() {
    const target = host();
    if (!target || typeof target.querySelector !== 'function') return false;
    const movies = target.querySelector('[data-home-discovery-rail="recent-movies"]');
    const series = target.querySelector('[data-home-discovery-rail="series"]');
    if (!movies || !series || movies === series) return false;
    if (movies.nextElementSibling === series) return true;
    if (typeof target.insertBefore !== 'function') return false;
    target.insertBefore(movies, series);
    return true;
  }

  function publicPath(path) {
    const value = text(path);
    const resolver = global.VdrSuitePublicUrl;
    return value && resolver && typeof resolver.resolvePath === 'function'
      ? resolver.resolvePath(value)
      : value;
  }

  function posterUrl(recording) {
    const owner = discoveryTestApi();
    return owner && typeof owner.recordingPosterUrl === 'function'
      ? text(owner.recordingPosterUrl(recording))
      : '';
  }

  function createArtwork(recording) {
    const artwork = doc.createElement('div');
    artwork.className = 'media-home-discovery-artwork';
    const title = recordingTitle(recording);
    const fallback = title.slice(0, 1).toUpperCase() || '▶';
    const url = posterUrl(recording);
    if (!url) {
      artwork.textContent = fallback;
      return artwork;
    }
    const image = doc.createElement('img');
    image.src = publicPath(url);
    image.alt = 'Poster zu ' + title;
    image.loading = 'lazy';
    image.decoding = 'async';
    image.fetchPriority = 'low';
    image.addEventListener('error', function () {
      image.remove();
      artwork.textContent = fallback;
    });
    artwork.appendChild(image);
    return artwork;
  }

  function openRecording(recording, backendId) {
    const owner = discoveryTestApi();
    return owner && typeof owner.openRecording === 'function'
      ? owner.openRecording(recording, backendId)
      : Promise.resolve(false);
  }

  function render(recordings, backendId, visibleLimit) {
    if (!recordings.length) {
      clear();
      return true;
    }
    const target = section();
    if (!target) return false;
    const previousRail = typeof target.querySelector === 'function'
      ? target.querySelector('.media-home-discovery-rail.recent-movies')
      : null;
    const previousScrollLeft = Number(previousRail && previousRail.scrollLeft) || 0;
    target.replaceChildren();

    const heading = doc.createElement('div');
    heading.className = 'media-home-section-heading';
    const name = doc.createElement('h3');
    name.textContent = TITLE;
    heading.appendChild(name);
    target.appendChild(heading);

    const rail = doc.createElement('div');
    rail.className = 'media-home-discovery-rail recent-movies';
    rail.setAttribute('aria-label', TITLE);
    const limit = Math.max(0, Number(visibleLimit) || LIMIT);
    recordings.slice(0, limit).forEach(function (recording) {
      const card = doc.createElement('button');
      card.type = 'button';
      card.className = 'media-home-discovery-card recording recent-movie';
      card.dataset.recordingId = recordingId(recording);
      card.dataset.backendId = backendId;
      card.dataset.movieYear = String(releaseYear(recording));
      card.appendChild(createArtwork(recording));

      const copy = doc.createElement('span');
      copy.className = 'media-home-discovery-copy';
      const label = doc.createElement('strong');
      label.textContent = recordingTitle(recording);
      const detail = doc.createElement('span');
      detail.textContent = String(releaseYear(recording));
      copy.append(label, detail);
      card.appendChild(copy);
      card.addEventListener('click', function () {
        openRecording(recording, backendId);
      });
      rail.appendChild(card);
    });
    target.appendChild(rail);
    rail.scrollLeft = previousScrollLeft;
    positionBeforeSeries();
    return true;
  }

  function renderState(message, error) {
    const target = section();
    if (!target) return false;
    target.replaceChildren();
    const heading = doc.createElement('div');
    heading.className = 'media-home-section-heading';
    const name = doc.createElement('h3');
    name.textContent = TITLE;
    heading.appendChild(name);
    target.appendChild(heading);
    const messageNode = doc.createElement('div');
    messageNode.className = 'media-home-discovery-state' + (error ? ' error' : '');
    messageNode.setAttribute('role', 'status');
    messageNode.textContent = message;
    target.appendChild(messageNode);
    positionBeforeSeries();
    return true;
  }

  function warmForBackend(backendId) {
    return state.backendId === backendId &&
      state.completedAt > 0 &&
      Date.now() - state.completedAt < WARM_RETURN_MS;
  }

  function refresh() {
    if (!homeIsActive()) return Promise.resolve(false);
    const client = clientApi();
    const owner = discoveryTestApi();
    if (!client || typeof client.fetchClientRecordings !== 'function' ||
        !owner || typeof owner.canonicalRecordings !== 'function' ||
        typeof owner.openRecording !== 'function') {
      return Promise.resolve(false);
    }
    const backendId = selectedBackendId();
    const generation = ++state.generation;
    const currentYear = new Date().getFullYear();
    state.movies = [];
    state.backendId = backendId;
    state.completedAt = 0;
    state.visibleLimit = LIMIT;
    renderState('Filme werden geladen …', false);
    const load = fetchAllRecordings(client, backendId, generation).then(function (recordings) {
      if (generation !== state.generation ||
          backendId !== selectedBackendId() ||
          !homeIsActive()) return false;
      const movies = sortMovies(recordings.filter(function (recording) {
        return recentMovie(recording, currentYear);
      }));
      state.movies = movies;
      state.backendId = backendId;
      state.completedAt = Date.now();
      state.visibleLimit = Math.min(LIMIT, movies.length);
      return render(movies, backendId, state.visibleLimit);
    }).catch(function () {
      if (generation !== state.generation) return false;
      state.completedAt = 0;
      return renderState('Filme sind vorübergehend nicht verfügbar.', true);
    });
    state.loadingBackendId = backendId;
    state.loadingPromise = load;
    return load.then(function (result) {
      if (state.loadingPromise === load) {
        state.loadingPromise = null;
        state.loadingBackendId = '';
      }
      return result;
    });
  }

  function refreshForHome() {
    if (!homeIsActive()) return Promise.resolve(false);
    const backendId = selectedBackendId();
    if (state.loadingPromise && state.loadingBackendId === backendId) {
      return state.loadingPromise;
    }
    if (warmForBackend(backendId)) return Promise.resolve(true);
    return refresh();
  }

  function railHasClass(rail, className) {
    if (!rail) return false;
    if (rail.classList && typeof rail.classList.contains === 'function') {
      return rail.classList.contains(className);
    }
    return String(rail.className || '').split(/\s+/).includes(className);
  }

  function loadMoreMovies() {
    if (!homeIsActive() || state.backendId !== selectedBackendId() ||
        state.visibleLimit >= state.movies.length) return false;
    state.visibleLimit = Math.min(state.visibleLimit + LIMIT, state.movies.length);
    return render(state.movies, state.backendId, state.visibleLimit);
  }

  function handleRailNearEnd(event) {
    const rail = event && event.detail && event.detail.rail;
    if (!railHasClass(rail, 'media-home-discovery-rail') ||
        !railHasClass(rail, 'recent-movies')) return false;
    return loadMoreMovies();
  }

  function bindNearEnd() {
    if (state.nearEndBound || !doc || typeof doc.addEventListener !== 'function') return false;
    state.nearEndBound = true;
    doc.addEventListener(HOME_RAIL_NEAR_END_EVENT, handleRailNearEnd);
    return true;
  }

  function installPlacementObserver() {
    const target = host();
    if (!target || state.placementObserver || typeof global.MutationObserver !== 'function') return;
    state.placementObserver = new global.MutationObserver(function () {
      positionBeforeSeries();
    });
    state.placementObserver.observe(target, {childList: true});
  }

  function installModuleObserver() {
    if (!doc || state.moduleObserver || typeof global.MutationObserver !== 'function' ||
        typeof doc.querySelector !== 'function') return;
    const navigation = doc.querySelector('#module-nav');
    if (!navigation) return;
    state.moduleObserver = new global.MutationObserver(function (mutations) {
      const enteredHome = Array.prototype.some.call(mutations || [], function (mutation) {
        const target = mutation && mutation.target;
        return Boolean(
          target &&
          target.dataset &&
          target.dataset.module === 'overview' &&
          target.classList &&
          typeof target.classList.contains === 'function' &&
          target.classList.contains('active')
        );
      });
      if (enteredHome && typeof global.setTimeout === 'function') global.setTimeout(refreshForHome, 0);
    });
    state.moduleObserver.observe(navigation, {
      subtree: true,
      attributes: true,
      attributeFilter: ['class']
    });
  }

  function install() {
    if (!doc) return false;
    installPlacementObserver();
    installModuleObserver();
    bindNearEnd();
    if (typeof doc.addEventListener === 'function') {
      doc.addEventListener('click', function (event) {
        const target = event && event.target;
        if (target && typeof target.closest === 'function' &&
            target.closest('[data-brand-module="overview"], .module-tab[data-module="overview"], #backends')) {
          global.setTimeout(refreshForHome, 0);
        }
      });
    }
    if (typeof global.setTimeout === 'function') global.setTimeout(refresh, 0);
    return true;
  }

  global.VdrSuiteHomeRecentMovies = Object.freeze({
    install: install,
    refresh: refresh,
    _test: Object.freeze({
      releaseYear: releaseYear,
      recentMovie: recentMovie,
      sortMovies: sortMovies,
      fetchAllRecordings: fetchAllRecordings,
      render: render,
      refreshForHome: refreshForHome,
      warmForBackend: warmForBackend,
      loadMoreMovies: loadMoreMovies,
      handleRailNearEnd: handleRailNearEnd,
      positionBeforeSeries: positionBeforeSeries,
      openRecording: openRecording
    })
  });

  if (doc) {
    if (doc.readyState === 'loading') doc.addEventListener('DOMContentLoaded', install, {once: true});
    else install();
  }
}(window));
