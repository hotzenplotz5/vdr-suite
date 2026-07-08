// Phase 60.8a: Active Timer browser module.
// Owns Timer list rendering through the frontend platform registry.
// Loading and Timer conflict fetching remain app-owned until the follow-up extraction slices.

(function(global) {
  'use strict';

  let timerBrowserContext = Object.freeze({});

  function configureContext(context) {
    timerBrowserContext = Object.freeze(Object.assign({}, context || {}));
  }

  function contextFunction(name, fallback) {
    if (timerBrowserContext && typeof timerBrowserContext[name] === 'function') {
      return timerBrowserContext[name];
    }

    return fallback;
  }

  function addText(element, text) {
    element.textContent = text;
    return element;
  }

  function firstValue(source, keys, fallback) {
    const helpers = timerBrowserContext.helpers || global.VdrSuiteFrontendHelpers || null;

    if (helpers && typeof helpers.firstValue === 'function') {
      return helpers.firstValue(source, keys, fallback);
    }

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
    const helpers = timerBrowserContext.helpers || global.VdrSuiteFrontendHelpers || null;

    if (helpers && typeof helpers.listFromResponse === 'function') {
      return helpers.listFromResponse(data, key);
    }

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

  function fallbackTimerStatus(timer) {
    const value = firstValue(timer, ['active', 'enabled', 'status'], '');

    if (value === true || value === 'true' || value === 1 || value === '1') {
      return 'aktiv';
    }

    if (value === false || value === 'false' || value === 0 || value === '0') {
      return 'inaktiv';
    }

    return value === '' ? '-' : String(value);
  }

  function fallbackClock(value) {
    const number = Number(value);

    if (!Number.isFinite(number) || number <= 0) {
      return '-';
    }

    return new Date(number * 1000).toLocaleTimeString('de-DE', {
      hour: '2-digit',
      minute: '2-digit'
    });
  }

  function renderList(data, conflictReport) {
    const timers = listFromResponse(data, 'timers');
    const mountTarget = timerBrowserContext.detailDataElement;

    if (!mountTarget) {
      throw new Error('Timer browser mount target is not configured');
    }

    const formatTimerStatus = contextFunction('formatTimerStatus', fallbackTimerStatus);
    const formatVdrClock = contextFunction('formatVdrClock', fallbackClock);
    const timerStartValue = contextFunction('timerStartValue', function(timer) {
      return firstValue(timer, ['startTime', 'start', 'startEpoch'], 0);
    });
    const timerEndValue = contextFunction('timerEndValue', function(timer) {
      return firstValue(timer, ['endTime', 'stop', 'stopTime', 'endEpoch'], 0);
    });

    mountTarget.replaceChildren();

    const list = document.createElement('section');
    list.className = 'list';

    if (timers.length === 0) {
      const empty = document.createElement('article');
      empty.className = 'module-placeholder';
      empty.appendChild(addText(document.createElement('h3'), 'Keine Timer gefunden'));
      empty.appendChild(addText(document.createElement('p'), 'Der Endpunkt /api/vdr/timers hat aktuell keine Timer geliefert.'));
      mountTarget.appendChild(empty);
      return;
    }

    timers.slice(0, 20).forEach((timer, index) => {
      const item = document.createElement('article');
      item.className = 'list-item';

      const title = firstValue(
        timer,
        ['title', 'name', 'file', 'eventTitle', 'description', 'id', 'timerId'],
        'Timer ' + String(index + 1)
      );
      const subtitle = firstValue(timer, ['subtitle'], '');
      const timerId = firstValue(timer, ['timerId', 'id', 'nativeId'], '-');
      const channel = firstValue(timer, ['channelName', 'channel', 'channelId'], '-');
      const status = formatTimerStatus(timer);
      const start = formatVdrClock(timerStartValue(timer));
      const stop = formatVdrClock(timerEndValue(timer));

      item.appendChild(addText(document.createElement('div'), String(title))).className = 'list-title';

      if (subtitle !== '') {
        item.appendChild(addText(document.createElement('div'), String(subtitle))).className = 'list-meta';
      }

      item.appendChild(addText(document.createElement('div'), 'Kanal: ' + String(channel) + ' · Status: ' + status)).className = 'list-meta';
      item.appendChild(addText(document.createElement('div'), 'Start: ' + start + ' · Ende: ' + stop)).className = 'list-meta';
      item.appendChild(addText(document.createElement('div'), 'ID: ' + String(timerId))).className = 'list-meta';

      list.appendChild(item);
    });

    if (timers.length > 20) {
      const info = document.createElement('article');
      info.className = 'module-placeholder';
      info.appendChild(addText(document.createElement('p'), 'Zeige 20 von ' + String(timers.length) + ' Timern.'));
      list.appendChild(info);
    }

    mountTarget.appendChild(list);
    (void conflictReport);
  }

  const timerBrowserApi = Object.freeze({
    configureContext: configureContext,
    renderList: renderList
  });

  global.VdrSuiteTimerBrowser = timerBrowserApi;

  if (global.VdrSuitePlatform &&
      typeof global.VdrSuitePlatform.registerModule === 'function' &&
      typeof global.VdrSuitePlatform.hasModule === 'function' &&
      !global.VdrSuitePlatform.hasModule('timers')) {
    global.VdrSuitePlatform.registerModule('timers', timerBrowserApi);
  }
})(window);
