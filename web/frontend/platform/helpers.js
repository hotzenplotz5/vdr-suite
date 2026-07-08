// Phase 60.6a: Shared frontend helper source foundation.
// Prepared DOM-free and HTTP-free helper namespace for future module extraction.
// This file is intentionally not loaded by index.html yet.

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
})(window);
