// Native editing controls attached to the existing Recording detail/playback owner.
(function (global) {
  'use strict';
  const VERIFY_ATTEMPTS = 24, VERIFY_DELAY_MS = 250;
  const STYLE_ID = 'vdr-suite-recordings2-marks-editor-style';
  function node(tag, label) { const value = global.document.createElement(tag); if (label !== undefined) value.textContent = label; return value; }
  function installStyles() {
    if (!global.document || !global.document.head || global.document.getElementById(STYLE_ID)) return;
    const style = node('style'); style.id = STYLE_ID; style.textContent = `
.recordings2-marks-editor{display:grid;gap:.35rem;margin-top:.2rem}.recordings2-marks-editor-hint,.recordings2-marks-editor-selection,.recordings2-marks-editor-status{margin:0;font-size:.8rem;line-height:1.35;color:#aebbd0}.recordings2-marks-editor-selection{color:#e5edf8;font-weight:700}
.recordings2-marks-toolbar{display:flex;flex-wrap:wrap;gap:.35rem;align-items:center;margin:.1rem 0}.recordings2-marks-editor button{min-height:2.15rem;padding:.32rem .58rem;border-radius:.45rem;font-size:.86rem;white-space:nowrap}.recordings2-marks-editor button.primary{background:#2563eb;border-color:#3b82f6;color:#f8fafc}.recordings2-marks-editor button.danger{border-color:rgba(248,113,113,.55);color:#fecaca}.recordings2-marks-editor button:focus-visible{outline:2px solid #93c5fd;outline-offset:2px}
.recordings2-marks-editor-list{display:grid;gap:.22rem;margin-top:.1rem}.recordings2-marks-editor-row{display:flex;gap:.5rem;align-items:center;min-width:0;padding:.22rem .3rem;border-radius:.4rem}.recordings2-marks-editor-row.selected{background:rgba(59,130,246,.14)}.recordings2-marks-editor-row>span{color:#94a3b8;font-size:.76rem}.recordings2-marks-editor-mark{text-align:left;font-weight:750}.recordings2-marks-editor-confirmation{display:flex;flex-wrap:wrap;gap:.4rem;align-items:center;font-size:.82rem}.recordings2-marks-editor-confirmation:empty,.recordings2-marks-editor-status:empty{display:none}.recordings2-marks-editor-status.error{color:#fecaca}.recordings2-marks-editor-status.success{color:#bbf7d0}.recordings2-marks-editor-status.pending{color:#bae6fd}
@media(max-width:720px){.recordings2-marks-toolbar{gap:.3rem}.recordings2-marks-editor button{min-height:2.35rem}.recordings2-marks-editor-row{align-items:flex-start;flex-direction:column;gap:.15rem}}
`; global.document.head.appendChild(style);
  }
  function message(error) {
    const code = String(error && error.message || error || '');
    if (/recording_marks_modify_rejected|recording_marks_modify_stale|replay_conflict|replay_ledger_full/.test(code)) return 'VDR hat die Änderung nicht übernommen. Der aktuelle Stand wurde neu geladen.';
    if (/revision_conflict|stale/.test(code)) return 'Die Schnittmarken wurden inzwischen geändert. Der aktuelle VDR-Stand wurde neu geladen.';
    if (/in_use/.test(code)) return 'Die Aufnahme wird gerade verwendet und kann momentan nicht bearbeitet werden.';
    if (/active_agent_lease_required|suitebridge_capability|backend_write|capability_unavailable/.test(code)) return 'Die native Bearbeitung ist für dieses Backend derzeit nicht verfügbar.';
    if (/permission|forbidden|read.only|denied|Authentication|CSRF/.test(code)) return 'Keine Schreibberechtigung oder Anmeldung abgelaufen.';
    if (/destination|result.*exist/.test(code)) return 'Eine geschnittene Ausgabe existiert bereits.';
    if (/native_readback_invalid|operation_response_invalid/.test(code)) return 'Der aktuelle VDR-Stand konnte nicht sicher bestätigt werden.';
    return 'Speichern wird noch bestätigt. Der aktuelle VDR-Stand wird automatisch geprüft.';
  }
  function attach(root, panel, recording, backendId, initial, options) {
    if (root.__vdrSuiteMarksEditor) return root.__vdrSuiteMarksEditor;
    installStyles(); panel.editor = true; options.renderPayload(panel, initial);
    const identity = {backendId: String(backendId), recordingId: String(recording.recordingId || recording.id)};
    const controls = node('div'), positionHint = node('p'), selectionHint = node('p'), toolbar = node('div'), list = node('div'), confirmation = node('div'), status = node('p');
    controls.className = 'recordings2-marks-editor'; positionHint.className = 'recordings2-marks-editor-hint'; selectionHint.className = 'recordings2-marks-editor-selection'; toolbar.className = 'recordings2-marks-toolbar'; list.className = 'recordings2-marks-editor-list'; confirmation.className = 'recordings2-marks-editor-confirmation'; status.className = 'recordings2-marks-editor-status';
    toolbar.setAttribute('role', 'group'); toolbar.setAttribute('aria-label', 'Schnittmarken bearbeiten'); list.setAttribute('role', 'list'); confirmation.setAttribute('role', 'group'); confirmation.setAttribute('aria-label', 'Schneiden bestätigen'); status.setAttribute('role', 'status'); status.setAttribute('aria-live', 'polite');
    panel.section.appendChild(controls); [positionHint, selectionHint, toolbar, list, confirmation, status].forEach(function (item) { controls.appendChild(item); });
    let payload = initial, busy = false, pending = null, selectedFrame = null, destroyed = false, unsubscribe = null, owner = null, lifecycleKey = '', verificationTimer = null, verificationAttempts = 0;
    function setStatus(type, text) { status.className = 'recordings2-marks-editor-status' + (type ? ' ' + type : ''); status.textContent = String(text || ''); }
    function request(path, body) {
      const api = global.VdrSuiteClientApi; if (!api || typeof api.requestJson !== 'function') return Promise.reject(new Error('client_unavailable'));
      const config = {cache: 'no-store', credentials: 'same-origin'};
      if (body) { const session = global.VdrSuiteBrowserSession; config.method = 'POST'; config.headers = Object.assign({'Content-Type': 'application/json'}, session && typeof session.csrfHeaders === 'function' ? session.csrfHeaders() : {}); config.body = JSON.stringify(body); }
      else config.query = {backend: identity.backendId, recordingId: identity.recordingId};
      return api.requestJson(path, config);
    }
    function button(label, action, disabled, parent, className) { const value = node('button', label); value.type = 'button'; value.disabled = Boolean(disabled); if (className) value.className = className; value.addEventListener('click', function () { if (!value.disabled && !destroyed) action(); }); (parent || toolbar).appendChild(value); return value; }
    function frame(value) { const number = Number(value); return String(value).trim() && Number.isSafeInteger(number) && number >= 0 && number <= 2147483647 ? number : null; }
    function editable() { return !busy && !pending && payload && payload.availability === 'available' && payload.inUse === false; }
    function playback() { return root.__vdrSuiteRecordingPlaybackOwner; }
    function snapshot() { const current = playback(); return current && typeof current.snapshot === 'function' ? current.snapshot() : null; }
    function activePlayback() { const current = playback(), state = snapshot(); return Boolean(current && state && state.sessionId && typeof current.position === 'function'); }
    function currentFrame() { const current = playback(), seconds = current && typeof current.position === 'function' ? Number(current.position()) : NaN, fps = Number(payload && payload.framesPerSecond), target = frame(Math.round(seconds * fps)); return Number.isFinite(seconds) && seconds >= 0 && fps > 0 ? target : null; }
    function marks() { return payload && Array.isArray(payload.marks) ? payload.marks : []; }
    function selectedMark() { return marks().find(function (mark) { return Number(mark.positionFrame) === selectedFrame; }) || null; }
    function nearestMark(target, values) { return target === null || !values.length ? null : values.reduce(function (best, mark) { return !best || Math.abs(Number(mark.positionFrame) - target) < Math.abs(Number(best.positionFrame) - target) ? mark : best; }, null); }
    function apply(next, operation) {
      if (!next || next.availability !== 'available' || next.backendId !== identity.backendId || String(next.recordingId) !== identity.recordingId || !Array.isArray(next.marks)) throw new Error('native_readback_invalid');
      const previous = selectedFrame; payload = next;
      if (!next.marks.some(function (mark) { return Number(mark.positionFrame) === previous; })) { const body = operation && operation.body || {}, target = (body.kind === 'add' || body.kind === 'move') ? frame(body.targetFrame) : null, nearest = nearestMark(target, next.marks); selectedFrame = nearest ? Number(nearest.positionFrame) : null; }
      options.renderPayload(panel, payload); panel.section.dataset.marksRevision = payload.marksRevision;
      const timeline = global.VdrSuiteRecordings2MarksTimeline; if (timeline) timeline.bind(root, recording, payload);
    }
    function syncCanonical(operation) { return request('/api/vdr/recordings/marks').then(function (next) { if (destroyed) return null; apply(next, operation); return next; }); }
    function reload() { if (busy) return; busy = true; render(); syncCanonical(null).then(function () { if (!destroyed && !pending) setStatus('', ''); }).catch(function (error) { if (!destroyed) setStatus('error', message(error)); }).finally(function () { busy = false; if (!destroyed) render(); }); }
    function token() { if (!global.crypto || typeof global.crypto.getRandomValues !== 'function') throw new Error('operation_identity_unavailable'); return 'edit-' + Array.from(global.crypto.getRandomValues(new Uint32Array(4))).map(function (part) { return part.toString(16).padStart(8, '0'); }).join(''); }
    function clearVerificationTimer() { if (verificationTimer !== null && typeof global.clearTimeout === 'function') global.clearTimeout(verificationTimer); verificationTimer = null; }
    function scheduleVerification() { if (!pending || destroyed || verificationTimer !== null) return; if (verificationAttempts >= VERIFY_ATTEMPTS || typeof global.setTimeout !== 'function') { setStatus('pending', 'Die Bestätigung dauert länger; angezeigt wird der aktuelle VDR-Stand.'); return; } verificationTimer = global.setTimeout(function () { verificationTimer = null; check(true); }, VERIFY_DELAY_MS); }
    function definitiveFailure(error) { return /recording_marks_modify_rejected|recording_marks_modify_stale|replay_conflict|replay_ledger_full|revision_conflict|recording_in_use|active_agent_lease_required|suitebridge_capability|backend_write|permission|forbidden|read.only|denied|Authentication|CSRF/.test(String(error && error.message || error || '')); }
    function submit(path, fields) { if (!editable()) return; try { pending = {path: path, body: Object.assign({}, identity, {operationId: token(), operationRevision: '1', expectedMarksRevision: payload.marksRevision}, fields || {})}; } catch (error) { setStatus('error', message(error)); return; } verificationAttempts = 0; clearVerificationTimer(); setStatus('pending', 'Wird in VDR gespeichert …'); check(false); }
    function check(automatic) {
      if (!pending || busy) return; const operation = pending; if (automatic) verificationAttempts += 1; busy = true; render();
      request(operation.path, operation.body).then(function (result) {
        if (destroyed || pending !== operation) return null;
        if (!result || result.operationId !== operation.body.operationId || result.accepted !== true) throw new Error('operation_response_invalid');
        if (result.verification !== 'verified') return syncCanonical(operation).catch(function () { return null; }).then(function () { if (destroyed || pending !== operation) return; setStatus('pending', 'Wird in VDR gespeichert …'); scheduleVerification(); });
        return syncCanonical(operation).then(function (next) { if (destroyed || pending !== operation || !next) return; pending = null; clearVerificationTimer(); verificationAttempts = 0; setStatus('success', operation.path.endsWith('/cut') ? 'Geschnittene Ausgabe bestätigt.' : 'Gespeichert.'); });
      }).catch(function (error) {
        if (destroyed || pending !== operation) return;
        if (definitiveFailure(error)) { pending = null; clearVerificationTimer(); verificationAttempts = 0; return syncCanonical(null).catch(function () { return null; }).then(function () { if (!destroyed) setStatus('error', message(error)); }); }
        setStatus('pending', message(error)); scheduleVerification();
      }).finally(function () { busy = false; if (!destroyed) render(); });
    }
    function confirmCut(label, action) { confirmation.replaceChildren(node('span', label)); button('Schneiden', function () { confirmation.replaceChildren(); action(); }, !editable(), confirmation, 'primary'); button('Abbrechen', function () { confirmation.replaceChildren(); }, false, confirmation); }
    function previewCut() {
      if (!editable()) return; busy = true; confirmation.replaceChildren(); render();
      request('/api/vdr/recordings/cut').then(function (preview) { if (destroyed) return; if (!preview || preview.backendId !== identity.backendId || String(preview.recordingId) !== identity.recordingId || preview.marksRevision !== payload.marksRevision) throw new Error('recording_marks_revision_conflict'); if (!preview.ready) { setStatus('error', preview.editedDestinationExists ? 'Eine geschnittene Ausgabe existiert bereits.' : preview.inUse ? 'Die Aufnahme wird gerade verwendet.' : 'VDR kann diese Markensequenz derzeit nicht schneiden.'); return; } confirmCut('Neue geschnittene Ausgabe erzeugen?', function () { submit('/api/vdr/recordings/cut'); }); }).catch(function (error) { if (!destroyed) setStatus('error', message(error)); }).finally(function () { busy = false; if (!destroyed) render(); });
    }
    function selectAndSeek(mark) { selectedFrame = Number(mark.positionFrame); render(); const active = playback(), state = snapshot(); if (!active || typeof active.seekAbsolute !== 'function' || !state || !state.sessionId) return; Promise.resolve(active.seekAbsolute(mark.positionSeconds)).catch(function () { if (!destroyed) setStatus('error', 'Springen ist im aktuellen Wiedergabestand nicht verfügbar.'); }); }
    function navigationTarget(direction) { const values = marks(); if (!values.length) return null; const selected = selectedMark(); if (selected) { const index = values.indexOf(selected) + direction; return index >= 0 && index < values.length ? values[index] : null; } const current = playback(), seconds = current && typeof current.position === 'function' ? Number(current.position()) : NaN; if (!Number.isFinite(seconds) || seconds < 0) return null; if (direction > 0) return values.find(function (mark) { return Number(mark.positionSeconds) > seconds + 0.001; }) || null; for (let index = values.length - 1; index >= 0; --index) if (Number(values[index].positionSeconds) < seconds - 0.001) return values[index]; return null; }
    function render() {
      toolbar.replaceChildren(); list.replaceChildren(); const blocked = !editable(), active = activePlayback(), selected = selectedMark(), previous = navigationTarget(-1), next = navigationTarget(1);
      positionHint.textContent = active ? 'Aktuelle Wiedergabeposition wird zum Setzen und Verschieben verwendet.' : 'Wiedergabe starten, um Marken zu setzen oder anzuspringen.';
      selectionHint.textContent = selected ? 'Auswahl: ' + String(selected.timecode || 'Schnittmarke') + ' · Frame ' + String(selected.positionFrame) : '';
      button('+ Marke', function () { const target = currentFrame(); if (target === null) { setStatus('error', 'Keine gültige Wiedergabeposition verfügbar.'); return; } submit('/api/vdr/recordings/marks', {kind: 'add', targetFrame: target}); }, blocked || !active, toolbar, 'primary');
      button('◀ Marke', function () { selectAndSeek(previous); }, !active || !previous, toolbar); button('Marke ▶', function () { selectAndSeek(next); }, !active || !next, toolbar);
      button('Verschieben', function () { const mark = selectedMark(), target = currentFrame(); if (mark && target !== null) submit('/api/vdr/recordings/marks', {kind: 'move', sourceFrame: mark.positionFrame, targetFrame: target}); }, blocked || !selected || !active, toolbar);
      button('Löschen', function () { const mark = selectedMark(); if (mark) submit('/api/vdr/recordings/marks', {kind: 'delete', sourceFrame: mark.positionFrame}); }, blocked || !selected, toolbar, 'danger');
      button('Alle löschen', function () { submit('/api/vdr/recordings/marks', {kind: 'reset'}); }, blocked || !marks().length, toolbar, 'danger'); button('Schneiden …', previewCut, blocked || !marks().length, toolbar);
      marks().forEach(function (mark, index) { const selectedNow = Number(mark.positionFrame) === selectedFrame, row = node('div'); row.className = 'recordings2-marks-editor-row' + (selectedNow ? ' selected' : ''); row.setAttribute('role', 'listitem'); list.appendChild(row); const choose = button(String(mark.timecode || ('Marke ' + String(index + 1))), function () { selectAndSeek(mark); }, false, row, 'recordings2-marks-editor-mark'); choose.setAttribute('aria-pressed', selectedNow ? 'true' : 'false'); row.appendChild(node('span', 'Frame ' + String(mark.positionFrame) + (index % 2 === 0 ? ' · behalten ab hier' : ' · entfernen ab hier'))); });
    }
    function observe() { const current = playback(); if (current === owner) return; if (unsubscribe) unsubscribe(); owner = current; unsubscribe = current && typeof current.subscribe === 'function' ? current.subscribe(function (state) { if (state && state.transition === 'destroyed') { destroy(); return; } const key = JSON.stringify([state && state.sessionId, state && state.state, state && state.transition]); if (!destroyed && key !== lifecycleKey) { lifecycleKey = key; render(); } }) : null; }
    function destroy() { destroyed = true; clearVerificationTimer(); if (unsubscribe) unsubscribe(); unsubscribe = null; confirmation.replaceChildren(); }
    const result = Object.freeze({reload: reload, destroy: destroy, observe: observe}); root.__vdrSuiteMarksEditor = result; observe(); render(); return result;
  }
  global.VdrSuiteRecordings2MarksEditor = Object.freeze({attach: attach});
}(window));
