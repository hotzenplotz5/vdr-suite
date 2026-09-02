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
    return normalizedText(value).startsWith('/api/vdr/recordings/metadata/image?');
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

  const helpersApi = Object.freeze({
    firstValue: firstValue,
    listFromResponse: listFromResponse,
    numberOrZero: numberOrZero,
    formatEpochClock: formatEpochClock,
    isPublicRecordingMetadataImageUrl: isPublicRecordingMetadataImageUrl,
    preferredRecordingMetadataArtworkUrl: preferredRecordingMetadataArtworkUrl,
    recordingMetadataPosterUrl: recordingMetadataPosterUrl
  });

  global.VdrSuiteFrontendHelpers = helpersApi;

  // Compatibility boundary for older classic scripts such as epg-cache.js.
  // New modules should use VdrSuiteFrontendHelpers directly.
  if (typeof global.listFromResponse !== 'function') {
    global.listFromResponse = listFromResponse;
  }
})(window);
