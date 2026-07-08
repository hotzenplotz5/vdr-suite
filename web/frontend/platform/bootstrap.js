// Phase 60.2b: Frontend platform module registry foundation.
// Phase 60.3: Frontend platform runtime context foundation.
// Runtime-safe platform foundation.
// This file must stay DOM-free and HTTP-free.

(function(global) {
  'use strict';

  const modules = Object.create(null);
  let runtimeContext = Object.freeze({});

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

  function configureRuntimeContext(context) {
    if (!context || typeof context !== 'object') {
      throw new Error('Frontend platform runtime context must be an object');
    }

    runtimeContext = Object.freeze(Object.assign({}, context));
    return runtimeContext;
  }

  function getRuntimeContext() {
    return runtimeContext;
  }

  function getClientApi() {
    if (runtimeContext.clientApi && typeof runtimeContext.clientApi === 'object') {
      return runtimeContext.clientApi;
    }

    return global.VdrSuiteClientApi || null;
  }

  function getMountTarget(name) {
    const normalizedName = normalizeModuleName(name);
    const mountTargets = runtimeContext.mountTargets;

    if (normalizedName && mountTargets && typeof mountTargets === 'object' && mountTargets[normalizedName]) {
      return mountTargets[normalizedName];
    }

    return runtimeContext.mountTarget || null;
  }

  function getSelectedBackendId() {
    if (typeof runtimeContext.getSelectedBackendId === 'function') {
      return String(runtimeContext.getSelectedBackendId() || '');
    }

    return String(runtimeContext.selectedBackendId || '');
  }

  function getSelectedModule() {
    if (typeof runtimeContext.getSelectedModule === 'function') {
      return String(runtimeContext.getSelectedModule() || '');
    }

    return String(runtimeContext.selectedModule || '');
  }

  const api = Object.freeze({
    version: '60.2b',
    isLoaded: function() {
      return true;
    },
    registerModule: registerModule,
    getModule: getModule,
    hasModule: hasModule,
    listModules: listModules,
    configureRuntimeContext: configureRuntimeContext,
    getRuntimeContext: getRuntimeContext,
    getClientApi: getClientApi,
    getMountTarget: getMountTarget,
    getSelectedBackendId: getSelectedBackendId,
    getSelectedModule: getSelectedModule
  });

  global.VdrSuitePlatform = api;
})(window);
