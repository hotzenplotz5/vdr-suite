'use strict';

(function (global) {
  const PREVIEW_REFRESH_WINDOW_SECONDS = 14 * 24 * 60 * 60;
  const PREVIEW_CHANNEL_EVENT_LIMIT = 96;

  function searchTimerNavigationButton() {
    return document.querySelector('[data-module="searchtimers"]');
  }

  function setFormValue(form, name, value, eventName) {
    const input = form.elements[name];
    if (!input) return;
    if (input.type === 'checkbox') input.checked = Boolean(value);
    else input.value = String(value === undefined || value === null ? '' : value);
    input.dispatchEvent(new Event(eventName || 'input', {bubbles: true}));
  }

  function showStatus(target, message, failed) {
    if (!target) return;
    let status = target.querySelector('[data-searchtimer-editor-status="true"]');
    if (!status) {
      status = document.createElement('p');
      status.dataset.searchtimerEditorStatus = 'true';
      status.setAttribute('role', 'status');
      status.setAttribute('aria-live', 'polite');
      target.appendChild(status);
    }
    status.className = failed ? 'channels2-feedback error' : 'channels2-feedback success';
    status.textContent = message;
  }

  function selectedBackendId() {
    const runtime = global.VdrSuitePlatform;
    if (runtime && typeof runtime.getSelectedBackendId === 'function') {
      const value = String(runtime.getSelectedBackendId() || '').trim();
      if (value) return value;
    }
    return 'default';
  }

  function refreshPreviewCache() {
    const client = global.VdrSuiteClientApi;
    if (!client || typeof client.requestJson !== 'function') {
      return Promise.reject(new Error('Preview-EPG-Cache kann nicht aktualisiert werden: Client API fehlt.'));
    }

    const options = {
      method: 'POST',
      query: {
        backend: selectedBackendId(),
        from: -1,
        timespan: PREVIEW_REFRESH_WINDOW_SECONDS,
        limit: 0,
        channelEventLimit: PREVIEW_CHANNEL_EVENT_LIMIT,
        _: Date.now()
      },
      cache: 'no-store',
      credentials: 'same-origin'
    };

    return client.requestJson('/api/vdr/searchtimers/preview/cache/refresh', options)
      .catch(function () {
        return client.requestJson('/api/searchtimers/preview/cache/refresh', options);
      })
      .then(function (result) {
        const available = result && result.available === true;
        const ready = result && String(result.status || '') === 'ready';
        const eventCount = Number(result && result.eventCount || 0);
        if (!available || !ready || eventCount <= 0) {
          throw new Error('Der Preview-EPG-Cache ist nicht bereit oder enthält keine Sendungen.');
        }
        return result;
      });
  }

  function previewFeedback(button) {
    const form = button.closest('form[data-searchtimer-editor-form="create"]');
    return form ? form.querySelector('[data-searchtimer-preview-result="true"]') : null;
  }

  function installPreviewCacheGuard() {
    if (document.documentElement.dataset.searchtimerPreviewCacheGuard === 'true') return;
    document.documentElement.dataset.searchtimerPreviewCacheGuard = 'true';

    document.addEventListener('click', function (event) {
      const button = event.target && event.target.closest
        ? event.target.closest('[data-searchtimer-action="preview"]')
        : null;
      if (!button || button.dataset.previewCacheReady === 'true') return;

      event.preventDefault();
      event.stopPropagation();
      if (typeof event.stopImmediatePropagation === 'function') event.stopImmediatePropagation();

      const feedback = previewFeedback(button);
      button.disabled = true;
      if (feedback) {
        feedback.className = 'searchtimer-feedback';
        feedback.textContent = 'EPG-Daten für die Vorschau werden aktualisiert …';
      }

      refreshPreviewCache()
        .then(function (result) {
          if (feedback) {
            feedback.className = 'searchtimer-feedback success';
            feedback.textContent = String(result.eventCount) + ' EPG-Sendungen geladen. Vorschau wird ausgeführt …';
          }
          button.dataset.previewCacheReady = 'true';
          button.disabled = false;
          button.click();
          delete button.dataset.previewCacheReady;
        })
        .catch(function (error) {
          button.disabled = false;
          if (feedback) {
            feedback.className = 'searchtimer-feedback error';
            feedback.textContent = String(error && error.message ? error.message : error);
          } else {
            global.alert(String(error && error.message ? error.message : error));
          }
        });
    }, true);
  }

  function openSearchTimerEditor(options) {
    const settings = options && typeof options === 'object' ? options : {};
    const title = String(settings.title || '').trim();
    const channelId = String(settings.channelId || '').trim();
    const channelGroup = String(settings.channelGroup || '').trim();
    const statusTarget = settings.statusTarget || null;
    const navigationButton = searchTimerNavigationButton();

    if (!title || !navigationButton || typeof navigationButton.click !== 'function') {
      showStatus(statusTarget, 'Der SearchTimer-Editor ist derzeit nicht erreichbar.', true);
      return Promise.reject(new Error('SearchTimer editor is not available'));
    }

    showStatus(statusTarget, 'SearchTimer-Editor für „' + title + '“ wird geöffnet …', false);
    navigationButton.click();

    return new Promise(function (resolve, reject) {
      let attempts = 0;
      const fill = function () {
        attempts += 1;
        const form = document.querySelector('form[data-searchtimer-editor-form="create"]');
        if (!form) {
          if (attempts < 40) { global.setTimeout(fill, 100); return; }
          showStatus(statusTarget, 'Der SearchTimer-Editor konnte nicht geöffnet werden.', true);
          reject(new Error('SearchTimer editor form did not appear'));
          return;
        }
        const panel = form.closest('details');
        if (panel) panel.open = true;
        setFormValue(form, 'name', title);
        setFormValue(form, 'query', title);
        setFormValue(form, 'active', true, 'change');
        setFormValue(form, 'compareTitle', true, 'change');
        setFormValue(form, 'avoidRepeats', true, 'change');
        if (channelId) {
          setFormValue(form, 'channelFilterMode', 1, 'change');
          global.setTimeout(function () {
            if (channelGroup) setFormValue(form, 'channelSelectorGroup', channelGroup, 'change');
            global.setTimeout(function () {
              setFormValue(form, 'channelId', channelId, 'change');
              form.scrollIntoView({behavior: 'smooth', block: 'start'});
              resolve(form);
            }, 80);
          }, 80);
          return;
        }
        form.scrollIntoView({behavior: 'smooth', block: 'start'});
        resolve(form);
      };
      global.setTimeout(fill, 80);
    });
  }

  installPreviewCacheGuard();
  global.VdrSuiteEpgSearchTimerActions = Object.freeze({
    openSearchTimerEditor: openSearchTimerEditor,
    refreshPreviewCache: refreshPreviewCache
  });
}(window));
