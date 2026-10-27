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
      filterVisibleSources(view);
      if (!view.sources.includes(sourceId)) {
        report(view, 'Film in der aktuellen Ansicht nicht verfügbar.', true);
        return;
      }
      view.select.value = sourceId;
      view.sourceDetails.textContent = 'Auswahl-ID: ' + sourceId;
      refreshSelectedPreview(view);
    });
  }

  function renderBatchReview(view) {
    const review = view.batchReview;
    if (!review) return;
    view.batchReviewList.replaceChildren();
    const labels = {
      ready: 'Für VDR-Import geeignet',
      encode: 'Zurückgestellt: Video-Umkodierung nötig',
      duplicate: 'Übersprungen: VDR-Aufnahme vorhanden',
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
      return row.state === 'duplicate';
    }).length;
    const confirmed = review.rows.filter(function (row) {
      return row.state === 'succeeded';
    }).length;
    view.batchSummary.textContent = ready + ' direkt importierbar, ' +
      deferred + ' zurückgestellt, ' + duplicates + ' Dubletten, ' +
      confirmed + ' bereits bestätigt. Andere Filme bleiben unverändert. ' +
      'Zurückgestellte Filme lassen sich über „Einzeln prüfen“ neu untersuchen; ' +
      'PC/HandBrake und Überspringen erfordern weiterhin eine Einzelentscheidung.';
    view.batchConfirmButton.hidden = Boolean(view.batch) || !ready;
    view.batchConfirmButton.disabled = view.inspectingBatch ||
      Boolean(view.attempt) || !ready;
  }

  function inspectBatchPreview(view, sourceId) {
    return publicClient().getMediaImportSourcePreview({
      backendId: view.backendId, sourceId: sourceId,
      credentials: 'same-origin', cache: 'no-store'
    }).then(function (data) {
      if (!data || data.backendId !== view.backendId ||
          data.sourceId !== sourceId) throw new Error('Fremde Vorschauantwort');
      if (data.state === 'pending') return {state: 'pending'};
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
    view.batchSummary.textContent = 'Technische VDR-Importeignung wird lesend geprüft: 0 von ' +
      ids.length + '. Noch kein Import gestartet.';
    const rows = [];
    let chain = Promise.resolve();
    ids.forEach(function (sourceId) {
      chain = chain.then(function () {
        if (!isCurrent(view) || generation !== view.reviewGeneration) return;
        return inspectBatchPreview(view, sourceId).catch(function (error) {
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
      view.batchButton.disabled = view.sources.length < 2;
      updateSelectionCount(view);
    });
  }

  function confirmBatch(view) {
    if (!isCurrent(view) || !view.batchReview || view.inspectingBatch ||
        view.attempt || view.batch ||
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
    view.selectedBatchButton.disabled = true;
    view.selectAllVisibleButton.disabled = true;
    view.localReencode.disabled = true;
    view.decision.disabled = true;
    view.markedSelect.disabled = true;
    view.releaseButton.disabled = true;
    view.reloadButton.disabled = true;
    view.searchInput.disabled = true;
    view.statusFilter.disabled = true;
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
      typeof data.title === 'string' && data.title.length > 0 && data.title.length <= 120);
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
    const navigate = function (path) {
      if (!isCurrent(view) || view.batch || view.attempt || !known.has(path))
        return;
      view.folderPath = path;
      view.checkedSourceIds.clear();
      filterVisibleSources(view);
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
    view.previewResult = null;
    view.previewStatus = '';
    resetBatchReview(view);
    const query = String(view.searchInput.value || '').trim().toLowerCase();
    const mode = view.statusFilter.value;
    renderSourceFolders(view, Boolean(query));
    const matches = function (item) {
      const labelMatches = !query ||
        (item.label || item.sourceId).toLowerCase().includes(query);
      // Searching is cross-folder. The root overview includes every verified
      // source in nested folders; an explicit folder selection narrows it.
      const folderMatches = Boolean(query) || view.folderPath === '' ||
        (item.folder || '') === view.folderPath;
      return labelMatches && folderMatches;
    };
    const normal = mode === 'marked' ? [] :
      view.allOrdinary.filter(matches);
    const marked = mode === 'normal' ? [] :
      view.allMarked.filter(matches);
    view.sources = normal.map(function (item) { return item.sourceId; });
    view.markedSources = marked.map(function (item) { return item.sourceId; });

    view.select.replaceChildren();
    const blank = element('option', '', 'Quelle auswählen');
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
    view.preview.textContent = 'Film auswählen. Die technische Importprüfung erfolgt automatisch.';
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
      const label = element('label', 'media-import-select-item', item.label);
      const checkbox = element('input', '');
      checkbox.type = 'checkbox';
      checkbox.checked = view.checkedSourceIds.has(item.sourceId);
      checkbox.addEventListener('change', function () {
        if (!isCurrent(view) || view.attempt || view.batch) return;
        if (checkbox.checked) view.checkedSourceIds.add(item.sourceId);
        else view.checkedSourceIds.delete(item.sourceId);
        updateSelectionCount(view);
      });
      label.appendChild(checkbox);
      view.selectionRows.appendChild(label);
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
    view.normalLabel.hidden = mode === 'marked';
    view.markedLabel.hidden = mode === 'normal';
    view.visibleCount.textContent = normal.length + ' importierbare und ' +
      marked.length + ' markierte Treffer in dieser Ansicht ' +
      '(aus maximal ' + MAX_IMPORT_FILMS + ' geprüften Quellen).';
  }

  function loadSources(view) {
    if (!isCurrent(view) || view.attempt) return Promise.resolve();
    if (view.previewTimer !== null) global.clearTimeout(view.previewTimer);
    view.previewTimer = null;
    ++view.previewGeneration;
    view.previewResult = null;
    view.previewStatus = '';
    resetBatchReview(view);
    const generation = ++view.sourcesGeneration;
    // A re-scan must not display stale counts or selectable entries while
    // the current backend snapshot is being fetched.
    view.scanNotice.textContent = 'Scan läuft: Quellen werden erneut geladen …';
    view.visibleCount.textContent = 'Scan läuft: Noch keine aktuellen Treffer.';
    view.preview.textContent = 'Scan läuft: Bitte die aktuelle Filmliste abwarten.';
    view.select.replaceChildren();
    view.markedSelect.replaceChildren();
    view.sourceDetails.textContent = '';
    view.select.disabled = true;
    view.startButton.disabled = true;
    view.markedSelect.disabled = true;
    view.releaseButton.disabled = true;
    view.batchButton.disabled = true;
    view.selectedBatchButton.disabled = true;
    view.selectAllVisibleButton.disabled = true;
    view.searchInput.disabled = true;
    view.statusFilter.disabled = true;
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
        report(view, ordinary.length + ' Filme verfügbar, ' + marked.length +
          ' zurückgestellt, übersprungen oder doppelt markiert.' +
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
  function refreshSelectedPreview(view) {
    if (!isCurrent(view)) return;
    if (view.previewTimer !== null) {
      global.clearTimeout(view.previewTimer);
      view.previewTimer = null;
    }
    const id = view.select.value;
    // A new observation never inherits an older film's encoding consent.
    view.decision.value = '';
    view.localReencode.checked = false;
    ++view.previewGeneration;
    const generation = view.previewGeneration;
    view.previewResult = null;
    view.previewStatus = '';
    view.previewReads = 0;
    updatePreview(view);
    if (!view.sources.includes(id)) return;
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
            view.previewStatus = 'Die Vorschau dauert zu lange. Quelle erneut auswählen.';
            updatePreview(view);
          }
          return;
        }
        if (!validMediaImportPreview(data, view.backendId, id))
          throw new Error('Ungültige Vorschauantwort');
        view.previewResult = data;
        view.previewStatus = '';
        updatePreview(view);
      }).catch(function (error) {
        if (!isCurrent(view) || generation !== view.previewGeneration ||
            view.select.value !== id) return;
        view.previewResult = null;
        view.previewStatus = error && error.status === 401
          ? 'Anmeldung abgelaufen. Bitte erneut anmelden.'
          : error && error.status === 403
            ? 'Keine Berechtigung für diese Importvorschau.'
            : error && error.status === 503
              ? 'Backend-Agent oder Quelle nicht mehr verfügbar. Bitte erneut scannen.'
              : 'Film konnte nicht geprüft werden. Bitte erneut auswählen.';
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

  function updatePreview(view) {
    if (!isCurrent(view)) return;
    const id = view.select.value;
    const selected = view.sources.includes(id);
    const item = view.allOrdinary.find(function (source) {
      return source.sourceId === id;
    });
    const plan = selected && view.previewResult &&
      view.previewResult.sourceId === id ? view.previewResult : null;
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
    view.skipButton.hidden = !plan || needsEncode || plan.status === 'unavailable';
    view.skipButton.disabled = view.submitting || Boolean(view.attempt) ||
      Boolean(view.batch);
    if (!selected) {
      view.preview.textContent = 'Film auswählen. Die technische Importprüfung erfolgt automatisch.';
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
      : !ready && !needsEncode
        ? 'Import derzeit blockiert. Die Ursache steht in der Diagnose.'
        : needsEncode
          ? decision === 'desktop'
        ? 'Externe Umkodierung gewählt. Für PC/HandBrake markieren. Kein Import.'
        : decision === 'skip'
          ? 'Film überspringen und vom normalen Import ausschließen. Kein Import.'
          : decision === 'import' && view.localReencode.checked
            ? 'Lokale H.264-Umkodierung nur für diesen Film nach ausdrücklicher Bestätigung.'
            : 'Video-Neukodierung erforderlich. Bitte Bearbeitungsweg auswählen.'
        : 'Video wird ohne Neukodierung in das VDR-Aufnahmeformat übernommen; ' +
        'Audio wird beim Import bei Bedarf angepasst.';
    let details = view.previewStatus || 'Film wird geprüft …';
    if (plan) {
      if (plan.status === 'unavailable') {
        details = 'Film konnte vom Backend nicht sicher geprüft werden.';
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
          '. Benötigt einschließlich Reserve: ' + humanBytes(plan.requiredBytes) +
          '. ' + outcome;
      }
    }
    view.preview.textContent = 'Vorschau: ' + name +
      '. Quellordner: ' + sourceFolder + '. ' + description +
      ' ' + details +
      ' Der Backend-Agent prüft Quelle und Ziel bei Auftragsannahme erneut.';
    view.startButton.textContent = needsEncode
      ? decision === 'desktop' ? 'Für PC markieren'
        : decision === 'skip' ? 'Film überspringen'
          : decision === 'import' ? 'Auf VDR umkodieren'
            : 'Bearbeitungsweg auswählen'
      : ready ? 'Import starten' : 'Import nicht möglich';
    if (!view.submitting && !view.attempt && !view.batch) {
      view.startButton.disabled = needsEncode
        ? !(decision === 'desktop' || decision === 'skip' ||
          (ready && decision === 'import' && view.localReencode.checked))
        : !ready;
    }
  }

  function startImport(view, releaseMode, forcedDecision) {
    if (!isCurrent(view) || view.submitting || view.attempt ||
        (view.inspectingBatch && !view.batch)) return Promise.resolve();
    const sourceId = releaseMode ? view.markedSelect.value :
      view.batch ? view.batch.remaining[0] : view.select.value;
    // Encoding authority is granted per film, per click, never to a batch.
    const selected = releaseMode ? 'release' :
      view.batch ? 'import' : (forcedDecision || view.decision.value);
    if (!['import', 'desktop', 'skip', 'release'].includes(selected) ||
        (releaseMode && view.batch)) return Promise.resolve();
    const previewPlan = view.previewResult &&
      view.previewResult.sourceId === sourceId ? view.previewResult : null;
    if (!releaseMode && !view.batch) {
      // Pending/failed preview never admits an import. A marker needs the
      // currently inspected source too; only reencode-required video can
      // be delegated to an external encoder.
      if (!previewPlan || previewPlan.status === 'unavailable' ||
          (selected === 'import' && (previewPlan.status !== 'ready' ||
            (previewPlan.videoReencodeRequired && !view.localReencode.checked))) ||
          (selected === 'desktop' &&
            !['ready', 'insufficient'].includes(previewPlan.status)) ||
          (selected === 'desktop' && !previewPlan.videoReencodeRequired))
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
    view.skipButton.disabled = true;
    view.markedSelect.disabled = true;
    view.releaseButton.disabled = true;
    report(view, 'Quelle und Berechtigung werden erneut geprüft …');
    // Source observations are ephemeral: revalidate before choosing the opaque token.
    return checkCapability(view.backendId)
      .then(function (available) {
        if (!isCurrent(view)) return null;
        if (!available) throw {status: 403};
        return readSourcePages(view.backendId);
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
      'Ein Suchlauf liest nur die Backend-Quellenliste und startet keinen Import. ' +
      'Maximal 100 Filme pro Durchlauf.'));
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
      ['normal', 'Importierbar'],
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
    const label = element('label', '', 'Film auswählen');
    const select = element('select', 'media-import-source');
    select.disabled = true;
    label.appendChild(select);
    root.appendChild(label);
    const preview = element('p', 'media-import-preview',
      'Film auswählen, Entscheidung prüfen und bestätigen.');
    preview.setAttribute('role', 'status');
    root.appendChild(preview);
    const choiceLabel = element('label', 'media-import-decision-label', 'Video muss umkodiert werden – wie möchtest du vorgehen?');
    choiceLabel.hidden = true;
    const decision = element('select', 'media-import-decision');
    decision.disabled = true;
    [
      ['', 'Bitte Bearbeitungsweg auswählen'],
      ['import', 'Auf dem VDR umkodieren (H.264)'],
      ['desktop', 'Auf PC umkodieren (z. B. HandBrake)'],
      ['skip', 'Film überspringen']
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
    const skipButton = element('button', 'media-import-skip', 'Film überspringen');
    skipButton.type = 'button';
    skipButton.hidden = true;
    skipButton.disabled = true;
    root.appendChild(skipButton);
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
    mountTarget.replaceChildren(root);
    const view = {owner: owner, backendId: backendId, root: root,
      select: select, startButton: startButton, reloadButton: reloadButton,
      searchInput: searchInput, statusFilter: statusFilter,
      folderBreadcrumb: folderBreadcrumb, folderList: folderList,
      folderPath: '',
      visibleCount: visibleCount, normalLabel: label, markedLabel: markedLabel,
      allOrdinary: [], allMarked: [],
      localReencode: localReencode, decision: decision,
      choiceLabel: choiceLabel, skipButton: skipButton,
      markedSelect: markedSelect, releaseButton: releaseButton, markedSources: [],
      batchButton: batchButton, batchStopButton: batchStopButton,
      batchSummary: batchSummary, batchReviewList: batchReviewList,
      batchConfirmButton: batchConfirmButton, batchReview: null,
      reviewGeneration: 0, inspectingBatch: false,
      selectAllVisibleButton: selectAllVisibleButton,
      selectionRows: selectionRows, selectionCount: selectionCount,
      selectedBatchButton: selectedBatchButton, checkedSourceIds: new Set(),
      batch: activeBatches[backendId] || null,
      refreshButton: refreshButton, newButton: newButton,
      message: message, operationDetail: operationDetail,
      scanNotice: scanNotice, sourceDetails: sourceDetails,
      preview: preview, sourceTitles: Object.create(null),
      previewTimer: null, previewGeneration: 0, previewReads: 0,
      previewResult: null, previewStatus: '',
      sources: [], sourcesGeneration: 0, submitting: false, reading: false,
      reads: 0, timer: null, attempt: activeAttempts[backendId] || null};
    current = view;
    searchInput.addEventListener('input', function () {
      view.checkedSourceIds.clear();
      filterVisibleSources(view);
    });
    statusFilter.addEventListener('change', function () {
      view.checkedSourceIds.clear();
      filterVisibleSources(view);
    });
    selectAllVisibleButton.addEventListener('click', function () {
      if (!isCurrent(view) || view.batch || view.attempt) return;
      view.sources.forEach(function (id) { view.checkedSourceIds.add(id); });
      filterVisibleSources(view);
    });
    selectedBatchButton.addEventListener('click', function () {
      return startBatch(view, true);
    });
    select.addEventListener('change', function () {
      startButton.disabled = view.submitting || !view.sources.includes(select.value);
      sourceDetails.textContent = view.sources.includes(select.value)
        ? 'Auswahl-ID: ' + select.value : '';
      refreshSelectedPreview(view);
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
    markedSelect.addEventListener('change', function () {
      releaseButton.disabled = view.submitting || Boolean(view.attempt) ||
        !view.markedSources.includes(markedSelect.value);
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
