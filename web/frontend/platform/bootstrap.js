// Phase 60.2b: Frontend platform module registry foundation.
// Runtime-safe platform foundation.
// This file must stay DOM-free and HTTP-free.

(function(global) {
  'use strict';

  const modules = Object.create(null);

  function normalizeModuleName(name) {
    return String(name || '').trim();
  }

  function registerModule(name, moduleApi) {
    const normalizedName = normalizeModuleName(name);

    if (!normalizedName) {
      throw new Error('Frontend platform module name must not be empty');
    }

    if (!moduleApi || typeof moduleApi !== 'object') {
      throw new Error('Frontend platform module API must be an object');
    }

    if (Object.prototype.hasOwnProperty.call(modules, normalizedName)) {
      throw new Error('Frontend platform module already registered: ' + normalizedName);
    }

    modules[normalizedName] = moduleApi;
    return moduleApi;
  }

  function getModule(name) {
    const normalizedName = normalizeModuleName(name);

    if (!Object.prototype.hasOwnProperty.call(modules, normalizedName)) {
      return null;
    }

    return modules[normalizedName];
  }

  function hasModule(name) {
    return getModule(name) !== null;
  }

  function listModules() {
    return Object.keys(modules).sort();
  }

  const api = Object.freeze({
    version: '60.2b',
    isLoaded: function() {
      return true;
    },
    registerModule: registerModule,
    getModule: getModule,
    hasModule: hasModule,
    listModules: listModules
  });

  global.VdrSuitePlatform = api;
})(window);
