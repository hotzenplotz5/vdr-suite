// Native editing controls attached to the existing Recording detail/playback owner.
(function (global) {
  'use strict';
  function node(tag, label) {
    const value = global.document.createElement(tag);
    if (label !== undefined) value.textContent = label;
    return value;
  }
  function message(error) {
    const code = String(error && error.message || error || '');
    if (/revision_conflict|stale/.test(code)) return 'Schnittmarken wurden inzwischen geändert. Neu laden und erneut prüfen.';
    if (/in_use/.test(code)) return 'Die Aufnahme wird gerade verwendet. Bearbeiten ist derzeit gesperrt.';
    if (/permission|forbidden|read.only|denied|Authentication|CSRF/.test(code)) return 'Keine Schreibberechtigung oder Anmeldung abgelaufen. Bitte Anmeldung und Backend-Zugriff prüfen.';
    if (/destination|result.*exist/.test(code)) return 'Eine geschnittene Ausgabe existiert bereits.';
    if (/capability|unavailable/.test(code)) return 'Die native Bearbeitung ist derzeit nicht verfügbar.';
    return 'Ergebnis noch nicht bestätigt. Den bestehenden Auftrag prüfen, bevor eine neue Änderung gestartet wird.';
  }

  function attach(root, panel, recording, backendId, initial, options) {
    if (root.__vdrSuiteMarksEditor) return root.__vdrSuiteMarksEditor;
    panel.editor = true;
    options.renderPayload(panel, initial);
    const identity = {backendId: String(backendId), recordingId: String(recording.recordingId || recording.id)};
    const controls = node('div');
    controls.className = 'recordings2-marks-editor';
    const status = node('p', '');
    status.setAttribute('role', 'status');
    status.setAttribute('aria-live', 'polite');
    const actions = node('div');
    actions.className = 'recordings2-marks-editor-actions';
    const confirmation = node('div');
    confirmation.setAttribute('role', 'group');
    confirmation.setAttribute('aria-label', 'Schnitt bestätigen');
    panel.section.appendChild(controls);
    controls.appendChild(actions);
    controls.appendChild(confirmation);
    controls.appendChild(status);
    let payload = initial;
    let busy = false;
    let pending = null;
    let destroyed = false;
    let unsubscribe = null;
    let owner = null;
    let lifecycleKey = '';

    function request(path, body) {
      const api = global.VdrSuiteClientApi;
      if (!api || typeof api.requestJson !== 'function') return Promise.reject(new Error('client_unavailable'));
      const config = {cache: 'no-store', credentials: 'same-origin'};
      if (body) {
        const session = global.VdrSuiteBrowserSession;
        config.method = 'POST';
        config.headers = Object.assign({'Content-Type': 'application/json'},
          session && typeof session.csrfHeaders === 'function' ? session.csrfHeaders() : {});
        config.body = JSON.stringify(body);
      } else config.query = {backend: identity.backendId, recordingId: identity.recordingId};
      return api.requestJson(path, config);
    }
    function button(label, action, disabled, parent) {
      const value = node('button', label);
      value.type = 'button';
      value.disabled = Boolean(disabled);
      value.addEventListener('click', function () {
        if (!value.disabled && !destroyed) action();
      });
      (parent || actions).appendChild(value);
      return value;
    }
    function frame(value) {
      const number = Number(value);
      return String(value).trim() && Number.isSafeInteger(number) && number >= 0 && number <= 2147483647 ? number : null;
    }
    function editable() {
      return !busy && !pending && payload && payload.availability === 'available' && payload.inUse === false;
    }
    function playback() { return root.__vdrSuiteRecordingPlaybackOwner; }
    function apply(next) {
      if (!next || next.availability !== 'available' || next.backendId !== identity.backendId ||
          String(next.recordingId) !== identity.recordingId || !Array.isArray(next.marks)) throw new Error('native_readback_invalid');
      payload = next;
      options.renderPayload(panel, payload);
      panel.section.dataset.marksRevision = payload.marksRevision;
      const timeline = global.VdrSuiteRecordings2MarksTimeline;
      if (timeline) timeline.bind(root, recording, payload);
    }
    function reload() {
      if (busy) return;
      busy = true;
      confirmation.replaceChildren();
      render();
      request('/api/vdr/recordings/marks').then(function (next) {
        if (destroyed) return;
        apply(next);
        status.textContent = pending ? 'Aktueller VDR-Stand geladen. Der bestehende Auftrag ist noch zu prüfen.' : 'Aktueller VDR-Stand geladen.';
      }).catch(function (error) { if (!destroyed) status.textContent = message(error); })
        .finally(function () { busy = false; if (!destroyed) render(); });
    }
    function token() {
      if (!global.crypto || typeof global.crypto.getRandomValues !== 'function') throw new Error('operation_identity_unavailable');
      return 'edit-' + Array.from(global.crypto.getRandomValues(new Uint32Array(4)))
        .map(function (part) { return part.toString(16).padStart(8, '0'); }).join('');
    }
    function submit(path, fields) {
      if (!editable()) return;
      try {
        pending = {path: path, body: Object.assign({}, identity, {
          operationId: token(), operationRevision: '1', expectedMarksRevision: payload.marksRevision
        }, fields || {})};
      } catch (error) { status.textContent = message(error); return; }
      check();
    }
    function check() {
      if (!pending || busy) return;
      const operation = pending;
      busy = true;
      confirmation.replaceChildren();
      render();
      // Replay uses the identical immutable operation. No automatic POST retry,
      // new operation ID, or success inferred from a changed native revision.
      request(operation.path, operation.body).then(function (result) {
        if (destroyed) return null;
        if (!result || result.operationId !== operation.body.operationId || result.accepted !== true) throw new Error('operation_response_invalid');
        if (result.verification !== 'verified') {
          status.textContent = 'Auftrag angenommen; native Bestätigung steht aus. „Auftrag prüfen“ verwendet denselben Auftrag.';
          return null;
        }
        return request('/api/vdr/recordings/marks').then(function (next) {
          if (destroyed) return;
          apply(next);
          pending = null;
          status.textContent = operation.path.endsWith('/cut')
            ? 'Native geschnittene Ausgabe bestätigt. Das Original bleibt erhalten.'
            : (next.marksRevision === result.canonicalMarksRevision
              ? 'Schnittmarken von VDR bestätigt.'
              : 'Auftrag bestätigt; danach wurden die nativen Marken erneut geändert. Aktueller Stand angezeigt.');
        });
      }).catch(function (error) {
        if (destroyed) return;
        if (/revision_conflict|recording_in_use|permission|forbidden|read.only|denied|Authentication|CSRF/.test(String(error.message))) pending = null;
        status.textContent = message(error);
      }).finally(function () { busy = false; if (!destroyed) render(); });
    }
    function confirm(label, action) {
      confirmation.replaceChildren(node('p', label));
      button('Bestätigen', function () { confirmation.replaceChildren(); action(); }, !editable(), confirmation);
      button('Abbrechen', function () { confirmation.replaceChildren(); }, false, confirmation);
    }
    function previewCut() {
      if (!editable()) return;
      busy = true;
      confirmation.replaceChildren();
      render();
      request('/api/vdr/recordings/cut').then(function (preview) {
        if (destroyed) return;
        busy = false;
        if (!preview || preview.backendId !== identity.backendId || String(preview.recordingId) !== identity.recordingId ||
            preview.marksRevision !== payload.marksRevision) throw new Error('recording_marks_revision_conflict');
        if (!preview.ready) {
          status.textContent = preview.editedDestinationExists ? 'Eine geschnittene Ausgabe existiert bereits.'
            : preview.inUse ? 'Die Aufnahme wird gerade verwendet.'
            : 'VDR kann diese Markensequenz derzeit nicht schneiden.';
          return;
        }
        confirm('„' + String(recording.title || 'Aufnahme') + '“ mit ' + preview.sequenceCount +
          ' Schnittbereichen nativ schneiden? VDR erstellt eine neue Ausgabe und erhält das Original.',
          function () { submit('/api/vdr/recordings/cut'); });
      }).catch(function (error) { if (!destroyed) status.textContent = message(error); })
        .finally(function () { busy = false; if (!destroyed) render(); });
    }
    function render() {
      actions.replaceChildren();
      const blocked = !editable();
      button('Neu laden', reload, busy);
      button('Auftrag prüfen', check, busy || !pending);
      const current = playback();
      const snapshot = current && typeof current.snapshot === 'function' ? current.snapshot() : null;
      button('Marke an Wiedergabeposition', function () {
        const seconds = Number(current.position());
        const fps = Number(payload.framesPerSecond);
        const target = frame(Math.round(seconds * fps));
        if (!Number.isFinite(seconds) || seconds < 0 || !(fps > 0) || target === null) {
          status.textContent = 'Keine gültige native Wiedergabeposition verfügbar.'; return;
        }
        submit('/api/vdr/recordings/marks', {kind: 'add', targetFrame: target});
      }, blocked || !current || typeof current.position !== 'function' || !snapshot || !snapshot.sessionId);
      button('Alle Marken entfernen', function () {
        confirm('Alle nativen Schnittmarken dieser Aufnahme entfernen?', function () {
          submit('/api/vdr/recordings/marks', {kind: 'reset'});
        });
      }, blocked || !payload.marks.length);
      button('Schneiden …', previewCut, blocked || !payload.marks.length);
      const list = node('div');
      list.className = 'recordings2-marks-editor-list';
      actions.appendChild(list);
      payload.marks.forEach(function (mark, index) {
        const row = node('div');
        row.className = 'recordings2-marks-editor-row';
        list.appendChild(row);
        row.appendChild(node('span', mark.timecode + (index % 2 === 0 ? ' · Behalten ab hier' : ' · Entfernen ab hier')));
        button('Zur Marke ' + mark.timecode, function () {
          const active = playback();
          Promise.resolve().then(function () { return active.seekAbsolute(mark.positionSeconds); })
            .catch(function () { if (!destroyed) status.textContent = 'Springen ist im aktuellen Wiedergabestand nicht verfügbar.'; });
        }, !current || typeof current.seekAbsolute !== 'function' || !snapshot || !snapshot.sessionId, row);
        const input = node('input');
        input.type = 'number'; input.min = '0'; input.max = '2147483647'; input.step = '1';
        input.value = String(mark.positionFrame);
        input.setAttribute('aria-label', 'Zielframe für Marke ' + mark.timecode);
        input.disabled = blocked;
        row.appendChild(input);
        button('Marke ' + mark.timecode + ' verschieben', function () {
          const target = frame(input.value);
          if (target === null) { status.textContent = 'Bitte eine gültige ganze Frameposition eingeben.'; return; }
          submit('/api/vdr/recordings/marks', {kind: 'move', sourceFrame: mark.positionFrame, targetFrame: target});
        }, blocked, row);
        button('Marke ' + mark.timecode + ' löschen', function () {
          submit('/api/vdr/recordings/marks', {kind: 'delete', sourceFrame: mark.positionFrame});
        }, blocked, row);
      });
    }
    function observe() {
      const current = playback();
      if (current === owner) return;
      if (unsubscribe) unsubscribe();
      owner = current;
      unsubscribe = current && typeof current.subscribe === 'function' ? current.subscribe(function (snapshot) {
        if (snapshot && snapshot.transition === 'destroyed') { destroy(); return; }
        const key = JSON.stringify([snapshot && snapshot.sessionId, snapshot && snapshot.state, snapshot && snapshot.transition]);
        if (!destroyed && key !== lifecycleKey) { lifecycleKey = key; render(); }
      }) : null;
    }
    function destroy() {
      destroyed = true;
      if (unsubscribe) unsubscribe();
      unsubscribe = null;
      confirmation.replaceChildren();
    }
    const result = Object.freeze({reload: reload, destroy: destroy, observe: observe});
    root.__vdrSuiteMarksEditor = result;
    observe();
    render();
    return result;
  }
  global.VdrSuiteRecordings2MarksEditor = Object.freeze({attach: attach});
}(window));
