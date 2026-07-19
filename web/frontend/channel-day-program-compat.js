// Compatibility bridge between the existing channel browser and the day programme extension.
(function(global) {
  'use strict';

  const SOURCE_SELECTOR = '.channel-browser-module';
  const SOURCE_CLASS = 'channel-browser-shell';
  const detailState = new WeakMap();

  function addSourceClass(element) {
    if (!element || !element.classList || !element.classList.contains('channel-browser-module')) {
      return false;
    }

    element.classList.add(SOURCE_CLASS);
    return true;
  }

  function markChannelBrowserShells(root) {
    const scope = root && typeof root.querySelectorAll === 'function' ? root : document;
    const shells = [];

    if (scope && typeof scope.matches === 'function' && scope.matches(SOURCE_SELECTOR)) {
      shells.push(scope);
    }

    if (scope && typeof scope.querySelectorAll === 'function') {
      scope.querySelectorAll(SOURCE_SELECTOR).forEach(shell => shells.push(shell));
    }

    let marked = 0;
    shells.forEach(shell => {
      if (addSourceClass(shell)) marked += 1;
    });
    return marked;
  }

  function normalizedActionLabel(value) {
    return String(value || '')
      .trim()
      .replace(/\u2026/g, '...')
      .replace(/\s+/g, ' ')
      .toLowerCase();
  }

  function normalizeDetailActions(scope) {
    if (!scope || typeof scope.querySelectorAll !== 'function') return;

    scope.querySelectorAll('.epg-detail-action').forEach(button => {
      const label = normalizedActionLabel(button.textContent);

      if (label === 'serie automatisch aufnehmen') {
        button.textContent = 'Serientimer';
        button.title = 'Serientimer als SearchTimer mit Treffer-Vorschau vorbereiten.';
      } else if (label === 'erweiterter searchtimer') {
        button.title = 'Vollständigen SearchTimer-Editor mit dieser Sendung öffnen.';
      }
    });
  }

  function installStyles() {
    if (document.getElementById('vdr-suite-channel-day-program-compat-style')) return;

    const style = document.createElement('style');
    style.id = 'vdr-suite-channel-day-program-compat-style';
    style.textContent = `
.channel-browser-shell[hidden]{display:none!important;}
.channel-day-program-view.channel-day-detail-mode .channel-day-date-controls,
.channel-day-program-view.channel-day-detail-mode .channel-day-heading,
.channel-day-program-view.channel-day-detail-mode .channel-day-status,
.channel-day-program-view.channel-day-detail-mode .channel-day-list{display:none!important;}
.channel-day-program-view.channel-day-detail-mode .channel-day-event-detail{margin:0;scroll-margin-top:.75rem;}
.channel-day-program-view.channel-day-detail-mode .channel-day-toolbar{align-items:flex-start;}
`;
    document.head.appendChild(style);
  }

  function activateDetailMode(row) {
    if (!row || typeof row.closest !== 'function') return;

    const view = row.closest('.channel-day-program-view');
    if (!view) return;

    global.setTimeout(() => {
      const wrapper = view.querySelector('.channel-day-event-detail');
      const channelHeader = view.querySelector('.channel-day-channel-head');
      if (!wrapper || !channelHeader) return;

      detailState.set(view, {
        row,
        windowY: global.scrollY || 0
      });

      view.classList.add('channel-day-detail-mode');
      channelHeader.insertAdjacentElement('afterend', wrapper);
      normalizeDetailActions(wrapper);
      wrapper.scrollIntoView({behavior: 'smooth', block: 'start'});
    }, 0);
  }

  function restoreProgrammeView(backButton) {
    if (!backButton || typeof backButton.closest !== 'function') return;

    const view = backButton.closest('.channel-day-program-view');
    if (!view) return;

    const saved = detailState.get(view) || null;
    global.setTimeout(() => {
      view.classList.remove('channel-day-detail-mode');
      detailState.delete(view);

      if (saved && saved.row && saved.row.isConnected) {
        global.scrollTo(0, saved.windowY);
        saved.row.focus({preventScroll: true});
      }
    }, 0);
  }

  function requestLateDayProgrammeRuntime(root) {
    if (!root || root.dataset.channelDayProgramBound === 'true') return;
    if (root.dataset.channelDayProgramLateRequested === 'true') return;

    root.dataset.channelDayProgramLateRequested = 'true';

    const script = document.createElement('script');
    script.src = '/frontend/channel-day-program.js?late=' + String(Date.now());
    script.async = false;
    script.dataset.channelDayProgramLateRuntime = 'true';
    script.addEventListener('error', () => {
      root.dataset.channelDayProgramLateRequested = 'false';
    });
    document.head.appendChild(script);
  }

  function installRoot(root) {
    if (!root) return false;

    installStyles();
    markChannelBrowserShells(root);
    requestLateDayProgrammeRuntime(root);

    if (root.dataset.channelDayProgramCompatBound === 'true') return true;
    root.dataset.channelDayProgramCompatBound = 'true';

    root.addEventListener('pointerdown', event => {
      const target = event.target && typeof event.target.closest === 'function'
        ? event.target.closest(SOURCE_SELECTOR)
        : null;
      addSourceClass(target);
    }, true);

    root.addEventListener('click', event => {
      const target = event.target && typeof event.target.closest === 'function'
        ? event.target
        : null;
      if (!target) return;

      const row = target.closest('.channel-day-event');
      if (row) activateDetailMode(row);

      const back = target.closest('.channel-day-event-back');
      if (back) restoreProgrammeView(back);
    }, true);

    const observer = new MutationObserver(records => {
      records.forEach(record => {
        record.addedNodes.forEach(node => {
          if (!node || node.nodeType !== 1) return;
          markChannelBrowserShells(node);
          normalizeDetailActions(node);
        });
      });
    });
    observer.observe(root, {childList: true, subtree: true});
    return true;
  }

  function installWhenReady() {
    const root = document.getElementById('detail-data');
    if (installRoot(root)) return true;
    return false;
  }

  function watchDocument() {
    installWhenReady();

    if (!document.documentElement || document.documentElement.dataset.channelDayProgramDocumentWatch === 'true') {
      return;
    }

    document.documentElement.dataset.channelDayProgramDocumentWatch = 'true';
    const observer = new MutationObserver(() => installWhenReady());
    observer.observe(document.documentElement, {childList: true, subtree: true});
  }

  global.VdrSuiteChannelDayProgramCompat = Object.freeze({
    markChannelBrowserShells,
    normalizedActionLabel,
    installWhenReady
  });

  if (typeof document !== 'undefined') {
    if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', watchDocument);
    else watchDocument();
  }
})(window);
