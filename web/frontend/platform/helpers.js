// Phase 60.6a: Shared frontend helper source foundation.
// Prepared DOM-free and HTTP-free helper namespace for future module extraction.
// The compatibility bridge below only reconnects already registered modules to
// the platform runtime context; it performs no HTTP access of its own.

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

  function workflowReload() {
    if (typeof document === 'undefined') {
      return;
    }

    const refresh = document.getElementById('refresh-detail');
    if (refresh && !refresh.disabled && typeof refresh.click === 'function') {
      refresh.click();
    }
  }

  function configureWorkflowModule(name, legacyApi) {
    const platform = global.VdrSuitePlatform;
    const moduleApi = platform && typeof platform.getModule === 'function'
      ? platform.getModule(name)
      : legacyApi;

    if (!moduleApi || typeof moduleApi.configureContext !== 'function') {
      return;
    }

    moduleApi.configureContext({
      detailDataElement: platform && typeof platform.getMountTarget === 'function'
        ? platform.getMountTarget(name)
        : typeof document !== 'undefined'
          ? document.getElementById('detail-data')
          : null,
      helpers: helpersApi,
      clientApi: platform && typeof platform.getClientApi === 'function'
        ? platform.getClientApi()
        : global.VdrSuiteClientApi || null,
      getSelectedBackendId: function() {
        return platform && typeof platform.getSelectedBackendId === 'function'
          ? platform.getSelectedBackendId()
          : 'default';
      },
      reload: workflowReload
    });
  }

  function reconnectTimerWorkflows() {
    configureWorkflowModule('timers', global.VdrSuiteTimerBrowser || null);
    configureWorkflowModule('searchtimers', global.VdrSuiteSearchTimerBrowser || null);
  }

  function scheduleWorkflowReconnect() {
    if (typeof global.setTimeout === 'function') {
      global.setTimeout(reconnectTimerWorkflows, 0);
    }
  }

  if (typeof document !== 'undefined') {
    document.addEventListener('click', function(event) {
      const target = event.target && typeof event.target.closest === 'function'
        ? event.target.closest('[data-module="timers"], [data-module="searchtimers"]')
        : null;
      if (target) {
        scheduleWorkflowReconnect();
      }
    });

    if (typeof MutationObserver === 'function') {
      const observer = new MutationObserver(function(mutations) {
        const relevant = mutations.some(function(mutation) {
          return Array.from(mutation.addedNodes || []).some(function(node) {
            return node && node.nodeType === 1 &&
              (node.matches && node.matches('.timer-module, .searchtimer-module') ||
               node.querySelector && node.querySelector('.timer-module, .searchtimer-module'));
          });
        });

        if (relevant) {
          scheduleWorkflowReconnect();
        }
      });

      const startObserver = function() {
        const detail = document.getElementById('detail-data');
        if (detail) {
          observer.observe(detail, {childList: true, subtree: true});
        }
      };

      if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', startObserver, {once: true});
      } else {
        startObserver();
      }
    }

    scheduleWorkflowReconnect();
  }
})(window);
