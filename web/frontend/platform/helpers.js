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

  const helpersApi = Object.freeze({
    firstValue: firstValue,
    listFromResponse: listFromResponse,
    numberOrZero: numberOrZero,
    formatEpochClock: formatEpochClock
  });

  global.VdrSuiteFrontendHelpers = helpersApi;

  // Compatibility boundary for older classic scripts such as epg-cache.js.
  // New modules should use VdrSuiteFrontendHelpers directly.
  if (typeof global.listFromResponse !== 'function') {
    global.listFromResponse = listFromResponse;
  }
})(window);
