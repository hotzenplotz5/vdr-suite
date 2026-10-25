(function (global) {
  'use strict';

  // Media Tools is a view of Core-owned capabilities and Operations, never a job queue.
  const activeAttempts = Object.create(null);
  const activeBatches = Object.create(null);
  const POLL_MS = 4000;
  const MAX_AUTOMATIC_READS = 15;
  // Long-running explicitly approved imports can use the Agent's bounded
  // 600-second execution window plus dispatch/readback headroom.
  const MAX_BATCH_READS = 225;
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

  function markerSuffix(item) {
    if (!item || typeof item.label !== 'string') return '';
    const match = /\.(pc_encode|skipped|duplicate)$/i.exec(item.label);
    return match ? match[1].toLowerCase() : '';
  }

  function operationLabel(state, decision) {
    const marker = decision === 'desktop' || decision === 'skip' ||
      decision === 'release';
    const labels = {
      accepted: 'Angenommen – Ausführung noch nicht bestätigt.',
      queued: 'In Warteschlange des bestehenden Backend-Agenten.',
      dispatching: marker ? 'Dateimarkierung wird vom Backend-Agenten ausgeführt.' :
        'Lokaler Import wird vom Backend-Agenten verarbeitet.',
      executed_unverified: marker
        ? 'Dateimarkierung ausgeführt, Core-Bestätigung noch ausstehend.'
        : 'Ausgeführt, VDR-Aufnahme noch nicht unabhängig bestätigt.',
      succeeded: decision === 'desktop'
        ? 'Für PC/HandBrake markiert – Dateimarkierung vom Core bestätigt, keine VDR-Aufnahme.'
        : decision === 'skip'
          ? 'Übersprungen – Dateimarkierung vom Core bestätigt, keine VDR-Aufnahme.'
          : decision === 'release'
            ? 'Datei ausdrücklich freigegeben – vom Core bestätigt, noch nicht importiert.'
            : 'Importiert – neue VDR-Aufnahme wurde unabhängig bestätigt.',
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
    view.newButton.hidden = !view.attempt ||
      !terminalState(view.attempt.state);
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
    // A navigation away from the tab cannot lose an already authorized
    // sequence: the pending next step remains in this session's batch state.
    batch.resumeNext = true;
  }

  function startBatch(view) {
    if (!isCurrent(view) || view.batch || view.attempt ||
        view.submitting || view.sources.length < 2 ||
        view.localReencode.checked || view.decision.value !== 'import') return;
    if (typeof global.confirm !== 'function' ||
        !global.confirm(view.sources.length + ' Filme nacheinander importieren? ' +
          'Jeder Auftrag wird vom Core einzeln geprüft. Bei einem ungewissen ' +
          'Ergebnis wird die Reihe angehalten. Keine Umkodierung freigegeben.')) return;
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
      report(view, operationLabel(operation.state, view.attempt.videoDecision), false);
      view.operationDetail.textContent =
        'Vorgang ' + operation.operationId + ' · ' + operation.state;
      if (terminalState(operation.state) && !view.batch) {
        view.newButton.hidden = false;
      }
      if (view.batch) advanceBatch(view, operation);
      const limit = view.batch ? MAX_BATCH_READS : MAX_AUTOMATIC_READS;
      if (!terminalState(operation.state) && view.reads < limit) {
        ++view.reads;
        view.timer = global.setTimeout(function () { readOperation(view); }, POLL_MS);
      } else if (!terminalState(operation.state) && view.batch) {
        stopBatch(view, 'Automatische Statusabfrage beendet. ' +
          'Aktuellen Core-Vorgang manuell prüfen; kein weiterer Import startet.');
      }
    }).catch(function (error) {
      if (isCurrent(view)) {
        if (view.batch)
          stopBatch(view, 'Operationsstatus nicht abrufbar. Kein Folgeimport.');
        else report(view, operationError(error), true);
      }
    }).finally(function () {
      view.reading = false;
      const batch = view.batch;
      if (!isCurrent(view) || !batch || !batch.resumeNext ||
          !view.attempt || view.attempt.state !== 'succeeded')
        return;
      batch.resumeNext = false;
      delete activeAttempts[view.backendId];
      view.attempt = null;
      view.reads = 0;
      view.refreshButton.hidden = true;
      view.newButton.hidden = true;
      view.select.value = batch.remaining[0];
      report(view, batch.completed + ' von ' + batch.total +
        ' bestätigt. Nächste Quelle wird geprüft …');
      return startImport(view);
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
    view.localReencode.disabled = true;
    view.decision.disabled = true;
    view.markedSelect.disabled = true;
    view.releaseButton.disabled = true;
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
    view.markedSelect.disabled = true;
    view.releaseButton.disabled = true;
    report(view, 'Autorisierte Importquellen werden geladen …');
    return publicClient().getMediaImportSources(readOptions(view.backendId))
      .then(function (response) {
        if (!isCurrent(view) || generation !== view.sourcesGeneration) return;
        const items = response && response.items;
        if (!Array.isArray(items) || items.length > 32 ||
            !items.every(function (item) {
              return item && typeof item.sourceId === 'string' &&
                /^[A-Za-z0-9_-]{16,64}$/.test(item.sourceId) &&
                (item.label === undefined ||
                  (typeof item.label === 'string' &&
                   item.label.length > 0 && item.label.length <= 160 &&
                   !/[\/\r\n\x00-\x1f\x7f]/.test(item.label)));
            })) throw new Error('Ungültige Quellenliste');
        // The backend currently exposes at most 32 visible IDs and does not
        // guarantee exhaustive pagination. Never present a bounded inventory
        // as a complete recursive scan.
        const bounded = Boolean(response.meta && response.meta.partial === true) ||
          items.length >= 32;
        const ordinary = items.filter(function (item) {
          return !markerSuffix(item);
        });
        const marked = items.filter(function (item) {
          return Boolean(markerSuffix(item));
        });
        // Marked sources can only be RELEASED. Never offer one to any
        // normal import, encoding choice or batch.
        view.sources = ordinary.map(function (item) { return item.sourceId; });
        view.sourceTitles = Object.create(null);
        ordinary.forEach(function (item, index) {
          view.sourceTitles[item.sourceId] = item.label || ('Film ' + (index + 1));
        });
        view.markedSources = marked.map(function (item) { return item.sourceId; });
        view.select.replaceChildren();
        const blank = element('option', '', 'Quelle auswählen');
        blank.value = '';
        view.select.appendChild(blank);
        ordinary.forEach(function (item, index) {
          const option = element('option', '',
            item.label || ('Film ' + (index + 1)));
          option.value = item.sourceId;
          view.select.appendChild(option);
        });
        view.select.value = '';
        view.sourceDetails.textContent = '';
        view.preview.textContent = 'Film auswählen, Entscheidung prüfen und bestätigen.';
        view.markedSelect.replaceChildren();
        const markedBlank = element('option', '', 'Markierte Datei auswählen');
        markedBlank.value = '';
        view.markedSelect.appendChild(markedBlank);
        marked.forEach(function (item) {
          const option = element('option', '',
            item.label + ' [' + markerSuffix(item) + ']');
          option.value = item.sourceId;
          view.markedSelect.appendChild(option);
        });
        view.markedSelect.value = '';
        view.markedSelect.disabled = marked.length === 0;
        view.releaseButton.disabled = true;
        view.select.disabled = ordinary.length === 0;
        view.localReencode.checked = false;
        view.localReencode.disabled = ordinary.length === 0;
        view.decision.value = 'import';
        view.decision.disabled = ordinary.length === 0;
        view.startButton.textContent = 'Import starten';
        view.batchButton.disabled = ordinary.length < 2;
        view.scanNotice.textContent = bounded
          ? 'Scan unvollständig: Das Backend liefert höchstens 32 sichtbare ' +
            'Einträge und derzeit keine Fortsetzung. Weitere Unterordner oder Filme ' +
            'können fehlen. Bitte keine Vollständigkeit annehmen.'
          : 'Ergebnis der Backend-Quellenliste; ein vollständiger rekursiver ' +
            'Dateisystemscan ist aktuell nicht bestätigt.';
        report(view, ordinary.length + ' Filme verfügbar, ' + marked.length +
          ' zurückgestellt, übersprungen oder doppelt markiert.' +
          (bounded ? ' Scan unvollständig.' : ''));
      }).catch(function (error) {
        if (!isCurrent(view) || generation !== view.sourcesGeneration) return;
        const message = error && error.status === 503
          ? 'Quellenliste abgelaufen oder Backend-Agent nicht erreichbar. Bitte aktualisieren.'
          : error && (error.status === 401 || error.status === 403 || error.status === 404)
            ? 'Für dieses Backend sind keine Importquellen zugänglich.'
            : 'Quellenliste konnte nicht geladen werden. Bitte aktualisieren.';
        view.scanNotice.textContent = 'Scan nicht abgeschlossen. Keine aktuellen ' +
          'Ergebnisse; bitte nur nach erfolgreicher Aktualisierung auswählen.';
        view.preview.textContent = 'Keine aktuelle Auswahl; noch kein Import vorbereitet.';
        report(view, message, true);
      });
  }

  function startImport(view, releaseMode) {
    if (!isCurrent(view) || view.submitting || view.attempt) return Promise.resolve();
    const sourceId = releaseMode ? view.markedSelect.value :
      view.batch ? view.batch.remaining[0] : view.select.value;
    // Encoding authority is granted per film, per click, never to a batch.
    const selected = releaseMode ? 'release' :
      view.batch ? 'import' : view.decision.value;
    if (!['import', 'desktop', 'skip', 'release'].includes(selected) ||
        (releaseMode && view.batch)) return Promise.resolve();
    const approveVideoReencode = selected === 'import' &&
      !view.batch && view.localReencode.checked === true;
    const videoDecision = selected === 'import'
      ? (approveVideoReencode ? 'local' : '') : selected;
    // Import alone is NEVER local FFmpeg encoding consent.
    const confirmation = approveVideoReencode
      ? 'Media Tools: Diesen Film bei Bedarf lokal mit FFmpeg in H.264 umkodieren? ' +
        'Die Originaldatei bleibt unverändert. Diese Zustimmung gilt nur für diesen Film.'
      : selected === 'desktop'
        ? 'Diesen Film für PC/HandBrake mit .pc_encode markieren? ' +
          'Es wird keine VDR-Aufnahme importiert.'
        : selected === 'skip'
          ? 'Diesen Film mit .skipped markieren und vom normalen Import ausschließen?'
          : selected === 'release'
            ? 'Diese markierte Datei ausdrücklich wieder freigeben? ' +
              'Es wird noch KEIN Import gestartet. Eine vorhandene Zieldatei wird niemals überschrieben.'
            : view.batch ? '' :
              'Diesen Film importieren? ' + (view.sourceTitles[sourceId] || 'Auswahl') +
              '. Die Datei wird vom Backend erneut geprüft; eine lokale ' +
              'Umkodierung ist nicht freigegeben.';
    if (confirmation &&
        (typeof global.confirm !== 'function' || !global.confirm(confirmation)))
      return Promise.resolve();
    if (!(releaseMode ? view.markedSources : view.sources).includes(sourceId)) {
      if (view.batch) stopBatch(view, 'Quellauswahl nicht mehr verfügbar.');
      return Promise.resolve();
    }
    view.submitting = true;
    view.startButton.disabled = true;
    view.localReencode.disabled = true;
    view.decision.disabled = true;
    view.markedSelect.disabled = true;
    view.releaseButton.disabled = true;
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
          return item && item.sourceId === sourceId &&
            (releaseMode ? Boolean(markerSuffix(item)) : !markerSuffix(item));
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
            videoDecision: videoDecision,
            state: '', pending: true};
          activeAttempts[view.backendId] = view.attempt;
          return publicClient().submitMediaImport({
            backendId: view.backendId, sourceId: sourceId, idempotencyKey: key,
            ...(videoDecision ? {videoDecision: videoDecision} : {}),
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
              target.decision.disabled = true;
              target.markedSelect.disabled = true;
              target.releaseButton.disabled = true;
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
        if (isCurrent(view) && !view.attempt && !view.batch) {
          view.localReencode.disabled = view.decision.value !== 'import';
          view.decision.disabled = false;
          view.markedSelect.disabled = view.markedSources.length === 0;
          view.releaseButton.disabled =
            !view.markedSources.includes(view.markedSelect.value);
        }
      });
  }

  function activate(backendId, mountTarget) {
    deactivate();
    if (!mountTarget || typeof mountTarget.replaceChildren !== 'function') return;
    const root = element('section', 'media-import-panel');
    root.appendChild(element('h3', '', 'Media Import'));
    root.appendChild(element('p', '', 'Filme aus den freigegebenen Quellen auswählen. ' +
      'Ein Suchlauf liest nur die Backend-Quellenliste und startet keinen Import.'));
    const reloadButton = element('button', 'media-import-reload', 'Verzeichnis scannen');
    reloadButton.type = 'button';
    root.appendChild(reloadButton);
    const scanNotice = element('p', 'media-import-scan-notice',
      'Noch nicht geprüft, ob die Quellenliste vollständig ist.');
    scanNotice.setAttribute('role', 'status');
    root.appendChild(scanNotice);
    const sourceDetails = element('p', 'media-import-source-details');
    const technical = element('details', 'media-import-technical-details');
    technical.appendChild(element('summary', '', 'Technische Details'));
    technical.appendChild(sourceDetails);
    const label = element('label', '', 'Film auswählen');
    const select = element('select', 'media-import-source');
    select.disabled = true;
    label.appendChild(select);
    root.appendChild(label);
    const preview = element('p', 'media-import-preview',
      'Film auswählen, Entscheidung prüfen und bestätigen.');
    preview.setAttribute('role', 'status');
    root.appendChild(preview);
    const choiceLabel = element('label', 'media-import-decision-label', 'Entscheidung für diesen Film');
    const decision = element('select', 'media-import-decision');
    decision.disabled = true;
    [
      ['import', 'Lokal importieren / bei Bedarf umkodieren'],
      ['desktop', 'Am PC bearbeiten (HandBrake) – .pc_encode'],
      ['skip', 'Überspringen – .skipped']
    ].forEach(function (choice) {
      const option = element('option', '', choice[1]);
      option.value = choice[0];
      decision.appendChild(option);
    });
    decision.value = 'import';
    choiceLabel.appendChild(decision);
    root.appendChild(choiceLabel);
    const encodeLabel = element('label', 'media-import-encode-label',
      'Lokale Umkodierung bei Bedarf ausdrücklich erlauben (nur dieser Film)');
    const localReencode = element('input', 'media-import-local-reencode');
    localReencode.type = 'checkbox';
    localReencode.checked = false;
    localReencode.disabled = true;
    encodeLabel.appendChild(localReencode);
    root.appendChild(encodeLabel);
    const markedLabel = element('label', 'media-import-marked-label',
      'Markierte Dateien (.pc_encode / .skipped / .duplicate)');
    const markedSelect = element('select', 'media-import-marked-source');
    markedSelect.disabled = true;
    markedLabel.appendChild(markedSelect);
    root.appendChild(markedLabel);
    const releaseButton = element('button', 'media-import-release',
      'Ausgewählte Markierung aufheben');
    releaseButton.type = 'button';
    releaseButton.disabled = true;
    root.appendChild(releaseButton);
    const startButton = element('button', 'media-import-start', 'Import starten');
    startButton.type = 'button';
    startButton.disabled = true;
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
    root.appendChild(technical);
    mountTarget.replaceChildren(root);
    const view = {owner: owner, backendId: backendId, root: root,
      select: select, startButton: startButton, reloadButton: reloadButton,
      localReencode: localReencode, decision: decision,
      markedSelect: markedSelect, releaseButton: releaseButton, markedSources: [],
      batchButton: batchButton, batchStopButton: batchStopButton,
      batch: activeBatches[backendId] || null,
      refreshButton: refreshButton, newButton: newButton,
      message: message, operationDetail: operationDetail,
      scanNotice: scanNotice, sourceDetails: sourceDetails,
      preview: preview, sourceTitles: Object.create(null),
      sources: [], sourcesGeneration: 0, submitting: false, reading: false,
      reads: 0, timer: null, attempt: activeAttempts[backendId] || null};
    current = view;
    select.addEventListener('change', function () {
      startButton.disabled = view.submitting || !view.sources.includes(select.value);
      sourceDetails.textContent = view.sources.includes(select.value)
        ? 'Auswahl-ID: ' + select.value : '';
      preview.textContent = view.sources.includes(select.value)
        ? 'Vorschau: ' + (view.sourceTitles[select.value] || 'Film') +
          '. Die Quelle bleibt erhalten. Das Backend ermittelt das Aufnahmeziel ' +
          'und prüft die Datei erneut vor dem Auftrag.'
        : 'Film auswählen, Entscheidung prüfen und bestätigen.';
    });
    localReencode.addEventListener('change', function () {
      if (!isCurrent(view) || view.batch) return;
      batchButton.disabled = localReencode.checked || view.sources.length < 2 ||
        decision.value !== 'import';
    });
    decision.addEventListener('change', function () {
      if (!isCurrent(view) || view.batch || view.attempt) return;
      localReencode.checked = false;
      localReencode.disabled = decision.value !== 'import';
      batchButton.disabled = decision.value !== 'import' || view.sources.length < 2;
      startButton.textContent = decision.value === 'desktop' ? 'Für PC markieren' :
        decision.value === 'skip' ? 'Film überspringen' : 'Import starten';
    });
    startButton.addEventListener('click', function () { return startImport(view); });
    markedSelect.addEventListener('change', function () {
      releaseButton.disabled = view.submitting || Boolean(view.attempt) ||
        !view.markedSources.includes(markedSelect.value);
    });
    releaseButton.addEventListener('click', function () {
      return startImport(view, true);
    });
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
      view.localReencode.checked = false;
      view.localReencode.disabled = false;
      view.decision.value = 'import';
      view.decision.disabled = false;
      view.startButton.textContent = 'Import starten';
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
