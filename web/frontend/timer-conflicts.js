// Timer conflict frontend renderer.
// Loaded after app.js through the lightweight frontend bootstrap.

(function installTimerConflictRenderer() {
  if (typeof renderTimerList !== 'function') {
    return;
  }

  if (renderTimerList.__timerConflictRendererInstalled === true) {
    return;
  }

  const originalRenderTimerList = renderTimerList;
  let timerConflictRenderToken = 0;

  function timerConflictNumber(value, fallback) {
    const number = Number(value);
    return Number.isFinite(number) ? number : fallback;
  }

  function timerConflictList(report) {
    if (report && Array.isArray(report.conflicts)) {
      return report.conflicts;
    }

    return [];
  }

  function formatTimerConflictClock(value) {
    const epoch = timerConflictNumber(value, 0);

    if (epoch <= 0) {
      return '-';
    }

    const millis = epoch > 100000000000 ? epoch : epoch * 1000;
    return new Date(millis).toLocaleString('de-DE', {
      day: '2-digit',
      month: '2-digit',
      hour: '2-digit',
      minute: '2-digit'
    });
  }

  function timerConflictTimerTitle(timer, index) {
    if (!timer) {
      return 'Timer #' + String(index);
    }

    const title = firstValue(
      timer,
      ['title', 'name', 'file', 'eventTitle', 'description', 'id', 'timerId'],
      'Timer #' + String(index)
    );

    const start = formatVdrClock(timerStartValue(timer));
    const end = formatVdrClock(timerEndValue(timer));

    return 'Timer #' + String(index) + ': ' + String(title) + ' · ' + start + '–' + end;
  }

  function timerConflictTimerByIndex(timers, timerIndex) {
    const index = timerConflictNumber(timerIndex, 0);

    if (index <= 0) {
      return null;
    }

    return timers[index - 1] || null;
  }

  function createTimerConflictPanel(title, message, className) {
    const panel = document.createElement('article');
    panel.className = 'list-item timer-conflict-panel ' + String(className || '');
    panel.dataset.timerConflictPanel = 'true';

    panel.appendChild(addText(document.createElement('div'), title)).className = 'list-title';
    panel.appendChild(addText(document.createElement('div'), message)).className = 'list-meta';

    return panel;
  }

  function appendTimerConflictDetails(panel, report, timers) {
    const source = String(firstValue(report, ['source'], 'unbekannt'));
    const conflicts = timerConflictList(report);
    const count = timerConflictNumber(firstValue(report, ['count'], conflicts.length), conflicts.length);
    const total = timerConflictNumber(firstValue(report, ['total'], count), count);
    const checkAdvised = firstValue(report, ['checkAdvised', 'check_advised'], false);

    panel.appendChild(addText(
      document.createElement('div'),
      'Quelle: ' + source + ' · count=' + String(count) + ' · total=' + String(total)
    )).className = 'list-meta';

    if (checkAdvised === true || checkAdvised === 'true' || checkAdvised === 1 || checkAdvised === '1') {
      panel.appendChild(addText(
        document.createElement('div'),
        'Hinweis: epgsearch empfiehlt eine erneute Konfliktprüfung.'
      )).className = 'list-meta';
    }

    conflicts.slice(0, 10).forEach((conflict, conflictIndex) => {
      const item = document.createElement('div');
      item.className = 'list-meta timer-conflict-entry';

      const headline = document.createElement('strong');
      headline.textContent = 'Konflikt ' + String(conflictIndex + 1) + ' · ' + formatTimerConflictClock(conflict.conflictTime);
      item.appendChild(headline);

      const entries = Array.isArray(conflict.entries) ? conflict.entries : [];
      if (entries.length > 0) {
        const lines = document.createElement('ul');
        entries.forEach(entry => {
          const timerIndex = timerConflictNumber(entry.timerIndex, 0);
          const timer = timerConflictTimerByIndex(timers, timerIndex);
          const concurrent = Array.isArray(entry.concurrentTimerIndices)
            ? entry.concurrentTimerIndices.join(', ')
            : '';
          const percentage = timerConflictNumber(entry.percentage, 0);
          const remoteServer = String(firstValue(entry, ['remoteServer'], ''));

          const parts = [timerConflictTimerTitle(timer, timerIndex)];
          if (percentage > 0) {
            parts.push(String(percentage) + '%');
          }
          if (concurrent !== '') {
            parts.push('gleichzeitig mit #' + concurrent);
          }
          if (remoteServer !== '') {
            parts.push('Remote: ' + remoteServer);
          }

          lines.appendChild(addText(document.createElement('li'), parts.join(' · ')));
        });
        item.appendChild(lines);
      } else if (conflict.raw) {
        item.appendChild(addText(document.createElement('div'), String(conflict.raw)));
      }

      panel.appendChild(item);
    });

    if (conflicts.length > 10) {
      panel.appendChild(addText(
        document.createElement('div'),
        'Zeige 10 von ' + String(conflicts.length) + ' Konflikten.'
      )).className = 'list-meta';
    }
  }

  function removeTimerConflictPanels() {
    detailDataElement
      .querySelectorAll('[data-timer-conflict-panel="true"]')
      .forEach(panel => panel.remove());
  }

  function prependTimerConflictPanel(panel) {
    removeTimerConflictPanels();

    const list = detailDataElement.querySelector('section.list');
    if (list) {
      list.insertBefore(panel, list.firstChild);
      return;
    }

    detailDataElement.prepend(panel);
  }

  function renderTimerConflictReport(report, timers) {
    const conflicts = timerConflictList(report);
    const count = timerConflictNumber(firstValue(report, ['count'], conflicts.length), conflicts.length);
    const available = firstValue(report, ['available'], false);

    if (!(available === true || available === 'true' || available === 1 || available === '1')) {
      const error = String(firstValue(report, ['error', 'message'], 'keine Detailmeldung'));
      prependTimerConflictPanel(createTimerConflictPanel(
        'Timer-Konfliktprüfung nicht verfügbar',
        error,
        'timer-conflict-unavailable'
      ));
      return;
    }

    if (count <= 0 && conflicts.length === 0) {
      prependTimerConflictPanel(createTimerConflictPanel(
        'Keine Timer-Konflikte gemeldet',
        'RESTfulAPI/epgsearch meldet aktuell keine Timerkonflikte.',
        'timer-conflict-clear'
      ));
      return;
    }

    const panel = createTimerConflictPanel(
      'Timer-Konflikte: ' + String(Math.max(count, conflicts.length)),
      'RESTfulAPI/epgsearch meldet aktive Timerkonflikte.',
      'timer-conflict-warning'
    );
    appendTimerConflictDetails(panel, report, timers);
    prependTimerConflictPanel(panel);
  }

  function fetchAndRenderTimerConflicts(renderToken, timers) {
    prependTimerConflictPanel(createTimerConflictPanel(
      'Timer-Konfliktprüfung',
      'Lade /api/vdr/timers/conflicts/live …',
      'timer-conflict-loading'
    ));

    fetch('/api/vdr/timers/conflicts/live', {
      cache: 'no-store',
      credentials: 'same-origin'
    })
      .then(response => {
        if (!response.ok) {
          throw new Error('HTTP ' + String(response.status));
        }

        return response.json();
      })
      .then(report => {
        if (renderToken !== timerConflictRenderToken) {
          return;
        }

        renderTimerConflictReport(report, timers);
      })
      .catch(error => {
        if (renderToken !== timerConflictRenderToken) {
          return;
        }

        prependTimerConflictPanel(createTimerConflictPanel(
          'Timer-Konflikte konnten nicht geladen werden',
          String(error && error.message ? error.message : error),
          'timer-conflict-error'
        ));
      });
  }

  renderTimerList = function renderTimerListWithConflicts(data) {
    const renderToken = ++timerConflictRenderToken;
    originalRenderTimerList(data);

    const timers = listFromResponse(data, 'timers');
    fetchAndRenderTimerConflicts(renderToken, timers);
  };

  renderTimerList.__timerConflictRendererInstalled = true;
})();
