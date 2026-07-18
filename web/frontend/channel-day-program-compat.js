// Compatibility bridge between the existing channel browser shell and the day programme extension.
(function(global) {
  'use strict';

  const SOURCE_SELECTOR = '.channel-browser-module';
  const SOURCE_CLASS = 'channel-browser-shell';

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

  function installHiddenStyle() {
    if (document.getElementById('vdr-suite-channel-day-program-compat-style')) return;

    const style = document.createElement('style');
    style.id = 'vdr-suite-channel-day-program-compat-style';
    style.textContent = '.channel-browser-shell[hidden]{display:none!important;}';
    document.head.appendChild(style);
  }

  function install() {
    const root = document.getElementById('detail-data');
    if (!root || root.dataset.channelDayProgramCompatBound === 'true') return;

    root.dataset.channelDayProgramCompatBound = 'true';
    installHiddenStyle();
    markChannelBrowserShells(root);

    root.addEventListener('pointerdown', event => {
      const target = event.target && typeof event.target.closest === 'function'
        ? event.target.closest(SOURCE_SELECTOR)
        : null;
      addSourceClass(target);
    }, true);

    const observer = new MutationObserver(records => {
      records.forEach(record => {
        record.addedNodes.forEach(node => {
          if (node && node.nodeType === 1) markChannelBrowserShells(node);
        });
      });
    });
    observer.observe(root, {childList: true, subtree: true});
  }

  global.VdrSuiteChannelDayProgramCompat = Object.freeze({
    markChannelBrowserShells
  });

  if (typeof document !== 'undefined') {
    if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', install);
    else install();
  }
})(window);
