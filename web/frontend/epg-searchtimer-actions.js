'use strict';

(function (global) {
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
          if (attempts < 40) {
            global.setTimeout(fill, 100);
            return;
          }
          const error = new Error('SearchTimer editor form did not appear');
          showStatus(statusTarget, 'Der SearchTimer-Editor konnte nicht geöffnet werden.', true);
          reject(error);
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

  global.VdrSuiteEpgSearchTimerActions = Object.freeze({
    openSearchTimerEditor: openSearchTimerEditor
  });
}(window));
