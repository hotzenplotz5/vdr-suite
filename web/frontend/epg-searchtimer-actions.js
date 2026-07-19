'use strict';

(function () {
  const upgraded = new WeakSet();

  function eventTitle(detail) {
    const title = detail.querySelector('.epg-detail-title, h3');
    return title ? String(title.textContent || '').trim() : '';
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
    const api = window.VdrSuiteClientApi;
    if (!title || !api || typeof api.fetchClientSearchTimerCreateAction !== 'function') {
      showStatus(detail, 'Serientimer kann derzeit nicht erstellt werden.', true);
      return;
    }

    button.disabled = true;
    showStatus(detail, 'Serientimer wird erstellt …', false);

    api.fetchClientSearchTimerCreateAction({
      cache: 'no-store',
      payload: {
        backendId: 'default',
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
        const text = String(element.textContent || '').trim().toLowerCase();
        return text === 'suchtimer' || text === 'searchtimer' || text.includes('suchtimer');
      });

    if (!navigationButton || typeof navigationButton.click !== 'function') {
      showStatus(detail, 'Der erweiterte SearchTimer-Editor ist derzeit nicht erreichbar.', true);
      return;
    }

    navigationButton.click();
    window.setTimeout(() => {
      const createButton = Array.from(document.querySelectorAll('button'))
        .find(button => /neuer searchtimer|searchtimer erstellen/i.test(String(button.textContent || '')));
      if (createButton && typeof createButton.click === 'function') createButton.click();

      window.setTimeout(() => {
        const query = document.querySelector('form [name="query"], form [name="search"]');
        const name = document.querySelector('form [name="name"]');
        if (query) {
          query.value = title;
          query.dispatchEvent(new Event('input', {bubbles: true}));
        }
        if (name && !String(name.value || '').trim()) {
          name.value = title;
          name.dispatchEvent(new Event('input', {bubbles: true}));
        }
      }, 100);
    }, 100);
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

  function scan() {
    document.querySelectorAll('.epg-event-detail').forEach(upgrade);
  }

  const observer = new MutationObserver(scan);
  observer.observe(document.documentElement, {childList: true, subtree: true});
  scan();

  window.VdrSuiteEpgSearchTimerActions = Object.freeze({scan});
}());
