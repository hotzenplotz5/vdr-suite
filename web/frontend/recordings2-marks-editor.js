// Native editing controls attached to the existing Recording detail/playback owner.
(function (global) {
  'use strict';
  const VERIFY_ATTEMPTS = 16, VERIFY_DELAY_MS = 350;
  const STYLE_ID = 'vdr-suite-recordings2-marks-editor-style';
  function installStyles() {
    if (!global.document || !global.document.head || global.document.getElementById(STYLE_ID)) return;
    const style = node('style'); style.id = STYLE_ID; style.textContent = `
.recordings2-marks-editor{display:grid;gap:.65rem;margin-top:.25rem}.recordings2-marks-editor-hint,.recordings2-marks-editor-selection{margin:0;color:#cbd5e1;font-size:.82rem;line-height:1.4}.recordings2-marks-editor-selection{font-weight:750;color:#f8fafc}
.recordings2-marks-editor-actions{display:grid;gap:.65rem}.recordings2-marks-editor-group{display:flex;flex-wrap:wrap;gap:.5rem;align-items:center}.recordings2-marks-editor-group.utility{padding-bottom:.15rem;border-bottom:1px solid rgba(148,163,184,.18)}
.recordings2-marks-editor button{min-height:2.7rem;padding:.5rem .8rem;border-radius:.62rem;white-space:normal}.recordings2-marks-editor button.primary{background:#2563eb;border-color:#3b82f6;color:#f8fafc}.recordings2-marks-editor button.danger{border-color:rgba(248,113,113,.65);background:rgba(153,27,27,.35);color:#fee2e2}.recordings2-marks-editor button:focus-visible{outline:2px solid #93c5fd;outline-offset:2px}
.recordings2-marks-editor-list{display:grid;gap:.5rem;width:100%;min-width:0}.recordings2-marks-editor-row{display:grid;grid-template-columns:minmax(7rem,auto) minmax(0,1fr);gap:.35rem .75rem;align-items:center;padding:.6rem .7rem;border:1px solid rgba(148,163,184,.28);border-radius:.65rem;background:rgba(15,23,42,.48)}.recordings2-marks-editor-row.selected{border-color:#60a5fa;background:rgba(30,64,175,.2)}
.recordings2-marks-editor-row>span{color:#cbd5e1;font-size:.8rem;line-height:1.35}.recordings2-marks-editor-mark{min-width:7rem;text-align:left;font-weight:800}.recordings2-marks-editor-confirmation{display:flex;flex-wrap:wrap;gap:.5rem;align-items:center}.recordings2-marks-editor-confirmation:empty{display:none}
.recordings2-marks-editor-status{min-height:1.35rem;margin:0;padding:.55rem .62rem;border:1px solid rgba(148,163,184,.2);border-radius:.62rem;background:rgba(30,41,59,.55);color:#cbd5e1;font-size:.8rem;line-height:1.4}.recordings2-marks-editor-status:empty{display:none}.recordings2-marks-editor-status.error{border-color:rgba(248,113,113,.5);color:#fecaca}.recordings2-marks-editor-status.success{border-color:rgba(34,197,94,.45);color:#bbf7d0}.recordings2-marks-editor-status.pending{border-color:rgba(56,189,248,.45);color:#bae6fd}
@media(max-width:720px){.recordings2-marks-editor-group{display:grid;grid-template-columns:1fr 1fr}.recordings2-marks-editor-group button{width:100%}.recordings2-marks-editor-row{grid-template-columns:1fr}.recordings2-marks-editor-mark{width:100%}}
`; global.document.head.appendChild(style);
  }
  function node(tag, label) { const value = global.document.createElement(tag); if (label !== undefined) value.textContent = label; return value; }
  function message(error) {
    const code = String(error && error.message || error || '');
    if (/recording_marks_modify_rejected/.test(code)) return 'VDR hat die Änderung abgelehnt. Der aktuelle native Stand wurde neu geladen.';
    if (/revision_conflict|stale/.test(code)) return 'Schnittmarken wurden inzwischen geändert. Der aktuelle VDR-Stand wurde neu geladen.';
    if (/in_use/.test(code)) return 'Die Aufnahme wird gerade verwendet. Bearbeiten ist derzeit gesperrt.';
    if (/permission|forbidden|read.only|denied|Authentication|CSRF/.test(code)) return 'Keine Schreibberechtigung oder Anmeldung abgelaufen. Bitte Anmeldung und Backend-Zugriff prüfen.';
    if (/destination|result.*exist/.test(code)) return 'Eine geschnittene Ausgabe existiert bereits.';
    if (/capability|unavailable/.test(code)) return 'Die native Bearbeitung ist derzeit nicht verfügbar.';
    if (/native_readback_invalid|operation_response_invalid/.test(code)) return 'Die Backend-Antwort konnte nicht sicher bestätigt werden. Bitte den aktuellen VDR-Stand neu laden.';
    return 'Änderung noch nicht bestätigt. Derselbe Auftrag wird weiter geprüft; bitte keine neue Änderung starten.';
  }
  function attach(root, panel, recording, backendId, initial, options) {
    if (root.__vdrSuiteMarksEditor) return root.__vdrSuiteMarksEditor;
    installStyles();
    panel.editor = true; options.renderPayload(panel, initial);
    const identity = {backendId: String(backendId), recordingId: String(recording.recordingId || recording.id)};
    const controls = node('div'), positionHint = node('p', ''), selectionHint = node('p', ''), actions = node('div'), confirmation = node('div'), status = node('p', '');
    controls.className = 'recordings2-marks-editor'; positionHint.className = 'recordings2-marks-editor-hint'; selectionHint.className = 'recordings2-marks-editor-selection';
    actions.className = 'recordings2-marks-editor-actions'; confirmation.className = 'recordings2-marks-editor-confirmation'; status.className = 'recordings2-marks-editor-status';
    confirmation.setAttribute('role', 'group'); confirmation.setAttribute('aria-label', 'Schnitt bestätigen'); status.setAttribute('role', 'status'); status.setAttribute('aria-live', 'polite');
    panel.section.appendChild(controls); controls.appendChild(positionHint); controls.appendChild(selectionHint); controls.appendChild(actions); controls.appendChild(confirmation); controls.appendChild(status);
    let payload = initial, busy = false, pending = null, selectedFrame = null, destroyed = false, unsubscribe = null, owner = null, lifecycleKey = '', verificationTimer = null, verificationAttempts = 0;
    function setStatus(type, text) { status.className = 'recordings2-marks-editor-status' + (type ? ' ' + type : ''); status.textContent = String(text || ''); }
    function request(path, body) {
      const api = global.VdrSuiteClientApi; if (!api || typeof api.requestJson !== 'function') return Promise.reject(new Error('client_unavailable'));
      const config = {cache: 'no-store', credentials: 'same-origin'};
      if (body) {
        const session = global.VdrSuiteBrowserSession; config.method = 'POST';
        config.headers = Object.assign({'Content-Type': 'application/json'}, session && typeof session.csrfHeaders === 'function' ? session.csrfHeaders() : {});
        config.body = JSON.stringify(body);
      } else config.query = {backend: identity.backendId, recordingId: identity.recordingId};
      return api.requestJson(path, config);
    }
    function button(label, action, disabled, parent, className) {
      const value = node('button', label); value.type = 'button'; value.disabled = Boolean(disabled); if (className) value.className = className;
      value.addEventListener('click', function () { if (!value.disabled && !destroyed) action(); }); (parent || actions).appendChild(value); return value;
    }
    function group(label, className) { const value = node('div'); value.className = 'recordings2-marks-editor-group ' + className; value.setAttribute('role', 'group'); value.setAttribute('aria-label', label); actions.appendChild(value); return value; }
    function frame(value) { const number = Number(value); return String(value).trim() && Number.isSafeInteger(number) && number >= 0 && number <= 2147483647 ? number : null; }
    function editable() { return !busy && !pending && payload && payload.availability === 'available' && payload.inUse === false; }
    function playback() { return root.__vdrSuiteRecordingPlaybackOwner; }
    function snapshot() { const current = playback(); return current && typeof current.snapshot === 'function' ? current.snapshot() : null; }
    function activePlayback() { const current = playback(), state = snapshot(); return Boolean(current && state && state.sessionId && typeof current.position === 'function'); }
    function currentFrame() {
      const current = playback(), seconds = current && typeof current.position === 'function' ? Number(current.position()) : NaN, fps = Number(payload && payload.framesPerSecond), target = frame(Math.round(seconds * fps));
      return Number.isFinite(seconds) && seconds >= 0 && fps > 0 ? target : null;
    }
    function marks() { return payload && Array.isArray(payload.marks) ? payload.marks : []; }
    function selectedMark() { return marks().find(function (mark) { return Number(mark.positionFrame) === selectedFrame; }) || null; }
    function nearestMark(target, values) { return target === null || !values.length ? null : values.reduce(function (best, mark) { return !best || Math.abs(Number(mark.positionFrame) - target) < Math.abs(Number(best.positionFrame) - target) ? mark : best; }, null); }
    function apply(next, operation) {
      if (!next || next.availability !== 'available' || next.backendId !== identity.backendId || String(next.recordingId) !== identity.recordingId || !Array.isArray(next.marks)) throw new Error('native_readback_invalid');
      const previous = selectedFrame; payload = next;
      if (!next.marks.some(function (mark) { return Number(mark.positionFrame) === previous; })) {
        const body = operation && operation.body || {}, target = (body.kind === 'add' || body.kind === 'move') ? frame(body.targetFrame) : null, nearest = nearestMark(target, next.marks);
        selectedFrame = nearest ? Number(nearest.positionFrame) : null;
      }
      options.renderPayload(panel, payload); panel.section.dataset.marksRevision = payload.marksRevision;
      const timeline = global.VdrSuiteRecordings2MarksTimeline; if (timeline) timeline.bind(root, recording, payload);
    }
    function syncCanonical(operation) { return request('/api/vdr/recordings/marks').then(function (next) { if (destroyed) return null; apply(next, operation); return next; }); }
    function reload() {
      if (busy) return; busy = true; confirmation.replaceChildren(); render();
      syncCanonical(null).then(function () { if (!destroyed) setStatus('', pending ? 'Aktueller VDR-Stand geladen. Die Bestätigung des bestehenden Auftrags läuft weiter.' : 'Aktueller VDR-Stand geladen.'); })
        .catch(function (error) { if (!destroyed) setStatus('error', message(error)); }).finally(function () { busy = false; if (!destroyed) render(); });
    }
    function token() {
      if (!global.crypto || typeof global.crypto.getRandomValues !== 'function') throw new Error('operation_identity_unavailable');
      return 'edit-' + Array.from(global.crypto.getRandomValues(new Uint32Array(4))).map(function (part) { return part.toString(16).padStart(8, '0'); }).join('');
    }
    function clearVerificationTimer() { if (verificationTimer !== null && typeof global.clearTimeout === 'function') global.clearTimeout(verificationTimer); verificationTimer = null; }
    function scheduleVerification() {
      if (!pending || destroyed || verificationTimer !== null) return;
      if (verificationAttempts >= VERIFY_ATTEMPTS || typeof global.setTimeout !== 'function') { setStatus('pending', 'Der aktuelle VDR-Stand wird angezeigt; die Auftragsbestätigung dauert länger. „Auftrag prüfen“ prüft denselben Auftrag weiter.'); return; }
      verificationTimer = global.setTimeout(function () { verificationTimer = null; check(true); }, VERIFY_DELAY_MS);
    }
    function definitiveFailure(error) { return /recording_marks_modify_rejected|revision_conflict|recording_in_use|permission|forbidden|read.only|denied|Authentication|CSRF/.test(String(error && error.message || error || '')); }
    function submit(path, fields) {
      if (!editable()) return;
      try { pending = {path: path, body: Object.assign({}, identity, {operationId: token(), operationRevision: '1', expectedMarksRevision: payload.marksRevision}, fields || {})}; }
      catch (error) { setStatus('error', message(error)); return; }
      verificationAttempts = 0; clearVerificationTimer(); check(false);
    }
    function check(automatic) {
      if (!pending || busy) return; const operation = pending; if (automatic) verificationAttempts += 1; else { verificationAttempts = 0; clearVerificationTimer(); }
      busy = true; confirmation.replaceChildren(); render();
      request(operation.path, operation.body).then(function (result) {
        if (destroyed || pending !== operation) return null;
        if (!result || result.operationId !== operation.body.operationId || result.accepted !== true) throw new Error('operation_response_invalid');
        if (result.verification !== 'verified') {
          return syncCanonical(operation).catch(function () { return null; }).then(function () {
            if (destroyed || pending !== operation) return; setStatus('pending', 'Änderung an VDR übergeben. Der aktuelle native Stand wird angezeigt; die Bestätigung wird automatisch geprüft.'); scheduleVerification();
          });
        }
        return syncCanonical(operation).then(function (next) {
          if (destroyed || pending !== operation || !next) return; pending = null; clearVerificationTimer(); verificationAttempts = 0;
          setStatus('success', operation.path.endsWith('/cut') ? 'Native geschnittene Ausgabe bestätigt. Das Original bleibt erhalten.' :
            (next.marksRevision === result.canonicalMarksRevision ? 'Schnittmarken von VDR bestätigt.' : 'Auftrag bestätigt; danach wurden die nativen Marken erneut geändert. Der aktuelle VDR-Stand wird angezeigt.'));
        });
      }).catch(function (error) {
        if (destroyed || pending !== operation) return;
        if (definitiveFailure(error)) {
          pending = null; clearVerificationTimer(); verificationAttempts = 0;
          return syncCanonical(null).catch(function () { return null; }).then(function () { if (!destroyed) setStatus('error', message(error)); });
        }
        setStatus('error', message(error)); scheduleVerification();
      }).finally(function () { busy = false; if (!destroyed) render(); });
    }
    function confirm(label, action) {
      confirmation.replaceChildren(node('p', label));
      button('Bestätigen', function () { confirmation.replaceChildren(); action(); }, !editable(), confirmation, 'primary');
      button('Abbrechen', function () { confirmation.replaceChildren(); }, false, confirmation);
    }
    function previewCut() {
      if (!editable()) return; busy = true; confirmation.replaceChildren(); render();
      request('/api/vdr/recordings/cut').then(function (preview) {
        if (destroyed) return; busy = false;
        if (!preview || preview.backendId !== identity.backendId || String(preview.recordingId) !== identity.recordingId || preview.marksRevision !== payload.marksRevision) throw new Error('recording_marks_revision_conflict');
        if (!preview.ready) { setStatus('error', preview.editedDestinationExists ? 'Eine geschnittene Ausgabe existiert bereits.' : preview.inUse ? 'Die Aufnahme wird gerade verwendet.' : 'VDR kann diese Markensequenz derzeit nicht schneiden.'); return; }
        confirm('„' + String(recording.title || 'Aufnahme') + '“ mit ' + preview.sequenceCount + ' Schnittbereichen nativ schneiden? VDR erstellt eine neue Ausgabe und erhält das Original.', function () { submit('/api/vdr/recordings/cut'); });
      }).catch(function (error) { if (!destroyed) setStatus('error', message(error)); }).finally(function () { busy = false; if (!destroyed) render(); });
    }
    function selectAndSeek(mark) {
      selectedFrame = Number(mark.positionFrame); render(); const active = playback(), state = snapshot();
      if (!active || typeof active.seekAbsolute !== 'function' || !state || !state.sessionId) { setStatus('', 'Schnittmarke ausgewählt. Zum Anspringen zuerst die Wiedergabe starten.'); return; }
      Promise.resolve().then(function () { return active.seekAbsolute(mark.positionSeconds); })
        .then(function () { if (!destroyed) setStatus('', 'Schnittmarke ausgewählt und angesprungen.'); })
        .catch(function () { if (!destroyed) setStatus('error', 'Springen ist im aktuellen Wiedergabestand nicht verfügbar.'); });
    }
    function navigationTarget(direction) {
      const values = marks(); if (!values.length) return null; const selected = selectedMark();
      if (selected) { const index = values.indexOf(selected) + direction; return index >= 0 && index < values.length ? values[index] : null; }
      const current = playback(), seconds = current && typeof current.position === 'function' ? Number(current.position()) : NaN; if (!Number.isFinite(seconds) || seconds < 0) return null;
      if (direction > 0) return values.find(function (mark) { return Number(mark.positionSeconds) > seconds + 0.001; }) || null;
      for (let index = values.length - 1; index >= 0; --index) if (Number(values[index].positionSeconds) < seconds - 0.001) return values[index]; return null;
    }
    function render() {
      actions.replaceChildren(); const blocked = !editable(), current = playback(), state = snapshot(), active = Boolean(current && state && state.sessionId && typeof current.position === 'function'), selected = selectedMark();
      positionHint.textContent = active ? 'Wiedergabe aktiv: Setzen und Verschieben verwenden die aktuelle Position.' : 'Zum Setzen, Verschieben oder Anspringen einer Marke zuerst die Wiedergabe starten.';
      selectionHint.textContent = selected ? 'Ausgewählt: ' + String(selected.timecode || 'Schnittmarke') + ' · Frame ' + String(selected.positionFrame) : 'Keine Schnittmarke ausgewählt.';
      const utility = group('Aktualisieren', 'utility'); button('Neu laden', reload, busy, utility); if (pending) button('Auftrag prüfen', function () { check(false); }, busy, utility);
      const navigation = group('Markennavigation', 'navigation'), previous = navigationTarget(-1), next = navigationTarget(1);
      button('Vorherige Marke', function () { selectAndSeek(previous); }, !active || !previous, navigation); button('Nächste Marke', function () { selectAndSeek(next); }, !active || !next, navigation);
      const editing = group('Schnittmarken bearbeiten', 'editing');
      button('Marke setzen', function () { const target = currentFrame(); if (target === null) { setStatus('error', 'Keine gültige Wiedergabeposition verfügbar.'); return; } submit('/api/vdr/recordings/marks', {kind: 'add', targetFrame: target}); }, blocked || !activePlayback(), editing, 'primary');
      button('Auswahl hierher verschieben', function () { const mark = selectedMark(), target = currentFrame(); if (!mark || target === null) { setStatus('error', 'Bitte zuerst eine Schnittmarke auswählen und die Wiedergabeposition festlegen.'); return; } submit('/api/vdr/recordings/marks', {kind: 'move', sourceFrame: mark.positionFrame, targetFrame: target}); }, blocked || !selected || !activePlayback(), editing);
      button('Auswahl löschen', function () { const mark = selectedMark(); if (mark) submit('/api/vdr/recordings/marks', {kind: 'delete', sourceFrame: mark.positionFrame}); }, blocked || !selected, editing, 'danger');
      const workflow = group('Weitere Schnittaktionen', 'workflow');
      button('Alle Marken entfernen', function () { confirm('Alle nativen Schnittmarken dieser Aufnahme entfernen?', function () { submit('/api/vdr/recordings/marks', {kind: 'reset'}); }); }, blocked || !marks().length, workflow, 'danger');
      button('Schneiden …', previewCut, blocked || !marks().length, workflow);
      const list = node('div'); list.className = 'recordings2-marks-editor-list'; list.setAttribute('role', 'list'); actions.appendChild(list);
      marks().forEach(function (mark, index) {
        const selectedNow = Number(mark.positionFrame) === selectedFrame, row = node('div'); row.className = 'recordings2-marks-editor-row' + (selectedNow ? ' selected' : ''); row.setAttribute('role', 'listitem'); list.appendChild(row);
        const choose = button(String(mark.timecode || ('Marke ' + String(index + 1))), function () { selectAndSeek(mark); }, false, row, 'recordings2-marks-editor-mark'); choose.setAttribute('aria-pressed', selectedNow ? 'true' : 'false');
        row.appendChild(node('span', 'Frame ' + String(mark.positionFrame) + (index % 2 === 0 ? ' · Behalten ab hier' : ' · Entfernen ab hier')));
      });
    }
    function observe() {
      const current = playback(); if (current === owner) return; if (unsubscribe) unsubscribe(); owner = current;
      unsubscribe = current && typeof current.subscribe === 'function' ? current.subscribe(function (state) {
        if (state && state.transition === 'destroyed') { destroy(); return; }
        const key = JSON.stringify([state && state.sessionId, state && state.state, state && state.transition]); if (!destroyed && key !== lifecycleKey) { lifecycleKey = key; render(); }
      }) : null;
    }
    function destroy() { destroyed = true; clearVerificationTimer(); if (unsubscribe) unsubscribe(); unsubscribe = null; confirmation.replaceChildren(); }
    const result = Object.freeze({reload: reload, destroy: destroy, observe: observe}); root.__vdrSuiteMarksEditor = result; observe(); render(); return result;
  }
  global.VdrSuiteRecordings2MarksEditor = Object.freeze({attach: attach});
}(window));
