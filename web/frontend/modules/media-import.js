(function (global) {
  'use strict';

  // Media Tools is a view of Core-owned capabilities and Operations, never a job queue.
  const activeAttempts = Object.create(null);
  const activeBatches = Object.create(null);
  const POLL_MS = 4000;
  const MAX_AUTOMATIC_READS = 15;
  let owner = 0;
  let current = null;
  let client = null;

  function publicClient() {
    if (!client) {
      const factory = global.VdrSuitePublicV1Client;
      if (!factory || typeof factory.createClient !== 'function') {
        throw new Error('Public API v1 ist nicht verfügbar.');
      }
      client = factory.createClient({
        baseUrl: global.location && global.location.origin ? global.location.origin : '',
        credentials: 'same-origin'
      });
    }
    return client;
  }

  function readOptions(backendId) {
    return {backendId: backendId, credentials: 'same-origin', cache: 'no-store'};
  }

  function checkCapability(backendId) {
    return publicClient().getMediaImportCapability(readOptions(backendId))
      .then(function (result) {
        return Boolean(result && result.available === true &&
          result.backendId === backendId &&
          result.capability === 'addons.media.import');
      });
  }

  function makeKey() {
    if (!global.crypto || typeof global.crypto.getRandomValues !== 'function') {
      throw new Error('Sichere Zufallsquelle für Idempotency-Key nicht verfügbar.');
    }
    const bytes = new Uint8Array(16);
    global.crypto.getRandomValues(bytes);
    return 'media-import-' + Array.from(bytes, function (byte) {
      return byte.toString(16).padStart(2, '0');
    }).join('');
  }

  function element(tag, className, text) {
    const node = global.document.createElement(tag);
    if (className) node.className = className;
    if (text !== undefined) node.textContent = text;
    return node;
  }

  function isCurrent(view) {
    return current === view && view.owner === owner;
  }

  function report(view, message, error) {
    if (!isCurrent(view)) return;
    view.message.textContent = message;
    view.message.className = error ? 'status error' : 'status';
  }

  function clearPolling(view) {
    if (view.timer !== null) {
      global.clearTimeout(view.timer);
      view.timer = null;
    }
  }

  function deactivate() {
    ++owner;
    if (current) clearPolling(current);
    current = null;
  }

  function clearSession() {
    deactivate();
    Object.keys(activeAttempts).forEach(function (id) {
      delete activeAttempts[id];
    });
    Object.keys(activeBatches).forEach(function (id) {
      delete activeBatches[id];
    });
  }

  function operationLabel(state) {
    const labels = {
      accepted: 'Angenommen – Ausführung noch nicht bestätigt.',
      queued: 'In Warteschlange des bestehenden Backend-Agenten.',
      dispatching: 'In Bearbeitung durch den Backend-Agenten.',
      executed_unverified: 'Ausgeführt, VDR-Aufnahme noch nicht unabhängig bestätigt.',
      succeeded: 'Erfolgreich – neue VDR-Aufnahme wurde unabhängig bestätigt.',
      rejected: 'Abgelehnt.',
      conflict: 'Konflikt, keine bestätigte Ausführung.',
      failed_before_dispatch: 'Vor der Ausführung fehlgeschlagen.',
      failed_verified: 'Fehlgeschlagen (verifiziert).',
      cancelled: 'Abgebrochen.',
      outcome_unknown: 'Ergebnis ungewiss – nicht erneut als neuen Import starten.'
    };
    return Object.prototype.hasOwnProperty.call(labels, state)
      ? labels[state] : 'Unbekannter Operationszustand – bitte prüfen.';
  }

  function terminalState(state) {
    return ['rejected', 'conflict', 'succeeded', 'failed_before_dispatch',
      'failed_verified', 'cancelled'].includes(state);
  }

  function operationError(error) {
    if (error && error.status === 401) return 'Anmeldung abgelaufen. Bitte erneut anmelden.';
    if (error && error.status === 403) return 'Media-Import-Berechtigung wurde entzogen.';
    if (error && error.status === 404) return 'Vorgang nicht sichtbar oder nicht mehr vorhanden.';
    if (error && error.status === 503) return 'Backend oder Operationsdienst derzeit nicht erreichbar.';
    return 'Operationsstatus derzeit nicht abrufbar. Bitte später erneut prüfen.';
  }

  // The browser coordinates only an explicitly requested sequence. Core owns
  // each durable Operation; unknown outcomes must never start a second import.
  function stopBatch(view, reason) {
    if (!isCurrent(view) || !view.batch) return;
    const completed = view.batch.completed;
    delete activeBatches[view.backendId];
    view.batch = null;
    view.batchStopButton.hidden = true;
    view.batchButton.disabled = true;
    if (reason) report(view, 'Mehrfachimport nach ' + completed +
      ' bestätigten Aufnahmen angehalten: ' + reason, true);
  }

  function advanceBatch(view, operation) {
    const batch = view.batch;
    if (!batch || batch.waiting !== operation.operationId) return;
    if (operation.state !== 'succeeded') {
      if (operation.state === 'outcome_unknown' || terminalState(operation.state))
        stopBatch(view, 'Operation ' + operation.state + '. Kein Folgeimport.');
      return;
    }
    batch.waiting = '';
    ++batch.completed;
    batch.remaining.shift();
    if (!batch.remaining.length) {
      const done = batch.completed;
      stopBatch(view);
      report(view, done + ' Aufnahmen unabhängig bestätigt. Mehrfachimport abgeschlossen.');
      return;
    }
    view.timer = global.setTimeout(function () {
      if (!isCurrent(view) || activeBatches[view.backendId] !== batch ||
          !view.attempt || view.attempt.operationId !== operation.operationId ||
          view.attempt.state !== 'succeeded') return;
      delete activeAttempts[view.backendId];
      view.attempt = null;
      view.reads = 0;
      view.newButton.hidden = true;
      view.refreshButton.hidden = true;
      view.select.value = batch.remaining[0];
      report(view, batch.completed + ' von ' + batch.total +
        ' bestätigt. Nächste Quelle wird geprüft …');
      startImport(view);
    }, 0);
  }

  function startBatch(view) {
    if (!isCurrent(view) || view.batch || view.attempt ||
        view.submitting || view.sources.length < 2) return;
    const ids = view.sources.slice();
    const batch = {remaining: ids, total: ids.length, completed: 0, waiting: ''};
    view.batch = batch;
    activeBatches[view.backendId] = batch;
    view.batchButton.disabled = true;
    view.batchStopButton.hidden = false;
    view.select.value = ids[0];
    return startImport(view);
  }

  function readOperation(view) {
    if (!isCurrent(view) || !view.attempt || !view.attempt.operationId ||
        view.reading) return Promise.resolve();
    view.reading = true;
    clearPolling(view);
    return publicClient().getOperation({
      operationId: view.attempt.operationId,
      credentials: 'same-origin', cache: 'no-store'
    }).then(function (response) {
      if (!isCurrent(view)) return;
      const operation = response && response.data;
      if (!operation || operation.operationId !== view.attempt.operationId ||
          operation.backendId !== view.backendId ||
          typeof operation.state !== 'string') {
        throw new Error('Ungültige Operationsantwort');
      }
      view.attempt.state = operation.state;
      report(view, operationLabel(operation.state), false);
      view.operationDetail.textContent =
        'Vorgang ' + operation.operationId + ' · ' + operation.state;
      if (terminalState(operation.state) && !view.batch) {
        view.newButton.hidden = false;
      }
      if (view.batch) advanceBatch(view, operation);
      if (!terminalState(operation.state) && view.reads < MAX_AUTOMATIC_READS) {
        ++view.reads;
        view.timer = global.setTimeout(function () { readOperation(view); }, POLL_MS);
      }
    }).catch(function (error) {
      if (isCurrent(view)) report(view, operationError(error), true);
    }).finally(function () {
      view.reading = false;
    });
  }

  function currentAttemptView(view, attempt) {
    if (isCurrent(view) && view.attempt === attempt) return view;
    if (current && isCurrent(current) && current.backendId === view.backendId &&
        current.attempt === attempt) return current;
    return null;
  }

  function showAttempt(view) {
    view.select.disabled = true;
    view.startButton.disabled = true;
    view.reloadButton.disabled = true;
    view.operationDetail.textContent = view.attempt.operationId
      ? 'Vorgang ' + view.attempt.operationId
      : view.attempt.pending
        ? 'Import-Anfrage übermittelt, Serverantwort steht noch aus.'
        : 'Anfrage gesendet, Antwort nicht bestätigt.';
    view.refreshButton.hidden = false;
    view.refreshButton.disabled = !view.attempt.operationId;
    view.newButton.hidden = Boolean(view.batch) || !terminalState(view.attempt.state);
    view.batchStopButton.hidden = !view.batch;
    if (view.attempt.operationId) {
      readOperation(view);
    } else if (view.attempt.pending) {
      report(view, 'Import-Anfrage läuft. Bis zur Serverantwort keinen weiteren Import starten.');
    } else {
      report(view, 'Ergebnis der Auftragsannahme ungewiss. Keine neue Anfrage erzeugen. ' +
        'Die ursprüngliche Idempotency-Key-Zuordnung bleibt für diese Browsersitzung gesperrt.', true);
    }
  }

  function loadSources(view) {
    if (!isCurrent(view) || view.attempt) return Promise.resolve();
    const generation = ++view.sourcesGeneration;
    view.select.disabled = true;
    view.startButton.disabled = true;
    report(view, 'Autorisierte Importquellen werden geladen …');
    return publicClient().getMediaImportSources(readOptions(view.backendId))
      .then(function (response) {
        if (!isCurrent(view) || generation !== view.sourcesGeneration) return;
        const items = response && response.items;
        if (!Array.isArray(items) || items.length > 32 ||
            !items.every(function (item) {
              return item && typeof item.sourceId === 'string' &&
                /^[A-Za-z0-9_-]{16,64}$/.test(item.sourceId);
            })) throw new Error('Ungültige Quellenliste');
        view.sources = items.map(function (item) { return item.sourceId; });
        view.select.replaceChildren();
        const blank = element('option', '', 'Quelle auswählen');
        blank.value = '';
        view.select.appendChild(blank);
        view.sources.forEach(function (sourceId, index) {
          const option = element('option', '', 'Quelle ' + (index + 1) + ' · ' + sourceId);
          option.value = sourceId;
          view.select.appendChild(option);
        });
        view.select.value = '';
        view.select.disabled = items.length === 0;
        view.batchButton.disabled = items.length < 2;
        report(view, items.length ? 'Quelle auswählen oder mehrere nacheinander importieren.' :
          'Keine freigegebenen Importquellen verfügbar.');
      }).catch(function (error) {
        if (!isCurrent(view) || generation !== view.sourcesGeneration) return;
        const message = error && error.status === 503
          ? 'Quellenliste abgelaufen oder Backend-Agent nicht erreichbar. Bitte aktualisieren.'
          : error && (error.status === 401 || error.status === 403 || error.status === 404)
            ? 'Für dieses Backend sind keine Importquellen zugänglich.'
            : 'Quellenliste konnte nicht geladen werden. Bitte aktualisieren.';
        report(view, message, true);
      });
  }

  function startImport(view) {
    if (!isCurrent(view) || view.submitting || view.attempt) return Promise.resolve();
    const sourceId = view.batch ? view.batch.remaining[0] : view.select.value;
    if (!view.sources.includes(sourceId)) {
      if (view.batch) stopBatch(view, 'Quellauswahl nicht mehr verfügbar.');
      return Promise.resolve();
    }
    view.submitting = true;
    view.startButton.disabled = true;
    report(view, 'Quelle und Berechtigung werden erneut geprüft …');
    // Source observations are ephemeral: revalidate before choosing the opaque token.
    return checkCapability(view.backendId)
      .then(function (available) {
        if (!isCurrent(view)) return null;
        if (!available) throw {status: 403};
        return publicClient().getMediaImportSources(readOptions(view.backendId));
      }).then(function (sources) {
        if (!isCurrent(view) || !sources) return null;
        if (!sources.items || !sources.items.some(function (item) {
          return item && item.sourceId === sourceId;
        })) {
          throw {status: 503};
        }
        const session = global.VdrSuiteBrowserSession;
        if (!session || typeof session.restore !== 'function' ||
            typeof session.csrfHeaders !== 'function') throw {status: 401};
        return Promise.resolve(session.restore()).then(function () {
          if (!isCurrent(view)) return null;
          // Reserve a unique key before dispatch; a lost HTTP response is not a failure.
          const key = makeKey();
          view.attempt = {sourceId: sourceId, key: key, operationId: '',
            state: '', pending: true};
          activeAttempts[view.backendId] = view.attempt;
          return publicClient().submitMediaImport({
            backendId: view.backendId, sourceId: sourceId, idempotencyKey: key,
            credentials: 'same-origin', headers: session.csrfHeaders()
          });
        });
      }).then(function (result) {
        if (!result || !view.attempt ||
            activeAttempts[view.backendId] !== view.attempt) return;
        const operation = result.data;
        if (!operation || typeof operation.operationId !== 'string' ||
            !/^op_[A-Za-z0-9_-]+$/.test(operation.operationId) ||
            operation.backendId !== view.backendId) {
          throw new Error('Unbekannte Auftragsantwort');
        }
        view.attempt.operationId = operation.operationId;
        view.attempt.state = operation.state || 'accepted';
        view.attempt.pending = false;
        const target = currentAttemptView(view, view.attempt);
        if (target && target.batch) target.batch.waiting = operation.operationId;
        if (target) showAttempt(target);
      }).catch(function (error) {
        const attempt = view.attempt;
        if (attempt && activeAttempts[view.backendId] === attempt) {
          const target = currentAttemptView(view, attempt);
          attempt.pending = false;
          // A validated HTTP 4xx is a definite rejection before admission.
          // Timeouts, 5xx, invalid 202 responses and 409 are NOT proof of failure.
          if (error && [400, 401, 403, 404, 422].includes(error.status)) {
            delete activeAttempts[view.backendId];
            view.attempt = null;
            if (target) {
              target.attempt = null;
              target.reloadButton.disabled = false;
              target.select.disabled = true;
              target.startButton.disabled = true;
              target.refreshButton.hidden = true;
              target.newButton.hidden = true;
              target.operationDetail.textContent = '';
            }
            const detail = error.status === 422
              ? 'Ausgewählte Importquelle ungültig oder abgelaufen. Quellen aktualisieren.'
              : error.status === 403
                ? 'Media-Import-Berechtigung wurde verweigert. Kein Auftrag angelegt.'
                : error.status === 401
                  ? 'Anmeldung ist abgelaufen. Bitte erneut anmelden.'
                  : error.status === 404
                    ? 'Backend nicht verfügbar oder nicht zugänglich.'
                    : 'Import-Anfrage wurde vom Server zurückgewiesen.';
            if (target && target.batch) stopBatch(target, detail);
            if (target) report(target, detail, true);
            return;
          }
          // Network errors and 5xx can occur after durable operation reservation.
          if (target && target.batch)
            stopBatch(target, 'Auftragsannahme ungewiss. Keine Folgeimporte.');
          if (target) showAttempt(target);
          return;
        }
        if (!isCurrent(view)) return;
        const message = error && error.status === 503
          ? 'Quellenliste veraltet oder Backend-Agent nicht verfügbar. Erneut laden.'
          : error && (error.status === 401 || error.status === 403)
            ? 'Keine gültige Anmeldung oder Media-Import-Berechtigung.'
            : 'Import konnte vor dem Absenden nicht vorbereitet werden.';
        if (view.batch) stopBatch(view, message);
        report(view, message, true);
      }).finally(function () {
        view.submitting = false;
      });
  }

  function activate(backendId, mountTarget) {
    deactivate();
    if (!mountTarget || typeof mountTarget.replaceChildren !== 'function') return;
    const root = element('section', 'media-import-panel');
    root.appendChild(element('h3', '', 'Media Tools · Import'));
    root.appendChild(element('p', '', 'Nur durch den Backend-Agenten freigegebene Quellen. ' +
      'Keine Dateipfade. Mehrfachimport arbeitet nur bei geöffnetem Browser; kein Nachtlauf.'));
    const label = element('label', '', 'Importquelle');
    const select = element('select', 'media-import-source');
    select.disabled = true;
    label.appendChild(select);
    root.appendChild(label);
    const startButton = element('button', 'media-import-start', 'Import starten');
    startButton.type = 'button';
    startButton.disabled = true;
    const reloadButton = element('button', 'media-import-reload', 'Quellen aktualisieren');
    reloadButton.type = 'button';
    const refreshButton = element('button', 'media-import-refresh', 'Vorgang prüfen');
    refreshButton.type = 'button';
    refreshButton.hidden = true;
    root.appendChild(startButton);
    const batchButton = element('button', 'media-import-batch',
      'Alle freigegebenen Quellen nacheinander importieren');
    batchButton.type = 'button';
    batchButton.disabled = true;
    root.appendChild(batchButton);
    const batchStopButton = element('button', 'media-import-batch-stop',
      'Nach aktuellem Auftrag anhalten');
    batchStopButton.type = 'button';
    batchStopButton.hidden = true;
    root.appendChild(batchStopButton);
    root.appendChild(reloadButton);
    root.appendChild(refreshButton);
    const newButton = element('button', 'media-import-new', 'Neuen Import vorbereiten');
    newButton.type = 'button';
    newButton.hidden = true;
    root.appendChild(newButton);
    const message = element('p', 'status');
    message.setAttribute('role', 'status');
    message.setAttribute('aria-live', 'polite');
    root.appendChild(message);
    const operationDetail = element('p', 'media-import-operation');
    root.appendChild(operationDetail);
    mountTarget.replaceChildren(root);
    const view = {owner: owner, backendId: backendId, root: root,
      select: select, startButton: startButton, reloadButton: reloadButton,
      batchButton: batchButton, batchStopButton: batchStopButton,
      batch: activeBatches[backendId] || null,
      refreshButton: refreshButton, newButton: newButton,
      message: message, operationDetail: operationDetail,
      sources: [], sourcesGeneration: 0, submitting: false, reading: false,
      reads: 0, timer: null, attempt: activeAttempts[backendId] || null};
    current = view;
    select.addEventListener('change', function () {
      startButton.disabled = view.submitting || !view.sources.includes(select.value);
    });
    startButton.addEventListener('click', function () { return startImport(view); });
    batchButton.addEventListener('click', function () { return startBatch(view); });
    batchStopButton.addEventListener('click', function () {
      stopBatch(view, 'Aktuelle Core-Operation bleibt unverändert.');
    });
    reloadButton.addEventListener('click', function () { return loadSources(view); });
    refreshButton.addEventListener('click', function () { return readOperation(view); });
    newButton.addEventListener('click', function () {
      if (!isCurrent(view) || view.batch || !view.attempt ||
          !terminalState(view.attempt.state) || view.reading) return;
      clearPolling(view);
      delete activeAttempts[backendId];
      view.attempt = null;
      view.reads = 0;
      view.newButton.hidden = true;
      view.refreshButton.hidden = true;
      view.reloadButton.disabled = false;
      view.operationDetail.textContent = '';
      return loadSources(view);
    });
    // Even an already-visible tab must recheck the live Core capability.
    return checkCapability(backendId).then(function (available) {
      if (!isCurrent(view)) return;
      if (!available) {
        report(view, 'Media Import für dieses Backend oder Konto nicht freigegeben.', true);
        return;
      }
      if (view.attempt) showAttempt(view);
      else return loadSources(view);
    }).catch(function (error) {
      if (isCurrent(view)) report(view, operationError(error), true);
    });
  }

  const api = Object.freeze({
    checkCapability: checkCapability,
    activate: activate,
    deactivate: deactivate,
    clearSession: clearSession
  });
  global.VdrSuiteMediaImport = api;
  if (global.VdrSuitePlatform && typeof global.VdrSuitePlatform.registerModule === 'function') {
    global.VdrSuitePlatform.registerModule('media-import', api);
  }
})(window);
