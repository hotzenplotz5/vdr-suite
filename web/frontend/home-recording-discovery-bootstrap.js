(function (global) {
  'use strict';

  function discoveryReady() {
    return Boolean(
      global.VdrSuiteHomeRecordingDiscovery &&
      typeof global.VdrSuiteHomeRecordingDiscovery.install === 'function'
    );
  }

  function load() {
    if (discoveryReady()) return Promise.resolve(true);
    if (typeof global.loadVdrSuiteDeferredRuntime !== 'function') {
      return Promise.resolve(false);
    }
    return Promise.resolve(global.loadVdrSuiteDeferredRuntime(
      'vdr-suite-home-recording-discovery-runtime',
      '/frontend/home-recording-discovery.js',
      discoveryReady
    )).then(discoveryReady).catch(function (error) {
      if (global.console && typeof global.console.error === 'function') {
        global.console.error('VDR-Suite Home Recording Discovery runtime failed', error);
      }
      return false;
    });
  }

  global.VdrSuiteHomeRecordingDiscoveryBootstrap = Object.freeze({load: load});
  load();
}(window));
