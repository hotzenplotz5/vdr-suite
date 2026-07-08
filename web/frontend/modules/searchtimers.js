// Phase 60.9a: Prepared SearchTimer browser module.
// SearchTimer is already visible in the frontend module navigation.
// This module establishes the platform boundary and Live parity capability slots.
// Rendering remains app-owned until the renderer is extracted.

(function(global) {
  'use strict';

  let searchTimerBrowserContext = Object.freeze({});

  const liveParityCapabilities = Object.freeze([
    'active',
    'vps',
    'blacklist',
    'channel-filter',
    'time-window',
    'weekdays',
    'duplicate-avoidance',
    'preview',
    'create',
    'update',
    'delete'
  ]);

  function configureContext(context) {
    searchTimerBrowserContext = Object.freeze(Object.assign({}, context || {}));
  }

  function getLiveParityCapabilities() {
    return liveParityCapabilities.slice();
  }

  function unavailableSearchTimerRenderList() {
    throw new Error('SearchTimer rendering is still owned by app.js');
  }

  const searchTimerBrowserApi = Object.freeze({
    configureContext: configureContext,
    getLiveParityCapabilities: getLiveParityCapabilities,
    renderList: unavailableSearchTimerRenderList
  });

  global.VdrSuiteSearchTimerBrowser = searchTimerBrowserApi;

  if (global.VdrSuitePlatform &&
      typeof global.VdrSuitePlatform.registerModule === 'function' &&
      typeof global.VdrSuitePlatform.hasModule === 'function' &&
      !global.VdrSuitePlatform.hasModule('searchtimers')) {
    global.VdrSuitePlatform.registerModule('searchtimers', searchTimerBrowserApi);
  }
})(window);
