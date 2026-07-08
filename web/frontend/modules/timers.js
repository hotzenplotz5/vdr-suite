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

  function timerConflictListFromReport(report) {
    if (report && Array.isArray(report.conflicts)) {
      return report.conflicts;
    }

    return [];
  }

  function formatTimerConflictTime(value) {
    const number = Number(value);
    const formatVdrClock = contextFunction('formatVdrClock', fallbackClock);

    if (Number.isFinite(number) && number > 1000000000) {
      return new Date(number * 1000).toLocaleString('de-DE', {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        hour: '2-digit',
        minute: '2-digit'
      });
    }

    return formatVdrClock(value);
  }

  function timerConflictTimerLabel(timers, timerIndex) {
    const index = Number(timerIndex);
    const timer = Array.isArray(timers) && Number.isFinite(index) && index > 0
      ? timers[index - 1]
      : null;

    if (!timer) {
      return 'Timer #' + String(timerIndex);
    }

    const formatVdrClock = contextFunction('formatVdrClock', fallbackClock);
    const timerStartValue = contextFunction('timerStartValue', function(candidate) {
      return firstValue(candidate, ['startTime', 'start', 'startEpoch'], 0);
    });
    const timerEndValue = contextFunction('timerEndValue', function(candidate) {
      return firstValue(candidate, ['endTime', 'stop', 'stopTime', 'endEpoch'], 0);
    });

    const title = firstValue(timer, ['title', 'name', 'eventTitle', 'description', 'id', 'timerId'], 'Timer ' + String(timerIndex));
    const start = formatVdrClock(timerStartValue(timer));
    const stop = formatVdrClock(timerEndValue(timer));
    const suffix = start !== '-' || stop !== '-' ? ' · ' + start + '–' + stop : '';

    return 'Timer #' + String(timerIndex) + ': ' + String(title) + suffix;
  }

  function renderConflicts(report, timers, error) {
    const mountTarget = timerBrowserContext.detailDataElement;

    if (!mountTarget) {
      throw new Error('Timer browser mount target is not configured');
    }

    const previous = mountTarget.querySelector('[data-timer-conflict-panel="true"]');

    if (previous) {
      previous.remove();
    }

    const panel = document.createElement('article');
    panel.className = 'module-placeholder timer-conflict-panel';
    panel.dataset.timerConflictPanel = 'true';

    if (error) {
      panel.appendChild(addText(document.createElement('h3'), 'Timer-Konflikte konnten nicht geladen werden'));
      panel.appendChild(addText(document.createElement('p'), error.message));
    } else if (report && report.available === false) {
      panel.appendChild(addText(document.createElement('h3'), 'Timer-Konfliktprüfung nicht verfügbar'));
      panel.appendChild(addText(document.createElement('p'), firstValue(report, ['error'], 'Der Konflikt-Endpunkt ist nicht verfügbar.')));
    } else {
      const conflicts = timerConflictListFromReport(report);
      const count = Number(firstValue(report || {}, ['count'], conflicts.length));
      const total = Number(firstValue(report || {}, ['total'], conflicts.length));
      const source = firstValue(report || {}, ['source'], 'unbekannt');
      const activeConflictCount = Number.isFinite(count) ? count : conflicts.length;

      if (activeConflictCount > 0 || conflicts.length > 0) {
        panel.classList.add('timer-conflict-panel-alert');
        panel.setAttribute('aria-label', 'Achtung: aktive Timer-Konflikte');
      } else {
        panel.classList.add('timer-conflict-panel-ok');
      }

      if (conflicts.length === 0 && count === 0) {
        panel.appendChild(addText(document.createElement('h3'), 'Keine Timer-Konflikte gemeldet'));
        panel.appendChild(addText(document.createElement('p'), 'Quelle: ' + String(source)));
      } else {
        panel.appendChild(addText(document.createElement('h3'), 'Timer-Konflikte: ' + String(activeConflictCount)));
        panel.appendChild(addText(document.createElement('p'), 'Quelle: ' + String(source) + ' · Gesamt: ' + String(Number.isFinite(total) ? total : conflicts.length)));

        conflicts.slice(0, 10).forEach((conflict, conflictIndex) => {
          const conflictBlock = document.createElement('div');
          conflictBlock.className = 'list-meta';

          const time = formatTimerConflictTime(firstValue(conflict, ['conflictTime', 'time'], ''));
          const entries = Array.isArray(conflict.entries) ? conflict.entries : [];

          conflictBlock.appendChild(addText(document.createElement('strong'), 'Konflikt ' + String(conflictIndex + 1) + ' · ' + time));

          entries.forEach(entry => {
            const timerIndex = firstValue(entry, ['timerIndex'], '?');
            const percentage = firstValue(entry, ['percentage'], '?');
            const concurrent = Array.isArray(entry.concurrentTimerIndices)
              ? entry.concurrentTimerIndices.join(', ')
              : String(firstValue(entry, ['concurrentTimerIndices'], '-'));

            conflictBlock.appendChild(addText(
              document.createElement('div'),
              timerConflictTimerLabel(timers, timerIndex) + ' · ' + String(percentage) + '% · parallel: ' + concurrent
            ));
          });

          panel.appendChild(conflictBlock);
        });
      }
    }

    const target = mountTarget.querySelector('.list') || mountTarget;

    if (target.firstChild) {
      target.insertBefore(panel, target.firstChild);
    } else {
      target.appendChild(panel);
    }
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
    renderList: renderList,
    renderConflicts: renderConflicts
  });

  global.VdrSuiteTimerBrowser = timerBrowserApi;

  if (global.VdrSuitePlatform &&
      typeof global.VdrSuitePlatform.registerModule === 'function' &&
      typeof global.VdrSuitePlatform.hasModule === 'function' &&
      !global.VdrSuitePlatform.hasModule('timers')) {
    global.VdrSuitePlatform.registerModule('timers', timerBrowserApi);
  }
})(window);
