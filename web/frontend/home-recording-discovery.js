(function (global) {
  'use strict';

  if (!global || global.VdrSuiteHomeRecordingDiscovery) return;

  const doc = global.document;
  const NEW_LIMIT = 12;
  const GENRE_LIMIT = 12;
  const RANDOM_GENRE_LIMIT = 12;
  const SERIES_PAGE_LIMIT = 100;
  const SERIES_METADATA_CONCURRENCY = 4;
  const SERIES_METADATA_TOTAL_CONCURRENCY = SERIES_METADATA_CONCURRENCY;
  const SERIES_WARM_TTL_MS = 60000;
  const SERIES_METADATA_RETRY_MS = 60000;
  const SERIES_DETAIL_METADATA_RETRY_MS = 2000;
  const SERIES_METADATA_RETRY_BATCH = 8;
  const FOLDER_LIMIT = 100;
  const HOME_RESUME_EVENT = 'vdr-suite:home-resume';
  const state = {
    generation: 0,
    loadedBackendId: '',
    observer: null,
    armed: false,
    refreshInFlight: null,
    seriesCompletionInFlight: null,
    seriesMetadataRetryTimer: null,
    seriesInvalidatedGeneration: -1,
    seriesProjection: [],
    seriesBackendId: '',
    seriesViewKey: '',
    seriesSeasonNumber: null,
    seriesWarmBackendId: '',
    seriesWarmCompletedAt: 0,
    folderProjection: {folders: [], rootRecordings: []},
    folderBackendId: '',
    randomGenreGeneration: -1,
    randomGenreId: '',
    randomFolderGeneration: -1,
    randomFolderPath: '',
    homeReadyBackendId: '',
    homeReadyGeneration: -1,
    seriesMetadataCache: null,

    seriesCoverBackendId: '',

    seriesCoverSettingsLoaded: false,

    seriesCoverOverrides: new Map(),

    seriesCoverSettingsInFlight: null
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

  function homePreview(path) {
    const helpers = global.VdrSuiteFrontendHelpers;
    return helpers && typeof helpers.homeArtworkPreviewUrl === 'function'
      ? helpers.homeArtworkPreviewUrl(path) : path;
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

  function recordingBackendNativeId(recording) {
    return text(recording && recording.backendNativeId);
  }

  function recordingPath(recording) {
    return text(recording && recording.path);
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

  function recordingMetadataPosterUrl(value) {
    const helpers = global.VdrSuiteFrontendHelpers;
    if (!helpers || typeof helpers.recordingMetadataPosterUrl !== 'function') return '';
    return text(helpers.recordingMetadataPosterUrl(value));
  }

  function seriesCoverPublicImageUrl(value) {
    const url = text(value);

    if (!url) return '';

    // Legacy internal Recording-metadata artwork remains valid for
    // already persisted overrides.
    if (
      url.indexOf(
        '/api/vdr/recordings/metadata/image?'
      ) === 0
    ) {
      return url;
    }

    // The TMDB Series-cover workflow owns its own internal image
    // endpoint.  Accept only the backend-scoped relative route;
    // never an absolute/external URL.
    if (
      /^\/api\/backends\/[^/?#]+\/settings\/series-artwork\/image\?/
        .test(url)
    ) {
      return url;
    }

    return '';
  }

  function seriesCoverSettingsPath(backendId) {
    const id = text(backendId) || 'default';

    return '/api/backends/' +
      encodeURIComponent(id) +
      '/settings/series-artwork';
  }

  function setSeriesCoverSettingsSnapshot(snapshot, backendId) {
    const id = text(backendId) || 'default';
    const overrides = new Map();
    const entries = Array.isArray(snapshot && snapshot.coverOverrides)
      ? snapshot.coverOverrides
      : [];

    entries.forEach(function (entry) {
      const seriesKey = text(entry && entry.seriesKey);
      const posterUrl = seriesCoverPublicImageUrl(
        entry && entry.posterUrl
      );
      const revision = Math.max(
        0,
        Number(entry && entry.revision) || 0
      );

      if (!seriesKey || !posterUrl || revision <= 0) return;

      overrides.set(seriesKey, {
        seriesKey: seriesKey,
        posterUrl: posterUrl,
        revision: revision
      });
    });

    state.seriesCoverBackendId = id;
    state.seriesCoverSettingsLoaded = true;
    state.seriesCoverOverrides = overrides;

    return overrides;
  }

  function seriesCoverOverride(series, backendId) {
    const id = text(backendId) || 'default';
    const key = text(series && series.key);

    if (!key ||
        state.seriesCoverBackendId !== id ||
        state.seriesCoverSettingsLoaded !== true ||
        !(state.seriesCoverOverrides instanceof Map)) {
      return null;
    }

    return state.seriesCoverOverrides.get(key) || null;
  }

  function seriesCoverPosterUrl(series, backendId) {
    const override = seriesCoverOverride(series, backendId);

    return text(override && override.posterUrl) ||
      text(series && series.posterUrl);
  }

  function seriesCoverCandidateImageUrl(
    backendId,
    candidate
  ) {
    const id = text(backendId) || 'default';

    const externalId =
      text(candidate && candidate.externalId);

    const posterReference =
      text(candidate && candidate.posterReference);

    if (!externalId || !posterReference) {
      return '';
    }

    return seriesCoverSettingsPath(id) +
      '/candidate-image?externalId=' +
      encodeURIComponent(externalId) +
      '&posterReference=' +
      encodeURIComponent(posterReference);
  }

  function searchSeriesCoverCandidates(
    backendId,
    query
  ) {
    const client = clientApi();
    const id = text(backendId) || 'default';
    const value = text(query).trim();

    if (!client ||
        typeof client.requestJson !== 'function') {
      return Promise.reject(
        new Error(
          'Client API ist nicht verfügbar.'
        )
      );
    }

    if (value.length < 2) {
      return Promise.reject(
        new Error(
          'Bitte mindestens zwei Zeichen eingeben.'
        )
      );
    }

    const path =
      '/api/backends/' +
      encodeURIComponent(id) +
      '/recordings/metadata/search';

    return Promise.resolve(
      client.requestJson(
        path,
        seriesCoverMutationOptions({
          query: value,
          kind: 'series',
          limit: 12
        })
      )
    ).then(function (result) {
      return (
        Array.isArray(
          result && result.candidates
        )
          ? result.candidates
          : []
      ).filter(function (candidate) {
        return candidate &&
          candidate.providerId === 'tmdb' &&
          candidate.kind === 'series' &&
          text(candidate.externalNamespace) === 'tv' &&
          text(candidate.externalId) &&
          text(candidate.posterReference);
      });
    });
  }

  function seriesCoverMutationOptions(payload) {
    const session = global.VdrSuiteBrowserSession;
    const csrf = session &&
      typeof session.csrfHeaders === 'function'
      ? session.csrfHeaders()
      : {};

    return {
      method: 'POST',
      headers: Object.assign({
        'Content-Type': 'application/json'
      }, csrf && typeof csrf === 'object' ? csrf : {}),
      body: JSON.stringify(payload),
      cache: 'no-store',
      credentials: 'same-origin'
    };
  }

  function rerenderSeriesCoverPresentation(backendId) {
    const id = text(backendId) || 'default';

    if (state.seriesBackendId !== id ||
        !state.seriesProjection.length) {
      return false;
    }

    if (!state.seriesViewKey) {
      return renderSeriesRail(
        state.seriesProjection,
        id
      );
    }

    const series = state.seriesProjection.find(function (entry) {
      return entry.key === state.seriesViewKey;
    });

    if (!series) return false;

    const selectedSeason = state.seriesSeasonNumber === null
      ? null
      : series.seasons.find(function (season) {
        return season.number === state.seriesSeasonNumber;
      }) || null;

    const section = sectionFor('series');
    const view = section &&
      section.__vdrSuiteSeriesDetail;

    const metadataLoading = Boolean(
      view &&
      view.key === series.key &&
      view.backendId === id &&
      view.metadataLoading === true
    );

    return renderSeriesDetail(
      series,
      metadataLoading ? null : selectedSeason,
      id,
      metadataLoading
        ? {metadataLoading: true}
        : null
    );
  }

  function loadSeriesCoverSettings(
    client,
    backendId,
    generation
  ) {
    const id = text(backendId) || 'default';

    if (!client ||
        typeof client.requestJson !== 'function') {
      return Promise.resolve(false);
    }

    if (state.seriesCoverBackendId === id &&
        state.seriesCoverSettingsLoaded === true) {
      return Promise.resolve(true);
    }

    const existing = state.seriesCoverSettingsInFlight;

    if (existing &&
        existing.backendId === id &&
        existing.generation === generation) {
      return existing.promise;
    }

    const request = {
      backendId: id,
      generation: generation,
      promise: null
    };

    request.promise = Promise.resolve(
      client.requestJson(
        seriesCoverSettingsPath(id),
        {
          cache: 'no-store',
          credentials: 'same-origin'
        }
      )
    ).then(function (snapshot) {
      if (!current(generation, id)) return false;

      setSeriesCoverSettingsSnapshot(snapshot, id);
      rerenderSeriesCoverPresentation(id);

      return true;
    }).catch(function () {
      return false;
    }).finally(function () {
      if (state.seriesCoverSettingsInFlight === request) {
        state.seriesCoverSettingsInFlight = null;
      }
    });

    state.seriesCoverSettingsInFlight = request;

    return request.promise;
  }

  function updateSeriesCoverOverride(
    series,
    backendId,
    candidate
  ) {
    const client = clientApi();
    const id = text(backendId) || 'default';
    const seriesKey =
      text(series && series.key);

    if (!client ||
        typeof client.requestJson !== 'function') {
      return Promise.reject(
        new Error(
          'Client API ist nicht verfügbar.'
        )
      );
    }

    if (!seriesKey) {
      return Promise.reject(
        new Error(
          'Die Serie besitzt keine stabile Identität.'
        )
      );
    }

    const selected =
      candidate &&
      typeof candidate === 'object'
        ? candidate
        : null;

    const payload = {
      backendId: id,
      operation: selected
        ? 'set-series-cover-tmdb'
        : 'clear-series-cover',
      seriesKey: seriesKey,
      operationId:
        'home-series-cover-' +
        String(Date.now())
    };

    if (selected) {
      if (text(selected.providerId) !== 'tmdb' ||
          text(selected.externalNamespace) !== 'tv' ||
          !text(selected.externalId) ||
          !text(selected.posterReference)) {
        return Promise.reject(
          new Error(
            'Dieser Seriencover-Treffer ist ungültig.'
          )
        );
      }

      payload.providerId = 'tmdb';
      payload.externalNamespace = 'tv';
      payload.externalId =
        text(selected.externalId);
      payload.posterReference =
        text(selected.posterReference);
    }

    return Promise.resolve(
      client.requestJson(
        seriesCoverSettingsPath(id),
        seriesCoverMutationOptions(payload)
      )
    ).then(function (snapshot) {
      setSeriesCoverSettingsSnapshot(
        snapshot,
        id
      );

      rerenderSeriesCoverPresentation(id);

      return snapshot;
    });
  }

  function recordingMetadataProjection(recording, richMetadata) {
    const rich = richMetadata && richMetadata.available === true ? richMetadata : {};
    const richTitle = text(rich.title);
    const recordingPath = text(recording && recording.path);
    const baseTitle = recordingTitle(recording);
    const fallbackTitle = baseTitle.indexOf('/') >= 0
      ? episodeLeafTitle(recording)
      : baseTitle;
    const richTitleIsPath = Boolean(
      richTitle &&
      (richTitle === recordingPath || richTitle.indexOf('/') >= 0)
    );
    const projectedTitle = richTitle && !richTitleIsPath
      ? richTitle
      : fallbackTitle;
    return {
      title: projectedTitle,
      subtitle: text(rich.episodeName) || recordingSubtitle(recording),
      posterUrl: recordingMetadataPosterUrl(rich) || recordingPosterUrl(recording)
    };
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

  function embeddedLeafRecording(entry, backendId) {
    if (!entry || entry.singleRecordingLeaf !== true) return null;
    const recording = entry.singleRecording;
    if (!recording || typeof recording !== 'object') return null;
    if (!recordingId(recording) || recordingBackendId(recording, backendId) !== backendId) return null;
    return recording;
  }

  function uniqueCanonicalRecordings(recordings, backendId) {
    const seen = new Set();
    return (recordings || []).filter(function (recording) {
      const id = recordingId(recording);
      const scopedBackend = recordingBackendId(recording, backendId);
      if (!id || !scopedBackend || scopedBackend !== backendId) return false;
      const key = scopedBackend + '\n' + id;
      if (seen.has(key)) return false;
      seen.add(key);
      return true;
    });
  }

  function projectRootFolderPayload(payload, backendId) {
    const folders = [];
    const rootRecordings = canonicalRecordings(payload, backendId).slice();
    canonicalFolders(payload).forEach(function (entry) {
      const embedded = embeddedLeafRecording(entry, backendId);
      if (embedded) rootRecordings.push(embedded);
      else folders.push(entry);
    });
    return {
      folders: folders,
      rootRecordings: uniqueCanonicalRecordings(rootRecordings, backendId)
    };
  }

  function genreLabel(entry) {
    return text(entry && (entry.label || entry.labelDe || entry.id)) || 'Genre';
  }

  function selectRandomGenre(entries, generation, randomValue) {
    const available = (entries || []).filter(function (entry) {
      return Boolean(entry && text(entry.id) && Number(entry.count || 0) > 0);
    });
    if (!available.length) {
      state.randomGenreGeneration = generation;
      state.randomGenreId = '';
      return null;
    }
    if (state.randomGenreGeneration === generation && state.randomGenreId) {
      const existing = available.find(function (entry) {
        return text(entry.id) === state.randomGenreId;
      });
      if (existing) return existing;
    }
    const raw = Number(randomValue);
    const bounded = Number.isFinite(raw)
      ? Math.max(0, Math.min(raw, 0.999999999999))
      : 0;
    const selected = available[Math.floor(bounded * available.length)] || available[0];
    state.randomGenreGeneration = generation;
    state.randomGenreId = text(selected.id);
    return selected;
  }

  function folderEntryCount(entry) {
    const candidates = [
      entry && entry.totalCount,
      entry && entry.count,
      entry && entry.recordingCount
    ];
    for (let index = 0; index < candidates.length; index += 1) {
      const value = Number(candidates[index]);
      if (Number.isFinite(value) && value > 0) return Math.floor(value);
    }
    return 0;
  }

  function selectRandomFolder(entries, generation, randomValue) {
    const available = (entries || []).filter(function (entry) {
      return Boolean(
        entry &&
        text(entry.path || entry.folderPath || entry.name) &&
        folderEntryCount(entry) > 0
      );
    });
    if (!available.length) {
      state.randomFolderGeneration = generation;
      state.randomFolderPath = '';
      return null;
    }
    if (state.randomFolderGeneration === generation && state.randomFolderPath) {
      const existing = available.find(function (entry) {
        return text(entry.path || entry.folderPath || entry.name) === state.randomFolderPath;
      });
      if (existing) return existing;
    }
    const raw = Number(randomValue);
    const bounded = Number.isFinite(raw)
      ? Math.max(0, Math.min(raw, 0.999999999999))
      : 0;
    const selected = available[Math.floor(bounded * available.length)] || available[0];
    state.randomFolderGeneration = generation;
    state.randomFolderPath = text(selected.path || selected.folderPath || selected.name);
    return selected;
  }

  function pageTotal(payload, fallback) {
    const raw = payload && payload.total !== undefined
      ? payload.total
      : payload && payload.totalCount;
    const value = Number(raw);
    return Number.isFinite(value) && value >= 0
      ? Math.floor(value)
      : Number(fallback || 0);
  }

  function pageHasMore(payload, nextOffset, total) {
    if (payload && typeof payload.hasMore === 'boolean') {
      return payload.hasMore;
    }
    return nextOffset < total;
  }

  function folderRecordingTotal(payload, fallback) {
    const value = Number(payload && payload.recordingCount);
    return Number.isFinite(value) && value >= 0
      ? Math.floor(value)
      : Number(fallback || 0);
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

  function createPosterArtwork(title, url, fallbackText) {
    const artwork = doc.createElement('div');
    artwork.className = 'media-home-discovery-artwork';
    const fallback = text(fallbackText) || text(title).slice(0, 1).toUpperCase() || '▶';
    const resolved = text(url);
    if (!resolved) {
      artwork.textContent = fallback;
      return artwork;
    }
    const image = doc.createElement('img');
    image.src = publicPath(homePreview(resolved));
    image.alt = 'Poster zu ' + text(title);
    image.loading = 'lazy';
    image.addEventListener('error', function () {
      image.remove();
      artwork.textContent = fallback;
    });
    artwork.appendChild(image);
    return artwork;
  }

  function createArtwork(recording) {
    return createPosterArtwork(
      recordingTitle(recording),
      recordingPosterUrl(recording),
      recordingTitle(recording).slice(0, 1).toUpperCase()
    );
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
    if (text(moduleName) !== 'overview') invalidateSeriesForHomeExit(state.generation);
    if (typeof global.selectModule !== 'function') return false;
    global.selectModule(moduleName);
    return true;
  }

  function returnHome() {
    selectShellModule('overview');
    if (typeof global.setTimeout === 'function') global.setTimeout(refreshForHome, 0);
  }

  function openRecording(recording, backendId, options) {
    const id = recordingId(recording);
    if (!id) return Promise.resolve(false);
    const scopedBackend = recordingBackendId(recording, backendId);
    if (!scopedBackend) return Promise.resolve(false);
    const config = options && typeof options === 'object' ? options : {};
    releasePreview('Recording Discovery Aufnahme geöffnet');
    return ensureRecordings2().then(function (ready) {
      if (!ready || !selectShellModule('recordings2')) return false;
      global.VdrSuiteRecordings2.openRecording(recording, {
        backendId: scopedBackend,
        backLabel: config.backLabel || '← Zurück zu Home',
        onClose: typeof config.onClose === 'function' ? config.onClose : returnHome
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

  function renderRecordingRail(key, title, recordings, backendId, options) {
    if (!recordings.length) {
      clearRail(key);
      return true;
    }
    const section = sectionFor(key);
    if (!section) return false;
    section.replaceChildren();
    const config = options && typeof options === 'object' ? options : {};
    const rich = config.richMetadataByNativeId instanceof Map
      ? config.richMetadataByNativeId
      : null;
    appendSectionHeading(section, title, config.backLabel, config.onBack);
    const rail = doc.createElement('div');
    rail.className = 'media-home-discovery-rail';
    recordings.forEach(function (recording) {
      const nativeId = recordingBackendNativeId(recording);
      const projected = recordingMetadataProjection(
        recording,
        rich && nativeId ? rich.get(nativeId) || null : null
      );
      const card = doc.createElement('button');
      card.type = 'button';
      card.className = 'media-home-discovery-card recording';
      card.dataset.recordingId = recordingId(recording);
      card.dataset.backendId = recordingBackendId(recording, backendId);
      card.appendChild(createPosterArtwork(
        projected.title,
        projected.posterUrl,
        projected.title.slice(0, 1).toUpperCase()
      ));
      const copy = doc.createElement('span');
      copy.className = 'media-home-discovery-copy';
      const name = doc.createElement('strong');
      name.textContent = projected.title;
      copy.appendChild(name);
      if (projected.subtitle) {
        const detail = doc.createElement('span');
        detail.textContent = projected.subtitle;
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
      label.textContent = genreLabel(entry);
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

  function renderFolderRail(entries, rootRecordings, backendId) {
    const roots = uniqueCanonicalRecordings(rootRecordings || [], backendId);
    if (!entries.length && !roots.length) {
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
    if (roots.length) {
      const rootCard = doc.createElement('button');
      rootCard.type = 'button';
      rootCard.className = 'media-home-discovery-card folder-root';
      rootCard.dataset.rootRecordingGroup = 'true';
      const rootCopy = doc.createElement('span');
      rootCopy.className = 'media-home-discovery-copy';
      const rootLabel = doc.createElement('strong');
      rootLabel.textContent = 'Hauptverzeichnis';
      const rootCount = doc.createElement('span');
      rootCount.textContent = String(roots.length) + ' Aufnahmen';
      rootCopy.append(rootLabel, rootCount);
      rootCard.appendChild(rootCopy);
      rootCard.addEventListener('click', function () {
        renderRecordingRail('folders', 'Hauptverzeichnis', roots, backendId, {
          backLabel: '← Aufnahmeordner',
          onBack: function () {
            renderFolderRail(
              state.folderProjection.folders,
              state.folderProjection.rootRecordings,
              state.folderBackendId || backendId
            );
          }
        });
      });
      rail.appendChild(rootCard);
    }
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

  function canonicalSeriesPath(recording) {
    const value = recordingPath(recording) || text(recording && recording.title);
    const parts = value.split('/').map(text).filter(Boolean);
    const seriesIndex = parts.findIndex(function (part) {
      return part.toLowerCase() === 'serien';
    });
    if (seriesIndex >= 0 && parts.length > seriesIndex + 1) {
      return 'Serien/' + parts[seriesIndex + 1];
    }
    if (parts.length >= 2) {
      return parts.slice(0, -1).join('/');
    }
    return '';
  }

  function seriesFolderTitle(recording) {
    const path = canonicalSeriesPath(recording);
    const parts = path.split('/').map(text).filter(Boolean);
    if (!parts.length) return '';
    return parts[parts.length - 1];
  }

  function episodeToken(recording) {
    const value = text(recording && recording.title) || recordingPath(recording);
    const leaf = value.split('/').filter(Boolean).pop() || value;
    const match = leaf.match(/\bS(\d{1,3})\s*E(\d{1,4})\b/i);
    if (!match) return {seasonNumber: 0, episodeNumber: 0};
    return {
      seasonNumber: Number(match[1]) || 0,
      episodeNumber: Number(match[2]) || 0
    };
  }

  function episodeLeafTitle(recording) {
    const value = text(recording && recording.title) || recordingPath(recording);
    const leaf = value.split('/').filter(Boolean).pop() || value;
    return text(leaf.replace(/^S\d{1,3}\s*E\d{1,4}\s*[-–—:.]?\s*/i, ''));
  }

  function seriesMemberProjection(recording, richMetadata, backendId) {
    const sourceProvider = provider(recording);
    const rich = richMetadata && typeof richMetadata === 'object' ? richMetadata :
      (recording && recording.seriesMetadata) || {};
    const token = episodeToken(recording);
    const folderPath = canonicalSeriesPath(recording);
    const folderTitle = seriesFolderTitle(recording);
    const providerSeriesTitle = text(sourceProvider.seriesTitle);
    const richMediaType = text(rich.mediaType).toLowerCase();
    const richSeriesTitle = richMediaType === 'episode' || richMediaType === 'series'
      ? text(rich.title)
      : '';
    const seriesTitle = richSeriesTitle || providerSeriesTitle || folderTitle || recordingTitle(recording);

    const providerSeriesId = text(sourceProvider.seriesId);
    const richProvider = text(rich.provider);
    const manualEpisode = richProvider === 'manual' &&
      richMediaType === 'episode';
    const manualSeriesPresentation = richProvider === 'manual' &&
      richMediaType === 'series';
    const embeddedSeriesMetadata =
      recording &&
      recording.seriesMetadata &&
      typeof recording.seriesMetadata === 'object'
        ? recording.seriesMetadata
        : {};
    const hierarchyMetadata =
      manualSeriesPresentation &&
      Object.keys(embeddedSeriesMetadata).length
        ? embeddedSeriesMetadata
        : rich;
    const parsedProviderId = Number(rich.providerId);
    const richProviderId = Number.isFinite(parsedProviderId) ? parsedProviderId : 0;
    const nativeMetadataAvailable = rich.available === true &&
      (richMediaType === 'episode' || richMediaType === 'series') &&
      Boolean(richProvider && richProviderId !== 0);
    let key = '';
    if (manualEpisode && folderPath) {
      key = 'folder:' + folderPath.toLowerCase();
    } else if (providerSeriesId) {
      key = 'provider:' + providerSeriesId;
    } else if (folderPath) {
      key = 'folder:' + folderPath.toLowerCase();
    } else if (richProvider && richProviderId !== 0) {
      key = 'native:' + richProvider + ':' + String(richProviderId);
    } else key = 'title:' + seriesTitle.toLowerCase();

    const seasonNumber = Number(
      hierarchyMetadata.seasonNumber ||
      sourceProvider.seasonNumber ||
      token.seasonNumber ||
      0
    );
    const episodeNumber = Number(
      hierarchyMetadata.episodeNumber ||
      sourceProvider.episodeNumber ||
      token.episodeNumber ||
      0
    );
    const metadataPosterUrl = recordingMetadataPosterUrl(rich);
    const nativeArtworkAvailable = rich.available === true && Boolean(metadataPosterUrl);
    const posterUrl = metadataPosterUrl || recordingPosterUrl(recording);
    const episodeTitle = text(
      hierarchyMetadata.episodeName ||
      rich.episodeName ||
      sourceProvider.episodeTitle ||
      episodeLeafTitle(recording) ||
      recordingSubtitle(recording)
    ) || 'Folge';

    return {
      recording: recording,
      backendId: recordingBackendId(recording, backendId),
      seriesKey: key,
      seriesTitle: seriesTitle,
      seriesPath: folderPath,
      posterUrl: posterUrl,
      nativeMetadataAvailable: nativeMetadataAvailable,
      nativeArtworkAvailable: nativeArtworkAvailable,
      seasonNumber: seasonNumber > 0 ? seasonNumber : 0,
      episodeNumber: episodeNumber > 0 ? episodeNumber : 0,
      episodeTitle: episodeTitle
    };
  }

  function seriesHierarchyOverride(recording) {
    const value = recording && recording.seriesHierarchyOverride;
    return value && typeof value === 'object' && value.available === true
      ? value
      : null;
  }

  function seriesHierarchySyntheticSeasonNumber(groupType, groupLabel) {
    const value = text(groupType) + '\n' + text(groupLabel);
    let hash = 2166136261;
    for (let index = 0; index < value.length; index += 1) {
      hash ^= value.charCodeAt(index);
      hash = Math.imul(hash, 16777619);
    }
    return -1 - ((hash >>> 0) % 1000000000);
  }

  function seriesHierarchySeasonSortOrder(member, seasonNumber) {
    const groupType = text(member && member.seriesHierarchyGroupType);
    if (groupType === 'special' || groupType === 'custom') {
      const configured = Number(member.seriesHierarchySortOrder);
      if (Number.isFinite(configured) && configured !== 0) return configured;
      return groupType === 'special' ? 500 : 750;
    }
    if (Number(seasonNumber) > 0) return Number(seasonNumber) * 10;
    return 1000000;
  }

  function applySeriesHierarchyOverride(member, recording) {
    if (!member) return member;

    const projected = Object.assign({}, member);
    const nativeSeason = Number(
      member.nativeSeasonNumber !== undefined
        ? member.nativeSeasonNumber
        : member.seasonNumber || 0
    );
    const nativeEpisode = Number(
      member.nativeEpisodeNumber !== undefined
        ? member.nativeEpisodeNumber
        : member.episodeNumber || 0
    );

    projected.nativeSeasonNumber =
      Number.isFinite(nativeSeason) && nativeSeason > 0 ? nativeSeason : 0;
    projected.nativeEpisodeNumber =
      Number.isFinite(nativeEpisode) && nativeEpisode > 0 ? nativeEpisode : 0;

    projected.seasonNumber = projected.nativeSeasonNumber;
    projected.episodeNumber = projected.nativeEpisodeNumber;
    projected.episodeEnd = 0;
    projected.seriesHierarchyOverrideAvailable = false;
    projected.seriesHierarchyGroupType = '';
    projected.seriesHierarchyGroupLabel = '';
    projected.seriesHierarchySortOrder = 0;
    projected.seriesHierarchyRevision = 0;

    const override = seriesHierarchyOverride(recording);
    if (!override) return projected;

    const groupType = text(override.groupType).toLowerCase();
    const seasonNumber = Number(override.seasonNumber || 0);
    const groupLabel = text(override.groupLabel);
    const episodeStart = Number(override.episodeStart || 0);
    const episodeEnd = Number(override.episodeEnd || 0);

    if (groupType === 'season' && seasonNumber > 0) {
      projected.seasonNumber = seasonNumber;
      projected.seriesHierarchyGroupLabel =
        'Staffel ' + String(seasonNumber);
    } else if (
      (groupType === 'special' || groupType === 'custom') &&
      groupLabel
    ) {
      projected.seasonNumber =
        seriesHierarchySyntheticSeasonNumber(groupType, groupLabel);
      projected.seriesHierarchyGroupLabel = groupLabel;
    } else {
      return projected;
    }

    projected.seriesHierarchyOverrideAvailable = true;
    projected.seriesHierarchyGroupType = groupType;
    projected.seriesHierarchySortOrder =
      Number.isFinite(Number(override.sortOrder))
        ? Number(override.sortOrder)
        : 0;
    projected.seriesHierarchyRevision =
      Number.isFinite(Number(override.revision))
        ? Number(override.revision)
        : 0;

    if (episodeStart > 0) projected.episodeNumber = episodeStart;
    if (episodeEnd > 0 && episodeEnd >= episodeStart) {
      projected.episodeEnd = episodeEnd;
    }

    return projected;
  }

  function seriesEpisodeNumberLabel(member) {
    const start = Number(member && member.episodeNumber || 0);
    const end = Number(member && member.episodeEnd || 0);
    if (start > 0 && end >= start && end !== start) {
      return 'Folge ' + String(start) + '–' + String(end);
    }
    return start > 0 ? 'Folge ' + String(start) : '';
  }

  function seriesHierarchyCanEdit(member) {
    if (!member || !member.recording) return false;
    if (!text(member.recording.resourceKey)) return false;
    return member.seriesHierarchyOverrideAvailable === true ||
      Number(member.nativeSeasonNumber || 0) <= 0;
  }

  function seriesHierarchyCsrfHeaders() {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.csrfHeaders !== 'function') return {};
    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object' ? headers : {};
  }

  function requestSeriesHierarchyOverride(client, backendId, recording, mutation) {
    if (!client || typeof client.requestJson !== 'function') {
      return Promise.reject(
        new Error('Client API für Serienzuordnung ist nicht verfügbar.')
      );
    }

    const resourceKey = text(recording && recording.resourceKey);
    if (!resourceKey) {
      return Promise.reject(
        new Error('Die Aufnahme besitzt keinen kanonischen resourceKey.')
      );
    }

    const payload = Object.assign({}, mutation || {}, {
      resourceKey: resourceKey
    });

    return client.requestJson(
      '/api/backends/' + encodeURIComponent(text(backendId) || 'default') +
        '/recordings/series-hierarchy',
      {
        method: 'POST',
        headers: Object.assign(
          {'Content-Type': 'application/json'},
          seriesHierarchyCsrfHeaders()
        ),
        body: JSON.stringify(payload),
        cache: 'no-store',
        credentials: 'same-origin'
      }
    );
  }

  function ensureSeriesHierarchyStyles() {
    if (!doc || !doc.head || typeof doc.createElement !== 'function') return;
    if (typeof doc.getElementById === 'function' &&
        doc.getElementById('vdr-suite-home-series-hierarchy-style')) return;

    const style = doc.createElement('style');
    style.id = 'vdr-suite-home-series-hierarchy-style';
    style.textContent = [
      '.media-home-series-hierarchy-action{display:inline-flex;align-items:center;justify-content:center;margin-top:.35rem;padding:.28rem .48rem;border:1px solid rgba(96,165,250,.45);border-radius:.55rem;color:#bfdbfe;background:rgba(30,64,175,.18);font-size:.78rem;font-weight:650;cursor:pointer}',
      '.media-home-series-hierarchy-editor{display:grid;gap:.85rem;margin:1rem;padding:1rem;border:1px solid rgba(96,165,250,.35);border-radius:.9rem;background:rgba(15,23,42,.92);color:#e2e8f0}',
      '.media-home-series-hierarchy-editor h4{margin:0;color:#f8fafc;font-size:1.05rem}',
      '.media-home-series-hierarchy-editor p{margin:0;color:#94a3b8}',
      '.media-home-series-hierarchy-fields{display:grid;grid-template-columns:repeat(auto-fit,minmax(9rem,1fr));gap:.65rem}',
      '.media-home-series-hierarchy-fields label{display:grid;gap:.28rem;color:#cbd5e1;font-size:.82rem}',
      '.media-home-series-hierarchy-fields input{min-width:0;border:1px solid rgba(148,163,184,.3);border-radius:.55rem;background:#0f172a;color:#f8fafc;padding:.52rem .58rem}',
      '.media-home-series-hierarchy-actions{display:flex;flex-wrap:wrap;gap:.55rem}',
      '.media-home-series-hierarchy-actions button{border:1px solid rgba(148,163,184,.3);border-radius:.6rem;background:rgba(30,41,59,.95);color:#e2e8f0;padding:.5rem .7rem;cursor:pointer}',
      '.media-home-series-hierarchy-actions button.primary{border-color:rgba(96,165,250,.55);background:rgba(37,99,235,.32);color:#eff6ff}',
      '.media-home-series-hierarchy-status{min-height:1.2rem;color:#93c5fd}',
      '.media-home-series-hierarchy-status.error{color:#fecaca}'
    ].join('');
    doc.head.appendChild(style);
  }

  function closeSeriesHierarchyEditor(section) {
    if (!section) return false;
    const editor = section.__vdrSuiteSeriesHierarchyEditor;
    if (editor && typeof editor.remove === 'function') editor.remove();
    section.__vdrSuiteSeriesHierarchyEditor = null;
    return true;
  }

  function updateSeriesHierarchyProjection(member, response, backendId) {
    if (!member || !member.recording || !response || typeof response !== 'object') {
      return false;
    }

    member.recording.seriesHierarchyOverride =
      response.available === true
        ? response
        : {available: false};

    const seriesIndex = state.seriesProjection.findIndex(function (candidate) {
      return candidate.key === member.seriesKey;
    });

    if (seriesIndex < 0) return false;

    const currentSeries = state.seriesProjection[seriesIndex];
    const rebuilt = buildSeriesProjection(currentSeries.episodes).find(function (candidate) {
      return candidate.key === currentSeries.key;
    });

    if (!rebuilt) return false;

    state.seriesProjection[seriesIndex] = rebuilt;
    state.seriesWarmBackendId = backendId;
    state.seriesWarmCompletedAt = Date.now();

    const recordingKey = JSON.stringify([
      member.backendId || backendId,
      recordingId(member.recording)
    ]);

    const updatedMember = rebuilt.episodes.find(function (candidate) {
      return JSON.stringify([
        candidate.backendId || backendId,
        recordingId(candidate.recording)
      ]) === recordingKey;
    }) || null;

    const selectedSeason = updatedMember
      ? rebuilt.seasons.find(function (season) {
          return season.number === updatedMember.seasonNumber;
        }) || null
      : null;

    const section = sectionFor('series');
    closeSeriesHierarchyEditor(section);

    return renderSeriesDetail(
      rebuilt,
      selectedSeason,
      backendId
    );
  }


  function ensureSeriesHierarchySimpleUiStyle() {
    if (!doc ||
        !doc.head ||
        typeof doc.createElement !== 'function') {
      return;
    }

    const styleId =
      'vdr-suite-series-hierarchy-simple-ui';

    if (typeof doc.getElementById === 'function' &&
        doc.getElementById(styleId)) {
      return;
    }

    const style = doc.createElement('style');
    style.id = styleId;
    style.textContent = [
      '.media-home-series-hierarchy-action{',
      'display:flex!important;',
      'align-items:center;',
      'justify-content:center;',
      'box-sizing:border-box;',
      'width:calc(100% - 16px)!important;',
      'min-height:44px!important;',
      'margin:8px!important;',
      'padding:9px 12px!important;',
      'border:1px solid rgba(117,163,255,.6)!important;',
      'border-radius:8px!important;',
      'background:#13294f!important;',
      'font-weight:700!important;',
      'font-size:14px!important;',
      'cursor:pointer!important;',
      'user-select:none;',
      '}',
      '.media-home-series-hierarchy-action:focus-visible{',
      'outline:2px solid #8ab4ff;',
      'outline-offset:2px;',
      '}',
      '.media-home-series-hierarchy-modal{',
      'position:fixed!important;',
      'left:50%!important;',
      'top:50%!important;',
      'transform:translate(-50%,-50%)!important;',
      'z-index:10050!important;',
      'box-sizing:border-box!important;',
      'width:min(520px,calc(100vw - 32px))!important;',
      'max-height:calc(100vh - 32px)!important;',
      'overflow:auto!important;',
      'margin:0!important;',
      'padding:20px!important;',
      'border:1px solid rgba(117,163,255,.6)!important;',
      'border-radius:14px!important;',
      'background:#0e1729!important;',
      'box-shadow:',
      '0 0 0 100vmax rgba(1,8,20,.78),',
      '0 18px 60px rgba(0,0,0,.5)!important;',
      '}',
      '.media-home-series-hierarchy-modal button,',
      '.media-home-series-hierarchy-modal [role="button"]{',
      'display:flex;',
      'align-items:center;',
      'justify-content:center;',
      'box-sizing:border-box;',
      'width:100%;',
      'min-height:46px;',
      'margin:7px 0!important;',
      'padding:10px 14px!important;',
      'font-weight:700;',
      'cursor:pointer;',
      '}',
      '.media-home-series-hierarchy-simple-intro{',
      'margin:4px 0 14px;',
      'color:#aebbd1;',
      '}',
      '@media (max-width:720px){',
      '.media-home-series-hierarchy-modal{',
      'width:calc(100vw - 20px)!important;',
      'padding:16px!important;',
      '}',
      '}'
    ].join('');

    doc.head.appendChild(style);
  }

  function simplifySeriesHierarchyEditor() {
    ensureSeriesHierarchySimpleUiStyle();

    if (!doc ||
        typeof doc.querySelector !== 'function') {
      return;
    }

    const editor = doc.querySelector(
      '.media-home-series-hierarchy-modal'
    );

    if (!editor) return;

    editor.setAttribute(
      'role',
      'dialog'
    );
    editor.setAttribute(
      'aria-modal',
      'true'
    );
    editor.setAttribute(
      'aria-label',
      'Aufnahme zuordnen'
    );

    if (editor.dataset) {
      editor.dataset.simpleHierarchyUi = '1';
    }

    if (typeof editor.querySelectorAll !== 'function') {
      return;
    }

    Array.from(
      editor.querySelectorAll(
        'input, select, textarea'
      )
    ).forEach(function (field) {
      if ('value' in field) {
        field.value = '';
      }

      const label =
        typeof field.closest === 'function'
          ? field.closest('label')
          : null;

      const container =
        label ||
        (
          field.parentNode &&
          field.parentNode !== editor
            ? field.parentNode
            : field
        );

      if (container && container.style) {
        container.style.display = 'none';
      }
    });

    Array.from(
      editor.querySelectorAll(
        'button, [role="button"]'
      )
    ).forEach(function (control) {
      const label = text(
        control.textContent
      );

      if (
        label === 'Staffel zuordnen' ||
        label === 'Eigene Gruppe übernehmen'
      ) {
        if (control.style) {
          control.style.display = 'none';
        }
      }
    });

    Array.from(
      editor.querySelectorAll(
        'h1, h2, h3, strong'
      )
    ).forEach(function (heading) {
      if (
        text(heading.textContent) ===
        'Folge zuordnen'
      ) {
        heading.textContent =
          'Aufnahme zuordnen';
      }
    });

    Array.from(
      editor.querySelectorAll(
        'p, span, div'
      )
    ).forEach(function (element) {
      const label = text(
        element.textContent
      );

      if (
        label.indexOf(
          'automatische Hierarchie bleibt unverändert gespeichert'
        ) >= 0 &&
        !element.querySelector(
          'button, [role="button"]'
        )
      ) {
        if (element.style) {
          element.style.display = 'none';
        }
      }
    });

    if (!editor.querySelector(
          '.media-home-series-hierarchy-simple-intro'
        )) {
      const intro =
        doc.createElement('p');

      intro.className =
        'media-home-series-hierarchy-simple-intro';

      intro.textContent =
        'Wohin gehört diese Aufnahme?';

      editor.insertBefore(
        intro,
        editor.children &&
        editor.children.length > 1
          ? editor.children[1]
          : null
      );
    }

    const firstAction =
      Array.from(
        editor.querySelectorAll(
          'button, [role="button"]'
        )
      ).find(function (control) {
        return !control.style ||
          control.style.display !== 'none';
      });

    if (firstAction &&
        typeof firstAction.focus === 'function') {
      firstAction.focus();
    }
  }

  function openSeriesHierarchyEditor(member, series, backendId) {
    if (!seriesHierarchyCanEdit(member)) return false;

    const section = sectionFor('series');
    if (!section) return false;

    ensureSeriesHierarchyStyles();
    closeSeriesHierarchyEditor(section);

    const editor = doc.createElement('div');
    editor.className = 'media-home-series-hierarchy-editor media-home-series-hierarchy-modal';
    editor.dataset.recordingId = recordingId(member.recording);

    const heading = doc.createElement('h4');
    heading.textContent = 'Folge zuordnen';

    const description = doc.createElement('p');
    description.textContent =
      member.episodeTitle +
      ' · automatische Hierarchie bleibt unverändert gespeichert';

    const fields = doc.createElement('div');
    fields.className = 'media-home-series-hierarchy-fields';

    function field(labelText, type, value, min) {
      const label = doc.createElement('label');
      label.textContent = labelText;
      const input = doc.createElement('input');
      input.type = type;
      input.value = value === undefined || value === null ? '' : String(value);
      if (min !== undefined) input.min = String(min);
      label.appendChild(input);
      fields.appendChild(label);
      return input;
    }

    const seasonInput = field('Staffel', 'number', '', 1);
    const episodeStartInput = field(
      'Folge von',
      'number',
      member.episodeNumber > 0 ? member.episodeNumber : '',
      1
    );
    const episodeEndInput = field(
      'Folge bis (optional)',
      'number',
      member.episodeEnd > 0 ? member.episodeEnd : '',
      1
    );
    const customInput = field('Eigene Gruppe', 'text', '');

    const status = doc.createElement('div');
    status.className = 'media-home-series-hierarchy-status';

    const actions = doc.createElement('div');
    actions.className = 'media-home-series-hierarchy-actions';

    function rangePayload() {
      const start = Number(episodeStartInput.value || 0);
      const end = Number(episodeEndInput.value || 0);
      return {
        episodeStart: Number.isFinite(start) && start > 0 ? Math.floor(start) : 0,
        episodeEnd: Number.isFinite(end) && end > 0 ? Math.floor(end) : 0
      };
    }

    function setStatus(message, error) {
      status.textContent = message || '';
      status.className =
        'media-home-series-hierarchy-status' + (error ? ' error' : '');
    }

    function submit(mutation) {
      const client = clientApi();
      setStatus('Zuordnung wird gespeichert …', false);

      const expectedRevision =
        member.seriesHierarchyOverrideAvailable === true
          ? Number(member.seriesHierarchyRevision || 0)
          : 0;

      const payload = Object.assign(
        {},
        mutation,
        rangePayload(),
        expectedRevision > 0
          ? {expectedRevision: expectedRevision}
          : {}
      );

      return requestSeriesHierarchyOverride(
        client,
        backendId,
        member.recording,
        payload
      ).then(function (response) {
        if (!response || typeof response !== 'object') {
          throw new Error('Ungültige Antwort der Serienzuordnung.');
        }
        if (!updateSeriesHierarchyProjection(member, response, backendId)) {
          throw new Error('Serienansicht konnte nicht aktualisiert werden.');
        }
        return true;
      }).catch(function (error) {
        setStatus(
          text(error && error.message) ||
            'Serienzuordnung konnte nicht gespeichert werden.',
          true
        );
        return false;
      });
    }

    function button(label, mutation, primary) {
      const control = doc.createElement('button');
      control.type = 'button';
      control.textContent = label;
      if (primary) control.className = 'primary';
      control.addEventListener('click', function (event) {
        if (event && typeof event.preventDefault === 'function') event.preventDefault();
        if (event && typeof event.stopPropagation === 'function') event.stopPropagation();
        submit(mutation);
      });
      actions.appendChild(control);
      return control;
    }

    const existingSeasonNumbers = [];
    (series && series.seasons || []).forEach(function (season) {
      if (Number(season.number) <= 0) return;
      if (existingSeasonNumbers.indexOf(Number(season.number)) >= 0) return;
      existingSeasonNumbers.push(Number(season.number));
    });

    existingSeasonNumbers.sort(function (left, right) {
      return left - right;
    }).forEach(function (seasonNumber) {
      button(
        'Staffel ' + String(seasonNumber),
        {
          operation: 'set',
          groupType: 'season',
          seasonNumber: seasonNumber,
          groupLabel: '',
          sortOrder: 0
        },
        false
      );
    });

    const manualSeason = doc.createElement('button');
    manualSeason.type = 'button';
    manualSeason.textContent = 'Staffel zuordnen';
    manualSeason.className = 'primary';
    manualSeason.addEventListener('click', function (event) {
      if (event && typeof event.preventDefault === 'function') event.preventDefault();
      if (event && typeof event.stopPropagation === 'function') event.stopPropagation();

      const seasonNumber = Number(seasonInput.value || 0);
      if (!Number.isFinite(seasonNumber) || seasonNumber <= 0) {
        setStatus('Bitte eine gültige Staffelnummer eingeben.', true);
        return;
      }

      submit({
        operation: 'set',
        groupType: 'season',
        seasonNumber: Math.floor(seasonNumber),
        groupLabel: '',
        sortOrder: 0
      });
    });
    actions.appendChild(manualSeason);

    button(
      'Pilot / Miniserie',
      {
        operation: 'set',
        groupType: 'special',
        seasonNumber: 0,
        groupLabel: 'Pilot / Miniserie',
        sortOrder: -100
      },
      true
    );

    button(
      'TV-Filme / Specials',
      {
        operation: 'set',
        groupType: 'special',
        seasonNumber: 0,
        groupLabel: 'TV-Filme / Specials',
        sortOrder: 1000
      },
      true
    );

    const custom = doc.createElement('button');
    custom.type = 'button';
    custom.textContent = 'Eigene Gruppe übernehmen';
    custom.addEventListener('click', function (event) {
      if (event && typeof event.preventDefault === 'function') event.preventDefault();
      if (event && typeof event.stopPropagation === 'function') event.stopPropagation();

      const groupLabel = text(customInput.value);
      if (!groupLabel) {
        setStatus('Bitte einen Gruppennamen eingeben.', true);
        return;
      }

      submit({
        operation: 'set',
        groupType: 'custom',
        seasonNumber: 0,
        groupLabel: groupLabel,
        sortOrder: 750
      });
    });
    actions.appendChild(custom);

    if (member.seriesHierarchyOverrideAvailable === true) {
      button(
        'Automatische Zuordnung',
        {
          operation: 'clear'
        },
        false
      );
    }

    const cancel = doc.createElement('button');
    cancel.type = 'button';
    cancel.textContent = 'Abbrechen';
    cancel.addEventListener('click', function (event) {
      if (event && typeof event.preventDefault === 'function') event.preventDefault();
      if (event && typeof event.stopPropagation === 'function') event.stopPropagation();
      closeSeriesHierarchyEditor(section);
    });
    actions.appendChild(cancel);

    editor.append(heading, description, fields, actions, status);
    section.appendChild(editor);
    section.__vdrSuiteSeriesHierarchyEditor = editor;

    if (typeof editor.scrollIntoView === 'function') {
      editor.scrollIntoView({block: 'nearest', behavior: 'smooth'});
    }

    return true;
  }

  function buildSeriesProjection(members) {
    const groups = new Map();
    (members || []).filter(Boolean).forEach(function (rawMember) {
      const member = applySeriesHierarchyOverride(
        rawMember,
        rawMember && rawMember.recording
      );
      let series = groups.get(member.seriesKey);
      if (!series) {
        series = {
          key: member.seriesKey,
          title: member.seriesTitle,
          path: member.seriesPath,
          posterUrl: member.posterUrl,
          nativeMetadataAvailable: Boolean(member.nativeMetadataAvailable),
          nativeArtworkAvailable: Boolean(member.nativeArtworkAvailable),
          episodes: [],
          seasons: []
        };
        groups.set(member.seriesKey, series);
      }
      if (member.nativeMetadataAvailable && !series.nativeMetadataAvailable) {
        series.nativeMetadataAvailable = true;
        if (member.seriesTitle) series.title = member.seriesTitle;
      }
      if (member.nativeArtworkAvailable && !series.nativeArtworkAvailable) {
        series.nativeArtworkAvailable = true;
        if (member.posterUrl) series.posterUrl = member.posterUrl;
      } else if (!series.posterUrl && member.posterUrl) {
        series.posterUrl = member.posterUrl;
      }
      series.episodes.push(member);
    });

    const result = Array.from(groups.values());
    result.forEach(function (series) {
      const seasons = new Map();
      series.episodes.forEach(function (member) {
        const number = member.seasonNumber;
        const key = String(number);
        let season = seasons.get(key);
        if (!season) {
          season = {
            number: number,
            label: text(member.seriesHierarchyGroupLabel) ||
              (number > 0 ? 'Staffel ' + String(number) : 'Staffel unbekannt'),
            groupType: text(member.seriesHierarchyGroupType) ||
              (number > 0 ? 'season' : 'unknown'),
            sortOrder: seriesHierarchySeasonSortOrder(member, number),
            episodes: []
          };
          seasons.set(key, season);
        }
        season.episodes.push(member);
      });
      series.seasons = Array.from(seasons.values()).sort(function (left, right) {
        const leftOrder = Number(left.sortOrder);
        const rightOrder = Number(right.sortOrder);
        if (Number.isFinite(leftOrder) && Number.isFinite(rightOrder) &&
            leftOrder !== rightOrder) {
          return leftOrder - rightOrder;
        }
        if (left.number === 0 && right.number !== 0) return 1;
        if (right.number === 0 && left.number !== 0) return -1;
        return left.number - right.number;
      });
      series.seasons.forEach(function (season) {
        season.episodes.sort(function (left, right) {
          if (left.episodeNumber && right.episodeNumber && left.episodeNumber !== right.episodeNumber) {
            return left.episodeNumber - right.episodeNumber;
          }
          if (left.episodeNumber && !right.episodeNumber) return -1;
          if (!left.episodeNumber && right.episodeNumber) return 1;
          return left.episodeTitle.localeCompare(right.episodeTitle, 'de');
        });
      });
    });
    return result.sort(function (left, right) {
      return left.title.localeCompare(right.title, 'de');
    });
  }

  function seriesCountLabel(series) {
    const episodeCount = series.episodes.length;
    const seasonCount = series.seasons.length;
    return String(seasonCount) + (seasonCount === 1 ? ' Staffel' : ' Staffeln') +
      ' · ' + String(episodeCount) + (episodeCount === 1 ? ' Folge' : ' Folgen');
  }

  function appendSectionHeading(section, title, backLabel, onBack) {
    const heading = doc.createElement('div');
    heading.className = 'media-home-section-heading media-home-series-heading';
    if (backLabel && typeof onBack === 'function') {
      const back = doc.createElement('button');
      back.type = 'button';
      back.className = 'media-home-series-back';
      back.textContent = backLabel;
      back.addEventListener('click', onBack);
      heading.appendChild(back);
    }
    const name = doc.createElement('h3');
    name.textContent = title;
    heading.appendChild(name);
    section.appendChild(heading);
  }

  function reconcileSeriesChildren(parent, children) {
    const previous = Array.from(parent.children);
    if (previous.length === children.length && previous.every(function (child, index) {
      return child === children[index];
    })) return;
    const scrollLeft = parent.scrollLeft;
    const focused = doc.activeElement;
    const retained = new Set(children);
    previous.forEach(function (child) {
      if (!retained.has(child)) child.remove();
    });
    children.forEach(function (child, index) {
      if (parent.children[index] !== child) parent.insertBefore(child, parent.children[index] || null);
    });
    if (focused && children.indexOf(focused) >= 0 && doc.activeElement !== focused &&
        typeof focused.focus === 'function') focused.focus({preventScroll: true});
    if (Number.isFinite(scrollLeft)) parent.scrollLeft = scrollLeft;
  }

  function renderSeriesCoverPicker(
    view,
    series,
    backendId
  ) {
    const picker = view && view.coverPicker;

    if (!picker) return false;

    picker.hidden = false;
    picker.replaceChildren();

    const heading =
      doc.createElement('div');

    heading.className =
      'media-home-series-cover-heading';

    const title =
      doc.createElement('h4');

    title.textContent =
      'Seriencover suchen';

    heading.appendChild(title);

    const override =
      seriesCoverOverride(
        series,
        backendId
      );

    if (override) {
      const reset =
        doc.createElement('button');

      reset.type = 'button';
      reset.className =
        'media-home-series-cover-action';

      reset.textContent =
        'Automatisches Cover';

      reset.addEventListener(
        'click',
        function () {
          reset.disabled = true;

          updateSeriesCoverOverride(
            view.series,
            backendId,
            null
          ).catch(function (error) {
            reset.disabled = false;
            status.textContent =
              error && error.message
                ? error.message
                : 'Cover konnte nicht zurückgesetzt werden.';
          });
        }
      );

      heading.appendChild(reset);
    }

    const close =
      doc.createElement('button');

    close.type = 'button';
    close.className =
      'media-home-series-cover-action';

    close.textContent = 'Schließen';

    close.addEventListener(
      'click',
      function () {
        picker.hidden = true;
      }
    );

    heading.appendChild(close);
    picker.appendChild(heading);

    const searchRow =
      doc.createElement('div');

    searchRow.className =
      'media-home-series-cover-search';

    const input =
      doc.createElement('input');

    input.type = 'search';
    input.value = text(series.title);
    input.placeholder = 'Serie bei TMDB suchen';
    input.setAttribute(
      'aria-label',
      'Serientitel'
    );

    const searchButton =
      doc.createElement('button');

    searchButton.type = 'button';
    searchButton.className =
      'media-home-series-cover-action primary';

    searchButton.textContent = 'Suchen';

    searchRow.append(
      input,
      searchButton
    );

    picker.appendChild(searchRow);

    const status =
      doc.createElement('div');

    status.className =
      'media-home-series-cover-status';

    status.setAttribute(
      'data-series-cover-status',
      'true'
    );

    status.setAttribute(
      'role',
      'status'
    );

    picker.appendChild(status);

    const results =
      doc.createElement('div');

    results.className =
      'media-home-series-cover-results';

    picker.appendChild(results);

    function renderResults(candidates) {
      results.replaceChildren();

      if (!candidates.length) {
        status.textContent =
          'Keine passende Serie mit Poster gefunden.';
        return;
      }

      status.textContent =
        String(candidates.length) +
        ' Treffer';

      candidates.forEach(
        function (candidate) {
          const card =
            doc.createElement('button');

          card.type = 'button';
          card.className =
            'media-home-series-cover-search-result';

          const image =
            doc.createElement('img');

          image.loading = 'lazy';
          image.alt = '';
          image.src =
            seriesCoverCandidateImageUrl(
              backendId,
              candidate
            );

          card.appendChild(image);

          const copy =
            doc.createElement('span');

          const candidateTitle =
            doc.createElement('strong');

          candidateTitle.textContent =
            text(candidate.title) ||
            'Ohne Titel';

          copy.appendChild(
            candidateTitle
          );

          const details = [
            text(candidate.releaseDate)
              .slice(0, 4),
            text(candidate.originalTitle) &&
            text(candidate.originalTitle) !==
              text(candidate.title)
              ? text(candidate.originalTitle)
              : ''
          ].filter(Boolean).join(' · ');

          if (details) {
            const small =
              doc.createElement('small');

            small.textContent = details;
            copy.appendChild(small);
          }

          card.appendChild(copy);

          card.addEventListener(
            'click',
            function () {
              card.disabled = true;

              status.textContent =
                'Seriencover wird gespeichert …';

              updateSeriesCoverOverride(
                view.series,
                backendId,
                candidate
              ).then(function () {
                picker.hidden = true;
              }).catch(function (error) {
                card.disabled = false;

                status.textContent =
                  error && error.message
                    ? error.message
                    : 'Seriencover konnte nicht gespeichert werden.';
              });
            }
          );

          results.appendChild(card);
        }
      );
    }

    function runSearch() {
      const query =
        input.value.trim();

      searchButton.disabled = true;

      status.textContent =
        'TMDB wird durchsucht …';

      results.replaceChildren();

      searchSeriesCoverCandidates(
        backendId,
        query
      ).then(function (candidates) {
        searchButton.disabled = false;
        renderResults(candidates);
      }).catch(function (error) {
        searchButton.disabled = false;

        status.textContent =
          error && error.message
            ? error.message
            : 'Seriensuche fehlgeschlagen.';
      });
    }

    searchButton.addEventListener(
      'click',
      runSearch
    );

    input.addEventListener(
      'keydown',
      function (event) {
        if (event.key !== 'Enter') return;

        event.preventDefault();
        runSearch();
      }
    );

    runSearch();

    return true;
  }

  function toggleSeriesCoverPicker(
    view,
    backendId
  ) {
    if (!view ||
        !view.coverPicker ||
        !view.series) {
      return false;
    }

    if (!view.coverPicker.hidden) {
      view.coverPicker.hidden = true;
      return true;
    }

    return renderSeriesCoverPicker(
      view,
      view.series,
      backendId
    );
  }

  function openSeriesDetail(series, backendId) {
    if (!series) return false;

    const client = clientApi();
    const generation = state.generation;
    const recordings = (series.episodes || []).map(function (member) {
      return member && member.recording;
    }).filter(Boolean);
    const canEnrich = Boolean(
      client &&
      typeof client.requestJson === 'function' &&
      recordings.length &&
      current(generation, backendId)
    );
    const hierarchyIncomplete = (series.episodes || []).some(function (member) {
      if (!member) return true;
      if (member.seriesHierarchyOverrideAvailable === true) return false;
      return Number(member.seasonNumber) <= 0 ||
        Number(member.episodeNumber) <= 0;
    });

    const rendered = renderSeriesDetail(
      series,
      null,
      backendId,
      canEnrich && hierarchyIncomplete
        ? {metadataLoading: true}
        : null
    );

    if (!canEnrich) return rendered;

    fetchSeriesRecordingMetadata(
      client,
      recordings,
      backendId,
      generation,
      null,
      {
        refreshUnsettled: true,
        refreshIncompleteHierarchy: true
      }
    ).then(function (rich) {
      if (!current(generation, backendId)) return false;

      const members = recordings.map(function (recording) {
        const nativeId = recordingBackendNativeId(recording);
        return seriesMemberProjection(
          recording,
          nativeId ? rich.get(nativeId) || null : null,
          backendId
        );
      });

      const projection = buildSeriesProjection(members);
      const enriched = projection.find(function (candidate) {
        return candidate.key === series.key;
      }) || projection[0];

      if (!enriched) return false;

      const index = state.seriesProjection.findIndex(function (candidate) {
        return candidate.key === series.key;
      });

      if (index >= 0) {
        state.seriesProjection[index] = enriched;
      }

      const selectedSeason = state.seriesSeasonNumber === null
        ? null
        : enriched.seasons.find(function (season) {
          return season.number === state.seriesSeasonNumber;
        }) || null;

      const renderedDetail = renderSeriesDetail(
        enriched,
        selectedSeason,
        backendId
      );

      if (!seriesMetadataComplete(
            recordings,
            generation,
            backendId
          )) {
        scheduleSeriesMetadataRetry(
          client,
          recordings,
          backendId,
          generation,
          SERIES_DETAIL_METADATA_RETRY_MS
        );
      }

      return renderedDetail;
    }).catch(function () {
      if (!current(generation, backendId)) return false;
      return renderSeriesDetail(series, null, backendId);
    });

    return rendered;
  }

  function renderSeriesRail(seriesEntries, backendId) {
    if (!seriesEntries.length) {
      clearRail('series');
      return true;
    }
    state.seriesViewKey = '';
    state.seriesSeasonNumber = null;
    const section = sectionFor('series');
    if (!section) return false;
    let rail = Array.from(section.children).find(function (child) {
      return child.className === 'media-home-discovery-rail series';
    });
    const saved = section.__vdrSuiteSeriesRail;
    let restore = null;
    if (!rail) {
      section.replaceChildren();
      appendSectionHeading(section, 'Serien');
      if (saved && saved.backendId === backendId) {
        rail = saved.rail;
        restore = saved;
      } else {
        rail = doc.createElement('div');
        rail.className = 'media-home-discovery-rail series';
      }
      section.appendChild(rail);
    }
    section.__vdrSuiteSeriesRail = {rail: rail, backendId: backendId};
    const previousScrollLeft = rail.scrollLeft;
    const existing = new Map(Array.from(rail.children).map(function (card) {
      return [card.dataset.seriesKey, card];
    }));
    const nextCards = [];
    seriesEntries.forEach(function (series) {
      const posterUrl = seriesCoverPosterUrl(series, backendId);
      const signature = JSON.stringify([backendId, series.title, posterUrl, seriesCountLabel(series)]);
      let card = existing.get(series.key);
      if (!card || card.dataset.presentation !== signature) {
        if (!card) {
          card = doc.createElement('button');
          card.addEventListener('click', function () {
            openSeriesDetail(card.__vdrSuiteSeries, card.dataset.backendId);
          });
        }
        card.replaceChildren();
        card.dataset.presentation = signature;
        card.type = 'button';
        card.className = 'media-home-discovery-card series';
        card.dataset.seriesKey = series.key;
        card.dataset.backendId = backendId;
        card.appendChild(createPosterArtwork(series.title, posterUrl, series.title.slice(0, 1)));
        const copy = doc.createElement('span');
        copy.className = 'media-home-discovery-copy';
        const label = doc.createElement('strong');
        label.textContent = series.title;
        const detail = doc.createElement('span');
        detail.textContent = seriesCountLabel(series);
        copy.append(label, detail);
        card.appendChild(copy);
      }
      card.__vdrSuiteSeries = series;
      nextCards.push(card);
    });
    reconcileSeriesChildren(rail, nextCards);
    if (restore) {
      if (restore.focused && nextCards.indexOf(restore.focused) >= 0 &&
          typeof restore.focused.focus === 'function') restore.focused.focus({preventScroll: true});
      if (Number.isFinite(restore.scrollLeft)) rail.scrollLeft = restore.scrollLeft;
    }
    return true;
  }

  function seriesMetadataPendingForDetail(series, backendId) {
    const cache = state.seriesMetadataCache;

    if (!series ||
        !cache ||
        cache.generation !== state.generation ||
        cache.backendId !== backendId) {
      return false;
    }

    return (series.episodes || []).some(function (member) {
      if (!member) return false;

      const hierarchyComplete =
        Number(member.seasonNumber) > 0 &&
        Number(member.episodeNumber) > 0;

      if (hierarchyComplete) return false;

      const nativeId = recordingBackendNativeId(
        member.recording
      );

      if (!nativeId) return false;

      // A hard failed or settled not-found result is no longer "loading".
      // It may legitimately remain unknown. Only unresolved/in-flight or
      // explicitly unsettled Metadata must keep provisional hierarchy hidden.
      if (cache.failedNativeIds.has(nativeId)) {
        return false;
      }

      if (cache.unsettledNativeIds.has(nativeId) ||
          cache.inflight.has(nativeId)) {
        return true;
      }

      return !cache.resolved.has(nativeId);
    });
  }

  function renderSeriesDetail(series, selectedSeason, backendId, options) {
    const config = options && typeof options === 'object' ? options : {};
    const metadataLoading =
      config.metadataLoading === true ||
      seriesMetadataPendingForDetail(series, backendId);
    state.seriesViewKey = series.key;
    state.seriesSeasonNumber = selectedSeason ? selectedSeason.number : null;
    const section = sectionFor('series');
    if (!section) return false;
    let view = section.__vdrSuiteSeriesDetail;
    if (!view || view.key !== series.key || view.backendId !== backendId ||
        view.summary.parentNode !== section) {
      const saved = section.__vdrSuiteSeriesRail;
      if (saved && saved.rail.parentNode === section && saved.backendId === backendId) {
        saved.scrollLeft = saved.rail.scrollLeft;
        saved.focused = doc.activeElement;
      }
      section.replaceChildren();
      appendSectionHeading(section, series.title, '← Serien', function () {
        renderSeriesRail(state.seriesProjection, state.seriesBackendId || backendId);
      });
      const summary = doc.createElement('div');
      summary.className = 'media-home-series-summary';
      section.appendChild(summary);
      const coverPicker = doc.createElement('div');
      coverPicker.className = 'media-home-series-cover-picker';
      coverPicker.hidden = true;
      section.appendChild(coverPicker);
      const seasonTitle = doc.createElement('h4');
      seasonTitle.className = 'media-home-series-subheading';
      seasonTitle.textContent = 'Staffeln';
      section.appendChild(seasonTitle);
      const seasonRail = doc.createElement('div');
      seasonRail.className = 'media-home-series-season-rail';
      section.appendChild(seasonRail);
      const episodeTitle = doc.createElement('h4');
      episodeTitle.className = 'media-home-series-subheading';
      const episodeRail = doc.createElement('div');
      episodeRail.className = 'media-home-discovery-rail series-episodes';
      view = {key: series.key, backendId: backendId, summary: summary,
        heading: section.children[0].children[1],
        coverPicker: coverPicker, seasonRail: seasonRail,
        episodeTitle: episodeTitle, episodeRail: episodeRail};
      section.__vdrSuiteSeriesDetail = view;
    }
    // Bound controls resolve current owner data, including non-visual Recording changes.
    view.series = series;
    view.selectedSeason = selectedSeason;
    view.metadataLoading = metadataLoading;
    if (view.heading.textContent !== series.title) view.heading.textContent = series.title;
    const detailCount = metadataLoading
      ? String(series.episodes.length) +
        (series.episodes.length === 1 ? ' Folge' : ' Folgen') +
        ' · Staffeln werden geladen …'
      : seriesCountLabel(series);
    const detailPosterUrl =
      seriesCoverPosterUrl(series, backendId);
    const summarySignature = JSON.stringify([
      series.title,
      detailPosterUrl,
      detailCount,
      metadataLoading
    ]);
    if (view.summarySignature !== summarySignature) {
      view.summarySignature = summarySignature;
      view.summary.replaceChildren();
      view.summary.appendChild(createPosterArtwork(
        series.title,
        detailPosterUrl,
        series.title.slice(0, 1)
      ));
      const copy = doc.createElement('div');
      copy.className = 'media-home-series-summary-copy';
      const title = doc.createElement('strong');
      title.textContent = series.title;
      const count = doc.createElement('span');
      count.textContent = detailCount;
      copy.append(title, count);

      const coverButton = doc.createElement('button');
      coverButton.type = 'button';
      coverButton.className =
        'media-home-series-cover-change';
      coverButton.textContent = 'Cover ändern';
      coverButton.disabled = metadataLoading;
      coverButton.addEventListener('click', function () {
        toggleSeriesCoverPicker(
          view,
          backendId
        );
      });

      copy.appendChild(coverButton);
      view.summary.appendChild(copy);
    }

    if (view.coverPicker &&
        !view.coverPicker.hidden) {
      renderSeriesCoverPicker(
        view,
        series,
        backendId
      );
    }

    if (metadataLoading) {
      const loading = doc.createElement('div');
      loading.className = 'media-home-discovery-state';
      loading.setAttribute('role', 'status');
      loading.textContent = 'Staffeln werden geladen …';
      reconcileSeriesChildren(view.seasonRail, [loading]);
      view.episodeTitle.remove();
      view.episodeRail.remove();
      return true;
    }

    const seasons = new Map(Array.from(view.seasonRail.children).map(function (button) {
      return [button.dataset.seasonNumber, button];
    }));
    const nextSeasons = series.seasons.map(function (season) {
      const key = String(season.number);
      let button = seasons.get(key);
      if (!button) {
        button = doc.createElement('button');
        button.type = 'button';
        button.dataset.seasonNumber = key;
        button.addEventListener('click', function () {
          const currentSeason = view.series.seasons.find(function (item) {
            return String(item.number) === button.dataset.seasonNumber;
          });
          renderSeriesDetail(view.series, currentSeason || null, backendId);
        });
      }
      const className = 'media-home-series-season' +
        (selectedSeason && selectedSeason.number === season.number ? ' selected' : '');
      if (button.className !== className) button.className = className;
      const label = season.label + ' · ' + String(season.episodes.length) +
        (season.episodes.length === 1 ? ' Folge' : ' Folgen');
      if (button.textContent !== label) button.textContent = label;
      return button;
    });
    reconcileSeriesChildren(view.seasonRail, nextSeasons);
    if (!selectedSeason) {
      view.episodeTitle.remove();
      view.episodeRail.remove();
      return true;
    }
    if (view.episodeTitle.textContent !== selectedSeason.label) view.episodeTitle.textContent = selectedSeason.label;
    if (view.episodeRail.parentNode !== section) section.append(view.episodeTitle, view.episodeRail);
    const episodes = new Map(Array.from(view.episodeRail.children).map(function (card) {
      return [JSON.stringify([card.dataset.backendId, card.dataset.recordingId]), card];
    }));
    const nextEpisodes = selectedSeason.episodes.map(function (member) {
      const recording = member.recording;
      const memberBackend = member.backendId || backendId;
      const id = recordingId(recording);
      const key = JSON.stringify([memberBackend, id]);
      let card = episodes.get(key);
      if (!card) {
        card = doc.createElement('button');
        card.type = 'button';
        card.className = 'media-home-discovery-card recording series-episode';
        card.dataset.recordingId = id;
        card.dataset.backendId = memberBackend;
        card.addEventListener('click', function () {
          const currentMember = card.__vdrSuiteSeriesMember;
          openRecording(currentMember.recording, currentMember.backendId || backendId, {
            backLabel: '← Zurück zur Staffel',
            onClose: function () {
              if (selectedBackendId() !== backendId || !selectShellModule('overview')) return;
              const currentSeries = state.seriesProjection.find(function (item) {
                return item.key === view.key;
              });
              if (state.seriesBackendId === backendId && !currentSeries) {
                renderSeriesRail(state.seriesProjection, backendId);
                return;
              }
              const returnedSeries = currentSeries || view.series;
              const returnedSeason = returnedSeries.seasons.find(function (item) {
                return view.selectedSeason && item.number === view.selectedSeason.number;
              }) || null;
              renderSeriesDetail(returnedSeries, returnedSeason, backendId);
              const seriesSection = sectionFor('series');
              if (seriesSection && typeof seriesSection.scrollIntoView === 'function') {
                seriesSection.scrollIntoView({block: 'start', behavior: 'auto'});
              }
            }
          });
        });
      }
      card.__vdrSuiteSeriesMember = member;
      const poster = member.posterUrl || recordingPosterUrl(recording);
      const signature = JSON.stringify([
        member.episodeNumber,
        member.episodeEnd,
        member.episodeTitle,
        poster,
        member.seriesHierarchyOverrideAvailable === true,
        member.seriesHierarchyGroupType,
        member.seriesHierarchyGroupLabel,
        member.seriesHierarchyRevision
      ]);
      if (card.dataset.presentation !== signature) {
        card.dataset.presentation = signature;
        card.dataset.episodeNumber = String(member.episodeNumber);
        card.replaceChildren();
        card.appendChild(createPosterArtwork(member.episodeTitle, poster, member.episodeTitle.slice(0, 1).toUpperCase()));
        const copy = doc.createElement('span');
        copy.className = 'media-home-discovery-copy';
        const label = doc.createElement('strong');
        label.textContent = seriesEpisodeNumberLabel(member) || member.episodeTitle;
        const detail = doc.createElement('span');
        detail.textContent = member.episodeTitle;
        copy.append(label, detail);
        card.appendChild(copy);

        if (seriesHierarchyCanEdit(member)) {
          ensureSeriesHierarchySimpleUiStyle();
          const hierarchyAction = doc.createElement('span');
          hierarchyAction.className = 'media-home-series-hierarchy-action';
          hierarchyAction.setAttribute('role', 'button');
          hierarchyAction.setAttribute('tabindex', '0');
          hierarchyAction.textContent =
            member.seriesHierarchyOverrideAvailable === true
              ? 'Zuordnung ändern'
              : 'Zuordnen';

          function openHierarchyEditor(event) {
            if (event && typeof event.preventDefault === 'function') event.preventDefault();
            if (event && typeof event.stopPropagation === 'function') event.stopPropagation();
            openSeriesHierarchyEditor(
              card.__vdrSuiteSeriesMember,
              view.series,
              backendId
            );
            simplifySeriesHierarchyEditor();
          }

          hierarchyAction.addEventListener('click', openHierarchyEditor);
          hierarchyAction.addEventListener('keydown', function (event) {
            const keyName = event && (event.key || event.code);
            if (keyName === 'Enter' || keyName === ' ' || keyName === 'Space') {
              openHierarchyEditor(event);
            }
          });

          card.appendChild(hierarchyAction);
        }
      }
      return card;
    });
    reconcileSeriesChildren(view.episodeRail, nextEpisodes);
    return true;
  }

  function applySeriesProjection(recordings, backendId, richMetadataByNativeId, options) {
    const rich = richMetadataByNativeId instanceof Map ? richMetadataByNativeId : null;
    const config = options && typeof options === 'object' ? options : {};
    const members = recordings.map(function (recording) {
      const nativeId = recordingBackendNativeId(recording);
      return seriesMemberProjection(
        recording,
        rich && nativeId ? rich.get(nativeId) || null : null,
        backendId
      );
    });
    let projection = buildSeriesProjection(members);
    if (config.requireRich === true && rich) {
      projection = projection.filter(function (series) {
        return series.episodes.some(function (member) {
          const nativeId = recordingBackendNativeId(member.recording);
          return Boolean(nativeId && rich.has(nativeId));
        });
      });
    }
    if (!projection.length && config.requireRich === true) return false;

    state.seriesProjection = projection;
    state.seriesBackendId = backendId;
    if (!projection.length) {
      state.seriesViewKey = '';
      state.seriesSeasonNumber = null;
      clearRail('series');
      return false;
    }

    if (state.seriesViewKey) {
      const selectedSeries = projection.find(function (series) {
        return series.key === state.seriesViewKey;
      });
      if (selectedSeries) {
        const selectedSeason = state.seriesSeasonNumber === null
          ? null
          : selectedSeries.seasons.find(function (season) {
            return season.number === state.seriesSeasonNumber;
          }) || null;
        return renderSeriesDetail(selectedSeries, selectedSeason, backendId);
      }
    }
    return renderSeriesRail(projection, backendId);
  }

  function clearSeriesMetadataRetry() {
    const timer = state.seriesMetadataRetryTimer;
    state.seriesMetadataRetryTimer = null;
    if (timer !== null && timer !== undefined && typeof global.clearTimeout === 'function') {
      global.clearTimeout(timer);
    }
  }

  function invalidateSeriesForHomeExit(generation) {
    if (generation !== state.generation || generation <= 0) return false;
    state.seriesInvalidatedGeneration = generation;
    if (state.refreshInFlight && state.refreshInFlight.generation === generation) {
      state.refreshInFlight.invalidated = true;
    }
    if (state.seriesCompletionInFlight &&
        state.seriesCompletionInFlight.generation === generation) {
      state.seriesCompletionInFlight.invalidated = true;
    }
    return true;
  }

  function current(generation, backendId) {
    const active = homeIsActive();
    if (!active) invalidateSeriesForHomeExit(generation);
    return generation === state.generation &&
      backendId === selectedBackendId() &&
      active;
  }

  function clearSeriesWarm() {
    state.seriesWarmBackendId = '';
    state.seriesWarmCompletedAt = 0;
  }

  function seriesWarm(backendId) {
    const completedAt = Number(state.seriesWarmCompletedAt || 0);
    return Boolean(
      state.seriesProjection.length &&
      state.seriesBackendId === backendId &&
      state.seriesWarmBackendId === backendId &&
      completedAt > 0 &&
      Date.now() - completedAt < SERIES_WARM_TTL_MS
    );
  }

  function markSeriesWarm(backendId) {
    if (!backendId || state.seriesBackendId !== backendId || !state.seriesProjection.length) {
      clearSeriesWarm();
      return false;
    }
    clearSeriesMetadataRetry();
    state.seriesWarmBackendId = backendId;
    state.seriesWarmCompletedAt = Date.now();
    return true;
  }

  function reuseWarmSeriesProjection(backendId) {
    const target = host();
    const section = target && typeof target.querySelector === 'function'
      ? target.querySelector('[data-home-discovery-rail="series"]')
      : null;
    if (section) return true;
    return renderSeriesRail(state.seriesProjection, backendId);
  }

  function seriesMetadataComplete(recordings, generation, backendId) {
    const cache = state.seriesMetadataCache;
    if (!cache || cache.generation !== generation || cache.backendId !== backendId) return false;
    if (state.seriesInvalidatedGeneration === generation) return false;
    if (state.refreshInFlight &&
        state.refreshInFlight.generation === generation &&
        state.refreshInFlight.invalidated === true) {
      return false;
    }
    if (state.seriesCompletionInFlight &&
        state.seriesCompletionInFlight.generation === generation &&
        state.seriesCompletionInFlight.invalidated === true) {
      return false;
    }
    const seen = new Set();
    for (let index = 0; index < (recordings || []).length; index += 1) {
      const recording = recordings[index];
      if (recordingBackendId(recording, backendId) !== backendId) continue;
      const nativeId = recordingBackendNativeId(recording);
      if (!nativeId || seen.has(nativeId)) continue;
      seen.add(nativeId);
      if (!cache.resolved.has(nativeId) ||
          cache.failedNativeIds.has(nativeId) ||
          cache.unsettledNativeIds.has(nativeId)) return false;
    }
    return true;
  }

  function cancelSeriesMetadataQueue(cache) {
    if (!cache) return;
    [cache.priorityQueue, cache.queue].forEach(function (queue) {
      if (!Array.isArray(queue)) return;
      while (queue.length) {
        const task = queue.shift();
        cache.inflight.delete(task.nativeId);
        task.resolve(null);
      }
    });
  }

  function seriesMetadataCache(generation, backendId) {
    const existing = state.seriesMetadataCache;
    if (existing && existing.generation === generation && existing.backendId === backendId) {
      return existing;
    }
    cancelSeriesMetadataQueue(existing);
    const cache = {
      generation: generation,
      backendId: backendId,
      resolved: new Map(),
      failedNativeIds: new Set(),
      unsettledNativeIds: new Set(),
      inflight: new Map(),
      representativePromises: new Map(),
      priorityQueue: [],
      queue: [],
      scheduledSeriesKeys: new Set(),
      readySeriesKeys: new Set(),
      active: 0
    };
    state.seriesMetadataCache = cache;
    return cache;
  }

  function seriesMetadataPriorityKey(recording, backendId) {
    const member = seriesMemberProjection(recording, null, backendId);
    return text(member && member.seriesKey);
  }

  function promoteSeriesMetadataTask(cache, nativeId, seriesKey) {
    if (!cache || !nativeId) return false;
    let index = cache.priorityQueue.findIndex(function (task) {
      return task.nativeId === nativeId;
    });
    if (index >= 0) {
      if (seriesKey) cache.priorityQueue[index].seriesKey = seriesKey;
      return true;
    }
    index = cache.queue.findIndex(function (task) {
      return task.nativeId === nativeId;
    });
    if (index < 0) return false;
    const task = cache.queue.splice(index, 1)[0];
    if (seriesKey) task.seriesKey = seriesKey;
    cache.priorityQueue.push(task);
    return true;
  }

  function readySeriesRecordings(recordings, generation, backendId) {
    const cache = state.seriesMetadataCache;
    if (!cache || cache.generation !== generation || cache.backendId !== backendId) return [];
    const nativeBySeries = new Map();
    (recordings || []).forEach(function (recording) {
      const key = seriesMetadataPriorityKey(recording, backendId);
      if (!key) return;
      if (!nativeBySeries.has(key)) nativeBySeries.set(key, false);
      if (recordingBackendNativeId(recording)) nativeBySeries.set(key, true);
    });
    const ready = new Set(cache.readySeriesKeys);
    (recordings || []).forEach(function (recording) {
      const nativeId = recordingBackendNativeId(recording);
      if (!nativeId || !cache.resolved.has(nativeId)) return;
      const key = seriesMetadataPriorityKey(recording, backendId);
      if (key) ready.add(key);
    });
    nativeBySeries.forEach(function (hasNativeId, key) {
      if (!hasNativeId) ready.add(key);
    });
    return (recordings || []).filter(function (recording) {
      const key = seriesMetadataPriorityKey(recording, backendId);
      return Boolean(key && ready.has(key));
    });
  }

  function waitForSeriesRepresentatives(client, recordings, backendId, generation) {
    const cache = seriesMetadataCache(generation, backendId);
    const representativeNativeIds = new Map();
    (recordings || []).forEach(function (recording) {
      const key = seriesMetadataPriorityKey(recording, backendId);
      if (!key) return;
      if (!representativeNativeIds.has(key)) representativeNativeIds.set(key, []);
      const nativeId = recordingBackendNativeId(recording);
      const nativeIds = representativeNativeIds.get(key);
      if (nativeId && nativeIds.indexOf(nativeId) < 0) nativeIds.push(nativeId);
    });

    const pending = [];
    representativeNativeIds.forEach(function (nativeIds, seriesKey) {
      if (!nativeIds.length) {
        cache.readySeriesKeys.add(seriesKey);
        return;
      }
      if (cache.readySeriesKeys.has(seriesKey)) return;

      let promise = cache.representativePromises.get(seriesKey);
      if (!promise) {
        cache.scheduledSeriesKeys.add(seriesKey);
        promise = requestSeriesRecordingMetadata(
          client,
          backendId,
          nativeIds[0],
          generation,
          {priority: true, seriesKey: seriesKey}
        );
        cache.representativePromises.set(seriesKey, promise);
      }

      // The scan itself starts only one representative per Series. If that
      // representative is still unresolved after pagination has completed,
      // hedge it with at most one different member so one slow metadata read
      // cannot block progressive Series visibility.
      const fallbackNativeId = nativeIds.find(function (nativeId) {
        return !cache.resolved.has(nativeId) && !cache.inflight.has(nativeId);
      });
      if (fallbackNativeId) {
        const fallback = requestSeriesRecordingMetadata(
          client,
          backendId,
          fallbackNativeId,
          generation,
          {priority: true, seriesKey: seriesKey}
        );
        pending.push(Promise.race([promise, fallback]));
        return;
      }

      pending.push(promise);
    });
    return Promise.allSettled(pending);
  }

  function pumpSeriesMetadataCache(cache) {
    if (!cache || state.seriesMetadataCache !== cache) return;
    if (!current(cache.generation, cache.backendId)) {
      cancelSeriesMetadataQueue(cache);
      return;
    }
    while (cache.active < SERIES_METADATA_TOTAL_CONCURRENCY &&
           (cache.priorityQueue.length || cache.queue.length)) {
      const task = cache.priorityQueue.length
        ? cache.priorityQueue.shift()
        : cache.queue.shift();
      cache.active += 1;
      Promise.resolve(task.client.requestJson('/api/vdr/recordings/metadata', {
        query: {
          backend: cache.backendId,
          backendNativeId: task.nativeId
        },
        cache: 'no-store',
        credentials: 'same-origin'
      })).then(function (value) {
        const active = current(cache.generation, cache.backendId);
        const result = active && value && value.available === true ? value : null;
        cache.failedNativeIds.delete(task.nativeId);
        if (active && value && value.available !== true && value.settled === false) {
          cache.unsettledNativeIds.add(task.nativeId);
        } else {
          cache.unsettledNativeIds.delete(task.nativeId);
        }
        cache.resolved.set(task.nativeId, result);
        if (task.seriesKey) cache.readySeriesKeys.add(task.seriesKey);
        task.resolve(result);
      }).catch(function () {
        cache.failedNativeIds.add(task.nativeId);
        cache.unsettledNativeIds.delete(task.nativeId);
        cache.resolved.set(task.nativeId, null);
        if (task.seriesKey) cache.readySeriesKeys.add(task.seriesKey);
        task.resolve(null);
      }).then(function () {
        cache.active = Math.max(0, cache.active - 1);
        cache.inflight.delete(task.nativeId);
        pumpSeriesMetadataCache(cache);
      });
    }
  }

  function requestSeriesRecordingMetadata(client, backendId, nativeId, generation, options) {
    if (!client || typeof client.requestJson !== 'function' || !nativeId) {
      return Promise.resolve(null);
    }
    const cache = seriesMetadataCache(generation, backendId);
    const config = options && typeof options === 'object' ? options : {};
    const seriesKey = text(config.seriesKey);
    const refreshUnsettled = config.refreshUnsettled === true;
    const refreshIncompleteHierarchy =
      config.refreshIncompleteHierarchy === true;

    if (cache.resolved.has(nativeId)) {
      const cached = cache.resolved.get(nativeId);
      const unsettled = !cached ||
        (cached.available !== true && cached.settled === false);
      const cachedMediaType = text(cached && cached.mediaType).toLowerCase();
      const incompleteHierarchy = Boolean(
        cached &&
        cached.available === true &&
        (cachedMediaType === 'series' || cachedMediaType === 'episode') &&
        (
          Number(cached.seasonNumber || 0) <= 0 ||
          Number(cached.episodeNumber || 0) <= 0
        )
      );

      if ((!refreshUnsettled || !unsettled) &&
          (!refreshIncompleteHierarchy || !incompleteHierarchy)) {
        if (seriesKey) cache.readySeriesKeys.add(seriesKey);
        return Promise.resolve(cached);
      }

      cache.resolved.delete(nativeId);
      cache.failedNativeIds.delete(nativeId);
      cache.unsettledNativeIds.delete(nativeId);
    }
    if (cache.inflight.has(nativeId)) {
      const existing = cache.inflight.get(nativeId);
      if (config.priority === true && seriesKey) {
        cache.representativePromises.set(seriesKey, existing);
        if (!promoteSeriesMetadataTask(cache, nativeId, seriesKey)) {
          existing.then(function () {
            if (state.seriesMetadataCache === cache) cache.readySeriesKeys.add(seriesKey);
          });
        }
      }
      return existing;
    }

    let resolveTask = null;
    const promise = new Promise(function (resolve) {
      resolveTask = resolve;
    });
    const task = {
      client: client,
      nativeId: nativeId,
      seriesKey: config.priority === true ? seriesKey : '',
      resolve: resolveTask
    };
    cache.inflight.set(nativeId, promise);
    if (config.priority === true) {
      if (seriesKey) cache.representativePromises.set(seriesKey, promise);
      cache.priorityQueue.push(task);
    } else {
      cache.queue.push(task);
    }
    pumpSeriesMetadataCache(cache);
    return promise;
  }

  function prefetchSeriesRepresentativeMetadata(client, recordings, backendId, generation, onResolved) {
    if (!client || typeof client.requestJson !== 'function') return [];
    const cache = seriesMetadataCache(generation, backendId);
    const pending = [];
    (recordings || []).forEach(function (recording) {
      if (recordingBackendId(recording, backendId) !== backendId) return;
      const embedded = recording.seriesMetadata;
      if (!embedded || embedded.available !== true) return;
      const nativeId = recordingBackendNativeId(recording);
      const key = seriesMetadataPriorityKey(recording, backendId);
      if (nativeId) cache.resolved.set(nativeId, embedded);
      if (recordingMetadataPosterUrl(embedded)) {
        cache.scheduledSeriesKeys.add(key);
        cache.readySeriesKeys.add(key);
      }
    });

    const seenSeriesKeys = new Set();
    (recordings || []).forEach(function (recording) {
      if (recordingBackendId(recording, backendId) !== backendId) return;

      const nativeId = recordingBackendNativeId(recording);
      const seriesKey = seriesMetadataPriorityKey(recording, backendId);
      if (!nativeId || !seriesKey || seenSeriesKeys.has(seriesKey)) return;

      seenSeriesKeys.add(seriesKey);
      cache.scheduledSeriesKeys.add(seriesKey);
      pending.push(requestSeriesRecordingMetadata(
        client,
        backendId,
        nativeId,
        generation,
        {priority: true, seriesKey: seriesKey}
      ).then(function (value) {
        if (typeof onResolved === 'function' && current(generation, backendId)) {
          onResolved(value, nativeId);
        }
        return value;
      }));
    });

    return pending;
  }

  function prefetchSeriesRecordingMetadata(client, recordings, backendId, generation, onResolved) {
    if (!client || typeof client.requestJson !== 'function') return [];
    const cache = seriesMetadataCache(generation, backendId);
    const seen = new Set();
    const pending = [];
    (recordings || []).forEach(function (recording) {
      if (recordingBackendId(recording, backendId) !== backendId) return;
      const nativeId = recordingBackendNativeId(recording);
      if (!nativeId || seen.has(nativeId)) return;
      seen.add(nativeId);
      const seriesKey = seriesMetadataPriorityKey(recording, backendId);
      const priority = Boolean(seriesKey && !cache.scheduledSeriesKeys.has(seriesKey));
      if (priority) cache.scheduledSeriesKeys.add(seriesKey);
      pending.push(requestSeriesRecordingMetadata(
        client,
        backendId,
        nativeId,
        generation,
        {priority: priority, seriesKey: priority ? seriesKey : ''}
      ).then(function (value) {
        if (typeof onResolved === 'function' && current(generation, backendId)) {
          onResolved(value, nativeId);
        }
        return value;
      }));
    });
    return pending;
  }

  function resolvedSeriesMetadata(generation, backendId) {
    const cache = state.seriesMetadataCache;
    const resolved = new Map();
    if (!cache || cache.generation !== generation || cache.backendId !== backendId) return resolved;
    cache.resolved.forEach(function (value, nativeId) {
      if (value && value.available === true) resolved.set(nativeId, value);
    });
    return resolved;
  }

  function seriesMetadataRetryDelay(value) {
    const parsed = Number(value);
    if (!Number.isFinite(parsed) || parsed <= 0) {
      return SERIES_METADATA_RETRY_MS;
    }
    return Math.max(
      SERIES_DETAIL_METADATA_RETRY_MS,
      Math.min(parsed, SERIES_METADATA_RETRY_MS)
    );
  }

  function scheduleSeriesMetadataRetry(
    client,
    recordings,
    backendId,
    generation,
    delayMs
  ) {
    const cache = state.seriesMetadataCache;
    if (!cache ||
        cache.generation !== generation ||
        cache.backendId !== backendId ||
        !cache.unsettledNativeIds.size ||
        generation !== state.generation ||
        backendId !== selectedBackendId()) {
      clearSeriesMetadataRetry();
      return false;
    }
    if (state.seriesMetadataRetryTimer !== null) return true;
    if (typeof global.setTimeout !== 'function') return false;

    const retryDelay = seriesMetadataRetryDelay(delayMs);

    state.seriesMetadataRetryTimer = global.setTimeout(function () {
      state.seriesMetadataRetryTimer = null;
      retryUnsettledSeriesMetadata(
        client,
        recordings,
        backendId,
        generation,
        retryDelay
      );
    }, retryDelay);
    return true;
  }

  function retryUnsettledSeriesMetadata(
    client,
    recordings,
    backendId,
    generation,
    delayMs
  ) {
    const retryDelay = seriesMetadataRetryDelay(delayMs);
    const nextRetryDelay = Math.min(
      retryDelay * 2,
      SERIES_METADATA_RETRY_MS
    );
    if (generation !== state.generation || backendId !== selectedBackendId()) {
      clearSeriesMetadataRetry();
      return Promise.resolve(false);
    }
    if (!homeIsActive()) {
      scheduleSeriesMetadataRetry(
        client,
        recordings,
        backendId,
        generation,
        nextRetryDelay
      );
      return Promise.resolve(false);
    }
    if (state.seriesInvalidatedGeneration === generation) {
      state.seriesInvalidatedGeneration = -1;
    }
    const cache = state.seriesMetadataCache;
    if (!cache || cache.generation !== generation || cache.backendId !== backendId) {
      clearSeriesMetadataRetry();
      return Promise.resolve(false);
    }

    const recordingNativeIds = new Set();
    (recordings || []).forEach(function (recording) {
      if (recordingBackendId(recording, backendId) !== backendId) return;
      const nativeId = recordingBackendNativeId(recording);
      if (nativeId) recordingNativeIds.add(nativeId);
    });
    const retryNativeIds = Array.from(cache.unsettledNativeIds).filter(function (nativeId) {
      return recordingNativeIds.has(nativeId);
    }).slice(0, SERIES_METADATA_RETRY_BATCH);

    if (!retryNativeIds.length) {
      clearSeriesMetadataRetry();
      return Promise.resolve(false);
    }

    const pending = retryNativeIds.map(function (nativeId) {
      cache.unsettledNativeIds.delete(nativeId);
      cache.failedNativeIds.delete(nativeId);
      cache.resolved.delete(nativeId);
      return requestSeriesRecordingMetadata(client, backendId, nativeId, generation);
    });

    return Promise.allSettled(pending).then(function () {
      if (generation !== state.generation || backendId !== selectedBackendId()) {
        clearSeriesMetadataRetry();
        return false;
      }
      if (!homeIsActive()) {
        scheduleSeriesMetadataRetry(client, recordings, backendId, generation);
        return false;
      }
      if (state.seriesInvalidatedGeneration === generation) {
        state.seriesInvalidatedGeneration = -1;
      }
      const rich = resolvedSeriesMetadata(generation, backendId);
      const rendered = applySeriesProjection(recordings, backendId, rich);
      if (rendered && seriesMetadataComplete(recordings, generation, backendId)) {
        markSeriesWarm(backendId);
        return true;
      }
      scheduleSeriesMetadataRetry(client, recordings, backendId, generation);
      return false;
    });
  }

  function fetchAllSeriesRecordings(client, backendId, genreId, generation, onProgress) {
    const recordings = [];

    function requestPage(offset) {
      return Promise.resolve(client.fetchClientGenreRecordings({
        backendId: backendId,
        genreId: genreId,
        limit: SERIES_PAGE_LIMIT,
        offset: offset,
        cache: 'no-store',
        credentials: 'same-origin'
      })).then(function (payload) {
        if (!current(generation, backendId)) return [];
        const rawPage = list(payload, 'recordings');
        const pageRecordings = canonicalRecordings(payload, backendId);
        Array.prototype.push.apply(recordings, pageRecordings);
        const nextOffset = offset + rawPage.length;
        const total = pageTotal(payload, nextOffset);
        const hasMore = pageHasMore(payload, nextOffset, total);
        if (typeof onProgress === 'function') {
          onProgress(recordings.slice());
        }
        if (!hasMore || nextOffset >= total) return recordings;
        if (!rawPage.length || nextOffset <= offset) {
          throw new Error('series pagination made no progress');
        }
        return requestPage(nextOffset);
      });
    }

    return requestPage(0);
  }

  function fetchBoundedRandomGenreRecordings(client, backendId, genreId, generation) {
    return Promise.resolve(client.fetchClientGenreRecordings({
      backendId: backendId,
      genreId: genreId,
      limit: RANDOM_GENRE_LIMIT,
      offset: 0,
      cache: 'no-store',
      credentials: 'same-origin'
    })).then(function (payload) {
      if (!current(generation, backendId)) return [];
      return canonicalRecordings(payload, backendId).slice(0, RANDOM_GENRE_LIMIT);
    });
  }

  function fetchRootFolderProjection(client, backendId, generation) {
    const folders = [];
    const rootRecordings = [];

    function requestPage(offset) {
      return Promise.resolve(client.fetchClientRecordingFolder({
        backendId: backendId,
        query: {
          path: '',
          limit: FOLDER_LIMIT,
          offset: offset
        },
        cache: 'no-store',
        credentials: 'same-origin'
      })).then(function (payload) {
        if (!current(generation, backendId)) return {folders: [], rootRecordings: []};
        const rawPage = list(payload, 'recordings');
        if (offset === 0) {
          const firstProjection = projectRootFolderPayload(payload, backendId);
          Array.prototype.push.apply(folders, firstProjection.folders);
          Array.prototype.push.apply(rootRecordings, firstProjection.rootRecordings);
        } else {
          Array.prototype.push.apply(rootRecordings, canonicalRecordings(payload, backendId));
        }

        const nextOffset = offset + rawPage.length;
        const total = folderRecordingTotal(payload, nextOffset);
        if (nextOffset >= total) {
          return {
            folders: folders,
            rootRecordings: uniqueCanonicalRecordings(rootRecordings, backendId)
          };
        }
        if (!rawPage.length || nextOffset <= offset) {
          throw new Error('root folder pagination made no progress');
        }
        return requestPage(nextOffset);
      });
    }

    return requestPage(0);
  }

  function fetchSeriesRecordingMetadata(
    client,
    recordings,
    backendId,
    generation,
    onProgress,
    requestOptions
  ) {
    const resolved = new Map();
    if (!client || typeof client.requestJson !== 'function') {
      return Promise.resolve(resolved);
    }

    const seen = new Set();
    let publishedSize = 0;
    function publishResolved() {
      if (!current(generation, backendId) ||
          typeof onProgress !== 'function' ||
          resolved.size <= publishedSize) {
        return;
      }
      publishedSize = resolved.size;
      onProgress(resolved);
    }

    const pending = [];
    (recordings || []).forEach(function (recording) {
      if (recordingBackendId(recording, backendId) !== backendId) return;
      const nativeId = recordingBackendNativeId(recording);
      if (!nativeId || seen.has(nativeId)) return;
      seen.add(nativeId);
      pending.push(requestSeriesRecordingMetadata(
        client,
        backendId,
        nativeId,
        generation,
        requestOptions
      ).then(function (value) {
        if (current(generation, backendId) && value && value.available === true) {
          resolved.set(nativeId, value);
          publishResolved();
        }
      }));
    });
    return Promise.all(pending).then(function () { return resolved; });
  }

  function startSeriesCompletion(client, recordings, backendId, generation) {
    const entry = {
      backendId: backendId,
      generation: generation,
      invalidated: false,
      promise: null
    };
    state.seriesCompletionInFlight = entry;
    const completion = fetchSeriesRecordingMetadata(
      client,
      recordings,
      backendId,
      generation
    ).then(function () {
      if (!current(generation, backendId) || entry.invalidated === true) {
        return false;
      }
      if (!seriesMetadataComplete(recordings, generation, backendId)) {
        scheduleSeriesMetadataRetry(client, recordings, backendId, generation);
        return false;
      }
      const rich = resolvedSeriesMetadata(generation, backendId);
      const rendered = applySeriesProjection(recordings, backendId, rich);
      if (rendered) markSeriesWarm(backendId);
      return rendered;
    }).catch(function () {
      return false;
    });
    entry.promise = completion.then(function (value) {
      if (state.seriesCompletionInFlight === entry) state.seriesCompletionInFlight = null;
      return value;
    }, function () {
      if (state.seriesCompletionInFlight === entry) state.seriesCompletionInFlight = null;
      return false;
    });
    return entry.promise;
  }

  function positionRandomGenreRail() {
    const target = host();
    if (!target || typeof target.querySelector !== 'function' ||
        typeof target.insertBefore !== 'function') return false;
    const recordings = target.querySelector('[data-home-discovery-rail="newly-recorded"]');
    const random = target.querySelector('[data-home-discovery-rail="random-genre"]');
    if (!recordings || !random || recordings === random) return false;
    if (recordings.nextElementSibling === random) return true;
    target.insertBefore(random, recordings.nextElementSibling || null);
    return true;
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

  function loadRandomGenre(client, backendId, generation, entry) {
    const id = text(entry && entry.id);
    const label = genreLabel(entry);
    if (!id) {
      clearRail('random-genre');
      return Promise.resolve(false);
    }

    function performLoad() {
      if (!current(generation, backendId)) return Promise.resolve(false);

      renderState(
        'random-genre',
        label,
        'Aufnahmen werden geladen …',
        false
      );
      positionRandomGenreRail();

      return fetchBoundedRandomGenreRecordings(
        client,
        backendId,
        id,
        generation
      ).then(function (recordings) {
        if (!current(generation, backendId)) return false;

        if (!recordings.length) {
          clearRail('random-genre');
          return false;
        }

        const rendered = renderRecordingRail(
          'random-genre',
          label,
          recordings,
          backendId
        );
        positionRandomGenreRail();

        if (client && typeof client.requestJson === 'function') {
          fetchSeriesRecordingMetadata(
            client,
            recordings,
            backendId,
            generation,
            function (rich) {
              if (!current(generation, backendId)) return;

              renderRecordingRail(
                'random-genre',
                label,
                recordings,
                backendId,
                {richMetadataByNativeId: rich}
              );
              positionRandomGenreRail();
            }
          ).then(function (rich) {
            if (!current(generation, backendId)) return false;

            renderRecordingRail(
              'random-genre',
              label,
              recordings,
              backendId,
              {richMetadataByNativeId: rich}
            );
            positionRandomGenreRail();
            return true;
          }).catch(function () {
            return false;
          });
        }

        return rendered;
      }).catch(function () {
        if (!current(generation, backendId)) return false;

        const rendered = renderState(
          'random-genre',
          label,
          'Die Aufnahmen dieses Genres sind vorübergehend nicht verfügbar.',
          true
        );
        positionRandomGenreRail();
        return rendered;
      });
    }

    if (typeof global.setTimeout === 'function') {
      global.setTimeout(function () {
        performLoad();
      }, 0);
      return Promise.resolve(true);
    }

    return performLoad();
  }

  function loadSeries(client, backendId, generation, genreEntries, options) {
    const config = options && typeof options === 'object' ? options : {};
    const seriesGenre = genreEntries.find(function (entry) {
      return text(entry.id).toLowerCase() === 'series';
    });

    if (!seriesGenre) {
      clearSeriesMetadataRetry();
      clearSeriesWarm();
      state.seriesProjection = [];
      state.seriesBackendId = '';
      state.seriesViewKey = '';
      state.seriesSeasonNumber = null;
      clearRail('series');
      return Promise.resolve(false);
    }

    if (client &&
        typeof client.requestJson === 'function') {
      loadSeriesCoverSettings(
        client,
        backendId,
        generation
      );
    }

    if (config.reuseWarm === true && seriesWarm(backendId)) {
      return Promise.resolve(
        reuseWarmSeriesProjection(backendId)
      );
    }

    clearSeriesWarm();

    const retainProjection =
      state.seriesBackendId === backendId &&
      state.seriesProjection.length > 0;

    if (!retainProjection) {
      state.seriesViewKey = '';
      state.seriesSeasonNumber = null;
      renderState(
        'series',
        'Serien',
        'Serien werden gruppiert …',
        false
      );
    }

    return fetchAllSeriesRecordings(
      client,
      backendId,
      text(seriesGenre.id),
      generation,
      function (recordings) {
        if (!current(generation, backendId)) return;
        if (!recordings.length) return;

        if (!retainProjection) {
          applySeriesProjection(
            recordings,
            backendId
          );
        }

        if (client && typeof client.requestJson === 'function') {
          prefetchSeriesRepresentativeMetadata(
            client,
            recordings,
            backendId,
            generation
          );
        }
      }
    ).then(function (recordings) {
      if (!current(generation, backendId)) return false;

      if (!recordings.length) {
        clearSeriesMetadataRetry();
        clearSeriesWarm();
        state.seriesProjection = [];
        state.seriesBackendId = '';
        state.seriesViewKey = '';
        state.seriesSeasonNumber = null;
        clearRail('series');
        return false;
      }

      const rendered = applySeriesProjection(
        recordings,
        backendId,
        resolvedSeriesMetadata(generation, backendId)
      );

      if (rendered && current(generation, backendId)) {
        markSeriesWarm(backendId);
      }

      if (client && typeof client.requestJson === 'function') {
        prefetchSeriesRepresentativeMetadata(
          client,
          recordings,
          backendId,
          generation,
          function () {
            if (!current(generation, backendId)) return;

            const rich = resolvedSeriesMetadata(
              generation,
              backendId
            );

            applySeriesProjection(
              recordings,
              backendId,
              rich
            );
          }
        );
      }

      return rendered;
    }).catch(function () {
      if (!current(generation, backendId)) return false;

      clearSeriesMetadataRetry();
      clearSeriesWarm();

      if (retainProjection) return false;

      state.seriesProjection = [];
      state.seriesBackendId = '';
      state.seriesViewKey = '';
      state.seriesSeasonNumber = null;

      return renderState(
        'series',
        'Serien',
        'Serien sind vorübergehend nicht verfügbar.',
        true
      );
    });
  }

  function loadGenres(client, backendId, generation) {
    const options = arguments.length > 3 && arguments[3] && typeof arguments[3] === 'object'
      ? arguments[3]
      : null;
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
      const randomGenre = selectRandomGenre(entries, generation, Math.random());
      if (!randomGenre) clearRail('random-genre');
      return Promise.allSettled([
        randomGenre
          ? loadRandomGenre(client, backendId, generation, randomGenre)
          : Promise.resolve(false),
        loadSeries(client, backendId, generation, entries, options)
      ]).then(function () { return true; });
    }).catch(function () {
      if (!current(generation, backendId)) return false;
      clearSeriesMetadataRetry();
      clearSeriesWarm();
      state.seriesProjection = [];
      state.seriesBackendId = '';
      state.seriesViewKey = '';
      state.seriesSeasonNumber = null;
      state.randomGenreGeneration = generation;
      state.randomGenreId = '';
      clearRail('random-genre');
      clearRail('series');
      return renderState(
        'genres',
        'Genres',
        'Genres sind vorübergehend nicht verfügbar.',
        true
      );
    });
  }

  function scheduleRandomFolderInline(entries, backendId, generation, randomValue) {
    const selected = selectRandomFolder(entries, generation, randomValue);
    if (!selected) return false;
    const selectedPath = text(selected.path || selected.folderPath || selected.name);

    function openSelectedFolder() {
      if (!current(generation, backendId)) return false;
      const bootstrap = global.VdrSuiteHomeRecordingDiscoveryBootstrap;
      if (!bootstrap || typeof bootstrap.installMouseDrag !== 'function') return false;
      bootstrap.installMouseDrag();
      const section = sectionFor('folders');
      if (!section || typeof section.querySelectorAll !== 'function') return false;
      const cards = section.querySelectorAll('.media-home-discovery-card.folder');
      let selectedCard = null;
      Array.prototype.some.call(cards, function (card) {
        if (text(card && card.dataset && card.dataset.folderPath) !== selectedPath) return false;
        selectedCard = card;
        return true;
      });
      if (!selectedCard || typeof selectedCard.click !== 'function') return false;
      selectedCard.click();
      return true;
    }

    if (typeof global.setTimeout === 'function') {
      global.setTimeout(openSelectedFolder, 0);
      return true;
    }
    return openSelectedFolder();
  }

  function loadFolders(client, backendId, generation) {
    renderState('folders', 'Aufnahmeordner', 'Aufnahmeordner werden geladen …', false);
    return fetchRootFolderProjection(client, backendId, generation).then(function (projection) {
      if (!current(generation, backendId)) return false;
      state.folderProjection = projection;
      state.folderBackendId = backendId;
      const rendered = renderFolderRail(
        projection.folders,
        projection.rootRecordings,
        backendId
      );
      if (rendered) {
        scheduleRandomFolderInline(
          projection.folders,
          backendId,
          generation,
          Math.random()
        );
      }
      return rendered;
    }).catch(function () {
      if (!current(generation, backendId)) return false;
      state.folderProjection = {folders: [], rootRecordings: []};
      state.folderBackendId = '';
      state.randomFolderGeneration = generation;
      state.randomFolderPath = '';
      return renderState(
        'folders',
        'Aufnahmeordner',
        'Aufnahmeordner sind vorübergehend nicht verfügbar.',
        true
      );
    });
  }

  function refresh(options) {
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
    const config = options && typeof options === 'object' ? options : {};
    if (config.coalesce === true && state.refreshInFlight &&
        state.refreshInFlight.backendId === backendId &&
        state.refreshInFlight.generation === state.generation &&
        state.refreshInFlight.invalidated !== true) {
      return state.refreshInFlight.promise;
    }
    if (config.coalesce === true && state.seriesCompletionInFlight &&
        state.seriesCompletionInFlight.backendId === backendId &&
        state.seriesCompletionInFlight.generation === state.generation &&
        state.seriesCompletionInFlight.invalidated !== true &&
        state.seriesInvalidatedGeneration !== state.generation) {
      return state.seriesCompletionInFlight.promise;
    }
    clearSeriesMetadataRetry();
    const generation = ++state.generation;
    state.loadedBackendId = backendId;
    state.homeReadyBackendId = '';
    state.homeReadyGeneration = -1;
    const entry = {
      backendId: backendId,
      generation: generation,
      invalidated: false,
      promise: null
    };
    const loadPromise = Promise.allSettled([
      loadNewly(client, backendId, generation),
      loadGenres(client, backendId, generation, {reuseWarm: config.reuseWarm === true}),
      loadFolders(client, backendId, generation)
    ]).then(function () {
      if (generation === state.generation &&
          backendId === selectedBackendId() &&
          homeIsActive()) {
        state.homeReadyBackendId = backendId;
        state.homeReadyGeneration = generation;
      }
      return true;
    });
    entry.promise = loadPromise.then(function (value) {
      if (state.refreshInFlight === entry) state.refreshInFlight = null;
      return value;
    }, function (error) {
      if (state.refreshInFlight === entry) state.refreshInFlight = null;
      throw error;
    });
    state.refreshInFlight = entry;
    return entry.promise;
  }


  // The cache-committed recordings feed invalidates this retained Home owner.
  // Coalesce bursts and fence reads through refresh()'s existing generation.
  let recordingSource = null;
  let recordingSequence = 0;
  let recordingConnectionSequence = 0;
  let recordingRefreshTimer = null;
  let recordingRefreshBusy = false;
  let recordingRefreshPending = false;

  function refreshRecordingPresentationDependents() {
    const owners = [
      global.VdrSuiteHomeContinueWatching,
      global.VdrSuiteHomeRecentlyWatched,
      global.VdrSuiteHomeRecentMovies
    ];
    const refreshes = [];

    owners.forEach(function (owner) {
      if (!owner || typeof owner.refresh !== 'function') return;
      refreshes.push(
        Promise.resolve()
          .then(function () { return owner.refresh(); })
          .catch(function () { return false; })
      );
    });

    return Promise.allSettled(refreshes).then(function () {
      return true;
    });
  }

  function scheduleRecordingChangeRefresh() {
    if (!recordingRefreshPending || recordingRefreshBusy ||
        recordingRefreshTimer !== null || !homeIsActive() || (doc && doc.hidden)) return;
    recordingRefreshTimer = global.setTimeout(function () {
      recordingRefreshTimer = null;
      if (!homeIsActive() || (doc && doc.hidden)) return;
      recordingRefreshPending = false;
      state.homeReadyBackendId = '';
      state.homeReadyGeneration = -1;
      clearSeriesWarm();
      recordingRefreshBusy = true;
      Promise.resolve(refresh({reuseWarm: false}))
        .then(function (refreshed) {
          return refreshed === true
            ? refreshRecordingPresentationDependents()
            : false;
        })
        .finally(function () {
          recordingRefreshBusy = false;
          scheduleRecordingChangeRefresh();
        });
    }, 0);
  }

  function stopRecordingChanges() {
    if (recordingSource) {
      recordingSource.close();
      recordingSource = null;
      // Catch up after returning, even if the finite feed has rolled over.
      recordingRefreshPending = true;
    }
    if (recordingRefreshTimer !== null) {
      global.clearTimeout(recordingRefreshTimer);
      recordingRefreshTimer = null;
    }
  }

  function subscribeRecordingChanges() {
    const client = clientApi();
    if (recordingSource || !homeIsActive() || (doc && doc.hidden) ||
        !client || typeof client.createClientLiveUpdateSource !== 'function') return;
    const source = client.createClientLiveUpdateSource();
    if (!source) return;
    recordingSource = source;
    source.onopen = function () { recordingConnectionSequence = 0; };
    source.addEventListener('update', function (event) {
      if (recordingSource !== source) return;
      let data;
      try { data = JSON.parse(event.data); } catch (_) { return; }
      const sequence = Number(data && data.sequenceNumber);
      if (!Number.isSafeInteger(sequence) || sequence < 1) return;
      recordingConnectionSequence = Math.max(recordingConnectionSequence, sequence);
      if (sequence <= recordingSequence) return;
      recordingSequence = sequence;
      if (String(data.backendId || 'default') !== selectedBackendId() ||
          !Array.isArray(data.changedDomains) ||
          !data.changedDomains.includes('recordings')) return;
      recordingRefreshPending = true;
      scheduleRecordingChangeRefresh();
    });
    source.onerror = function () {
      if (recordingSource !== source) return;
      // This endpoint replays a finite feed, then reconnects. Only a sequence
      // reset needs a catch-up read, not every normal stream termination.
      if (recordingConnectionSequence < recordingSequence) {
        recordingSequence = recordingConnectionSequence;
        recordingRefreshPending = true;
        scheduleRecordingChangeRefresh();
      }
    };
    scheduleRecordingChangeRefresh();
  }

  function refreshForHome() {
    subscribeRecordingChanges();
    const backendId = selectedBackendId();
    if (state.homeReadyBackendId === backendId &&
        state.homeReadyGeneration === state.generation) {
      if (state.seriesInvalidatedGeneration === state.generation) {
        state.seriesInvalidatedGeneration = -1;
      }
      return Promise.resolve(true);
    }
    return refresh({reuseWarm: true, coalesce: true});
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
        refreshForHome();
      }, {rootMargin: '320px 0px'});
      state.observer.observe(target);
      return true;
    }
    global.setTimeout(refreshForHome, 0);
    return true;
  }

  function scheduleForHome() {
    if (!homeIsActive()) { stopRecordingChanges(); return; }
    subscribeRecordingChanges();
    const backendId = selectedBackendId();
    if (state.loadedBackendId && state.loadedBackendId !== backendId) {
      state.generation += 1;
      state.loadedBackendId = '';
      state.homeReadyBackendId = '';
      state.homeReadyGeneration = -1;
      state.refreshInFlight = null;
      state.seriesCompletionInFlight = null;
      clearSeriesMetadataRetry();
      clearSeriesWarm();
      state.seriesProjection = [];
      state.seriesBackendId = '';
      state.seriesViewKey = '';
      state.seriesSeasonNumber = null;
      state.seriesCoverBackendId = '';
      state.seriesCoverSettingsLoaded = false;
      state.seriesCoverOverrides = new Map();
      state.seriesCoverSettingsInFlight = null;
      state.folderProjection = {folders: [], rootRecordings: []};
      state.folderBackendId = '';
      state.randomGenreGeneration = -1;
      state.randomGenreId = '';
      state.randomFolderGeneration = -1;
      state.randomFolderPath = '';
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
      '.media-home-discovery-card:focus-visible,.media-home-series-season:focus-visible,.media-home-series-back:focus-visible{outline:3px solid rgba(125,211,252,.86);outline-offset:2px}' +
      '.media-home-discovery-artwork{display:grid;place-items:center;width:100%;aspect-ratio:2/3;background:linear-gradient(135deg,#1e293b,#334155);font-size:2rem;font-weight:800}' +
      '.media-home-discovery-artwork img{display:block;width:100%;height:100%;object-fit:cover}' +
      '.media-home-discovery-copy{display:grid;gap:.25rem;padding:.7rem}.media-home-discovery-copy strong{color:#f8fafc}.media-home-discovery-copy span{color:#94a3b8;font-size:.8rem}' +
      '.media-home-discovery-card.genre,.media-home-discovery-card.folder,.media-home-discovery-card.folder-root{min-height:7rem;padding:.35rem;background:linear-gradient(145deg,rgba(30,41,59,.88),rgba(2,6,23,.94))}' +
      '.media-home-discovery-state{margin:0 1rem 1rem;padding:1rem;border:1px solid rgba(148,163,184,.16);border-radius:.9rem;color:#94a3b8;background:rgba(15,23,42,.5)}' +
      '.media-home-discovery-state.error{border-color:rgba(239,68,68,.48);color:#fecaca}' +
      '.media-home-series-heading{display:flex;align-items:center;gap:.7rem}.media-home-series-back{border:1px solid rgba(148,163,184,.25);border-radius:.7rem;background:rgba(15,23,42,.78);color:#e2e8f0;padding:.5rem .65rem;cursor:pointer}' +
      '.media-home-series-summary{display:grid;grid-template-columns:minmax(5rem,7rem) 1fr;gap:1rem;align-items:center;margin:0 1rem 1rem}.media-home-series-summary .media-home-discovery-artwork{border-radius:.8rem;overflow:hidden}.media-home-series-summary-copy{display:grid;gap:.35rem}.media-home-series-summary-copy strong{font-size:1.1rem;color:#f8fafc}.media-home-series-summary-copy span{color:#94a3b8}' +
      '.media-home-series-subheading{margin:.4rem 1rem .65rem;color:#f8fafc;font-size:1rem}.media-home-series-season-rail{display:flex;gap:.6rem;overflow-x:auto;padding:0 1rem 1rem}.media-home-series-season{flex:0 0 auto;border:1px solid rgba(148,163,184,.25);border-radius:.75rem;background:rgba(30,41,59,.86);color:#e2e8f0;padding:.65rem .8rem;cursor:pointer}.media-home-series-season.selected{border-color:rgba(125,211,252,.72);background:rgba(30,64,175,.45)}' +
      '.media-home-series-cover-change,.media-home-series-cover-action{width:max-content;border:1px solid rgba(125,211,252,.38);border-radius:.7rem;background:rgba(30,64,175,.32);color:#e0f2fe;padding:.48rem .68rem;cursor:pointer}.media-home-series-cover-change:disabled{opacity:.45;cursor:wait}' +
      '.media-home-series-cover-search{display:flex;gap:.55rem;align-items:center;margin:.65rem 0}.media-home-series-cover-search input{min-width:14rem;flex:1 1 22rem;padding:.58rem .7rem;border:1px solid rgba(148,163,184,.45);border-radius:.6rem;background:#0f172a;color:#f8fafc}.media-home-series-cover-results{display:grid;grid-template-columns:repeat(auto-fill,minmax(10rem,1fr));gap:.75rem;margin-top:.75rem}.media-home-series-cover-search-result{display:flex;flex-direction:column;overflow:hidden;padding:0;border:1px solid rgba(148,163,184,.28);border-radius:.8rem;background:rgba(15,23,42,.9);color:#f8fafc;text-align:left;cursor:pointer}.media-home-series-cover-search-result img{display:block;width:100%;aspect-ratio:2/3;object-fit:cover;background:#111827}.media-home-series-cover-search-result>span{display:flex;flex-direction:column;gap:.2rem;padding:.55rem}.media-home-series-cover-search-result small{color:#94a3b8}.media-home-series-cover-search-result:disabled{opacity:.55;cursor:wait}' +
      '.media-home-series-cover-picker{margin:0 1rem 1rem;padding:.8rem;border:1px solid rgba(148,163,184,.2);border-radius:.9rem;background:rgba(2,6,23,.72)}.media-home-series-cover-picker[hidden]{display:none!important}.media-home-series-cover-heading{display:flex;gap:.55rem;align-items:center;flex-wrap:wrap;margin-bottom:.7rem}.media-home-series-cover-heading h4{margin:0 auto 0 0;color:#f8fafc}.media-home-series-cover-status{min-height:1.3rem;margin-bottom:.55rem;color:#94a3b8}' +
      '.media-home-series-cover-rail{display:grid;grid-auto-flow:column;grid-auto-columns:minmax(7rem,9rem);gap:.7rem;overflow-x:auto;padding-bottom:.45rem}.media-home-series-cover-candidate{overflow:hidden;padding:0;border:2px solid transparent;border-radius:.8rem;background:rgba(15,23,42,.88);color:#e2e8f0;cursor:pointer;text-align:left}.media-home-series-cover-candidate.selected{border-color:#7dd3fc}.media-home-series-cover-candidate .media-home-discovery-artwork{border-radius:0}.media-home-series-cover-candidate>span{display:block;padding:.45rem .5rem;font-size:.72rem}' +
      '@media(max-width:46rem){.media-home-discovery-rail{grid-auto-columns:minmax(42vw,11rem);padding:0 .78rem 1rem}.media-home-discovery-state{margin:0 .78rem 1rem}.media-home-series-summary{margin:0 .78rem 1rem}.media-home-series-subheading{margin-left:.78rem;margin-right:.78rem}.media-home-series-season-rail{padding-left:.78rem;padding-right:.78rem}}';
    doc.head.appendChild(style);
  }

  function install() {
    if (!doc) return false;
    installStyles();
    armLazyLoad();
    if (typeof doc.addEventListener === 'function') {
      doc.addEventListener(HOME_RESUME_EVENT, function () {
        subscribeRecordingChanges();
        scheduleRecordingChangeRefresh();
      });
      doc.addEventListener('visibilitychange', function () {
        if (doc.hidden || !homeIsActive()) stopRecordingChanges();
        else subscribeRecordingChanges();
      });
      doc.addEventListener('click', function (event) {
        const target = event && event.target;
        if (!target || typeof target.closest !== 'function') return;
        const moduleTarget = target.closest('.module-tab[data-module], [data-brand-module]');
        const requestedModule = text(
          moduleTarget && moduleTarget.dataset &&
          (moduleTarget.dataset.module || moduleTarget.dataset.brandModule)
        );
        if (requestedModule && requestedModule !== 'overview') {
          stopRecordingChanges();
          invalidateSeriesForHomeExit(state.generation);
          return;
        }
        if (target.closest('[data-brand-module="overview"], .module-tab[data-module="overview"], #backends')) {
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
      embeddedLeafRecording: embeddedLeafRecording,
      projectRootFolderPayload: projectRootFolderPayload,
      uniqueCanonicalRecordings: uniqueCanonicalRecordings,
      selectRandomGenre: selectRandomGenre,
      selectRandomFolder: selectRandomFolder,
      folderEntryCount: folderEntryCount,
      genreLabel: genreLabel,
      recordingPosterUrl: recordingPosterUrl,
      recordingMetadataPosterUrl: recordingMetadataPosterUrl,
      recordingMetadataProjection: recordingMetadataProjection,
      recordingBackendNativeId: recordingBackendNativeId,
      canonicalSeriesPath: canonicalSeriesPath,
      seriesMemberProjection: seriesMemberProjection,
      buildSeriesProjection: buildSeriesProjection,
      seriesHierarchyOverride: seriesHierarchyOverride,
      applySeriesHierarchyOverride: applySeriesHierarchyOverride,
      seriesEpisodeNumberLabel: seriesEpisodeNumberLabel,
      requestSeriesHierarchyOverride: requestSeriesHierarchyOverride,
      setSeriesCoverSettingsSnapshot: setSeriesCoverSettingsSnapshot,
      seriesCoverPosterUrl: seriesCoverPosterUrl,
      seriesCoverCandidateImageUrl: seriesCoverCandidateImageUrl,
      searchSeriesCoverCandidates: searchSeriesCoverCandidates,
      applySeriesProjection: applySeriesProjection,
      fetchAllSeriesRecordings: fetchAllSeriesRecordings,
      fetchBoundedRandomGenreRecordings: fetchBoundedRandomGenreRecordings,
      fetchRootFolderProjection: fetchRootFolderProjection,
      fetchSeriesRecordingMetadata: fetchSeriesRecordingMetadata,
      requestSeriesRecordingMetadata: requestSeriesRecordingMetadata,
      prefetchSeriesRecordingMetadata: prefetchSeriesRecordingMetadata,
      prefetchSeriesRepresentativeMetadata: prefetchSeriesRepresentativeMetadata,
      resolvedSeriesMetadata: resolvedSeriesMetadata,
      readySeriesRecordings: readySeriesRecordings,
      waitForSeriesRepresentatives: waitForSeriesRepresentatives,
      seriesMetadataComplete: seriesMetadataComplete,
      seriesWarm: seriesWarm,
      refreshForHome: refreshForHome,
      renderRecordingRail: renderRecordingRail,
      renderFolderRail: renderFolderRail,
      renderSeriesRail: renderSeriesRail,
      renderSeriesDetail: renderSeriesDetail,
      positionRandomGenreRail: positionRandomGenreRail,
      scheduleRandomFolderInline: scheduleRandomFolderInline,
      openRecording: openRecording,
      openFolder: openFolder,
      openGenre: openGenre,
      loadNewly: loadNewly,
      loadRandomGenre: loadRandomGenre,
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