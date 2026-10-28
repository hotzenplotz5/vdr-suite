(function (global) {
  'use strict';

  // Media Tools is a view of Core-owned capabilities and Operations, never a job queue.
  const activeAttempts = Object.create(null);
  const activeBatches = Object.create(null);
  // Browser-session HISTORY of a Core 'succeeded' recording readback, keyed
  // by the operation's source ID. A pinned inventory ID may survive a file
  // replacement: do not claim a newly scanned file is the verified original.
  // No source mutation, media identity or durable ledger follows from it.
  const confirmedImports = Object.create(null);
  // Avoid repeated automatic duplicate operations for a single source observation.
  const automaticDuplicates = Object.create(null);
  const POLL_MS = 4000;
  const MAX_AUTOMATIC_READS = 15;
  // Long-running explicitly approved imports can use the Agent's bounded
  // 600-second execution window plus dispatch/readback headroom.
  const MAX_BATCH_READS = 225;
  const MAX_IMPORT_FILMS = 100;
  // Sort human-readable labels naturally (S01E02 before S01E10);
  // opaque source IDs remain the authoritative selection identities.
  const sourceCollator = new Intl.Collator('de', {numeric: true, sensitivity: 'base'});
  function compareSources(left, right) {
    return sourceCollator.compare(left.folder || '', right.folder || '') ||
      sourceCollator.compare(left.label || '', right.label || '') ||
      sourceCollator.compare(left.sourceId, right.sourceId);
  }
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
    if (current) {
      clearPolling(current);
      if (current.previewTimer !== null) global.clearTimeout(current.previewTimer);
      current.previewTimer = null;
      ++current.previewGeneration;
      resetOnline(current);
      ++current.reviewGeneration;
    }
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
    Object.keys(automaticDuplicates).forEach(function (id) {
      delete automaticDuplicates[id];
    });
    Object.keys(confirmedImports).forEach(function (id) {
      delete confirmedImports[id];
    });
  }

  function markerSuffix(item) {
    if (!item || typeof item.label !== 'string') return '';
    const match = /\.(pc_encode|skipped|duplicate)$/i.exec(item.label);
    return match ? match[1].toLowerCase() : '';
  }

  // Rectools-compatible PC handoff: the unmarked PC output and the parked
  // .pc_encode file remain separate source IDs. This is a read-only name/folder
  // hint, never a media identity, import approval or cleanup instruction.
  // The backend owns codec checks and the Core owns VDR recording readback.
  function pcEncodeCandidates(view, marked) {
    if (markerSuffix(marked) !== 'pc_encode') return [];
    const expected = marked.label.slice(0, -'.pc_encode'.length);
    return view.allOrdinary.filter(function (item) {
      return item.folder === marked.folder && item.label === expected;
    });
  }

  function parkedPcEncodeMatches(view, candidate) {
    if (!candidate) return [];
    return view.allMarked.filter(function (marked) {
      return markerSuffix(marked) === 'pc_encode' &&
        marked.folder === candidate.folder &&
        marked.label.slice(0, -'.pc_encode'.length) === candidate.label;
    });
  }

  function possiblePcReturn(view, candidate) {
    const parked = parkedPcEncodeMatches(view, candidate);
    // Two returned file extensions or two parked originals are ambiguous.
    // No arbitrary "first matching file" association is ever made.
    return parked.length === 1 && pcEncodeCandidates(view, parked[0]).length === 1
      ? parked[0] : null;
  }

  function confirmedImport(view, sourceId) {
    return confirmedImports[view.backendId + '|' + sourceId] || null;
  }

  function operationLabel(state, decision) {
    const marker = decision === 'desktop' || decision === 'skip' ||
      decision === 'duplicate' || decision === 'release' ||
      decision === 'delete_duplicate';
    const labels = {
      accepted: 'Angenommen – Ausführung noch nicht bestätigt.',
      queued: 'In Warteschlange des bestehenden Backend-Agenten.',
      dispatching: marker ? 'Dateimarkierung wird vom Backend-Agenten ausgeführt.' :
        'Lokaler Import wird vom Backend-Agenten verarbeitet.',
      executed_unverified: marker
        ? 'Dateimarkierung ausgeführt, Core-Bestätigung noch ausstehend.'
        : 'Ausgeführt, VDR-Aufnahme noch nicht unabhängig bestätigt.',
      succeeded: decision === 'delete_duplicate'
        ? 'Doppelte Quelldatei gelöscht – VDR-Aufnahme bleibt erhalten.'
        : decision === 'desktop'
        ? 'Für PC/HandBrake markiert – Dateimarkierung vom Core bestätigt, keine VDR-Aufnahme.'
        : decision === 'skip'
          ? 'Übersprungen – Dateimarkierung vom Core bestätigt, keine VDR-Aufnahme.'
          : decision === 'duplicate'
            ? 'Als Duplikat markiert (.duplicate) – keine neue VDR-Aufnahme.'
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

  // Only Core-owned, journal-backed Operation results may assert cleanup.
  function cleanupLabel(status) {
    const labels = {
      pending: 'Quellenbereinigung vom Backend-Agenten noch nicht bestätigt.',
      removed: 'Importierte Quelldatei erfolgreich entfernt.',
      already_absent: 'Importierte Quelldatei bereits nicht mehr vorhanden.',
      not_performed: 'Quellenbereinigung nicht durchgeführt.',
      failed: 'Quellenbereinigung fehlgeschlagen; die VDR-Aufnahme bleibt bestätigt.',
      outcome_unknown: 'Quellenbereinigung nach Agent-Unterbrechung unbestätigt; Original prüfen, keine automatische Wiederholung.'
    };
    return Object.prototype.hasOwnProperty.call(labels, status)
      ? labels[status] : '';
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
    if (view.batchReview) renderBatchReview(view);
    view.batchButton.disabled = true;
    view.newButton.hidden = !view.attempt ||
      !terminalState(view.attempt.state);
    if (reason) report(view, 'Mehrfachimport nach ' + completed +
      ' bestätigten Aufnahmen angehalten: ' + reason, true);
  }

  function advanceBatch(view, operation) {
    const batch = view.batch;
    if (!batch || batch.waiting !== operation.operationId) return;
    const row = batch.review && batch.review.rows.find(function (entry) {
      return entry.sourceId === batch.remaining[0];
    });
    if (row) {
      row.state = operation.state === 'succeeded' ? 'succeeded' :
        operation.state === 'outcome_unknown' || terminalState(operation.state)
          ? 'stopped' : 'running';
      view.batchReview = batch.review;
      renderBatchReview(view);
    }
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

  // Local, read-only preflight. It is an ephemeral review of the existing
  // Public-v1 source catalogue, NOT an operation reservation or a job queue.
  function resetBatchReview(view) {
    ++view.reviewGeneration;
    view.inspectingBatch = false;
    view.batchReview = null;
    view.duplicateQueue = [];
    view.duplicateReviewPending = false;
    view.batchReviewList.replaceChildren();
    view.batchSummary.textContent = '';
    view.batchConfirmButton.hidden = true;
    view.batchConfirmButton.disabled = true;
  }

  function openDeferredFilm(view, sourceId) {
    if (!isCurrent(view) || view.batch || view.inspectingBatch ||
        (view.attempt && !terminalState(view.attempt.state))) return Promise.resolve();
    // The previous operation is complete; opening a deferred film starts a
    // new explicit review, never a retry of an uncertain admission.
    if (view.attempt) {
      clearPolling(view);
      delete activeAttempts[view.backendId];
      view.attempt = null;
      view.reads = 0;
      view.refreshButton.hidden = true;
      view.newButton.hidden = true;
      view.reloadButton.disabled = false;
      view.operationDetail.textContent = '';
    }
    return loadSources(view).then(function () {
      if (!isCurrent(view) || view.attempt || view.batch) return;
      const item = view.allOrdinary.find(function (entry) {
        return entry.sourceId === sourceId;
      });
      if (!item) {
        report(view, 'Die zurückgestellte Quelle ist nicht mehr aktuell. Bitte neu scannen.', true);
        return;
      }
      view.searchInput.value = '';
      view.folderPath = item.folder || '';
      view.seasonKey = '';
      filterVisibleSources(view);
      if (!view.sources.includes(sourceId)) {
        report(view, 'Film in der aktuellen Ansicht nicht verfügbar.', true);
        return;
      }
      view.select.value = sourceId;
      view.sourceDetails.textContent = 'Auswahl-ID: ' + sourceId;
      refreshSelectedPreview(view);
      setMobilePane(view, 'details');
    });
  }

  // Reuse the already implemented single-source preview and Core marker
  // operation for each batch-discovered duplicate, one at a time.
  function nextConfirmedDuplicate(view) {
    if (!isCurrent(view) || view.attempt || view.batch ||
        view.submitting || view.inspectingBatch) return;
    while (view.duplicateQueue.length) {
      const sourceId = view.duplicateQueue.shift();
      if (!view.sources.includes(sourceId)) continue;
      view.duplicateReviewPending = true;
      view.select.value = sourceId;
      view.sourceDetails.textContent = 'Auswahl-ID: ' + sourceId;
      // Batch-discovered duplicates must be independently rechecked before
      // the existing marker operation, even if a previous read was ready.
      refreshSelectedPreview(view, true);
      return;
    }
    view.duplicateReviewPending = false;
    if (view.batchReview) renderBatchReview(view);
  }

  function renderBatchReview(view) {
    const review = view.batchReview;
    if (!review) return;
    view.batchReviewList.replaceChildren();
    const labels = {
      ready: 'Für VDR-Import geeignet',
      encode: 'Zurückgestellt: Video-Umkodierung nötig',
      duplicate: 'VDR-Aufnahme vorhanden – automatische .duplicate-Markierung läuft',
      marked: 'Als .duplicate markiert – Core-Bestätigung liegt vor',
      insufficient: 'Zurückgestellt: Speicherplatz fehlt',
      unavailable: 'Zurückgestellt: Backend nicht verfügbar',
      pending: 'Zurückgestellt: Prüfung noch ausstehend',
      failed: 'Zurückgestellt: Prüfung fehlgeschlagen',
      running: 'Import läuft / Bestätigung ausstehend',
      succeeded: 'VDR-Aufnahme unabhängig bestätigt',
      stopped: 'Import angehalten'
    };
    review.rows.forEach(function (row) {
      const line = element('p', 'media-import-batch-row',
        row.title + ' – ' + (labels[row.state] || 'Prüfung ausstehend'));
      if (['encode', 'insufficient', 'unavailable', 'pending', 'failed'].includes(row.state)) {
        const action = element('button', 'media-import-batch-deferred',
          'Einzeln prüfen');
        action.type = 'button';
        action.disabled = Boolean(view.batch) || view.inspectingBatch ||
          Boolean(view.attempt && !terminalState(view.attempt.state));
        action.addEventListener('click', function () {
          return openDeferredFilm(view, row.sourceId);
        });
        line.appendChild(action);
      }
      view.batchReviewList.appendChild(line);
    });
    const ready = review.rows.filter(function (row) {
      return row.state === 'ready';
    }).length;
    const deferred = review.rows.filter(function (row) {
      return ['encode', 'insufficient', 'unavailable', 'pending', 'failed'].includes(row.state);
    }).length;
    const duplicates = review.rows.filter(function (row) {
      return row.state === 'duplicate' || row.state === 'marked';
    }).length;
    const confirmed = review.rows.filter(function (row) {
      return row.state === 'succeeded';
    }).length;
    view.batchSummary.textContent = ready + ' direkt importierbar, ' +
      deferred + ' zurückgestellt, ' + duplicates + ' Dubletten, ' +
      confirmed + ' bereits bestätigt.' +
      (view.duplicateReviewPending ? ' Bestätigte Dubletten werden automatisch markiert.' : '') +
      ' Andere Filme bleiben unverändert. ' +
      'Zurückgestellte Filme lassen sich über „Einzeln prüfen“ neu untersuchen; ' +
      'PC/HandBrake und Überspringen erfordern weiterhin eine Einzelentscheidung.';
    view.batchConfirmButton.hidden = Boolean(view.batch) || !ready;
    view.batchConfirmButton.disabled = view.inspectingBatch ||
      Boolean(view.attempt) || Boolean(view.duplicateReviewPending) || !ready;
  }

  function inspectBatchPreview(view, sourceId, generation) {
    // The existing Agent can answer pending until its next regular scan.
    // Re-read the same source rather than treating one HTTP 202 as a failure.
    function poll(attempt) {
      if (!isCurrent(view) || generation !== view.reviewGeneration)
        return Promise.resolve({state: 'pending'});
      return publicClient().getMediaImportSourcePreview({
        backendId: view.backendId, sourceId: sourceId,
        credentials: 'same-origin', cache: 'no-store'
      }).then(function (data) {
        if (!data || data.backendId !== view.backendId ||
            data.sourceId !== sourceId) throw new Error('Fremde Vorschauantwort');
        if (data.state === 'pending') {
          if (attempt >= 6) return {state: 'pending'};
          return new Promise(function (resolve) {
            global.setTimeout(resolve, POLL_MS);
          }).then(function () { return poll(attempt + 1); });
        }
        if (!validMediaImportPreview(data, view.backendId, sourceId))
          throw new Error('Ungültige Importdiagnose');
        if (data.status === 'duplicate') return {state: 'duplicate', title: data.title};
        if (data.status === 'unavailable') return {state: 'unavailable', title: data.title};
        if (data.videoReencodeRequired)
          return {state: 'encode', title: data.title};
        if (data.status === 'insufficient')
          return {state: 'insufficient', title: data.title};
        return {state: 'ready', title: data.title};
      });
    }
    return poll(0);
  }

  function startBatch(view, selectedOnly) {
    if (!isCurrent(view) || view.batch || view.attempt ||
        view.submitting || view.inspectingBatch) return Promise.resolve();
    const ids = selectedOnly
      ? view.sources.filter(function (id) { return view.checkedSourceIds.has(id); })
      : view.sources.slice();
    if (ids.length < 2 || ids.length > MAX_IMPORT_FILMS) {
      if (ids.length > MAX_IMPORT_FILMS)
        report(view, 'Höchstens 100 Filme pro Importdurchlauf erlaubt.', true);
      return Promise.resolve();
    }
    resetBatchReview(view);
    const generation = view.reviewGeneration;
    const sourceGeneration = view.sourcesGeneration;
    view.inspectingBatch = true;
    view.batchButton.disabled = true;
    view.selectedBatchButton.disabled = true;
    view.batchConfirmButton.hidden = true;
    view.seasonFilter.disabled = true;
    view.batchSummary.textContent = 'Technische VDR-Importeignung wird lesend geprüft: 0 von ' +
      ids.length + '. Noch kein Import gestartet.';
    const rows = [];
    let chain = Promise.resolve();
    ids.forEach(function (sourceId) {
      chain = chain.then(function () {
        if (!isCurrent(view) || generation !== view.reviewGeneration) return;
        return inspectBatchPreview(view, sourceId, generation).catch(function (error) {
          if (error && (error.status === 401 || error.status === 403)) throw error;
          return {state: 'failed'};
        }).then(function (result) {
          if (!isCurrent(view) || generation !== view.reviewGeneration) return;
          rows.push({sourceId: sourceId,
            title: result.title || view.sourceTitles[sourceId] || 'Film',
            state: result.state});
          view.batchSummary.textContent = 'Technische VDR-Importeignung wird lesend geprüft: ' +
            rows.length + ' von ' + ids.length + '. Noch kein Import gestartet.';
        });
      });
    });
    return chain.then(function () {
      if (!isCurrent(view) || generation !== view.reviewGeneration ||
          sourceGeneration !== view.sourcesGeneration) return;
      view.batchReview = {rows: rows, ids: ids, sourceGeneration: sourceGeneration};
      view.duplicateQueue = rows.filter(function (row) {
        return row.state === 'duplicate';
      }).map(function (row) { return row.sourceId; });
      view.duplicateReviewPending = view.duplicateQueue.length > 0;
      view.inspectingBatch = false;
      renderBatchReview(view);
      const ready = rows.filter(function (row) { return row.state === 'ready'; }).length;
      report(view, ready ? ready + ' geeignete Filme geprüft. Erst die separate ' +
        'Bestätigung startet den Import.' :
        'Kein Film für einen direkten Import freigegeben. ' +
        'Zurückgestellte Filme bitte einzeln prüfen.', !ready);
    }).catch(function (error) {
      if (!isCurrent(view) || generation !== view.reviewGeneration) return;
      resetBatchReview(view);
      view.batchButton.disabled = view.sources.length < 2;
      updateSelectionCount(view);
      report(view, operationError(error) + ' Keine Importaufträge erstellt.', true);
    }).finally(function () {
      if (!isCurrent(view) || generation !== view.reviewGeneration) return;
      view.inspectingBatch = false;
      view.seasonFilter.disabled = Boolean(view.batch) || Boolean(view.attempt);
      view.batchButton.disabled = view.sources.length < 2;
      updateSelectionCount(view);
      if (view.duplicateReviewPending) nextConfirmedDuplicate(view);
    });
  }

  function confirmBatch(view) {
    if (!isCurrent(view) || !view.batchReview || view.inspectingBatch ||
        view.duplicateReviewPending || view.attempt || view.batch ||
        view.batchReview.sourceGeneration !== view.sourcesGeneration) return;
    const review = view.batchReview;
    const ids = review.rows.filter(function (row) {
      return row.state === 'ready' && view.sources.includes(row.sourceId);
    }).map(function (row) { return row.sourceId; });
    if (!ids.length) return;
    const deferred = review.rows.length - ids.length;
    if (typeof global.confirm !== 'function' ||
        !global.confirm(ids.length + ' technisch geeignete Filme nacheinander ' +
          'in VDR importieren? ' + deferred + ' bleiben zurückgestellt oder ' +
          'werden wegen einer Dublette übersprungen. ' +
          'Jeder Auftrag wird separat geprüft. Keine Videoumkodierung freigegeben.')) return;
    const batch = {remaining: ids, total: ids.length, completed: 0, waiting: '',
      review: review};
    view.batch = batch;
    activeBatches[view.backendId] = batch;
    view.batchConfirmButton.hidden = true;
    view.batchButton.disabled = true;
    view.selectedBatchButton.disabled = true;
    view.selectAllVisibleButton.disabled = true;
    view.searchInput.disabled = true;
    view.statusFilter.disabled = true;
    view.seasonFilter.disabled = true;
    view.batchStopButton.hidden = false;
    view.select.value = ids[0];
    renderBatchReview(view);
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
      renderImportFeedback(view);
      const recordingConfirmed = operation.state === 'succeeded' &&
        (!view.attempt.videoDecision || view.attempt.videoDecision === 'local');
      if (recordingConfirmed) {
        // Core only reaches 'succeeded' for a recording after committed
        // recording-cache readback. Keep a historical operation reference
        // only: a fixed inventory source ID may later refer to new bytes.
        confirmedImports[view.backendId + '|' + view.attempt.sourceId] = {
          operationId: operation.operationId,
          possiblePcReturn: view.attempt.pcReturn === true
        };
      }
      const possibleReturnCompleted = recordingConfirmed && view.attempt.pcReturn === true;
      const cleanup = recordingConfirmed && typeof operation.sourceCleanup === 'string'
        ? cleanupLabel(operation.sourceCleanup) : '';
      report(view, operationLabel(operation.state, view.attempt.videoDecision) +
        (possibleReturnCompleted
          ? ' Möglicher PC-Rücklauf: VDR-Readback bestätigt den Import ' +
            'der neuen Datei, nicht ihre Medienidentität mit dem geparkten Original.'
          : '') + (cleanup ? ' ' + cleanup : ''), false);
      view.operationDetail.textContent =
        'Vorgang ' + operation.operationId + ' · ' + operation.state +
        (cleanup ? ' · ' + cleanup : '');
      if (terminalState(operation.state) && !view.batch) {
        view.newButton.hidden = false;
      }
      if (view.batch) advanceBatch(view, operation);
      if (!view.batch && operation.state === 'succeeded' &&
          (view.attempt.videoDecision === 'duplicate' ||
           view.attempt.videoDecision === 'delete_duplicate')) {
        // Core has verified the marker operation; refresh the source inventory
        // to show the actual Rectools filename, not an imagined local status.
        const finishedId = view.attempt.sourceId;
        const previousReview = view.batchReview;
        const remaining = view.duplicateQueue.slice();
        delete activeAttempts[view.backendId];
        view.attempt = null;
        view.refreshButton.hidden = true;
        view.newButton.hidden = true;
        view.reloadButton.disabled = false;
        return loadSources(view).then(function () {
          if (!isCurrent(view)) return;
          if (previousReview) {
            const row = previousReview.rows.find(function (entry) {
              return entry.sourceId === finishedId;
            });
            if (row && row.state === 'duplicate') row.state = 'marked';
            previousReview.sourceGeneration = view.sourcesGeneration;
            view.batchReview = previousReview;
            view.duplicateQueue = remaining;
            view.duplicateReviewPending = remaining.length > 0;
            renderBatchReview(view);
            view.resumeDuplicateReview = remaining.length > 0;
          }
        });
      }
      const limit = view.batch ? MAX_BATCH_READS : MAX_AUTOMATIC_READS;
      const awaitingCleanup = recordingConfirmed &&
        operation.sourceCleanup === 'pending';
      if ((!terminalState(operation.state) || awaitingCleanup) &&
          view.reads < limit) {
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
      if (isCurrent(view) && view.resumeDuplicateReview) {
        view.resumeDuplicateReview = false;
        nextConfirmedDuplicate(view);
      }
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

  // Display the established Core operation in ordinary language, right
  // beside the preview action. This does not start or retry operations.
  function renderImportFeedback(view) {
    if (!view.importFeedback) return;
    const attempt = view.attempt;
    view.importFeedback.hidden = !view.submitting && !attempt;
    if (view.importFeedback.hidden) return;
    const marker = attempt && ['desktop', 'skip', 'duplicate', 'release',
      'delete_duplicate'].includes(attempt.videoDecision);
    const noun = marker ? 'Dateiaktion' : 'Import';
    const state = attempt && attempt.state || '';
    const titles = {
      accepted: noun + ' angenommen',
      queued: noun + ' wartet auf Verarbeitung',
      dispatching: noun + ' läuft',
      executed_unverified: noun + ' ausgeführt – Bestätigung läuft',
      succeeded: noun + ' abgeschlossen',
      rejected: noun + ' abgelehnt',
      conflict: noun + ' nicht ausgeführt',
      failed_before_dispatch: noun + ' fehlgeschlagen',
      failed_verified: noun + ' fehlgeschlagen',
      cancelled: noun + ' abgebrochen',
      outcome_unknown: 'Ergebnis ungewiss – nicht erneut starten'
    };
    const descriptions = {
      accepted: 'Der Server hat den Auftrag angenommen. Die Ausführung wird automatisch überwacht.',
      queued: 'Der Import wartet auf den Start. Diese Anzeige aktualisiert sich automatisch.',
      dispatching: marker
        ? 'Die Datei wird gerade markiert.'
        : 'Die Aufnahme wird verarbeitet. Diese Anzeige aktualisiert sich automatisch.',
      executed_unverified: marker
        ? 'Die Dateiaktion wurde ausgeführt. Die abschließende Bestätigung steht noch aus.'
        : 'Die Datei ist verarbeitet. Wir prüfen noch, ob die Aufnahme im VDR angekommen ist.',
      succeeded: attempt && attempt.videoDecision === 'delete_duplicate'
        ? 'Die doppelte Quelldatei wurde gelöscht. Die VDR-Aufnahme bleibt erhalten.'
        : marker ? 'Die Dateiaktion wurde bestätigt.'
        : 'Die Aufnahme ist im VDR vorhanden.',
      rejected: 'Der Auftrag wurde nicht angenommen.',
      conflict: 'Der Auftrag konnte wegen eines Konflikts nicht ausgeführt werden.',
      failed_before_dispatch: 'Die Ausführung wurde nicht gestartet.',
      failed_verified: 'Die Ausführung ist fehlgeschlagen.',
      cancelled: 'Der Auftrag wurde abgebrochen.',
      outcome_unknown: 'Der Ausgang ist nicht bestätigt. Bitte „Vorgang prüfen“ verwenden und nicht erneut importieren.'
    };
    view.importFeedbackTitle.textContent = !attempt
      ? 'Aktion wird vorbereitet'
      : attempt.pending ? 'Anfrage wird übermittelt'
      : !attempt.operationId ? 'Antwort ungewiss – nicht erneut starten'
      : titles[state] || 'Auftragsstatus wird geprüft';
    view.importFeedbackText.textContent = !attempt
      ? 'Quelle und Berechtigung werden geprüft. Noch kein neuer Auftrag bestätigt.'
      : attempt.pending ? 'Die Anfrage wurde gesendet; eine Antwort steht noch aus.'
      : !attempt.operationId
        ? 'Der Server hat keine eindeutige Antwort geliefert. Bitte nicht erneut klicken.'
        : descriptions[state] || 'Der vorhandene Auftrag wird über „Vorgang prüfen“ abgefragt.';
    view.importFeedback.className = 'media-import-import-feedback' +
      (attempt && (terminalState(state) || state === 'outcome_unknown')
        ? ' is-finished' : ' is-running');
  }

  function showAttempt(view) {
    view.select.disabled = true;
    view.startButton.disabled = true;
    view.selectedBatchButton.disabled = true;
    view.selectAllVisibleButton.disabled = true;
    view.localReencode.disabled = true;
    view.decision.disabled = true;
    view.markedSelect.disabled = true;
    view.releaseButton.disabled = true;
    view.reloadButton.disabled = true;
    view.searchInput.disabled = true;
    view.statusFilter.disabled = true;
    view.seasonFilter.disabled = true;
    view.operationDetail.textContent = view.attempt.operationId
      ? 'Vorgang ' + view.attempt.operationId
      : view.attempt.pending
        ? 'Import-Anfrage übermittelt, Serverantwort steht noch aus.'
        : 'Anfrage gesendet, Antwort nicht bestätigt.';
    view.refreshButton.hidden = false;
    view.refreshButton.disabled = !view.attempt.operationId;
    view.newButton.hidden = Boolean(view.batch) || !terminalState(view.attempt.state);
    view.batchStopButton.hidden = !view.batch;
    if (view.batch && view.batch.review) {
      view.batchReview = view.batch.review;
      renderBatchReview(view);
    }
    // The same operation indicator is always visible after owner remounts.
    if (view.attempt.videoDecision === 'delete_duplicate') {
      view.previewCard.replaceChildren(
        element('h5', 'media-import-preview-title', 'Dublette aus Importquelle löschen'),
        view.importFeedback);
      renderImportFeedback(view);
    } else {
      renderPreviewCard(view, view.previewResult,
        view.allOrdinary.find(function (item) {
          return item.sourceId === view.attempt.sourceId;
        }));
    }
    if (view.attempt.operationId) {
      readOperation(view);
    } else if (view.attempt.pending) {
      report(view, 'Import-Anfrage läuft. Bis zur Serverantwort keinen weiteren Import starten.');
    } else {
      report(view, 'Ergebnis der Auftragsannahme ungewiss. Keine neue Anfrage erzeugen. ' +
        'Die ursprüngliche Idempotency-Key-Zuordnung bleibt für diese Browsersitzung gesperrt.', true);
    }
  }

  function validRelativeFolder(folder) {
    if (typeof folder !== 'string' || folder.length > 240 ||
        /[\\\r\n\x00-\x1f\x7f]/.test(folder) ||
        folder.startsWith('/') || folder.endsWith('/')) return false;
    return !folder || folder.split('/').every(function (part) {
      return part.length > 0 && part !== '.' && part !== '..';
    });
  }

  function validLocalArtwork(value) {
    if (typeof value !== 'string' || value.length > 349600) return false;
    const image = /^(data:image\/(?:jpeg|png|webp);base64,)([A-Za-z0-9+/]+={0,2})$/.exec(value);
    if (!image || image[2].length % 4 !== 0) return false;
    return (image[1] === 'data:image/jpeg;base64,' && image[2].startsWith('/9j/')) ||
      (image[1] === 'data:image/png;base64,' && image[2].startsWith('iVBORw0KGgo')) ||
      (image[1] === 'data:image/webp;base64,' && image[2].startsWith('UklGR'));
  }


  // MT.META5: transient, explicit provider selection for an import source.
  // No online value enters the local NFO, Core import payload or persistence.
  function resetOnline(view) {
    ++view.onlineSeq;
    view.onlineItems = [];
    view.onlineSelected = null;
    view.onlineSeries = null;
    view.onlineSeason = null;
    view.onlineMode = 'search';
    view.onlineQuery = '';
    view.onlineStatus = '';
    view.onlinePending = false;
    view.onlineAuto = false;
    view.onlineAutoSelected = false;
  }

  function onlineValid(item, kind, series, season) {
    const namespaces = {movie: 'movie', series: 'tv',
      season: 'tv-season', episode: 'tv-episode'};
    return Boolean(item && item.providerId === 'tmdb' && item.kind === kind &&
      item.externalNamespace === namespaces[kind] &&
      typeof item.externalId === 'string' &&
      /^[0-9]{1,16}$/.test(item.externalId) &&
      typeof item.title === 'string' && item.title.length > 0 &&
      item.title.length <= 512 &&
      (typeof item.overview === 'string' && item.overview.length <= 16384) &&
      (typeof item.releaseDate === 'string' && item.releaseDate.length <= 64) &&
      (kind !== 'season' ||
        (series && item.parentExternalId === series.externalId &&
         Number.isInteger(item.seasonNumber) && item.seasonNumber >= 0)) &&
      (kind !== 'episode' ||
        (series && season && item.parentExternalId === series.externalId &&
         item.seasonNumber === season.seasonNumber &&
         Number.isInteger(item.episodeNumber) && item.episodeNumber > 0)));
  }

  function onlineImageUrl(backendId, candidate) {
    if (!candidate || !['movie', 'tv', 'tv-season', 'tv-episode']
        .includes(candidate.externalNamespace) ||
        !/^[0-9]{1,16}$/.test(candidate.externalId || '') ||
        typeof candidate.posterReference !== 'string' ||
        !/^\/[A-Za-z0-9_.-]{5,255}$/.test(candidate.posterReference) ||
        candidate.posterReference.includes('..')) return '';
    const path = '/api/backends/' + encodeURIComponent(backendId) +
      '/settings/series-artwork/candidate-image?externalNamespace=' +
      encodeURIComponent(candidate.externalNamespace) +
      '&externalId=' + encodeURIComponent(candidate.externalId) +
      '&posterReference=' + encodeURIComponent(candidate.posterReference);
    const resolver = global.VdrSuitePublicUrl;
    return resolver && typeof resolver.resolvePath === 'function'
      ? resolver.resolvePath(path) : path;
  }

  function onlineLookup(view, endpoint, payload, kind) {
    if (!isCurrent(view) || view.onlinePending ||
        !view.sources.includes(view.select.value)) return Promise.resolve();
    const seq = ++view.onlineSeq;
    const sourceId = view.select.value;
    view.onlinePending = true;
    view.onlineItems = [];
    view.onlineSelected = null;
    view.onlineStatus = 'Online-Treffer werden gesucht …';
    updatePreview(view);
    const session = global.VdrSuiteBrowserSession;
    const csrf = session && typeof session.csrfHeaders === 'function'
      ? session.csrfHeaders() : {};
    const route = '/api/backends/' + encodeURIComponent(view.backendId) +
      '/recordings/metadata/' + endpoint;
    return Promise.resolve().then(function () {
      const api = global.VdrSuiteClientApi;
      if (!api || typeof api.requestJson !== 'function')
        throw new Error('Metadaten-API fehlt');
      return api.requestJson(route, {
        method: 'POST',
        headers: Object.assign({'Content-Type': 'application/json'},
          csrf && typeof csrf === 'object' ? csrf : {}),
        body: JSON.stringify(payload),
        cache: 'no-store',
        credentials: 'same-origin'
      });
    }).then(function (data) {
      if (!isCurrent(view) || seq !== view.onlineSeq ||
          sourceId !== view.select.value) return;
      view.onlinePending = false;
      if (!data || data.providerId !== 'tmdb' || data.providerAvailable !== true ||
          !Array.isArray(data.candidates) || data.candidates.length > 20) {
        view.onlineStatus = 'Keine gültige Online-Antwort. Lokale Informationen bleiben erhalten.';
      } else {
        view.onlineItems = data.candidates.filter(function (item) {
          return onlineValid(item, kind, view.onlineSeries, view.onlineSeason);
        });
        view.onlineStatus = view.onlineItems.length
          ? 'TMDb-Treffer gefunden. Eindeutige Episoden werden automatisch zugeordnet.'
          : 'Kein passender Treffer. Lokale Metadaten bleiben erhalten.';
      }
      updatePreview(view);
      return view.onlineItems.slice();
    }).catch(function (error) {
      if (!isCurrent(view) || seq !== view.onlineSeq ||
          sourceId !== view.select.value) return;
      view.onlinePending = false;
      view.onlineStatus = error && (error.status === 401 || error.status === 403)
        ? 'Keine Berechtigung zur TMDb-Suche; lokale Daten bleiben verfügbar.'
        : 'TMDb nicht erreichbar oder nicht eingerichtet; lokale Daten bleiben verfügbar.';
      updatePreview(view);
      return [];
    });
  }

  // Use the existing TMDb search -> seasons -> episodes API after selecting
  // one recognized series episode. No import, metadata write or automatic
  // selection of an ambiguous provider result is authorized here.
  function normalizedSeriesName(value) {
    return String(value || '').normalize('NFKC').toLocaleLowerCase('de')
      .replace(/[^\p{L}\p{N}]+/gu, ' ').replace(/\s+/g, ' ').trim();
  }

  function autoIdentifyEpisode(view, source) {
    const coords = episodeCoordinates(source);
    const title = onlineSeriesSearchTitle(source);
    if (coords.episode === null || !title || title.length < 2 ||
        title.length > 160 || !view.sources.includes(source.sourceId)) return;
    const sourceId = source.sourceId;
    const generation = view.previewGeneration;
    const active = function () {
      return isCurrent(view) && view.onlineAuto &&
        generation === view.previewGeneration && view.select.value === sourceId;
    };
    const normalized = normalizedSeriesName(title);
    const alternative = normalized.replace(/^star wars /, '');
    const match = function (candidates) {
      return candidates.filter(function (candidate) {
        const name = normalizedSeriesName(candidate.title);
        return name === normalized || (alternative !== normalized &&
          name === alternative);
      });
    };
    const lookupSeries = function (query) {
      return onlineLookup(view, 'search',
        {query: query, kind: 'series', limit: 12}, 'series');
    };
    view.onlineAuto = true;
    view.onlineKind = 'series';
    view.onlineQuery = title;
    Promise.resolve().then(function () {
      if (!active()) return [];
      return lookupSeries(title);
    }).then(function (candidates) {
      if (!active()) return null;
      const matches = match(candidates || []);
      // A known franchise prefix can differ from TMDb's displayed title
      // (e.g. local 'Star Wars-Andor', provider 'Andor'). Search its name
      // separately only when the first response had no title match.
      if (!matches.length && alternative !== normalized &&
          alternative.length >= 2 && alternative.length <= 160) {
        return lookupSeries(alternative).then(function (fallback) {
          return active() ? match(fallback || []) : null;
        });
      }
      return matches;
    }).then(function (matches) {
      if (!active() || !matches) return null;
      if (matches.length !== 1) {
        view.onlineStatus = matches.length
          ? 'Mehrdeutige Serie: Bitte den TMDb-Treffer manuell auswählen.'
          : 'Keine eindeutige Serie erkannt. TMDb-Suche bleibt manuell möglich.';
        updatePreview(view);
        return null;
      }
      view.onlineSeries = matches[0];
      view.onlineMode = 'seasons';
      return onlineLookup(view, 'seasons',
        {seriesExternalId: matches[0].externalId, limit: 20}, 'season');
    }).then(function (seasons) {
      if (!active() || !Array.isArray(seasons)) return null;
      const matches = seasons.filter(function (item) {
        return item.seasonNumber === coords.season;
      });
      if (matches.length !== 1) {
        view.onlineStatus = 'Staffel nicht eindeutig gefunden. Manuelle TMDb-Auswahl möglich.';
        updatePreview(view);
        return null;
      }
      view.onlineSeason = matches[0];
      view.onlineMode = 'episodes';
      return onlineLookup(view, 'episodes', {
        seriesExternalId: view.onlineSeries.externalId,
        seasonNumber: coords.season, limit: 20
      }, 'episode');
    }).then(function (episodes) {
      if (!active() || !Array.isArray(episodes)) return;
      const matches = episodes.filter(function (item) {
        return item.seasonNumber === coords.season &&
          item.episodeNumber === coords.episode;
      });
      if (matches.length === 1) {
        view.onlineSelected = matches[0];
        view.onlineAutoSelected = true;
        view.onlineStatus = 'TMDb automatisch zugeordnet: Staffel ' +
          coords.season + ', Folge ' + coords.episode +
          '. Nur Anzeige; kein Import und keine NFO-Änderung.';
      } else {
        view.onlineStatus = matches.length
          ? 'Episode mehrdeutig. Bitte den TMDb-Treffer manuell auswählen.'
          : 'Episode nicht gefunden. Manuelle TMDb-Auswahl möglich.';
      }
      updatePreview(view);
    }).catch(function () {
      if (!active()) return;
      view.onlinePending = false;
      view.onlineStatus = 'Automatischer TMDb-Abgleich fehlgeschlagen. Manuelle Suche möglich.';
      updatePreview(view);
    });
  }

  function renderOnline(view, card, plan, source) {
    if (!source || !view.sources.includes(view.select.value)) return;
    const box = element('section', 'media-import-online');
    box.appendChild(element('h6', '', 'Online-Metadaten · TMDb (optional)'));
    box.appendChild(element('p', 'media-import-online-note',
      'Automatischer Episodenabgleich bei eindeutigen Treffern. NFO und Import bleiben unverändert.'));
    const tools = element('div', 'media-import-online-tools');
    const coordinates = episodeCoordinates(source);
    const seriesTitle = onlineSeriesSearchTitle(source);
    // The currently browsed TMDb series is the primary context while choosing
    // a season/episode. Keep recognized local coordinates visible beneath it.
    if (view.onlineSeries && view.onlineMode !== 'search')
      box.appendChild(element('p', 'media-import-online-context',
        'Serie: ' + view.onlineSeries.title +
        (view.onlineSeason ? ' · Staffel ' + view.onlineSeason.seasonNumber : '')));
    if (coordinates.episode !== null) {
      box.appendChild(element('p', 'media-import-online-context',
        'Erkannt: Staffel ' + coordinates.season + ' · Folge ' +
        coordinates.episode));
    }
    const query = element('input', 'media-import-online-query');
    query.type = 'search';
    query.maxLength = 160;
    query.value = view.onlineQuery || seriesTitle ||
      (plan && plan.title) || source.label || '';
    query.setAttribute('aria-label', 'TMDb-Titel');
    query.addEventListener('input', function () { view.onlineQuery = query.value; });
    tools.appendChild(query);
    const kind = element('select', 'media-import-online-kind');
    kind.setAttribute('aria-label', 'Medientyp');
    [['movie', 'Film'], ['series', 'Serie']].forEach(function (value) {
      const option = element('option', '', value[1]);
      option.value = value[0];
      kind.appendChild(option);
    });
    kind.value = view.onlineKind;
    kind.addEventListener('change', function () {
      if (!isCurrent(view)) return;
      const newKind = kind.value;
      const value = query.value;
      resetOnline(view);
      view.onlineKind = newKind;
      view.onlineQuery = value;
      updatePreview(view);
    });
    tools.appendChild(kind);
    const search = element('button', 'media-import-online-search', 'TMDb suchen');
    search.type = 'button';
    search.disabled = view.onlinePending;
    search.addEventListener('click', function () {
      const value = String(query.value || '').trim();
      if (!isCurrent(view) || view.onlinePending) return;
      view.onlineQuery = value;
      if (value.length < 2 || value.length > 160) {
        view.onlineStatus = 'Mindestens zwei Zeichen für die Online-Suche eingeben.';
        updatePreview(view);
        return;
      }
      view.onlineAuto = false;
      view.onlineAutoSelected = false;
      view.onlineMode = 'search';
      view.onlineSeries = null;
      view.onlineSeason = null;
      return onlineLookup(view, 'search',
        {query: value, kind: view.onlineKind, limit: 12}, view.onlineKind);
    });
    tools.appendChild(search);
    box.appendChild(tools);
    if (view.onlineStatus) {
      const status = element('p', 'media-import-online-status', view.onlineStatus);
      status.setAttribute('role', 'status');
      box.appendChild(status);
    }
    const list = element('div', 'media-import-online-results');
    view.onlineItems.forEach(function (candidate) {
      const result = element('article', 'media-import-online-result');
      result.appendChild(element('strong', '', candidate.title));
      result.appendChild(element('small', '',
        candidate.kind + ' · TMDb #' + candidate.externalId +
        (candidate.releaseDate ? ' · ' + candidate.releaseDate : '')));
      if (candidate.overview)
        result.appendChild(element('p', '', candidate.overview));
      const choose = element('button', 'media-import-online-choose',
        candidate.kind === 'movie' ? 'Film auswählen' :
          candidate.kind === 'episode' ? 'Episode auswählen' : 'Serie auswählen');
      choose.type = 'button';
      choose.disabled = view.onlinePending || candidate.kind === 'season';
      choose.addEventListener('click', function () {
        if (!isCurrent(view) || view.onlinePending ||
            !view.sources.includes(view.select.value)) return;
        const coords = episodeCoordinates(source);
        if (candidate.kind === 'series' && coords.episode !== null) {
          view.onlineStatus = 'Für diese Folge bitte Staffel und Episode auswählen.';
        } else if (candidate.kind === 'episode' && coords.episode !== null &&
            (coords.season !== candidate.seasonNumber ||
             coords.episode !== candidate.episodeNumber)) {
          view.onlineStatus = 'Episodenkennung passt nicht zur Quelle. Keine Zuordnung.';
        } else {
          view.onlineSelected = candidate;
          view.onlineAutoSelected = false;
          view.onlineStatus = 'TMDb-Treffer manuell ausgewählt – nur Vorschau, keine NFO-/VDR-Änderung.';
        }
        updatePreview(view);
      });
      result.appendChild(choose);
      if (candidate.kind === 'series' || candidate.kind === 'season') {
        const next = element('button', 'media-import-online-next',
          candidate.kind === 'series' ? 'Staffeln anzeigen' : 'Episoden anzeigen');
        next.type = 'button';
        next.disabled = view.onlinePending;
        next.addEventListener('click', function () {
          if (!isCurrent(view) || view.onlinePending) return;
          if (candidate.kind === 'series') {
            view.onlineSeries = candidate;
            view.onlineSeason = null;
            view.onlineMode = 'seasons';
            return onlineLookup(view, 'seasons',
              {seriesExternalId: candidate.externalId, limit: 20}, 'season');
          }
          if (!view.onlineSeries) return;
          view.onlineSeason = candidate;
          view.onlineMode = 'episodes';
          return onlineLookup(view, 'episodes', {
            seriesExternalId: view.onlineSeries.externalId,
            seasonNumber: candidate.seasonNumber, limit: 20
          }, 'episode');
        });
        result.appendChild(next);
      }
      list.appendChild(result);
    });
    box.appendChild(list);
    if (view.onlineSelected) {
      const selected = view.onlineSelected;
      const confirmed = element('div', 'media-import-online-confirmed');
      confirmed.appendChild(element('strong', '',
        (view.onlineAutoSelected ? 'Automatisch erkannte TMDb-Episode: ' :
          'Bestätigte TMDb-Vorschau: ') + selected.title));
      if (selected.overview)
        confirmed.appendChild(element('p', '', selected.overview));
      const url = onlineImageUrl(view.backendId, selected);
      if (url && !view.onlineAutoSelected) {
        const poster = element('img', 'media-import-online-poster');
        poster.alt = 'TMDb-Poster für ' + selected.title;
        poster.loading = 'lazy';
        poster.src = url;
        poster.addEventListener('error', function () {
          // Missing provider artwork must not erase the confirmed title/plot.
          if (typeof poster.remove === 'function') poster.remove();
          confirmed.appendChild(element('p', '',
            'Online-Poster nicht verfügbar; lokale Bilder bleiben erhalten.'));
        });
        confirmed.appendChild(poster);
      }
      box.appendChild(confirmed);
    }
    card.appendChild(box);
  }

  function validMediaImportPreview(data, backendId, sourceId) {
    return Boolean(data && data.backendId === backendId &&
      data.sourceId === sourceId && data.state === 'ready' &&
      ['ready', 'duplicate', 'insufficient', 'unavailable'].includes(data.status) &&
      [data.sourceBytes, data.availableBytes, data.requiredBytes].every(
        function (n) { return Number.isSafeInteger(n) && n >= 0; }) &&
      typeof data.videoReencodeRequired === 'boolean' &&
      typeof data.targetFolder === 'string' &&
      (data.status === 'unavailable' || validPreviewTarget(data.targetFolder)) &&
      typeof data.videoCodec === 'string' && /^[a-z0-9_]{0,64}$/.test(data.videoCodec) &&
      typeof data.title === 'string' && data.title.length <= 120 &&
      (data.failureCode === undefined ||
        (typeof data.failureCode === 'string' && /^[a-z_]{0,32}$/.test(data.failureCode))) &&
      ['nfoOutline', 'nfoPlot', 'nfoGenre'].every(function (key) {
        return data[key] === undefined ||
          (typeof data[key] === 'string' && data[key].length <= 4096 &&
           !/[\r\n\x00-\x1f\x7f]/.test(data[key]));
      }) &&
      (data.nfoYear === undefined ||
        (typeof data.nfoYear === 'string' &&
         (data.nfoYear === '' || /^[0-9]{4}$/.test(data.nfoYear)))) &&
      (data.durationSeconds === undefined ||
        (Number.isSafeInteger(data.durationSeconds) &&
         data.durationSeconds >= 0 && data.durationSeconds <= 4294967295)) &&
      (data.status === 'unavailable' || data.title.length > 0));
  }

  function validPreviewTarget(folder) {
    return typeof folder === 'string' && folder.length > 0 &&
      folder.length <= 512 && !/[\\\r\n\x00-\x1f\x7f]/.test(folder) &&
      !folder.startsWith('/') && !folder.endsWith('/') &&
      folder.split('/').every(function (part) {
        return part.length > 0 && part !== '.' && part !== '..';
      });
  }

  // One existing authorized source endpoint; no client filesystem paths or
  // secondary scanner. Always discard partial pages when a snapshot changes.
  function readSourcePages(backendId) {
    const all = [];
    const seen = new Set();
    let partial = false;
    const prefix = '/api/v1/backends/' + backendId + '/media-import-sources?cursor=';
    function page(cursor, count) {
      const options = readOptions(backendId);
      if (cursor) options.cursor = cursor;
      return publicClient().getMediaImportSources(options).then(function (response) {
        if (!response || !Array.isArray(response.items) ||
            response.items.length > 32 || count > 3)
          throw new Error('Ungültige Quellenliste');
        response.items.forEach(function (item) {
          if (!item || typeof item.sourceId !== 'string' ||
              !/^[A-Za-z0-9_-]{16,64}$/.test(item.sourceId) ||
              seen.has(item.sourceId) ||
              (item.folder !== undefined && !validRelativeFolder(item.folder)) ||
              (item.label !== undefined &&
               (typeof item.label !== 'string' || !item.label ||
                item.label.length > 160 ||
                /[\/\\\r\n\x00-\x1f\x7f]/.test(item.label))))
            throw new Error('Ungültige Quellenliste');
          seen.add(item.sourceId);
          all.push(item);
        });
        if (all.length > MAX_IMPORT_FILMS)
          throw new Error('Mehr als 100 Importquellen in der Serverantwort');
        partial = partial || Boolean(response.meta && response.meta.partial);
        const link = response.links && response.links.next;
        if (!link) return {items: all, meta: {partial: partial}};
        if (typeof link !== 'string' || !link.startsWith(prefix) ||
            response.items.length !== 32 || count >= 3)
          throw new Error('Ungültige Scan-Fortsetzung');
        const next = link.slice(prefix.length);
        if (!/^mi1_[0-9]{1,20}_[0-9]{1,3}$/.test(next))
          throw new Error('Ungültige Scan-Fortsetzung');
        return page(next, count + 1);
      });
    }
    return page('', 0);
  }

  function updateSelectionCount(view) {
    const count = view.sources.filter(function (id) {
      return view.checkedSourceIds.has(id);
    }).length;
    view.selectionCount.textContent = count + ' von ' +
      view.sources.length + ' sichtbaren Filmen für den Stapel gewählt.';
    view.selectedBatchButton.disabled = count < 2 || Boolean(view.batch) ||
      Boolean(view.attempt) || view.localReencode.checked ||
      view.decision.value !== 'import';
  }

  // MT.META1: presentation-only phone navigation. The same Core/Agent
  // source selection, preview, import and marker handlers remain the owner.
  function setMobilePane(view, pane) {
    if (!isCurrent(view) || !view.mobileButtons[pane]) return;
    view.root.dataset.mobilePane = pane;
    Object.keys(view.mobileButtons).forEach(function (name) {
      const button = view.mobileButtons[name];
      const active = name === pane;
      button.setAttribute('aria-pressed', active ? 'true' : 'false');
      button.className = 'media-import-mobile-switch' +
        (active ? ' is-active' : '');
    });
  }

  // META4: advisory grouping from authenticated relative folders and explicit
  // S01E02/Staffel 1 tokens only. Neither names nor grouping confer identity
  // or permission: the opaque sourceId remains the ONLY import selection.
  function episodeCoordinates(item) {
    const parts = (item.folder || '').split('/').filter(Boolean);
    const seasonName = parts.length &&
      /^(?:Staffel|Season)[ ._-]*(0?[1-9]|[1-9][0-9])$/i.exec(parts[parts.length - 1]);
    const filename = /(?:^|[\s._-])S(0?[1-9]|[1-9][0-9])E(0?[1-9]|[1-9][0-9]{1,2})(?=$|[\s._-])/i
      .exec(item.label || '');
    const named = /(?:^|[\s._-])(?:Staffel|Season)[\s._-]*(0?[1-9]|[1-9][0-9])[\s._-]+(?:Folge|Episode)[\s._-]*(0?[1-9]|[1-9][0-9]{1,2})(?=$|[\s._-])/i
      .exec(item.label || '');
    const folderSeason = seasonName ? Number(seasonName[1]) : null;
    const labelSeason = filename ? Number(filename[1]) :
      named ? Number(named[1]) : null;
    const season = folderSeason !== null && labelSeason !== null &&
      folderSeason !== labelSeason ? null :
      folderSeason !== null ? folderSeason : labelSeason;
    const folderEpisode = folderSeason !== null && !filename && !named
      ? /(?:^|[\s._-])(?:Folge|Episode)[\s._-]*(0?[1-9]|[1-9][0-9]{1,2})(?=$|[\s._-])/i.exec(item.label || '')
      : null;
    const episode = filename ? Number(filename[2]) :
      named ? Number(named[2]) :
      folderEpisode ? Number(folderEpisode[1]) : null;
    return {
      seriesPath: seasonName ? parts.slice(0, -1).join('/') : item.folder || '',
      season: season,
      episode: season !== null ? episode : null
    };
  }

  function onlineSeriesSearchTitle(source) {
    const coords = episodeCoordinates(source);
    if (coords.episode === null) return '';
    const folders = coords.seriesPath.split('/').filter(Boolean);
    const raw = folders.length ? folders[folders.length - 1] : '';
    if (/^(serien|series|tv|shows)$/i.test(raw)) return '';
    return raw.replace(/[_-]+/g, ' ')
      .replace(/\s*\((?:19|20)\d{2}\)\s*$/, '')
      .replace(/\s+/g, ' ').trim();
  }

  function renderSeasonChoices(view, searching) {
    const choices = view.seasonFilter;
    const eligible = view.allOrdinary.concat(view.allMarked).filter(function (item) {
      const folder = item.folder || '';
      return folder === view.folderPath ||
        folder.startsWith(view.folderPath + '/');
    });
    const seasons = Array.from(new Set(eligible.map(function (item) {
      return episodeCoordinates(item).season;
    }).filter(function (n) { return n !== null; }))).sort(function (a, b) { return a - b; });
    const unknown = eligible.some(function (item) {
      return episodeCoordinates(item).season === null;
    });
    const available = new Set(seasons.map(function (n) { return 's:' + n; }));
    if (unknown) available.add('other');
    if (!available.has(view.seasonKey)) view.seasonKey = '';
    choices.replaceChildren();
    const all = element('option', '', 'Alle Staffeln');
    all.value = '';
    choices.appendChild(all);
    seasons.forEach(function (season) {
      const option = element('option', '', 'Staffel ' + season);
      option.value = 's:' + season;
      choices.appendChild(option);
    });
    if (unknown) {
      const option = element('option', '', 'Ohne Staffelzuordnung');
      option.value = 'other';
      choices.appendChild(option);
    }
    choices.value = view.seasonKey;
    view.seasonLabel.hidden = searching || !view.folderPath ||
      (seasons.length === 0 && !unknown);
    const heading = view.folderPath ?
      view.folderPath.split('/').join(' › ') : 'Alle Medien';
    view.seriesCaption.textContent = 'Auswahl: ' + heading +
      ' · Staffelangaben nur bei eindeutigen Datei-/Ordnerkennungen';
  }

  // Build navigation only from previously validated Agent/Core relative
  // folders. This is NOT a server path resolver; clicks never send a path.
  function renderSourceFolders(view, searching) {
    const catalog = view.allOrdinary.concat(view.allMarked);
    const known = new Set(['']);
    catalog.forEach(function (item) {
      const parts = (item.folder || '').split('/').filter(Boolean);
      for (let i = 1; i <= parts.length; ++i)
        known.add(parts.slice(0, i).join('/'));
    });
    if (!known.has(view.folderPath)) view.folderPath = '';
    view.folderBreadcrumb.replaceChildren();
    view.folderList.replaceChildren();
    view.seriesGroups.replaceChildren();
    const navigate = function (path) {
      if (!isCurrent(view) || view.batch || view.attempt || !known.has(path))
        return;
      view.folderPath = path;
      view.seasonKey = '';
      view.searchInput.value = '';
      view.checkedSourceIds.clear();
      filterVisibleSources(view);
      setMobilePane(view, 'media');
    };
    const root = element('button', '', 'Importwurzeln');
    root.type = 'button';
    root.disabled = view.folderPath === '';
    root.addEventListener('click', function () { navigate(''); });
    view.folderBreadcrumb.appendChild(root);
    const parts = view.folderPath ? view.folderPath.split('/') : [];
    parts.forEach(function (name, i) {
      const path = parts.slice(0, i + 1).join('/');
      const part = element('button', '', name);
      part.type = 'button';
      part.disabled = path === view.folderPath;
      part.addEventListener('click', function () { navigate(path); });
      view.folderBreadcrumb.appendChild(part);
    });
    const children = Array.from(known).filter(function (path) {
      if (!path || path === view.folderPath) return false;
      const parent = path.lastIndexOf('/') < 0 ? '' :
        path.slice(0, path.lastIndexOf('/'));
      return parent === view.folderPath;
    }).sort(function (left, right) { return sourceCollator.compare(left, right); });
    children.forEach(function (path) {
      const name = path.slice(path.lastIndexOf('/') + 1);
      const button = element('button', 'media-import-folder', '📁 ' + name);
      button.type = 'button';
      button.addEventListener('click', function () { navigate(path); });
      view.folderList.appendChild(button);
    });
    view.folderList.hidden = searching || children.length === 0;
    view.folderBreadcrumb.hidden = known.size <= 1;

    // All groups are grounded in real relative source folders. Distinct
    // folders with identically named episodes MUST remain separate.
    const groups = new Map();
    catalog.forEach(function (item) {
      const group = episodeCoordinates(item).seriesPath;
      if (group && known.has(group)) groups.set(group, (groups.get(group) || 0) + 1);
    });
    Array.from(groups).sort(function (a, b) {
      return sourceCollator.compare(a[0], b[0]);
    }).forEach(function (entry) {
      const button = element('button', 'media-import-series-group',
        entry[0].split('/').join(' › ') + ' · ' + entry[1] + ' Dateien');
      button.type = 'button';
      button.dataset.folder = entry[0];
      button.setAttribute('aria-pressed', view.folderPath === entry[0] ? 'true' : 'false');
      button.addEventListener('click', function () { navigate(entry[0]); });
      view.seriesGroups.appendChild(button);
    });
    view.seriesGroups.hidden = searching || groups.size === 0;
  }

  // UX3 step 1: search/filter only the already authenticated, bounded
  // Agent/Core snapshot. No path guesses, filesystem requests or new grants.
  function filterVisibleSources(view) {
    if (!isCurrent(view) || view.attempt || view.batch) return;
    // A filter or folder change clears the selection and invalidates every
    // outstanding preview, including a late reply for a reselected source.
    if (view.previewTimer !== null) global.clearTimeout(view.previewTimer);
    view.previewTimer = null;
    ++view.previewGeneration;
    resetOnline(view);
    view.previewResult = null;
    view.previewStatus = '';
    view.previewFailed = false;
    // A filter change or rescan clears selection; do not leave an inert retry visible.
    view.previewRetryButton.hidden = true;
    resetBatchReview(view);
    const query = String(view.searchInput.value || '').trim().toLowerCase();
    const mode = view.statusFilter.value;
    renderSourceFolders(view, Boolean(query));
    renderSeasonChoices(view, Boolean(query));
    const matches = function (item) {
      const labelMatches = !query ||
        (item.label || item.sourceId).toLowerCase().includes(query);
      // A selected folder shows all verified descendants; opening a series
      // reveals its episodes without drilling down to a leaf directory.
      const folder = item.folder || '';
      const folderMatches = Boolean(query) || !view.folderPath ||
        folder === view.folderPath || folder.startsWith(view.folderPath + '/');
      const details = episodeCoordinates(item);
      const seasonMatches = Boolean(query) || !view.seasonKey ||
        (view.seasonKey === 'other' ? details.season === null :
          view.seasonKey === 's:' + details.season);
      return labelMatches && folderMatches && seasonMatches;
    };
    const normal = mode === 'marked' ? [] :
      view.allOrdinary.filter(matches);
    const marked = mode === 'normal' ? [] :
      view.allMarked.filter(matches);
    view.sources = normal.map(function (item) { return item.sourceId; });
    view.markedSources = marked.map(function (item) { return item.sourceId; });

    view.select.replaceChildren();
    const blank = element('option', '', 'Film oder Serienfolge auswählen');
    blank.value = '';
    view.select.appendChild(blank);
    normal.forEach(function (item) {
      const option = element('option', '', item.label || 'Film');
      option.value = item.sourceId;
      view.select.appendChild(option);
    });
    view.select.value = '';
    view.select.disabled = normal.length === 0;
    view.startButton.disabled = true;
    view.sourceDetails.textContent = '';
    view.preview.textContent = normal.length
      ? 'Folge in der Episodenliste anklicken. Die technische Vorschau startet automatisch.'
      : 'In diesem Ordner wurden keine passenden Filme gefunden. Anderen Ordner wählen oder Suche zurücksetzen.';
    renderPreviewCard(view, null, null);
    view.localReencode.checked = false;
    view.localReencode.disabled = true;
    view.decision.value = 'import';
    view.decision.disabled = true;
    view.choiceLabel.hidden = true;
    view.skipButton.hidden = true;
    view.skipButton.disabled = true;
    view.startButton.textContent = 'Import starten';
    view.batchButton.disabled = normal.length < 2 ||
      normal.length > MAX_IMPORT_FILMS;
    view.selectionRows.replaceChildren();
    normal.forEach(function (item) {
      const row = element('div', 'media-import-select-item');
      row.dataset.sourceId = item.sourceId;
      const checkbox = element('input', 'media-import-episode-check');
      checkbox.type = 'checkbox';
      checkbox.setAttribute('aria-label', 'Für Stapel auswählen: ' + item.label);
      checkbox.checked = view.checkedSourceIds.has(item.sourceId);
      checkbox.addEventListener('change', function () {
        if (!isCurrent(view) || view.attempt || view.batch) return;
        if (checkbox.checked) view.checkedSourceIds.add(item.sourceId);
        else view.checkedSourceIds.delete(item.sourceId);
        updateSelectionCount(view);
      });
      const open = element('button', 'media-import-episode-open', item.label);
      open.type = 'button';
      open.addEventListener('click', function () {
        if (!isCurrent(view) || view.batch || view.attempt) return;
        view.select.value = item.sourceId;
        view.markedSelect.value = '';
        view.releaseButton.disabled = true;
        view.releaseButton.hidden = true;
        view.sourceDetails.textContent = 'Auswahl-ID: ' + item.sourceId;
        refreshSelectedPreview(view);
        setMobilePane(view, 'details');
      });
      const parked = parkedPcEncodeMatches(view, item);
      const pcHint = possiblePcReturn(view, item)
        ? ' · Möglicher PC-Encode-Rücklauf'
        : parked.length ? ' · PC-Encode-Zuordnung mehrdeutig' : '';
      const confirmed = confirmedImport(view, item.sourceId);
      const coords = episodeCoordinates(item);
      const episodeLabel = coords.episode !== null
        ? ' · S' + String(coords.season).padStart(2, '0') +
          'E' + String(coords.episode).padStart(2, '0') : '';
      const previousCheck = view.previewCache.get(item.sourceId);
      const cachedStatus = previousCheck
        ? previousCheck.videoReencodeRequired ? 'Umkodierung nötig'
          : previousCheck.status === 'insufficient' ? 'Speicherplatz fehlt'
          : 'Direktimport möglich'
        : '';
      const meta = element('span', 'media-import-episode-meta',
        (item.folder ? item.folder.split('/').join(' › ') : 'Importwurzel') +
        episodeLabel +
        (previousCheck
          ? ' · ✓ Technisch geprüft · ' + cachedStatus : ' · Noch nicht geprüft') +
        (confirmed
          ? ' · Frühere VDR-Operation bestätigt; aktueller Quellinhalt ungeprüft'
          : '') + pcHint);
      row.appendChild(checkbox);
      row.appendChild(open);
      row.appendChild(meta);
      view.selectionRows.appendChild(row);
    });
    marked.forEach(function (item) {
      const marker = markerSuffix(item);
      const returns = pcEncodeCandidates(view, item);
      const returned = returns.length === 1 && possiblePcReturn(view, returns[0])
        ? returns[0] : null;
      const row = element('div', 'media-import-select-item media-import-marked-item');
      row.dataset.sourceId = item.sourceId;
      const open = element('button', 'media-import-episode-open', item.label || 'Markierte Datei');
      open.type = 'button';
      open.addEventListener('click', function () {
        if (!isCurrent(view) || view.batch || view.attempt) return;
        view.select.value = '';
        view.markedSelect.value = item.sourceId;
        view.releaseButton.disabled = false;
        view.releaseButton.hidden = false;
        refreshSelectedPreview(view);
        const statusText = marker === 'duplicate'
          ? 'Bereits als Duplikat markiert (.duplicate). Kein erneuter Import.'
          : marker === 'skipped' ? 'Von Rectools oder Media Tools übersprungen (.skipped).'
            : returned
              ? 'PC-Encode: möglicher Rücklauf als neue Datei gefunden.'
              : returns.length
                ? 'PC-Encode: mehrere gleichnamige Kandidaten, Zuordnung unklar.'
                : 'Wartet auf die extern umkodierte Datei (.pc_encode).';
        const hint = returned
          ? 'Neue Datei „' + returned.label +
            '“ ist ein separater Importkandidat. Sie wird beim Auswählen ' +
            'technisch geprüft. Gleicher Name ist noch kein Nachweis für ' +
            'identischen Inhalt. Die markierte Originaldatei bleibt erhalten.'
          : marker === 'pc_encode' && returns.length
            ? 'Mehrere mögliche Dateien haben denselben Titel und Ordner. ' +
              'Ohne eindeutige Zuordnung wird keine Datei automatisch gewählt, ' +
              'markiert oder gelöscht. Jede Quelle getrennt prüfen.'
            : marker === 'pc_encode'
              ? 'Nach der Umkodierung die neue Videodatei ohne .pc_encode ' +
                'im selben Importordner ablegen. Sie wird beim nächsten Scan ' +
                'normal erkannt; das markierte Original wird nicht importiert.'
            : 'Diese Rectools-Markierung wird respektiert; keine erneute ' +
              'Importausführung für die markierte Datei.';
        view.preview.textContent = statusText + ' ' + hint;
        view.previewCard.replaceChildren(
          element('h5', 'media-import-preview-title', item.label || 'Markierte Quelle'),
          element('div', 'media-import-preview-state is-marked', statusText),
          element('p', 'media-import-preview-hint', hint));
        view.deleteDuplicateButton.hidden = marker !== 'duplicate';
        view.deleteDuplicateButton.disabled = marker !== 'duplicate';
        if (marker === 'duplicate')
          view.previewCard.appendChild(view.deleteDuplicateButton);
        view.previewCard.appendChild(view.importFeedback);
        renderImportFeedback(view);
        setMobilePane(view, 'details');
      });
      const coords = episodeCoordinates(item);
      const episodeLabel = coords.episode !== null
        ? ' · S' + String(coords.season).padStart(2, '0') +
          'E' + String(coords.episode).padStart(2, '0') : '';
      const meta = element('span', 'media-import-episode-meta',
        (item.folder ? item.folder.split('/').join(' › ') : 'Importwurzel') +
        episodeLabel + ' · ' + (marker === 'duplicate' ? 'Bereits vorhanden' :
          marker === 'skipped' ? 'Übersprungen' : returned
            ? 'PC-Version gefunden – noch prüfen' : returns.length
              ? 'PC-Rücklauf mehrdeutig' : 'Wartet auf PC-Bearbeitung'));
      row.appendChild(open);
      row.appendChild(meta);
      if (returned) {
        const inspect = element('button', 'media-import-pc-return-open',
          'PC-Version technisch prüfen');
        inspect.type = 'button';
        inspect.addEventListener('click', function () {
          // Navigates to the independently authorized ordinary candidate;
          // this does not execute an import or release the parked original.
          return openDeferredFilm(view, returned.sourceId);
        });
        row.appendChild(inspect);
      }
      view.selectionRows.appendChild(row);
    });
    view.selectAllVisibleButton.disabled = normal.length === 0;
    updateSelectionCount(view);

    view.markedSelect.replaceChildren();
    const markedBlank = element('option', '', 'Markierte Datei auswählen');
    markedBlank.value = '';
    view.markedSelect.appendChild(markedBlank);
    marked.forEach(function (item) {
      const option = element('option', '',
        (item.label || 'Film') + ' [' + markerSuffix(item) + ']');
      option.value = item.sourceId;
      view.markedSelect.appendChild(option);
    });
    view.markedSelect.value = '';
    view.markedSelect.disabled = marked.length === 0;
    view.releaseButton.disabled = true;
    view.releaseButton.hidden = true;
    view.normalLabel.hidden = mode === 'marked';
    view.markedLabel.hidden = mode !== 'marked' || marked.length === 0;
    view.visibleCount.textContent = normal.length + ' unmarkierte (Eignung noch ungeprüft) und ' +
      marked.length + ' markierte Quellen in dieser Ansicht ' +
      '(aus maximal ' + MAX_IMPORT_FILMS + ' erfassten Quellen).';
  }

  function loadSources(view) {
    if (!isCurrent(view) || view.attempt) return Promise.resolve();
    // A new authorized inventory starts a new preview lifetime. Do not
    // reuse a result after a scan, import or source change.
    view.previewCache.clear();
    if (view.previewTimer !== null) global.clearTimeout(view.previewTimer);
    view.previewTimer = null;
    ++view.previewGeneration;
    resetOnline(view);
    view.previewResult = null;
    view.previewStatus = '';
    view.previewFailed = false;
    // A filter change or rescan clears selection; do not leave an inert retry visible.
    view.previewRetryButton.hidden = true;
    resetBatchReview(view);
    const generation = ++view.sourcesGeneration;
    // A re-scan must not display stale counts or selectable entries while
    // the current backend snapshot is being fetched.
    view.scanNotice.textContent = 'Scan läuft: Quellen werden erneut geladen …';
    view.visibleCount.textContent = 'Scan läuft: Noch keine aktuellen Treffer.';
    view.preview.textContent = 'Scan läuft: Bitte die aktuelle Filmliste abwarten.';
    view.previewCard.replaceChildren(element('p', 'media-import-preview-hint',
      'Importquellen werden gescannt …'));
    view.select.replaceChildren();
    view.markedSelect.replaceChildren();
    view.sourceDetails.textContent = '';
    view.select.disabled = true;
    view.startButton.disabled = true;
    view.markedSelect.disabled = true;
    view.releaseButton.disabled = true;
    view.releaseButton.hidden = true;
    view.batchButton.disabled = true;
    view.selectedBatchButton.disabled = true;
    view.selectAllVisibleButton.disabled = true;
    view.searchInput.disabled = true;
    view.statusFilter.disabled = true;
    view.seasonFilter.disabled = true;
    view.sources = [];
    view.markedSources = [];
    view.checkedSourceIds.clear();
    view.selectionRows.replaceChildren();
    view.selectedBatchButton.disabled = true;
    view.selectAllVisibleButton.disabled = true;
    view.allOrdinary = [];
    view.allMarked = [];
    view.folderPath = '';
    view.folderBreadcrumb.replaceChildren();
    view.folderList.replaceChildren();
    view.seriesGroups.replaceChildren();
    view.seasonFilter.replaceChildren();
    view.seriesCaption.textContent = 'Scan läuft';
    view.folderBreadcrumb.hidden = true;
    view.folderList.hidden = true;
    report(view, 'Autorisierte Importquellen werden geladen …');
    return readSourcePages(view.backendId)
      .then(function (response) {
        if (!isCurrent(view) || generation !== view.sourcesGeneration) return;
        const items = response && response.items;
        if (!Array.isArray(items) || items.length > MAX_IMPORT_FILMS ||
            !items.every(function (item) {
              return item && typeof item.sourceId === 'string' &&
                /^[A-Za-z0-9_-]{16,64}$/.test(item.sourceId) &&
                (item.folder === undefined || validRelativeFolder(item.folder)) &&
                (item.label === undefined ||
                  (typeof item.label === 'string' &&
                   item.label.length > 0 && item.label.length <= 160 &&
                   !/[\/\\\r\n\x00-\x1f\x7f]/.test(item.label)));
            })) throw new Error('Ungültige Quellenliste');
        // The Agent reports whether the bounded local walk reached its end.
        // Missing/legacy completion metadata is conservatively incomplete.
        const bounded = !(response.meta && response.meta.partial === false);
        const ordinary = items.filter(function (item) {
          return !markerSuffix(item);
        });
        const marked = items.filter(function (item) {
          return Boolean(markerSuffix(item));
        });
        // Only the verified Agent snapshot can supply selectable opaque IDs.
        // Browser search/status filtering is a view, never a new authority.
        view.allOrdinary = ordinary.map(function (item, index) {
          return {sourceId: item.sourceId,
            label: item.label || ('Film ' + (index + 1)),
            folder: item.folder || '',
            folderKnown: Object.prototype.hasOwnProperty.call(item, 'folder')};
        }).sort(compareSources);
        view.allMarked = marked.slice().sort(compareSources);
        view.sourceTitles = Object.create(null);
        view.allOrdinary.forEach(function (item) {
          view.sourceTitles[item.sourceId] = item.label;
        });
        filterVisibleSources(view);
        view.searchInput.disabled = false;
        view.statusFilter.disabled = false;
        view.seasonFilter.disabled = false;
        view.scanNotice.textContent = bounded && items.length >= MAX_IMPORT_FILMS
          ? 'Importgrenze: Höchstens 100 Filme pro Durchlauf. ' +
            '100 geprüfte Quellen werden angezeigt. Der Scan ist nicht ' +
            'vollständig. Da Originale erhalten bleiben, für weitere ' +
            'Filme die freigegebene Quellauswahl vor einem neuen Scan anpassen.'
          : bounded
            ? 'Scan unvollständig: Der sichere Suchumfang wurde begrenzt. ' +
              'Weitere Dateien oder Unterordner können fehlen. Maximal ' +
              '100 Filme pro Importdurchlauf.'
          : 'Scan abgeschlossen (konfigurierte Quellen): alle geeigneten ' +
            'Quellen wurden geprüft. Das bedeutet nicht, dass jeder andere ' +
            'Dateipfad dieser Importwurzel freigegeben ist.';
        report(view, ordinary.length + ' unmarkierte Quellen (Importfähigkeit noch ungeprüft), ' + marked.length +
          ' bereits markiert.' +
          (bounded ? ' Scan unvollständig.' : ''));
      }).catch(function (error) {
        if (!isCurrent(view) || generation !== view.sourcesGeneration) return;
        const exceeded = error && typeof error.message === 'string' &&
          error.message.includes('Mehr als 100 Importquellen');
        const message = exceeded
          ? 'Es sind höchstens 100 Filme pro Importdurchlauf erlaubt. ' +
            'Bitte die Quelle in kleineren Durchläufen importieren.'
          : error && error.status === 503
            ? 'Quellenliste abgelaufen oder Backend-Agent nicht erreichbar. Bitte aktualisieren.'
          : error && (error.status === 401 || error.status === 403 || error.status === 404)
            ? 'Für dieses Backend sind keine Importquellen zugänglich.'
            : 'Quellenliste konnte nicht geladen werden. Bitte aktualisieren.';
        view.scanNotice.textContent = exceeded
          ? 'Importgrenze überschritten: Maximal 100 Filme pro Durchlauf. ' +
            'Keine Auswahl freigegeben; bitte erneut scannen.'
          : 'Scan nicht abgeschlossen. Keine aktuellen ' +
            'Ergebnisse; bitte nur nach erfolgreicher Aktualisierung auswählen.';
        view.preview.textContent = 'Keine aktuelle Auswahl; noch kein Import vorbereitet.';
        report(view, message, true);
      });
  }

  // Exactly one selected source, using the canonical Public-v1 actor/backend
  // gate. The control plane can answer "pending" until the next Agent scan.
  // This is read-only; it never creates an import operation or writes a marker.
  function refreshSelectedPreview(view, force) {
    if (!isCurrent(view)) return;
    if (view.previewTimer !== null) {
      global.clearTimeout(view.previewTimer);
      view.previewTimer = null;
    }
    const id = view.select.value;
    if (force) view.previewCache.delete(id);
    // A new observation never inherits an older film's encoding consent.
    view.decision.value = '';
    view.localReencode.checked = false;
    ++view.previewGeneration;
    resetOnline(view);
    const source = view.allOrdinary.find(function (item) {
      return item.sourceId === id;
    });
    // MEDIA4 already recognizes episodes. Reuse those coordinates for the
    // MEDIA5 provider kind, rather than defaulting every source to a movie.
    view.onlineKind = source && episodeCoordinates(source).episode !== null
      ? 'series' : 'movie';
    const generation = view.previewGeneration;
    view.previewResult = null;
    view.previewStatus = '';
    view.previewFailed = false;
    view.previewReads = 0;
    updatePreview(view);
    if (!view.sources.includes(id)) return;
    // A successful technical read is reusable only within this authorized
    // source scan. The Agent still independently rechecks on a real import.
    const cached = view.previewCache.get(id);
    if (cached) {
      view.previewResult = cached;
      view.previewStatus = 'Technische Prüfung aus diesem Scan wiederverwendet.';
      updatePreview(view);
    }
    if (source && view.onlineKind === 'series') autoIdentifyEpisode(view, source);
    if (cached) return;
    function poll() {
      if (!isCurrent(view) || generation !== view.previewGeneration ||
          view.select.value !== id) return;
      // Reuse the canonical Public-v1 client. No second ad-hoc fetch path.
      Promise.resolve(publicClient().getMediaImportSourcePreview({
        backendId: view.backendId, sourceId: id,
        credentials: 'same-origin', cache: 'no-store'
      })).then(function (data) {
        if (!isCurrent(view) || generation !== view.previewGeneration ||
            view.select.value !== id) return;
        if (!data || data.backendId !== view.backendId ||
            data.sourceId !== id) throw new Error('Veraltete Vorschauantwort');
        if (data.state === 'pending') {
          view.previewStatus = 'Film wird geprüft …';
          updatePreview(view);
          if (++view.previewReads < 40) {
            view.previewTimer = global.setTimeout(function () {
              view.previewTimer = null;
              poll();
            }, 4000);
          } else {
            view.previewStatus = 'Zeitüberschreitung: Für die ausgewählte Quelle kam keine fertige Backend-Vorschau.';
            view.previewFailed = true;
            updatePreview(view);
          }
          return;
        }
        if (!validMediaImportPreview(data, view.backendId, id))
          throw new Error('Ungültige Vorschauantwort');
        view.previewResult = data;
        view.previewStatus = '';
        view.previewFailed = data.status === 'unavailable';
        // Do not cache pending, failed or duplicate observations. A
        // duplicate requires its existing independently confirmed marker
        // operation and may change the source inventory immediately.
        if (data.status === 'ready' || data.status === 'insufficient')
          view.previewCache.set(id, data);
        updatePreview(view);
        if (view.duplicateReviewPending && data.status !== 'duplicate') {
          // The target changed during reinspection; do not mark another file.
          view.duplicateQueue = [];
          view.duplicateReviewPending = false;
          if (view.batchReview) renderBatchReview(view);
        }
        if (data.status === 'duplicate' && !view.attempt && !view.batch &&
            !view.submitting && !automaticDuplicates[view.backendId + '|' + id]) {
          automaticDuplicates[view.backendId + '|' + id] = true;
          // The selected Agent preview is verified before this automatic
          // decision; the same Core/Agent operation rechecks source and target.
          return startImport(view, false, 'duplicate', true);
        }
      }).catch(function (error) {
        if (!isCurrent(view) || generation !== view.previewGeneration ||
            view.select.value !== id) return;
        view.previewResult = null;
        view.previewFailed = true;
        view.previewStatus = error && error.status === 401
          ? 'HTTP 401: Anmeldung abgelaufen. Bitte erneut anmelden.'
          : error && error.status === 403
            ? 'HTTP 403: Keine Berechtigung für diese Importvorschau.'
            : error && error.status === 404
              ? 'HTTP 404: Quelle oder Vorschau-Route nicht gefunden. Erneut scannen.'
              : error && error.status === 503
                ? 'HTTP 503: Backend-Agent oder Vorschau derzeit nicht erreichbar.'
                : error && [408, 504].includes(error.status)
                  ? 'Zeitüberschreitung der Vorschau (HTTP ' + error.status + ').'
                  : error && Number.isInteger(error.status) && error.status > 0
                    ? 'Vorschau-Anfrage fehlgeschlagen (HTTP ' + error.status + ').'
                    : 'Vorschau fehlgeschlagen: Keine gültige Antwort für die gewählte Quelle.';
        updatePreview(view);
      });
    }
    poll();
  }

  function humanBytes(n) {
    if (n < 1024 * 1024) return Math.round(n / 1024) + ' KiB';
    if (n < 1024 * 1024 * 1024) return (n / (1024 * 1024)).toFixed(1) + ' MiB';
    return (n / (1024 * 1024 * 1024)).toFixed(1) + ' GiB';
  }

  function renderPreviewCard(view, plan, source) {
    if (!view.previewCard) return;
    const card = view.previewCard;
    card.replaceChildren();
    const selected = view.sources.includes(view.select.value);
    const label = source && source.label ||
      (selected ? view.sourceTitles[view.select.value] : '');
    const title = plan && plan.title || label || 'Noch keine Folge ausgewählt';
    const heading = element('h5', 'media-import-preview-title', title);
    card.appendChild(heading);
    const artwork = element('div', 'media-import-artwork');
    const localPoster = plan && validLocalArtwork(plan.posterDataUri)
      ? plan.posterDataUri : '';
    // Use the recognized series cover as the main provider image, never
    // the episode still. Retain source-bound local artwork where available.
    const seriesPoster = view.onlineSeries
      ? onlineImageUrl(view.backendId, view.onlineSeries) : '';
    const selectedPoster = view.onlineSelected
      ? onlineImageUrl(view.backendId, view.onlineSelected) : '';
    const episodeStill = view.onlineSelected && view.onlineSelected.kind === 'episode'
      ? selectedPoster : '';
    const poster = localPoster || seriesPoster || selectedPoster;
    const fanart = plan && validLocalArtwork(plan.fanartDataUri)
      ? plan.fanartDataUri : '';
    const showEpisodeStill = Boolean(episodeStill && episodeStill !== poster);
    if (showEpisodeStill) artwork.className += ' has-episode';
    const picture = function (kind, uri, alt) {
      const frame = element('div', 'media-import-artwork-frame media-import-artwork-' + kind);
      const label = kind === 'poster'
        ? (seriesPoster || selectedPoster) && !localPoster
          ? 'Online-Poster nicht verfügbar' : 'Kein lokales Poster'
        : kind === 'episode' ? 'Kein Episodenbild' : 'Keine lokale Fanart';
      if (uri) {
        const img = element('img', 'media-import-artwork-image');
        img.src = uri;
        img.alt = alt;
        img.loading = 'lazy';
        img.decoding = 'async';
        img.addEventListener('error', function () {
          frame.replaceChildren(element('span', 'media-import-artwork-empty', label));
        });
        frame.appendChild(img);
      } else {
        frame.appendChild(element('span', 'media-import-artwork-empty', label));
      }
      return frame;
    };
    artwork.appendChild(picture('poster', poster,
      seriesPoster && poster === seriesPoster
        ? 'Seriencover für ' + view.onlineSeries.title : 'Poster für ' + title));
    if (showEpisodeStill)
      artwork.appendChild(picture('episode', episodeStill,
        'Episodenbild für ' + title));
    artwork.appendChild(picture('fanart', fanart, 'Lokale Fanart für ' + title));
    card.appendChild(artwork);
    const state = !selected ? 'idle' :
      !plan ? (view.previewFailed ? 'failed' : 'pending') : plan.status;
    const stateText = state === 'ready'
      ? plan.videoReencodeRequired ? 'Video-Umkodierung erforderlich' : 'Direktimport möglich'
      : state === 'duplicate' ? 'VDR-Aufnahme bereits vorhanden'
      : state === 'insufficient' ? 'Nicht genügend Speicherplatz'
      : state === 'unavailable' ? 'Quelle konnte nicht geprüft werden'
      : state === 'failed' ? 'Vorschau fehlgeschlagen'
      : state === 'pending' ? 'Technische Prüfung läuft'
      : 'Folge aus der Liste auswählen';
    const actions = element('div', 'media-import-preview-actions');
    actions.appendChild(element('div', 'media-import-preview-state is-' + state,
      stateText));
    // Move the *existing* import button; never create a second click handler.
    view.startButton.hidden = !selected || !plan ||
      !['ready', 'insufficient'].includes(plan.status);
    actions.appendChild(view.startButton);
    card.appendChild(actions);
    card.appendChild(view.importFeedback);
    renderImportFeedback(view);
    if (selected && source && source.folderKnown) {
      card.appendChild(element('p', 'media-import-preview-path',
        'Quelle: ' + (source.folder ? source.folder.split('/').join(' › ') : 'Importwurzel')));
    }
    if (plan && plan.status !== 'unavailable') {
      const facts = element('dl', 'media-import-preview-facts');
      const addFact = function (label, value) {
        const fact = element('div', 'media-import-preview-fact');
        fact.appendChild(element('dt', '', label));
        fact.appendChild(element('dd', '', value));
        facts.appendChild(fact);
      };
      if (plan.videoCodec) addFact('Video-Codec', plan.videoCodec.toUpperCase());
      if (plan.sourceBytes) addFact('Dateigröße', humanBytes(plan.sourceBytes));
      if (plan.targetFolder) addFact('VDR-Ziel', plan.targetFolder);
      if (plan.availableBytes) addFact('Freier Platz', humanBytes(plan.availableBytes));
      if (plan.requiredBytes) addFact('Zwischenbedarf (nur Info)', humanBytes(plan.requiredBytes));
      if (plan.durationSeconds) {
        const seconds = plan.durationSeconds;
        const minutes = Math.floor(seconds / 60);
        const hours = Math.floor(minutes / 60);
        addFact('Laufzeit (FFprobe)', hours
          ? hours + ' Std. ' + (minutes % 60) + ' Min.'
          : minutes + ' Min.');
      }
      card.appendChild(facts);
    }
    if (plan && ['ready', 'duplicate', 'insufficient'].includes(plan.status)) {
      const hasNfo = ['nfoOutline', 'nfoPlot', 'nfoYear', 'nfoGenre'].some(
        function (key) { return Boolean(plan[key]); });
      if (hasNfo) {
        const section = element('section', 'media-import-local-metadata');
        section.appendChild(element('h6', '', 'Lokale Metadaten (NFO)'));
        if (plan.nfoOutline)
          section.appendChild(element('p', 'media-import-nfo-outline', plan.nfoOutline));
        if (plan.nfoPlot)
          section.appendChild(element('p', 'media-import-nfo-plot', plan.nfoPlot));
        const facts = element('dl', 'media-import-preview-facts');
        const add = function (label, value) {
          const item = element('div', 'media-import-preview-fact');
          item.appendChild(element('dt', '', label));
          item.appendChild(element('dd', '', value));
          facts.appendChild(item);
        };
        if (plan.nfoYear) add('Jahr', plan.nfoYear);
        if (plan.nfoGenre) add('Genre', plan.nfoGenre);
        if (facts.children.length) section.appendChild(facts);
        card.appendChild(section);
      }
    }
    renderOnline(view, card, plan, source);
    const diagnostics = {
      agent_unavailable: 'Der Backend-Agent oder Media Tools ist nicht erreichbar.',
      maintenance_busy: 'Media Tools ist gerade beschäftigt. Vorschau erneut prüfen.',
      inventory_missing: 'Die Quelle ist im aktuellen Backend-Quelleninventar nicht mehr auffindbar.',
      source_invalid: 'Quelldatei oder Quellpfad konnte nicht geprüft werden.',
      source_changed: 'Die Quelldatei wurde seit dem Scan verändert.',
      target_invalid: 'Der VDR-Zielordner konnte nicht sicher ermittelt werden.',
      target_collision: 'Der VDR-Zielordner ist belegt, aber keine vollständige passende Aufnahme nachgewiesen.',
      ffprobe_failed: 'FFprobe konnte den Videocodec nicht bestimmen (möglicherweise Zeitüberschreitung).',
      capacity_failed: 'Die Berechnung des freien Speicherplatzes ist fehlgeschlagen.',
      target_not_writable: 'Der VDR-Zielordner ist nicht beschreibbar.',
      inspection_failed: 'Die Backend-Vorschau ist ohne nähere Fehlerdiagnose fehlgeschlagen.'
    };
    const failure = plan && plan.status === 'unavailable' &&
      diagnostics[plan.failureCode] || '';
    const note = !selected ? 'Ordner wählen und eine Episode anklicken.' :
      !plan ? (view.previewStatus || 'Quelle wird vom Backend-Agent geprüft …') :
      plan.status === 'duplicate'
        ? 'Die bestätigte Dublette wird automatisch über den bestehenden Core als .duplicate markiert.'
      : plan.status === 'ready' && !plan.videoReencodeRequired
        ? 'Ohne Video-Neukodierung übernehmen. Jetzt direkt importieren.'
      : plan.videoReencodeRequired
        ? 'Auf dem VDR umkodieren oder am PC mit HandBrake bzw. anderer Software bearbeiten. Der Browser kodiert nicht selbst.'
      : plan.status === 'insufficient'
        ? 'Für den lokalen Import fehlt Speicherplatz.'
        : failure || 'Die Quelle oder das Ziel konnte nicht vollständig geprüft werden. Technische Details anzeigen oder erneut prüfen.';
    card.appendChild(element('p', 'media-import-preview-hint', note));
    if (selected && confirmedImport(view, view.select.value)) {
      const history = confirmedImport(view, view.select.value);
      card.appendChild(element('p', 'media-import-confirmed-readback',
        'Frühere VDR-Operation ' + history.operationId +
        ' wurde in dieser Browsersitzung durch Core-Recording-Readback ' +
        'bestätigt. Das ist KEIN Nachweis für den aktuell unter dieser ' +
        'Quellen-ID liegenden Dateiinhalt. Die neue Vorschau prüft ihn ' +
        'unabhängig. Ein bestätigtes Agent-Bereinigungsergebnis bezieht ' +
        'sich auf die damalige Datei; ein neuer Scan zeigt den aktuellen Dateibestand.'));
    }
    const parked = selected ? parkedPcEncodeMatches(view, source) : [];
    if (parked.length) {
      const unique = Boolean(possiblePcReturn(view, source));
      card.appendChild(element('p', 'media-import-pc-return-hint',
        unique
          ? 'Möglicher PC-Encode-Rücklauf erkannt (gleicher Titel und Ordner). ' +
            'Die zurückgestellte .pc_encode-Datei bleibt getrennt. ' +
            (plan && plan.status === 'ready'
              ? 'Codec und Importfähigkeit der neuen Quelle sind geprüft; ' +
                'Medienidentität ist damit nicht nachgewiesen.'
              : 'Erst die technische Vorschau entscheidet über die Importfähigkeit.')
          : 'PC-Encode-Rücklauf mehrdeutig: mehrere Quellen teilen Titel und Ordner. ' +
            'Keine automatische Zuordnung oder Bereinigung.'));
    }
  }

  function updatePreview(view) {
    if (!isCurrent(view)) return;
    const id = view.select.value;
    const selected = view.sources.includes(id);
    const item = view.allOrdinary.find(function (source) {
      return source.sourceId === id;
    });
    const plan = selected && view.previewResult &&
      view.previewResult.sourceId === id ? view.previewResult : null;
    // Recheck is also available after success; switching back is instant.
    view.previewRetryButton.hidden = !selected || (!plan && !view.previewFailed);
    view.previewRetryButton.disabled = Boolean(view.attempt) || Boolean(view.batch) || view.submitting;
    const ready = Boolean(plan && plan.status === 'ready');
    // A space shortage blocks local import, not a PC-only marker decision.
    const needsEncode = Boolean(plan && plan.videoReencodeRequired &&
      (plan.status === 'ready' || plan.status === 'insufficient'));
    // Rectools-style: compatible media has one ordinary import action.
    // Encoding choices appear only when a checked video needs conversion.
    if (ready && !needsEncode) view.decision.value = 'import';
    if (!needsEncode) view.localReencode.checked = false;
    view.choiceLabel.hidden = !needsEncode;
    view.decision.disabled = !needsEncode || view.submitting ||
      Boolean(view.attempt) || Boolean(view.batch);
    // Skip is a separate Rectools action, not an encoding technique.
    view.skipButton.hidden = !plan || !['ready', 'insufficient'].includes(plan.status);
    view.skipButton.disabled = view.submitting || Boolean(view.attempt) ||
      Boolean(view.batch);
    Array.from(view.selectionRows.children).forEach(function (row) {
      const rowId = row.dataset.sourceId;
      const rowSelected = selected && rowId === id;
      const marked = row.className.includes('media-import-marked-item');
      row.className = marked ? 'media-import-select-item media-import-marked-item'
        : 'media-import-select-item' + (rowSelected ? ' is-selected' : '');
      if (!marked && row.children[2]) {
        const rowItem = view.allOrdinary.find(function (source) {
          return source.sourceId === rowId;
        });
        const observed = rowSelected ? plan : view.previewCache.get(rowId);
        const status = !observed
          ? rowSelected ? 'Technische Prüfung läuft' : 'Noch nicht geprüft'
          : observed.status === 'ready' ? (observed.videoReencodeRequired
            ? 'Umkodierung nötig' : 'Direktimport möglich')
          : observed.status === 'duplicate' ? 'VDR-Aufnahme vorhanden'
          : observed.status === 'insufficient' ? 'Speicherplatz fehlt'
          : 'Prüfung nicht möglich';
        const folder = rowItem && rowItem.folder
          ? rowItem.folder.split('/').join(' › ') : 'Importwurzel';
        const coords = rowItem ? episodeCoordinates(rowItem) : {season: null, episode: null};
        const episodeLabel = coords.episode !== null
          ? ' · S' + String(coords.season).padStart(2, '0') +
            'E' + String(coords.episode).padStart(2, '0') : '';
        // A cached technical result is not a confirmed VDR import.
        const parked = parkedPcEncodeMatches(view, rowItem);
        const pcHint = possiblePcReturn(view, rowItem)
          ? ' · Möglicher PC-Encode-Rücklauf'
          : parked.length ? ' · PC-Encode-Zuordnung mehrdeutig' : '';
        const confirmed = confirmedImport(view, rowId);
        row.children[2].textContent = folder + episodeLabel +
          (view.previewCache.has(rowId) ? ' · ✓ Technisch geprüft' : '') +
          ' · ' + status +
          (confirmed
            ? ' · Frühere VDR-Operation bestätigt; aktueller Quellinhalt ungeprüft'
            : '') + pcHint;
      }
    });
    if (!selected) {
      view.preview.textContent = 'Schritt 2: Film oder Serienfolge auswählen. Die Vorschau startet automatisch.';
      renderPreviewCard(view, null, item);
      view.startButton.disabled = true;
      return;
    }
    const name = plan && plan.title ? plan.title :
      (view.sourceTitles[id] || 'Film');
    const sourceFolder = item && item.folderKnown
      ? (item.folder ? item.folder.split('/').join(' › ') : 'Importwurzel')
      : 'wird vom Backend beim Import geprüft';
    const decision = view.decision.value;
    const description = !plan
      ? 'Diagnose ausstehend. Es wurde noch kein Importweg freigegeben.'
      : plan.status === 'unavailable'
        ? 'Der Agent konnte Quelle, Codec oder Ziel nicht vollständig prüfen; ein genauerer Grund wurde nicht geliefert. Kein Import möglich.'
        : !ready && !needsEncode
          ? 'Import derzeit blockiert. Die Ursache steht in der Diagnose.'
        : needsEncode
          ? decision === 'desktop'
        ? 'Externe Bearbeitung: Die Originaldatei über ihren bisherigen Datei-/Netzwerkzugang am PC mit HandBrake oder einem anderen Programm umkodieren. Das Webfrontend konvertiert und überträgt nicht selbst. Die .pc_encode-Markierung kennzeichnet die zurückgestellte Quelle. Die bearbeitete Datei anschließend in der Importquelle bereitstellen und neu prüfen. Kein Import im aktuellen Schritt.'
        : decision === 'import' && view.localReencode.checked
            ? 'Lokale H.264-Umkodierung nur für diesen Film nach ausdrücklicher Bestätigung.'
            : 'Video-Neukodierung erforderlich. Bitte Bearbeitungsweg auswählen.'
        : 'Video wird ohne Neukodierung in das VDR-Aufnahmeformat übernommen; ' +
        'Audio wird beim Import bei Bedarf angepasst.';
    let details = view.previewStatus || 'Film wird geprüft …';
    if (plan) {
      if (plan.status === 'unavailable') {
        details = 'Die ausgewählte Quelle konnte vom Backend nicht sicher geprüft werden (unavailable).';
      } else {
        const outcome = plan.status === 'duplicate'
          ? 'Bereits als VDR-Aufnahme vorhanden – nicht überschreiben.'
          : plan.status === 'insufficient'
            ? 'Nicht genügend freier Platz für den Import.'
            : plan.videoReencodeRequired
              ? 'Video muss für eine VDR-Aufnahme umkodiert werden.'
              : 'Video technisch für den VDR-Import geeignet – verlustfreies Remuxing möglich.';
        details = 'VDR-Zielordner: ' + plan.targetFolder +
          '. Video-Codec: ' + plan.videoCodec +
          '. Quelldatei: ' + humanBytes(plan.sourceBytes) +
          '. Frei: ' + humanBytes(plan.availableBytes) +
          '. Geschätzter Zwischenbedarf (nur Info): ' + humanBytes(plan.requiredBytes) +
          '. ' + outcome;
      }
    }
    const nextStep = !plan
      ? 'Bitte das Ergebnis der automatischen Prüfung abwarten.'
      : ready && !needsEncode
        ? 'Nächster Schritt: „Diesen Film importieren“ anklicken und bestätigen.'
        : plan.status === 'duplicate'
          ? 'Bestehende VDR-Aufnahme bestätigt. Die Quelle wird automatisch über den Core als .duplicate markiert.'
        : needsEncode
          ? 'Nächster Schritt: Oben einen Bearbeitungsweg wählen. Lokale Umkodierung braucht eine gesonderte Bestätigung.'
          : 'Dieser Film ist derzeit nicht importierbar. Einen anderen Film wählen.';
    view.preview.textContent = 'Vorschau: ' + name +
      '. Quellordner: ' + sourceFolder + '. ' + description +
      ' ' + details + ' ' + nextStep +
      ' Der Backend-Agent prüft Quelle und Ziel bei Auftragsannahme erneut.';
    renderPreviewCard(view, plan, item);
    view.startButton.textContent = needsEncode
      ? decision === 'desktop' ? 'Für PC markieren'
        : decision === 'import' ? 'Auf VDR umkodieren'
            : 'Bearbeitungsweg auswählen'
      : ready ? 'Diesen Film importieren' : 'Import nicht möglich';
    if (!view.submitting && !view.attempt && !view.batch) {
      view.startButton.disabled = needsEncode
        ? !(decision === 'desktop' || decision === 'skip' ||
          (ready && decision === 'import' && view.localReencode.checked))
        : !ready;
    }
  }

  function startImport(view, releaseMode, forcedDecision, automaticDuplicate) {
    if (!isCurrent(view) || view.submitting || view.attempt ||
        (view.inspectingBatch && !view.batch)) return Promise.resolve();
    const deleteDuplicate = forcedDecision === 'delete_duplicate';
    const sourceId = releaseMode || deleteDuplicate
      ? view.markedSelect.value :
      view.batch ? view.batch.remaining[0] : view.select.value;
    // Encoding authority is granted per film, per click, never to a batch.
    const selected = releaseMode ? 'release' :
      view.batch ? 'import' : (forcedDecision || view.decision.value);
    if (!['import', 'desktop', 'skip', 'duplicate', 'release',
          'delete_duplicate'].includes(selected) ||
        ((releaseMode || deleteDuplicate) && view.batch) ||
        (deleteDuplicate && !view.allMarked.some(function (item) {
          return item.sourceId === sourceId && markerSuffix(item) === 'duplicate';
        }))) return Promise.resolve();
    const previewPlan = view.previewResult &&
      view.previewResult.sourceId === sourceId ? view.previewResult : null;
    if (!releaseMode && !deleteDuplicate && !view.batch) {
      // Pending/failed preview never admits an import. A marker needs the
      // currently inspected source too; only reencode-required video can
      // be delegated to an external encoder.
      if (!previewPlan || previewPlan.status === 'unavailable' ||
          (selected === 'import' && (previewPlan.status !== 'ready' ||
            (previewPlan.videoReencodeRequired && !view.localReencode.checked))) ||
          (selected === 'desktop' &&
            !['ready', 'insufficient'].includes(previewPlan.status)) ||
          (selected === 'desktop' && !previewPlan.videoReencodeRequired) ||
          (selected === 'duplicate' && previewPlan.status !== 'duplicate'))
        return Promise.resolve();
    }
    const approveVideoReencode = selected === 'import' &&
      !view.batch && previewPlan && previewPlan.status === 'ready' &&
      previewPlan.videoReencodeRequired && view.localReencode.checked === true;
    const videoDecision = selected === 'import'
      ? (approveVideoReencode ? 'local' : '') : selected;
    // Import alone is NEVER local FFmpeg encoding consent.
    const confirmation = approveVideoReencode
      ? 'Media Tools: Diesen Film bei Bedarf lokal mit FFmpeg in H.264 umkodieren? ' +
        'Die Originaldatei bleibt unverändert. Diese Zustimmung gilt nur für diesen Film.'
      : selected === 'delete_duplicate'
        ? 'Die markierte doppelte Quelldatei endgültig löschen? ' +
          'Die VDR-Aufnahme bleibt erhalten. Quelle und Aufnahme werden ' +
          'vor dem Löschen erneut geprüft. Das Löschen ist endgültig.'
      : selected === 'desktop'
        ? 'Diesen Film für PC/HandBrake mit .pc_encode markieren? ' +
          'Es wird keine VDR-Aufnahme importiert.'
        : selected === 'skip'
          ? 'Diesen Film mit .skipped markieren und vom normalen Import ausschließen?'
          : selected === 'duplicate'
            ? (automaticDuplicate ? '' : 'Die vorhandene VDR-Aufnahme bleibt erhalten. Diese Quelldatei als .duplicate markieren?')
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
    if (!((releaseMode || deleteDuplicate) ?
          view.markedSources : view.sources).includes(sourceId)) {
      if (view.batch) stopBatch(view, 'Quellauswahl nicht mehr verfügbar.');
      return Promise.resolve();
    }
    view.submitting = true;
    view.startButton.disabled = true;
    renderImportFeedback(view);
    view.localReencode.disabled = true;
    view.decision.disabled = true;
    view.skipButton.disabled = true;
    view.markedSelect.disabled = true;
    view.releaseButton.disabled = true;
    report(view, 'Quelle und Berechtigung werden erneut geprüft …');
    // Source observations are ephemeral: revalidate before choosing the opaque token.
    let preAdmissionFailed = false;
    return checkCapability(view.backendId)
      .then(function (available) {
        if (!isCurrent(view)) return null;
        if (!available) throw {status: 403};
        return readSourcePages(view.backendId);
      }).then(function (sources) {
        if (!isCurrent(view) || !sources) return null;
        if (!sources.items || !sources.items.some(function (item) {
          return item && item.sourceId === sourceId &&
            (deleteDuplicate ? markerSuffix(item) === 'duplicate'
              : releaseMode ? Boolean(markerSuffix(item))
              : !markerSuffix(item));
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
          const selectedSource = view.allOrdinary.find(function (item) {
            return item.sourceId === sourceId;
          });
          view.attempt = {sourceId: sourceId, key: key, operationId: '',
            videoDecision: videoDecision,
            pcReturn: !releaseMode && Boolean(possiblePcReturn(view, selectedSource)),
            state: '', pending: true};
          activeAttempts[view.backendId] = view.attempt;
          renderImportFeedback(view);
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
            if (attempt.videoDecision === 'duplicate') {
              delete automaticDuplicates[view.backendId + '|' + attempt.sourceId];
              view.duplicateQueue = [];
              view.duplicateReviewPending = false;
              if (view.batchReview) renderBatchReview(view);
            }
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
              renderImportFeedback(target);
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
        // No request was admitted: allow a fresh verified preview to retry
        // automatic marking instead of permanently suppressing this source.
        if (automaticDuplicate && !view.attempt)
          delete automaticDuplicates[view.backendId + '|' + sourceId];
        const message = error && error.status === 503
          ? 'Quellenliste veraltet oder Backend-Agent nicht verfügbar. Erneut laden.'
          : error && (error.status === 401 || error.status === 403)
            ? 'Keine gültige Anmeldung oder Media-Import-Berechtigung.'
            : 'Import konnte vor dem Absenden nicht vorbereitet werden.';
        if (view.batch) stopBatch(view, message);
        view.duplicateQueue = [];
        view.duplicateReviewPending = false;
        if (view.batchReview) renderBatchReview(view);
        // No operation was admitted. The old preview cannot authorize another
        // click after the permission/source check failed; offer a fresh read.
        preAdmissionFailed = true;
        view.previewCache.delete(sourceId);
        view.previewResult = null;
        view.previewFailed = true;
        view.previewStatus = message;
        report(view, message, true);
      }).finally(function () {
        view.submitting = false;
        if (isCurrent(view)) renderImportFeedback(view);
        if (isCurrent(view) && !view.attempt && !view.batch) {
          // Only a pre-admission failure permits read-only retry. A definite
          // HTTP rejection still requires an explicit sources refresh.
          if (preAdmissionFailed) updatePreview(view);
          else {
            view.localReencode.disabled = view.decision.value !== 'import';
            view.decision.disabled = false;
          }
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
    root.dataset.mobilePane = 'media';
    const heading = element('h3', '', 'Media Import');
    const guidance = element('p', '', 'Importquellen durchsuchen, Folgen prüfen und passende Filme ' +
      'gezielt importieren. Erkannten Dubletten wird automatisch die Rectools-Markierung zugewiesen.');
    root.appendChild(heading);
    root.appendChild(guidance);
    const reloadButton = element('button', 'media-import-reload', 'Verzeichnis scannen');
    reloadButton.type = 'button';
    root.appendChild(reloadButton);
    const scanNotice = element('p', 'media-import-scan-notice',
      'Noch nicht geprüft, ob die Quellenliste vollständig ist.');
    scanNotice.setAttribute('role', 'status');
    root.appendChild(scanNotice);
    const folderBreadcrumb = element('nav', 'media-import-breadcrumb');
    folderBreadcrumb.setAttribute('aria-label', 'Importordner');
    const folderList = element('div', 'media-import-folders');
    root.appendChild(folderBreadcrumb);
    root.appendChild(folderList);
    const searchLabel = element('label', 'media-import-search-label',
      'In gefundenen Filmen suchen');
    const searchInput = element('input', 'media-import-search');
    searchInput.type = 'search';
    searchInput.maxLength = 128;
    searchInput.placeholder = 'Filmtitel oder Quellenname';
    searchLabel.appendChild(searchInput);
    root.appendChild(searchLabel);
    const statusLabel = element('label', 'media-import-filter-label', 'Statusfilter');
    const statusFilter = element('select', 'media-import-filter');
    [
      ['all', 'Alle Quellen'],
      ['normal', 'Unmarkiert (noch ungeprüft)'],
      ['marked', 'Markierte Dateien']
    ].forEach(function (entry) {
      const option = element('option', '', entry[1]);
      option.value = entry[0];
      statusFilter.appendChild(option);
    });
    statusFilter.value = 'all';
    statusLabel.appendChild(statusFilter);
    root.appendChild(statusLabel);
    const visibleCount = element('p', 'media-import-visible-count');
    root.appendChild(visibleCount);
    const sourceDetails = element('p', 'media-import-source-details');
    const technical = element('details', 'media-import-technical-details');
    technical.appendChild(element('summary', '', 'Technische Details'));
    technical.appendChild(sourceDetails);
    root.appendChild(element('h4', '', 'Einzelimport'));
    const label = element('label', '', '1. Film oder Serienfolge auswählen');
    const select = element('select', 'media-import-source');
    select.disabled = true;
    label.appendChild(select);
    root.appendChild(label);
    const preview = element('p', 'media-import-preview',
      'Nach der Filmauswahl wird die Diagnose automatisch erstellt.');
    preview.setAttribute('role', 'status');
    root.appendChild(preview);
    const previewCard = element('div', 'media-import-preview-card');
    previewCard.setAttribute('role', 'status');
    const importFeedback = element('div', 'media-import-import-feedback');
    importFeedback.hidden = true;
    importFeedback.setAttribute('role', 'status');
    importFeedback.setAttribute('aria-live', 'polite');
    const importFeedbackTitle = element('strong', 'media-import-import-title');
    const importFeedbackText = element('p', 'media-import-import-description');
    importFeedback.appendChild(importFeedbackTitle);
    importFeedback.appendChild(importFeedbackText);
    const previewRetryButton = element('button', 'media-import-preview-retry', 'Vorschau erneut prüfen');
    previewRetryButton.type = 'button';
    previewRetryButton.hidden = true;
    root.appendChild(previewRetryButton);
    const choiceLabel = element('label', 'media-import-decision-label', 'Video muss umkodiert werden – wie möchtest du vorgehen?');
    choiceLabel.hidden = true;
    const decision = element('select', 'media-import-decision');
    decision.disabled = true;
    [
      ['', 'Bitte Bearbeitungsweg auswählen'],
      ['import', 'Auf dem VDR umkodieren (H.264)'],
      ['desktop', 'Auf PC umkodieren (z. B. HandBrake)']
    ].forEach(function (choice) {
      const option = element('option', '', choice[1]);
      option.value = choice[0];
      decision.appendChild(option);
    });
    decision.value = 'import';
    choiceLabel.appendChild(decision);
    root.appendChild(choiceLabel);
    const encodeLabel = element('label', 'media-import-encode-label',
      'Lokale Umkodierung ausschließlich nach gesonderter Zustimmung');
    encodeLabel.hidden = true;
    const localReencode = element('input', 'media-import-local-reencode');
    localReencode.type = 'checkbox';
    localReencode.checked = false;
    localReencode.disabled = true;
    encodeLabel.appendChild(localReencode);
    root.appendChild(encodeLabel);
    const deleteDuplicateButton = element('button', 'media-import-delete-duplicate',
      'Dublette aus Importquelle löschen');
    deleteDuplicateButton.type = 'button';
    deleteDuplicateButton.hidden = true;
    deleteDuplicateButton.disabled = true;
    const startButton = element('button', 'media-import-start', 'Import starten');
    startButton.type = 'button';
    startButton.disabled = true;
    root.appendChild(startButton);
    const skipButton = element('button', 'media-import-skip', 'Film überspringen');
    skipButton.type = 'button';
    skipButton.hidden = true;
    skipButton.disabled = true;
    root.appendChild(skipButton);
    const markedLabel = element('label', 'media-import-marked-label',
      'Nur zur Fehlerkorrektur: falsch markierte Datei auswählen');
    const markedSelect = element('select', 'media-import-marked-source');
    markedSelect.disabled = true;
    markedLabel.appendChild(markedSelect);
    root.appendChild(markedLabel);
    const releaseButton = element('button', 'media-import-release',
      'Ausnahme: Markierung zurücksetzen');
    releaseButton.type = 'button';
    releaseButton.disabled = true;
    releaseButton.hidden = true;
    root.appendChild(releaseButton);
    const refreshButton = element('button', 'media-import-refresh', 'Vorgang prüfen');
    refreshButton.type = 'button';
    refreshButton.hidden = true;
    root.appendChild(element('h4', '', 'Mehrfachauswahl'));
    const selectAllVisibleButton = element('button', 'media-import-select-all',
      'Alle sichtbaren Filme auswählen');
    selectAllVisibleButton.type = 'button';
    selectAllVisibleButton.disabled = true;
    root.appendChild(selectAllVisibleButton);
    const selectionRows = element('div', 'media-import-selection');
    root.appendChild(selectionRows);
    const selectionCount = element('p', 'media-import-selection-count');
    root.appendChild(selectionCount);
    const selectedBatchButton = element('button', 'media-import-batch-selected',
      'Ausgewählte Filme prüfen');
    selectedBatchButton.type = 'button';
    selectedBatchButton.disabled = true;
    root.appendChild(selectedBatchButton);
    const batchButton = element('button', 'media-import-batch',
      'Alle sichtbaren Filme prüfen');
    batchButton.type = 'button';
    batchButton.disabled = true;
    root.appendChild(batchButton);
    const batchSummary = element('p', 'media-import-batch-summary');
    batchSummary.setAttribute('role', 'status');
    root.appendChild(batchSummary);
    const batchReviewList = element('div', 'media-import-batch-review');
    root.appendChild(batchReviewList);
    const batchConfirmButton = element('button', 'media-import-batch-confirm',
      'Geeignete Filme jetzt importieren');
    batchConfirmButton.type = 'button';
    batchConfirmButton.hidden = true;
    batchConfirmButton.disabled = true;
    root.appendChild(batchConfirmButton);
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
    // Present the existing actions in a full-width media-library layout.
    // DOM ownership, handlers and Core/Agent operations remain unchanged.
    const titleBlock = element('div', 'media-import-title-block');
    titleBlock.appendChild(heading);
    titleBlock.appendChild(guidance);
    const header = element('header', 'media-import-header');
    header.appendChild(titleBlock);
    header.appendChild(reloadButton);
    const toolbar = element('div', 'media-import-toolbar');
    toolbar.appendChild(searchLabel);
    toolbar.appendChild(statusLabel);
    const statusStrip = element('div', 'media-import-status-strip');
    statusStrip.appendChild(visibleCount);
    statusStrip.appendChild(scanNotice);
    const library = element('section', 'media-import-pane media-import-library');
    library.appendChild(element('h4', '', 'Serien und Ordner'));
    library.appendChild(folderBreadcrumb);
    library.appendChild(folderList);
    library.appendChild(element('h5', 'media-import-groups-title',
      'Serien und Ordnergruppen'));
    const seriesGroups = element('div', 'media-import-series-groups');
    library.appendChild(seriesGroups);
    const episodes = element('section', 'media-import-pane media-import-episodes');
    episodes.appendChild(element('h4', '', 'Filme und Episoden'));
    const seriesCaption = element('p', 'media-import-series-caption');
    episodes.appendChild(seriesCaption);
    const seasonLabel = element('label', 'media-import-season-label', 'Staffel auswählen');
    const seasonFilter = element('select', 'media-import-season-filter');
    seasonLabel.appendChild(seasonFilter);
    seasonLabel.hidden = true;
    episodes.appendChild(seasonLabel);
    label.className = 'media-import-select-label';
    episodes.appendChild(label);
    episodes.appendChild(selectionRows);
    const detail = element('section', 'media-import-pane media-import-detail');
    const mobileBack = element('button', 'media-import-mobile-back',
      '← Zur Medienliste');
    mobileBack.type = 'button';
    mobileBack.addEventListener('click', function () {
      setMobilePane(view, 'media');
    });
    detail.appendChild(mobileBack);
    detail.appendChild(element('h4', '', 'Vorschau und Aktionen'));
    detail.appendChild(previewCard);
    detail.appendChild(previewRetryButton);
    detail.appendChild(choiceLabel);
    detail.appendChild(encodeLabel);
    detail.appendChild(skipButton);
    // Rectools markers are normal workflow state, not a request to release.
    // Keep the existing explicit recovery path exclusively under collapsed
    // technical details, away from ordinary import/PC handoff actions.
    technical.appendChild(markedLabel);
    technical.appendChild(releaseButton);
    technical.appendChild(preview);
    technical.appendChild(operationDetail);
    detail.appendChild(technical);
    const mobileNav = element('nav', 'media-import-mobile-nav');
    mobileNav.setAttribute('aria-label', 'Media-Import-Ansichten');
    const mobileButtons = Object.create(null);
    [['folders', 'Ordner'], ['media', 'Medien'], ['details', 'Details']]
      .forEach(function (entry) {
        const button = element('button', 'media-import-mobile-switch', entry[1]);
        button.type = 'button';
        button.setAttribute('aria-pressed', 'false');
        button.addEventListener('click', function () {
          setMobilePane(view, entry[0]);
        });
        mobileButtons[entry[0]] = button;
        mobileNav.appendChild(button);
      });
    const workspace = element('div', 'media-import-workspace');
    workspace.appendChild(library);
    workspace.appendChild(episodes);
    workspace.appendChild(detail);
    const footer = element('section', 'media-import-footer');
    footer.appendChild(selectionCount);
    footer.appendChild(selectAllVisibleButton);
    footer.appendChild(selectedBatchButton);
    footer.appendChild(batchButton);
    footer.appendChild(batchSummary);
    footer.appendChild(batchReviewList);
    footer.appendChild(batchConfirmButton);
    footer.appendChild(batchStopButton);
    footer.appendChild(message);
    importFeedback.appendChild(refreshButton);
    importFeedback.appendChild(newButton);
    root.replaceChildren(header, toolbar, statusStrip, mobileNav, workspace, footer);
    mountTarget.replaceChildren(root);
    const view = {owner: owner, backendId: backendId, root: root,
      select: select, startButton: startButton,
      reloadButton: reloadButton,
      searchInput: searchInput, statusFilter: statusFilter,
      folderBreadcrumb: folderBreadcrumb, folderList: folderList,
      seriesGroups: seriesGroups, seriesCaption: seriesCaption,
      seasonLabel: seasonLabel, seasonFilter: seasonFilter, seasonKey: '',
      mobileButtons: mobileButtons, folderPath: '',
      visibleCount: visibleCount, normalLabel: label, markedLabel: markedLabel,
      allOrdinary: [], allMarked: [],
      localReencode: localReencode, decision: decision,
      choiceLabel: choiceLabel, skipButton: skipButton,
      deleteDuplicateButton: deleteDuplicateButton,
      markedSelect: markedSelect, releaseButton: releaseButton, markedSources: [],
      batchButton: batchButton, batchStopButton: batchStopButton,
      batchSummary: batchSummary, batchReviewList: batchReviewList,
      batchConfirmButton: batchConfirmButton, batchReview: null,
      reviewGeneration: 0, inspectingBatch: false,
      duplicateQueue: [], duplicateReviewPending: false,
      resumeDuplicateReview: false,
      selectAllVisibleButton: selectAllVisibleButton,
      selectionRows: selectionRows, selectionCount: selectionCount,
      selectedBatchButton: selectedBatchButton, checkedSourceIds: new Set(),
      batch: activeBatches[backendId] || null,
      refreshButton: refreshButton, newButton: newButton,
      message: message, operationDetail: operationDetail,
      importFeedback: importFeedback, importFeedbackTitle: importFeedbackTitle,
      importFeedbackText: importFeedbackText,
      scanNotice: scanNotice, sourceDetails: sourceDetails,
      preview: preview, previewCard: previewCard,
      previewRetryButton: previewRetryButton,
      sourceTitles: Object.create(null),
      previewTimer: null, previewGeneration: 0, previewReads: 0,
      previewResult: null, previewStatus: '', previewFailed: false,
      previewCache: new Map(),
      onlineSeq: 0, onlineItems: [], onlineSelected: null,
      onlineSeries: null, onlineSeason: null, onlineMode: 'search',
      onlineKind: 'movie', onlineQuery: '', onlineStatus: '', onlinePending: false,
      onlineAuto: false, onlineAutoSelected: false,
      sources: [], sourcesGeneration: 0, submitting: false, reading: false,
      reads: 0, timer: null, attempt: activeAttempts[backendId] || null};
    current = view;
    setMobilePane(view, 'media');
    searchInput.addEventListener('input', function () {
      view.checkedSourceIds.clear();
      filterVisibleSources(view);
      setMobilePane(view, 'media');
    });
    statusFilter.addEventListener('change', function () {
      view.checkedSourceIds.clear();
      filterVisibleSources(view);
      setMobilePane(view, 'media');
    });
    seasonFilter.addEventListener('change', function () {
      if (!isCurrent(view) || view.batch || view.attempt) return;
      view.seasonKey = seasonFilter.value;
      view.checkedSourceIds.clear();
      filterVisibleSources(view);
      setMobilePane(view, 'media');
    });
    selectAllVisibleButton.addEventListener('click', function () {
      if (!isCurrent(view) || view.batch || view.attempt) return;
      view.sources.forEach(function (id) { view.checkedSourceIds.add(id); });
      filterVisibleSources(view);
    });
    selectedBatchButton.addEventListener('click', function () {
      return startBatch(view, true);
    });
    previewRetryButton.addEventListener('click', function () {
      if (!isCurrent(view) || view.attempt || view.batch || view.submitting ||
          !view.sources.includes(view.select.value)) return;
      refreshSelectedPreview(view, true);
    });
    select.addEventListener('change', function () {
      markedSelect.value = '';
      releaseButton.disabled = true;
      releaseButton.hidden = true;
      startButton.disabled = view.submitting || !view.sources.includes(select.value);
      sourceDetails.textContent = view.sources.includes(select.value)
        ? 'Auswahl-ID: ' + select.value : '';
      refreshSelectedPreview(view);
      setMobilePane(view, view.sources.includes(select.value) ? 'details' : 'media');
    });
    decision.addEventListener('change', function () {
      if (!isCurrent(view) || view.batch || view.attempt ||
          !view.previewResult ||
          !['ready', 'insufficient'].includes(view.previewResult.status) ||
          !view.previewResult.videoReencodeRequired) return;
      // The visible VDR choice is per-film permission for local encoding.
      // The existing 'local' Core request still needs its own confirmation.
      localReencode.checked = decision.value === 'import';
      batchButton.disabled = true;
      updateSelectionCount(view);
      updatePreview(view);
    });
    startButton.addEventListener('click', function () { return startImport(view); });
    skipButton.addEventListener('click', function () { return startImport(view, false, 'skip'); });
    deleteDuplicateButton.addEventListener('click', function () {
      return startImport(view, false, 'delete_duplicate');
    });
    markedSelect.addEventListener('change', function () {
      releaseButton.disabled = view.submitting || Boolean(view.attempt) ||
        !view.markedSources.includes(markedSelect.value);
      releaseButton.hidden = !view.markedSources.includes(markedSelect.value);
    });
    releaseButton.addEventListener('click', function () {
      return startImport(view, true);
    });
    batchButton.addEventListener('click', function () { return startBatch(view); });
    batchConfirmButton.addEventListener('click', function () { return confirmBatch(view); });
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
      view.localReencode.disabled = true;
      view.decision.value = 'import';
      view.decision.disabled = false;
      view.startButton.textContent = 'Import starten';
      view.newButton.hidden = true;
      view.refreshButton.hidden = true;
      view.reloadButton.disabled = false;
      view.operationDetail.textContent = '';
      renderImportFeedback(view);
      return loadSources(view);
    });
    // Even an already-visible tab must recheck the live Core capability.
    return checkCapability(backendId).then(function (available) {
      if (!isCurrent(view)) return;
      if (!available) {
        report(view, 'Media Import zurzeit nicht verfügbar. Aktivierung, Benutzerrechte und Backend-Agent werden erneut geprüft.', true);
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
