// Phase 60.1c: Frontend platform bootstrap namespace anchor.
// Runtime-safe platform foundation.
// This file must stay DOM-free until it is explicitly loaded by index.html.

(function(global) {
  'use strict';

  const api = Object.freeze({
    version: '60.1c',
    isLoaded: function() {
      return true;
    }
  });

  global.VdrSuitePlatform = api;
})(window);
