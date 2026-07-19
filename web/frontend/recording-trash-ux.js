// Deferred frontend UX for recording trash and EPG SearchTimer actions.
(function(global) {
  'use strict';

  function text(value) {
    return String(value || '').trim();
  }

  function recordingTitleForEditor(editor) {
    const detail = editor && typeof editor.closest === 'function'
      ? editor.closest('.recording-detail')
      : null;
    const heading = detail && typeof detail.querySelector === 'function'
      ? detail.querySelector('h3')
      : null;
    return text(heading ? heading.textContent : '');
  }

  function findActionButton(editor, predicate) {
    return Array.from(editor.querySelectorAll('button')).find(button =>
      predicate(text(button.textContent), button)) || null;
  }

  function installStyles() {
    if (document.getElementById('vdr-suite-recording-trash-ux-style')) return;

    const style = document.createElement('style');
    style.id = 'vdr-suite-recording-trash-ux-style';
    style.textContent = `
.recording-trash-editor-body{display:grid;gap:.7rem;padding:.8rem!important;background:rgba(15,23,42,.76)!important;border-color:rgba(148,163,184,.22)!important}
.recording-trash-editor-body>p{margin:0}
.recording-trash-warning{padding:.65rem .72rem!important;border:1px solid rgba(248,113,113,.28)!important;border-radius:.72rem!important;background:rgba(127,29,29,.14)!important;color:#fecaca!important;font-weight:650!important}
.recording-trash-status{padding:.62rem .7rem!important;border-radius:.68rem!important;margin:0!important}
.recording-trash-status.neutral,.recording-trash-status.pending{border:1px solid rgba(96,165,250,.3)!important;background:rgba(30,64,175,.12)!important;color:#bfdbfe!important}
.recording-trash-status.success{border:1px solid rgba(74,222,128,.35)!important;background:rgba(20,83,45,.16)!important;color:#bbf7d0!important}
.recording-trash-status.error{border:1px solid rgba(248,113,113,.38)!important;background:rgba(127,29,29,.18)!important;color:#fecaca!important}
.recording-trash-editor .recording-action-buttons{display:grid;grid-template-columns:1fr;gap:.55rem}
.recording-trash-editor .recording-action-buttons button[hidden]{display:none!important}
.recording-trash-technical{padding:.55rem .65rem;border:1px solid rgba(148,163,184,.2);border-radius:.65rem;background:rgba(2,6,23,.46)}
.recording-trash-technical summary{cursor:pointer;font-weight:750;color:#cbd5e1}
.recording-trash-technical pre{white-space:pre-wrap;overflow-wrap:anywhere;margin:.55rem 0 0;color:#94a3b8;font:inherit;font-size:.82rem}
.recording-action-panel:has(.recording-trash-editor[open])>.recording-action-result{display:none!important}
`;
    document.head.appendChild(style);
  }

  function enhanceTrashEditor(editor) {
    if (!editor || editor.dataset.recordingTrashUxBound === 'true') return false;
    editor.dataset.recordingTrashUxBound = 'true';

    const summary = editor.querySelector('summary');
    const body = editor.querySelector('.recording-trash-editor-body');
    if (!summary || !body) return false;

    summary.textContent = 'In Papierkorb verschieben';

    const title = recordingTitleForEditor(editor);
    const recordingLine = Array.from(body.children).find(element =>
      element.tagName === 'P' &&
      !element.classList.contains('recording-trash-warning') &&
      !element.classList.contains('recording-trash-status')) || null;
    if (recordingLine && title !== '') recordingLine.textContent = 'Aufnahme: ' + title;

    const warning = body.querySelector('.recording-trash-warning');
    if (warning) {
      warning.textContent = 'VDR verschiebt die Aufnahme in den Papierkorb. Die endgültige Bereinigung erfolgt später nach den VDR-Regeln.';
    }

    const status = body.querySelector('.recording-trash-status');
    const validateButton = findActionButton(editor, label =>
      label === 'Papierkorb-Aktion prüfen' ||
      label === 'Sicherheitsprüfung starten' ||
      label === 'Erneut prüfen' ||
      label === 'Prüfung läuft …');
    const executeButton = editor.querySelector('.recording-trash-execute-button');
    const panel = editor.closest('.recording-action-panel');
    const resultBox = panel ? panel.querySelector(':scope > .recording-action-result') : null;

    const technical = document.createElement('details');
    technical.className = 'recording-trash-technical';
    technical.hidden = true;
    const technicalSummary = document.createElement('summary');
    technicalSummary.textContent = 'Technische Details';
    const technicalText = document.createElement('pre');
    technical.appendChild(technicalSummary);
    technical.appendChild(technicalText);
    body.appendChild(technical);

    let syncing = false;
    let lastDetailedStatus = '';

    function sync() {
      if (syncing) return;
      syncing = true;

      const resultText = text(resultBox ? resultBox.textContent : '');
      const currentStatusText = text(status ? status.textContent : '');
      if (currentStatusText !== '' &&
          currentStatusText !== 'Papierkorb derzeit nicht verfügbar.' &&
          currentStatusText !== 'Sicherheitsprüfung läuft …' &&
          currentStatusText !== 'Sicherheitsprüfung erfolgreich.') {
        lastDetailedStatus = currentStatusText;
      }

      const details = [lastDetailedStatus, resultText]
        .filter(value => value !== '')
        .filter((value, index, values) => values.indexOf(value) === index)
        .join('\n\n');
      technicalText.textContent = details;
      technical.hidden = details === '';
      if (resultBox) resultBox.hidden = editor.open;

      const state = status ? Array.from(status.classList) : [];
      if (state.includes('error')) {
        status.textContent = 'Papierkorb derzeit nicht verfügbar.';
        if (validateButton) {
          validateButton.hidden = false;
          validateButton.disabled = false;
          validateButton.textContent = 'Erneut prüfen';
        }
        if (executeButton) executeButton.hidden = true;
      } else if (state.includes('success')) {
        status.textContent = 'Sicherheitsprüfung erfolgreich.';
        if (validateButton) validateButton.hidden = true;
        if (executeButton) executeButton.hidden = false;
      } else if (state.includes('pending')) {
        status.textContent = 'Sicherheitsprüfung läuft …';
        if (validateButton) {
          validateButton.hidden = false;
          validateButton.textContent = 'Prüfung läuft …';
        }
        if (executeButton) executeButton.hidden = true;
      } else {
        if (validateButton) {
          validateButton.hidden = false;
          validateButton.textContent = 'Sicherheitsprüfung starten';
        }
        if (executeButton) executeButton.hidden = true;
      }

      syncing = false;
    }

    editor.addEventListener('toggle', () => {
      sync();
      if (!editor.open || editor.dataset.recordingTrashAutoPreflight === 'true') return;
      editor.dataset.recordingTrashAutoPreflight = 'true';
      global.setTimeout(() => {
        if (validateButton && !validateButton.disabled) validateButton.click();
      }, 0);
    });

    const observer = new MutationObserver(sync);
    observer.observe(body, {
      subtree: true,
      childList: true,
      characterData: true,
      attributes: true,
      attributeFilter: ['class', 'disabled', 'hidden']
    });
    if (resultBox) {
      observer.observe(resultBox, {
        subtree: true,
        childList: true,
        characterData: true
      });
    }

    sync();
    return true;
  }

  function enhanceTrashEditors(root) {
    const scope = root && typeof root.querySelectorAll === 'function' ? root : document;
    let count = 0;

    if (scope && typeof scope.matches === 'function' && scope.matches('.recording-trash-editor')) {
      if (enhanceTrashEditor(scope)) count += 1;
    }
    scope.querySelectorAll('.recording-trash-editor').forEach(editor => {
      if (enhanceTrashEditor(editor)) count += 1;
    });
    return count;
  }

  function install() {
    installStyles();
    enhanceTrashEditors(document);

    const observer = new MutationObserver(records => {
      records.forEach(record => record.addedNodes.forEach(node => {
        if (node && node.nodeType === 1) enhanceTrashEditors(node);
      }));
    });
    observer.observe(document.documentElement, {childList: true, subtree: true});
  }

  global.VdrSuiteRecordingTrashUx = Object.freeze({
    recordingTitleForEditor,
    enhanceTrashEditors
  });

  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', install);
  else install();
})(window);

(function(global) {
  'use strict';

  const upgraded = new WeakSet();

  function eventTitle(detail) {
    const title = detail.querySelector('.epg-detail-title, h3');
    return title ? String(title.textContent || '').trim() : '';
  }

  function selectedBackendId() {
    if (global.VdrSuitePlatform &&
        typeof global.VdrSuitePlatform.getSelectedBackendId === 'function') {
      const backendId = String(global.VdrSuitePlatform.getSelectedBackendId() || '').trim();
      if (backendId !== '') return backendId;
    }
    return 'default';
  }

  function showStatus(detail, message, failed) {
    let status = detail.querySelector('[data-epg-searchtimer-status="true"]');
    if (!status) {
      status = document.createElement('p');
      status.dataset.epgSearchtimerStatus = 'true';
      status.setAttribute('role', 'status');
      status.setAttribute('aria-live', 'polite');
      const actions = detail.querySelector('.epg-detail-actions');
      if (actions) actions.insertAdjacentElement('afterend', status);
      else detail.appendChild(status);
    }
    status.textContent = message;
    status.className = failed ? 'epg-searchtimer-status error' : 'epg-searchtimer-status success';
  }

  function createSeriesTimer(detail, button) {
    const title = eventTitle(detail);
    const api = global.VdrSuiteClientApi;
    if (!title || !api || typeof api.fetchClientSearchTimerCreateAction !== 'function') {
      showStatus(detail, 'Serientimer kann derzeit nicht erstellt werden.', true);
      return;
    }

    button.disabled = true;
    showStatus(detail, 'Serientimer wird erstellt …', false);

    api.fetchClientSearchTimerCreateAction({
      cache: 'no-store',
      payload: {
        backendId: selectedBackendId(),
        name: title,
        query: title,
        active: true,
        compareTitle: true,
        compareSubtitle: false,
        compareSummary: false,
        useSeriesRecording: true
      }
    }).then(result => {
      if (result && result.success === false) {
        throw new Error(result.message || 'Backend hat den Serientimer abgelehnt.');
      }
      button.textContent = 'Serientimer erstellt';
      showStatus(detail, 'Serientimer für „' + title + '“ wurde erstellt.', false);
    }).catch(error => {
      button.disabled = false;
      button.textContent = 'Serientimer';
      showStatus(detail, error && error.message ? error.message : String(error), true);
    });
  }

  function openAdvancedSearchTimer(detail) {
    const title = eventTitle(detail);
    const navigationButton = Array.from(document.querySelectorAll('button, [role="button"]'))
      .find(element => {
        if (detail.contains(element)) return false;
        const label = String(element.textContent || '').trim().toLowerCase();
        return label === 'suchtimer' || label === 'searchtimer' || label.includes('suchtimer');
      });

    if (!navigationButton || typeof navigationButton.click !== 'function') {
      showStatus(detail, 'Der erweiterte SearchTimer-Editor ist derzeit nicht erreichbar.', true);
      return;
    }

    navigationButton.click();
    global.setTimeout(() => {
      const createPanel = document.querySelector('.searchtimer-create-panel');
      if (createPanel) createPanel.open = true;

      const query = document.querySelector('.searchtimer-create-panel form [name="query"]');
      const name = document.querySelector('.searchtimer-create-panel form [name="name"]');
      if (query) {
        query.value = title;
        query.dispatchEvent(new Event('input', {bubbles: true}));
        query.dispatchEvent(new Event('change', {bubbles: true}));
      }
      if (name && !String(name.value || '').trim()) {
        name.value = title;
        name.dispatchEvent(new Event('input', {bubbles: true}));
        name.dispatchEvent(new Event('change', {bubbles: true}));
      }
      if (createPanel && typeof createPanel.scrollIntoView === 'function') {
        createPanel.scrollIntoView({behavior: 'smooth', block: 'start'});
      }
    }, 150);
  }

  function upgrade(detail) {
    if (upgraded.has(detail)) return;
    const actions = detail.querySelector('.epg-detail-actions');
    if (!actions) return;

    const buttons = Array.from(actions.querySelectorAll('button'));
    const search = buttons.find(button => String(button.textContent || '').trim() === 'Suchtimer');
    const more = buttons.find(button => String(button.textContent || '').trim() === 'Mehr …');
    if (!search || !more) return;

    upgraded.add(detail);

    search.disabled = false;
    search.textContent = 'Serientimer';
    search.title = 'Aktiven Serientimer aus dem Sendungstitel erstellen.';
    search.addEventListener('click', () => createSeriesTimer(detail, search));

    more.disabled = false;
    more.textContent = 'Erweiterter SearchTimer';
    more.title = 'SearchTimer-Editor mit dem Sendungstitel öffnen.';
    more.addEventListener('click', () => openAdvancedSearchTimer(detail));
  }

  function scan(root) {
    const scope = root && typeof root.querySelectorAll === 'function' ? root : document;
    if (scope && typeof scope.matches === 'function' && scope.matches('.epg-event-detail')) {
      upgrade(scope);
    }
    scope.querySelectorAll('.epg-event-detail').forEach(upgrade);
  }

  const observer = new MutationObserver(records => {
    records.forEach(record => record.addedNodes.forEach(node => {
      if (node && node.nodeType === 1) scan(node);
    }));
  });
  observer.observe(document.documentElement, {childList: true, subtree: true});
  scan(document);

  global.VdrSuiteEpgSearchTimerActions = Object.freeze({scan});
})(window);
