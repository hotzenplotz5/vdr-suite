(function (global) {
  'use strict';

  if (!global || global.VdrSuiteHomeRecordingDiscovery) return;

  const doc = global.document;
  const NEW_LIMIT = 12;
  const GENRE_LIMIT = 12;
  const SERIES_PAGE_LIMIT = 100;
  const SERIES_METADATA_CONCURRENCY = 4;
  const FOLDER_LIMIT = 12;
  const state = {
    generation: 0,
    loadedBackendId: '',
    observer: null,
    armed: false,
    seriesProjection: [],
    seriesBackendId: '',
    seriesViewKey: '',
    seriesSeasonNumber: null
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
    image.src = publicPath(resolved);
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

  function canonicalSeriesPath(recording) {
    const value = recordingPath(recording) || text(recording && recording.title);
    const parts = value.split('/').map(text).filter(Boolean);
    if (parts.length >= 2 && parts[0].toLowerCase() === 'serien') {
      return 'Serien/' + parts[1];
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
    const value = recordingPath(recording) || text(recording && recording.title);
    const leaf = value.split('/').filter(Boolean).pop() || value;
    const match = leaf.match(/\bS(\d{1,3})\s*E(\d{1,4})\b/i);
    if (!match) return {seasonNumber: 0, episodeNumber: 0};
    return {
      seasonNumber: Number(match[1]) || 0,
      episodeNumber: Number(match[2]) || 0
    };
  }

  function episodeLeafTitle(recording) {
    const value = recordingPath(recording) || text(recording && recording.title);
    const leaf = value.split('/').filter(Boolean).pop() || value;
    return text(leaf.replace(/^S\d{1,3}\s*E\d{1,4}\s*[-–—:.]?\s*/i, ''));
  }

  function seriesMemberProjection(recording, richMetadata, backendId) {
    const sourceProvider = provider(recording);
    const rich = richMetadata && typeof richMetadata === 'object' ? richMetadata : {};
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
    const parsedProviderId = Number(rich.providerId);
    const richProviderId = Number.isFinite(parsedProviderId) ? parsedProviderId : 0;
    let key = '';
    if (providerSeriesId) key = 'provider:' + providerSeriesId;
    else if (folderPath) key = 'folder:' + folderPath.toLowerCase();
    else if (richProvider && richProviderId !== 0) {
      key = 'native:' + richProvider + ':' + String(richProviderId);
    } else key = 'title:' + seriesTitle.toLowerCase();

    const seasonNumber = Number(rich.seasonNumber || sourceProvider.seasonNumber || token.seasonNumber || 0);
    const episodeNumber = Number(rich.episodeNumber || sourceProvider.episodeNumber || token.episodeNumber || 0);
    const richArtwork = rich.preferredArtwork && typeof rich.preferredArtwork === 'object'
      ? rich.preferredArtwork
      : {};
    const posterUrl = text(
      (richArtwork.available !== false && richArtwork.url) ||
      recordingPosterUrl(recording)
    );
    const episodeTitle = text(
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
      seasonNumber: seasonNumber > 0 ? seasonNumber : 0,
      episodeNumber: episodeNumber > 0 ? episodeNumber : 0,
      episodeTitle: episodeTitle
    };
  }

  function buildSeriesProjection(members) {
    const groups = new Map();
    (members || []).filter(Boolean).forEach(function (member) {
      let series = groups.get(member.seriesKey);
      if (!series) {
        series = {
          key: member.seriesKey,
          title: member.seriesTitle,
          path: member.seriesPath,
          posterUrl: member.posterUrl,
          episodes: [],
          seasons: []
        };
        groups.set(member.seriesKey, series);
      }
      if (!series.posterUrl && member.posterUrl) series.posterUrl = member.posterUrl;
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
            label: number > 0 ? 'Staffel ' + String(number) : 'Staffel unbekannt',
            episodes: []
          };
          seasons.set(key, season);
        }
        season.episodes.push(member);
      });
      series.seasons = Array.from(seasons.values()).sort(function (left, right) {
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

  function renderSeriesRail(seriesEntries, backendId) {
    if (!seriesEntries.length) {
      clearRail('series');
      return true;
    }
    state.seriesViewKey = '';
    state.seriesSeasonNumber = null;
    const section = sectionFor('series');
    if (!section) return false;
    section.replaceChildren();
    appendSectionHeading(section, 'Serien');
    const rail = doc.createElement('div');
    rail.className = 'media-home-discovery-rail series';
    seriesEntries.forEach(function (series) {
      const card = doc.createElement('button');
      card.type = 'button';
      card.className = 'media-home-discovery-card series';
      card.dataset.seriesKey = series.key;
      card.dataset.backendId = backendId;
      card.appendChild(createPosterArtwork(series.title, series.posterUrl, series.title.slice(0, 1)));
      const copy = doc.createElement('span');
      copy.className = 'media-home-discovery-copy';
      const label = doc.createElement('strong');
      label.textContent = series.title;
      const detail = doc.createElement('span');
      detail.textContent = seriesCountLabel(series);
      copy.append(label, detail);
      card.appendChild(copy);
      card.addEventListener('click', function () {
        renderSeriesDetail(series, null, backendId);
      });
      rail.appendChild(card);
    });
    section.appendChild(rail);
    return true;
  }

  function renderSeriesDetail(series, selectedSeason, backendId) {
    state.seriesViewKey = series.key;
    state.seriesSeasonNumber = selectedSeason ? selectedSeason.number : null;
    const section = sectionFor('series');
    if (!section) return false;
    section.replaceChildren();
    appendSectionHeading(section, series.title, '← Serien', function () {
      renderSeriesRail(state.seriesProjection, state.seriesBackendId || backendId);
    });

    const summary = doc.createElement('div');
    summary.className = 'media-home-series-summary';
    summary.appendChild(createPosterArtwork(series.title, series.posterUrl, series.title.slice(0, 1)));
    const summaryCopy = doc.createElement('div');
    summaryCopy.className = 'media-home-series-summary-copy';
    const title = doc.createElement('strong');
    title.textContent = series.title;
    const count = doc.createElement('span');
    count.textContent = seriesCountLabel(series);
    summaryCopy.append(title, count);
    summary.appendChild(summaryCopy);
    section.appendChild(summary);

    const seasonTitle = doc.createElement('h4');
    seasonTitle.className = 'media-home-series-subheading';
    seasonTitle.textContent = 'Staffeln';
    section.appendChild(seasonTitle);
    const seasonRail = doc.createElement('div');
    seasonRail.className = 'media-home-series-season-rail';
    series.seasons.forEach(function (season) {
      const button = doc.createElement('button');
      button.type = 'button';
      button.className = 'media-home-series-season' + (selectedSeason === season ? ' selected' : '');
      button.dataset.seasonNumber = String(season.number);
      button.textContent = season.label + ' · ' + String(season.episodes.length) +
        (season.episodes.length === 1 ? ' Folge' : ' Folgen');
      button.addEventListener('click', function () {
        renderSeriesDetail(series, season, backendId);
      });
      seasonRail.appendChild(button);
    });
    section.appendChild(seasonRail);

    if (!selectedSeason) return true;

    const episodeTitle = doc.createElement('h4');
    episodeTitle.className = 'media-home-series-subheading';
    episodeTitle.textContent = selectedSeason.label;
    section.appendChild(episodeTitle);
    const episodeRail = doc.createElement('div');
    episodeRail.className = 'media-home-discovery-rail series-episodes';
    selectedSeason.episodes.forEach(function (member) {
      const recording = member.recording;
      const card = doc.createElement('button');
      card.type = 'button';
      card.className = 'media-home-discovery-card recording series-episode';
      card.dataset.recordingId = recordingId(recording);
      card.dataset.backendId = member.backendId || backendId;
      card.dataset.episodeNumber = String(member.episodeNumber);
      card.appendChild(createPosterArtwork(
        member.episodeTitle,
        member.posterUrl || recordingPosterUrl(recording),
        member.episodeTitle.slice(0, 1).toUpperCase()
      ));
      const copy = doc.createElement('span');
      copy.className = 'media-home-discovery-copy';
      const label = doc.createElement('strong');
      label.textContent = member.episodeNumber > 0
        ? 'Folge ' + String(member.episodeNumber)
        : member.episodeTitle;
      const detail = doc.createElement('span');
      detail.textContent = member.episodeTitle;
      copy.append(label, detail);
      card.appendChild(copy);
      card.addEventListener('click', function () {
        openRecording(recording, member.backendId || backendId);
      });
      episodeRail.appendChild(card);
    });
    section.appendChild(episodeRail);
    return true;
  }

  function applySeriesProjection(recordings, backendId, richMetadataByNativeId) {
    const rich = richMetadataByNativeId instanceof Map ? richMetadataByNativeId : null;
    const members = recordings.map(function (recording) {
      const nativeId = recordingBackendNativeId(recording);
      return seriesMemberProjection(
        recording,
        rich && nativeId ? rich.get(nativeId) || null : null,
        backendId
      );
    });
    const projection = buildSeriesProjection(members);
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

  function current(generation, backendId) {
    return generation === state.generation &&
      backendId === selectedBackendId() &&
      homeIsActive();
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
        if (!hasMore || nextOffset >= total) return recordings;
        if (!rawPage.length || nextOffset <= offset) {
          throw new Error('series pagination made no progress');
        }
        if (typeof onProgress === 'function') {
          onProgress(recordings.slice());
        }
        return requestPage(nextOffset);
      });
    }

    return requestPage(0);
  }

  function fetchSeriesRecordingMetadata(client, recordings, backendId, generation) {
    const resolved = new Map();
    if (!client || typeof client.requestJson !== 'function') {
      return Promise.resolve(resolved);
    }

    const queue = [];
    const seen = new Set();
    (recordings || []).forEach(function (recording) {
      if (recordingBackendId(recording, backendId) !== backendId) return;
      const nativeId = recordingBackendNativeId(recording);
      if (!nativeId || seen.has(nativeId)) return;
      seen.add(nativeId);
      queue.push({recording: recording, nativeId: nativeId});
    });

    let nextIndex = 0;
    function worker() {
      function next() {
        if (!current(generation, backendId) || nextIndex >= queue.length) {
          return Promise.resolve();
        }
        const candidate = queue[nextIndex++];
        return Promise.resolve(client.requestJson('/api/vdr/recordings/metadata', {
          query: {
            backend: backendId,
            backendNativeId: candidate.nativeId
          },
          cache: 'no-store',
          credentials: 'same-origin'
        })).then(function (value) {
          if (current(generation, backendId) && value && value.available === true) {
            resolved.set(candidate.nativeId, value);
          }
        }).catch(function () {
          // Per-recording metadata is enrichment only; keep the canonical recording fallback.
        }).then(next);
      }
      return next();
    }

    const workers = [];
    const workerCount = Math.min(SERIES_METADATA_CONCURRENCY, queue.length);
    for (let index = 0; index < workerCount; index += 1) workers.push(worker());
    return Promise.all(workers).then(function () { return resolved; });
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
      state.seriesProjection = [];
      state.seriesBackendId = '';
      state.seriesViewKey = '';
      state.seriesSeasonNumber = null;
      clearRail('series');
      return Promise.resolve(false);
    }
    state.seriesViewKey = '';
    state.seriesSeasonNumber = null;
    renderState('series', 'Serien', 'Serien werden gruppiert …', false);
    return fetchAllSeriesRecordings(
      client,
      backendId,
      text(seriesGenre.id),
      generation,
      function (recordings) {
        if (!current(generation, backendId) || !recordings.length) return;
        applySeriesProjection(recordings, backendId);
      }
    ).then(function (recordings) {
      if (!current(generation, backendId)) return false;
      if (!recordings.length) {
        state.seriesProjection = [];
        state.seriesBackendId = '';
        state.seriesViewKey = '';
        state.seriesSeasonNumber = null;
        clearRail('series');
        return false;
      }
      applySeriesProjection(recordings, backendId);
      return fetchSeriesRecordingMetadata(client, recordings, backendId, generation).then(function (rich) {
        if (!current(generation, backendId)) return false;
        if (rich.size > 0) return applySeriesProjection(recordings, backendId, rich);
        return true;
      });
    }).catch(function () {
      if (!current(generation, backendId)) return false;
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
      state.seriesProjection = [];
      state.seriesBackendId = '';
      state.seriesViewKey = '';
      state.seriesSeasonNumber = null;
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
      state.seriesProjection = [];
      state.seriesBackendId = '';
      state.seriesViewKey = '';
      state.seriesSeasonNumber = null;
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
      '.media-home-discovery-card.genre,.media-home-discovery-card.folder{min-height:7rem;padding:.35rem;background:linear-gradient(145deg,rgba(30,41,59,.88),rgba(2,6,23,.94))}' +
      '.media-home-discovery-state{margin:0 1rem 1rem;padding:1rem;border:1px solid rgba(148,163,184,.16);border-radius:.9rem;color:#94a3b8;background:rgba(15,23,42,.5)}' +
      '.media-home-discovery-state.error{border-color:rgba(239,68,68,.48);color:#fecaca}' +
      '.media-home-series-heading{display:flex;align-items:center;gap:.7rem}.media-home-series-back{border:1px solid rgba(148,163,184,.25);border-radius:.7rem;background:rgba(15,23,42,.78);color:#e2e8f0;padding:.5rem .65rem;cursor:pointer}' +
      '.media-home-series-summary{display:grid;grid-template-columns:minmax(5rem,7rem) 1fr;gap:1rem;align-items:center;margin:0 1rem 1rem}.media-home-series-summary .media-home-discovery-artwork{border-radius:.8rem;overflow:hidden}.media-home-series-summary-copy{display:grid;gap:.35rem}.media-home-series-summary-copy strong{font-size:1.1rem;color:#f8fafc}.media-home-series-summary-copy span{color:#94a3b8}' +
      '.media-home-series-subheading{margin:.4rem 1rem .65rem;color:#f8fafc;font-size:1rem}.media-home-series-season-rail{display:flex;gap:.6rem;overflow-x:auto;padding:0 1rem 1rem}.media-home-series-season{flex:0 0 auto;border:1px solid rgba(148,163,184,.25);border-radius:.75rem;background:rgba(30,41,59,.86);color:#e2e8f0;padding:.65rem .8rem;cursor:pointer}.media-home-series-season.selected{border-color:rgba(125,211,252,.72);background:rgba(30,64,175,.45)}' +
      '@media(max-width:46rem){.media-home-discovery-rail{grid-auto-columns:minmax(42vw,11rem);padding:0 .78rem 1rem}.media-home-discovery-state{margin:0 .78rem 1rem}.media-home-series-summary{margin:0 .78rem 1rem}.media-home-series-subheading{margin-left:.78rem;margin-right:.78rem}.media-home-series-season-rail{padding-left:.78rem;padding-right:.78rem}}';
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
      recordingBackendNativeId: recordingBackendNativeId,
      canonicalSeriesPath: canonicalSeriesPath,
      seriesMemberProjection: seriesMemberProjection,
      buildSeriesProjection: buildSeriesProjection,
      applySeriesProjection: applySeriesProjection,
      fetchAllSeriesRecordings: fetchAllSeriesRecordings,
      fetchSeriesRecordingMetadata: fetchSeriesRecordingMetadata,
      renderSeriesRail: renderSeriesRail,
      renderSeriesDetail: renderSeriesDetail,
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
