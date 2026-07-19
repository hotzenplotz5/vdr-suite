'use strict';

(function (global) {
  const upgraded = new WeakSet();

  function detailTitle(detail) {
    const title = detail.querySelector(
      '.epg-detail-title, .channels2-detail h3, h3'
    );
    return title ? String(title.textContent || '').trim() : '';
  }

  function showStatus(detail, message, failed) {
    let status = detail.querySelector(
      '[data-epg-searchtimer-status="true"], .channels2-feedback'
    );

    if (!status) {
      status = document.createElement('p');
      status.dataset.epgSearchtimerStatus = 'true';
      status.setAttribute('role', 'status');
      status.setAttribute('aria-live', 'polite');

      const actions = detail.querySelector(
        '.epg-detail-actions, .channels2-actions'
      );

      if (actions) actions.insertAdjacentElement('afterend', status);
      else detail.appendChild(status);
    }

    status.textContent = message;
    status.className = failed
      ? 'epg-searchtimer-status channels2-feedback error'
      : 'epg-searchtimer-status channels2-feedback success';
  }

  function searchTimerNavigationButton() {
    return Array.from(
      document.querySelectorAll('[data-module="searchtimers"], button, [role="button"]')
    ).find(element => {
      if (element.dataset && element.dataset.module === 'searchtimers') return true;
      const text = String(element.textContent || '').trim().toLowerCase();
      return text === 'suchtimer' ||
        text === 'searchtimer' ||
        text.includes('suchtimer');
    }) || null;
  }

  function setFormValue(form, name, value, eventName) {
    const input = form.elements[name];
    if (!input) return;

    if (input.type === 'checkbox') input.checked = Boolean(value);
    else input.value = String(value === undefined || value === null ? '' : value);

    input.dispatchEvent(new Event(eventName || 'input', {bubbles: true}));
  }

  function openSearchTimerEditor(detail, options) {
    const title = String(
      options && options.title ? options.title : detailTitle(detail)
    ).trim();
    const channelId = String(
      options && options.channelId ? options.channelId : ''
    ).trim();
    const navigationButton = searchTimerNavigationButton();

    if (!title || !navigationButton || typeof navigationButton.click !== 'function') {
      showStatus(
        detail,
        'Der SearchTimer-Editor ist derzeit nicht erreichbar.',
        true
      );
      return;
    }

    showStatus(
      detail,
      'SearchTimer-Editor für „' + title + '“ wird geöffnet …',
      false
    );
    navigationButton.click();

    let attempts = 0;
    const fill = function () {
      attempts += 1;

      const form = document.querySelector(
        'form[data-searchtimer-editor-form="create"]'
      );

      if (!form) {
        if (attempts < 30) {
          global.setTimeout(fill, 100);
          return;
        }

        showStatus(
          detail,
          'Der SearchTimer-Editor konnte nicht geöffnet werden.',
          true
        );
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
          setFormValue(form, 'channelId', channelId, 'change');
        }, 80);
      }

      form.scrollIntoView({behavior: 'smooth', block: 'start'});
    };

    global.setTimeout(fill, 80);
  }

  function upgradeEpgDetail(detail) {
    if (upgraded.has(detail)) return;

    const actions = detail.querySelector('.epg-detail-actions');
    if (!actions) return;

    const buttons = Array.from(actions.querySelectorAll('button'));
    const search = buttons.find(
      button => String(button.textContent || '').trim() === 'Suchtimer'
    );
    const more = buttons.find(
      button => String(button.textContent || '').trim() === 'Mehr …'
    );

    if (!search || !more) return;
    upgraded.add(detail);

    search.disabled = false;
    search.textContent = 'Serientimer vorbereiten';
    search.title =
      'SearchTimer-Editor mit dem Sendungstitel öffnen. Die direkte Runtime-Mutation bleibt sicher gesperrt.';
    search.addEventListener('click', () => openSearchTimerEditor(detail));

    more.disabled = false;
    more.textContent = 'Erweiterter SearchTimer';
    more.title = 'SearchTimer-Editor mit dem Sendungstitel öffnen.';
    more.addEventListener('click', () => openSearchTimerEditor(detail));
  }

  function interceptChannels2SeriesAction(event) {
    const button = event.target && event.target.closest
      ? event.target.closest('.channels2-actions button')
      : null;

    if (!button) return;

    const label = String(button.textContent || '').trim();
    if (label !== 'Serientimer erstellen') return;

    const detail = button.closest('.channels2-detail');
    if (!detail) return;

    event.preventDefault();
    event.stopImmediatePropagation();

    button.textContent = 'Serientimer vorbereiten';
    openSearchTimerEditor(detail);
  }

  function scan() {
    document.querySelectorAll('.epg-event-detail').forEach(upgradeEpgDetail);
  }

  document.addEventListener('click', interceptChannels2SeriesAction, true);

  const observer = new MutationObserver(scan);
  observer.observe(document.documentElement, {childList: true, subtree: true});
  scan();

  global.VdrSuiteEpgSearchTimerActions = Object.freeze({
    scan,
    openSearchTimerEditor
  });
}(window));
