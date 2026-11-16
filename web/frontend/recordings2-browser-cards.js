// Recording card DOM and capability-gated HEVC menu for Recordings 2.
// Bundled before recordings2-browser-view.js; native paths and IDs are intact.
(function (global) {
  'use strict';
  const shared = global.VdrSuiteRecordings2Shared;
  if (!shared) return;

  function createRecordingCard(recording, onSelect) {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'recordings2-recording';
    button.appendChild(shared.createPoster(recording));
    const copy = document.createElement('span');
    copy.className = 'recordings2-recording-copy';
    copy.appendChild(shared.node('span', 'recordings2-title', shared.recordingTitle(recording)));
    const subtitle = shared.recordingSubtitle(recording);
    if (subtitle) copy.appendChild(shared.node('span', 'recordings2-subtitle', subtitle));
    copy.appendChild(shared.node('span', 'recordings2-meta',
      shared.formatStart(shared.first(recording, ['startTime', 'start'], '')) + ' · ' +
        shared.formatDuration(shared.first(recording, ['durationSeconds', 'duration'], 0)) + ' · ' +
        shared.formatSize(shared.first(recording, ['sizeMb'], 0))
    ));
    const summary = shared.recordingSummary(recording);
    if (summary) copy.appendChild(shared.node('span', 'recordings2-summary-text', summary));
    button.appendChild(copy);
    button.addEventListener('click', function () {
      if (typeof onSelect === 'function') onSelect(recording);
    });
    return button;
  }

  function installCardActionsStyle() {
    if (typeof document.getElementById === 'function' &&
        document.getElementById('recordings2-card-actions-style')) return;
    const style = document.createElement('style');
    style.id = 'recordings2-card-actions-style';
    style.textContent = [
      '.recordings2-card-action{position:relative;min-width:0}',
      '.recordings2-card-action>.recordings2-recording{width:100%;padding-right:3.5rem}',
      '.recordings2-card-action-menu{position:absolute;top:.5rem;right:.5rem;width:2.5rem;z-index:2}',
      '.recordings2-card-action-menu[open]{z-index:5}',
      '.recordings2-card-action-menu>summary{display:grid;place-items:center;width:2.5rem;height:2.5rem;list-style:none;border:1px solid #64748b;border-radius:.65rem;background:#172233;color:white;font-size:1.5rem;cursor:pointer}',
      '.recordings2-card-action-menu>summary::-webkit-details-marker{display:none}',
      '.recordings2-card-action-menu>summary:focus-visible{outline:2px solid #7dd3fc}',
      '.recordings2-card-action-panel{position:absolute;right:0;top:2.75rem;display:grid;gap:.35rem;width:min(19rem,78vw);padding:.5rem;border:1px solid #475569;border-radius:.7rem;background:#111827;box-shadow:0 .7rem 2rem rgba(0,0,0,.35)}',
      '.recordings2-card-action-panel>button,.recordings2-card-action-panel>p{margin:0;padding:.55rem .65rem;width:100%;border:1px solid #475569;border-radius:.55rem;background:#182235;color:#f8fafc;text-align:left;font-size:.8rem;line-height:1.35}',
      '.recordings2-card-action-panel>button:disabled{opacity:.6}'
    ].join('');
    document.head.appendChild(style);
  }

  function recordingCardWithActions(recording, options) {
    const wrapper = document.createElement('div');
    wrapper.className = 'recordings2-card-action';
    wrapper.appendChild(createRecordingCard(recording, options.selectRecording));
    const menu = document.createElement('details');
    menu.className = 'recordings2-card-action-menu';
    const summary = document.createElement('summary');
    summary.textContent = '⋯';
    summary.setAttribute('aria-label', 'Aufnahmeaktionen für ' + shared.recordingTitle(recording));
    menu.appendChild(summary);
    const action = shared.createButton('Aufnahme verkleinern (HEVC)', function () {
      if (action.disabled || typeof options.selectRecording !== 'function') return;
      menu.open = false;
      options.selectRecording(Object.assign({}, recording, {__vdrSuiteOpenHevcShrink: true}));
    });
    action.disabled = true;
    const note = shared.node('p', '', 'HEVC-Verfügbarkeit wird beim Öffnen geprüft.');
    const popover = document.createElement('div');
    popover.className = 'recordings2-card-action-panel';
    popover.append(action, note);
    menu.appendChild(popover);
    let requested = false;
    menu.addEventListener('toggle', function () {
      if (!menu.open || requested) return;
      requested = true;
      const factory = global.VdrSuitePublicV1Client;
      const client = factory && typeof factory.createClient === 'function'
        ? factory.createClient({baseUrl: global.location && global.location.origin || '',
          credentials: 'same-origin'}) : null;
      if (!client || typeof client.getMediaShrinkCapability !== 'function') {
        note.textContent = 'HEVC-Vorprüfung ist auf diesem Client nicht verfügbar.';
        return;
      }
      const backendId = String(options.getState().backendId || shared.selectedBackendId() || 'default');
      note.textContent = 'HEVC-Verfügbarkeit wird geprüft …';
      client.getMediaShrinkCapability({
        backendId: backendId, credentials: 'same-origin', cache: 'no-store'
      }).then(function (capability) {
        if (!capability || capability.backendId !== backendId ||
            capability.capability !== 'addons.media.shrink' ||
            capability.planningAvailable !== true) {
          note.textContent = 'HEVC-Vorprüfung ist auf diesem Backend nicht verfügbar.';
          return;
        }
        action.disabled = false;
        note.textContent = capability.available === true
          ? 'Das Original bleibt erhalten; Umkodierung und Veröffentlichung erfordern Bestätigung.'
          : 'Vorprüfung möglich; HEVC-Konvertierung ist derzeit nicht freigeschaltet.';
      }).catch(function (error) {
        note.textContent = 'HEVC-Status nicht verfügbar: ' +
          String(error && error.message ? error.message : error);
      });
    });
    wrapper.appendChild(menu);
    return wrapper;
  }

  global.VdrSuiteRecordings2Cards = Object.freeze({
    createRecordingCard: createRecordingCard,
    installStyles: installCardActionsStyle,
    withActions: recordingCardWithActions
  });
}(window));
