// Phase 60.6a: Shared frontend helper source foundation.
// DOM-free and HTTP-free helpers shared by modular and legacy frontend runtimes.

(function(global) {
  'use strict';

  function firstValue(source, keys, fallback) {
    if (!source || !Array.isArray(keys)) {
      return fallback;
    }

    for (let index = 0; index < keys.length; index += 1) {
      const key = keys[index];
      const value = source[key];

      if (value !== undefined && value !== null && value !== '') {
        return value;
      }
    }

    return fallback;
  }

  function listFromResponse(data, key) {
    if (Array.isArray(data)) {
      return data;
    }

    if (data && key && Array.isArray(data[key])) {
      return data[key];
    }

    if (data && Array.isArray(data.items)) {
      return data.items;
    }

    if (data && Array.isArray(data.results)) {
      return data.results;
    }

    return [];
  }

  function numberOrZero(value) {
    const number = Number(value);
    return Number.isFinite(number) ? number : 0;
  }

  function formatEpochClock(epochSeconds) {
    const epoch = Number(epochSeconds);

    if (!Number.isFinite(epoch) || epoch <= 0) {
      return '-';
    }

    return new Date(epoch * 1000).toLocaleTimeString('de-DE', {
      hour: '2-digit',
      minute: '2-digit'
    });
  }

  function normalizedText(value) {
    return value === undefined || value === null ? '' : String(value).trim();
  }

  function isPublicRecordingMetadataImageUrl(value) {
    const url = normalizedText(value);
    return url.startsWith('/api/vdr/recordings/metadata/image?') ||
      /^\/api\/v1\/recordings\/rec_[0-9a-f]{32}\/artwork\/[0-9a-f]{32}\?backendId=[A-Za-z0-9._-]+$/.test(url) ||
      /^\/recording-artwork\/[^/?#]+\/[0-9a-fA-F]{32}(?:\?|$)/.test(url);
  }

  function preferredRecordingMetadataArtworkUrl(value) {
    const artwork = value && value.preferredArtwork;
    return artwork && artwork.available === true ? normalizedText(artwork.url) : '';
  }

  function recordingMetadataPosterUrl(value) {
    const manual = value && value.manualAssignment && value.manualAssignment.active === true;
    if (manual) return preferredRecordingMetadataArtworkUrl(value);

    const images = Array.isArray(value && value.images) ? value.images : [];
    for (let index = 0; index < images.length; index += 1) {
      const entry = images[index];
      if (!entry || entry.orientation !== 'portrait' || !entry.image ||
          entry.image.available !== true ||
          !isPublicRecordingMetadataImageUrl(entry.image.url)) {
        continue;
      }
      return normalizedText(entry.image.url);
    }

    return preferredRecordingMetadataArtworkUrl(value);
  }

  // Keep the existing metadata view and DOM owners. This is a pure
  // translation of the typed Public-v1 detail, not a second read model.
  function publicRecordingMetadataView(detail) {
    if (!detail || !detail.metadata) throw new Error('Ungültige Aufnahme-Metadaten.');
    const data = detail.metadata;
    const manual = data.source === 'manual';
    const images = Array.isArray(data.images) ? data.images : [];
    const people = Array.isArray(data.people) ? data.people : [];
    const artwork = data.preferredArtwork || {available: false};
    const genres = Array.isArray(data.genres) ? data.genres :
      normalizedText(data.genreText).split(/[|,;]/).map(normalizedText).filter(Boolean);
    return {
      available: data.state !== 'absent' || artwork.available === true || images.length > 0,
      recordingId: detail.recordingId, backendId: detail.backendId,
      provider: manual ? 'manual' : (data.source === 'restfulapi-scraper-bridge' ? 'tvscraper' : 'recording-cache'),
      mediaType: data.contentKind === 'series-episode' ? 'episode' : data.contentKind,
      title: data.providerTitle || data.seriesTitle || data.epgTitle || detail.title,
      originalTitle: data.originalTitle, episodeName: data.episodeTitle,
      overview: data.overview || detail.description || data.epgDescription,
      tagline: data.tagline, releaseDate: data.releaseDate, firstAired: data.firstAired,
      seasonNumber: data.seasonNumber, episodeNumber: data.episodeNumber,
      absoluteEpisodeNumber: data.absoluteEpisodeNumber, runtimeMinutes: data.providerRuntimeMinutes,
      voteAverage: data.voteAverage, voteCount: data.voteCount, statusText: data.statusText,
      collectionName: data.collectionName, imdbId: data.imdbId, genres,
      productionCountries: data.productionCountries || [], networks: data.networks || [],
      people, images, preferredArtwork: artwork,
      trailerLookupAvailable: data.trailerLookupAvailable === true,
      manualAssignment: manual ? {active: true, relationshipLocked: true,
        revision: data.manualRevision, metadataAssignmentId: data.manualAssignmentId,
        metadataEntityId: data.metadataEntityId} : {active: false}
    };
  }

  function homeArtworkPreviewUrl(value) {
    const url = normalizedText(value);
    if (!/^\/(?:recording-artwork\/[^/?#]+\/[^/?#]+|api\/(?:vdr\/)?recordings\/metadata\/image|api\/epg\/cache\/(?:artwork|metadata\/image)|api\/backends\/[A-Za-z0-9_.-]+\/settings\/series-artwork\/image)(?:\?|$)/.test(url)) return url;
    const hashIndex = url.indexOf('#');
    const hash = hashIndex < 0 ? '' : url.slice(hashIndex);
    const base = hashIndex < 0 ? url : url.slice(0, hashIndex);
    const separator = base.indexOf('?');
    const path = separator < 0 ? base : base.slice(0, separator);
    const query = (separator < 0 ? [] : base.slice(separator + 1).split('&')).filter(function (part) {
      if (!part) return false;
      try { return decodeURIComponent(part.split('=', 1)[0]) !== 'variant'; }
      catch (error) { return true; }
    });
    query.push('variant=home');
    return path + '?' + query.join('&') + hash;
  }

  const helpersApi = Object.freeze({
    homeArtworkPreviewUrl: homeArtworkPreviewUrl,
    firstValue: firstValue,
    listFromResponse: listFromResponse,
    numberOrZero: numberOrZero,
    formatEpochClock: formatEpochClock,
    isPublicRecordingMetadataImageUrl: isPublicRecordingMetadataImageUrl,
    preferredRecordingMetadataArtworkUrl: preferredRecordingMetadataArtworkUrl,
    recordingMetadataPosterUrl: recordingMetadataPosterUrl,
    publicRecordingMetadataView: publicRecordingMetadataView
  });

  global.VdrSuiteFrontendHelpers = helpersApi;

  // Compatibility boundary for older classic scripts such as epg-cache.js.
  // New modules should use VdrSuiteFrontendHelpers directly.
  if (typeof global.listFromResponse !== 'function') {
    global.listFromResponse = listFromResponse;
  }
})(window);
