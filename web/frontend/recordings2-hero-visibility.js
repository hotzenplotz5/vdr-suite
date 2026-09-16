// Enforce Recordings 2 hero/detail mode visibility against the real nested DOM.
(function (global) {
  'use strict';

  const owner = global.VdrSuiteRecordings2HeroDetail;
  if (!owner || typeof owner.enhance !== 'function') {
    console.error('VDR-Suite Recordings 2 hero detail owner is unavailable for visibility wiring');
    return;
  }

  const observers = typeof WeakMap === 'function' ? new WeakMap() : null;

  function elements(root, selector) {
    if (!root || typeof root.querySelectorAll !== 'function') return [];
    return Array.from(root.querySelectorAll(selector));
  }

  function setHidden(root, selector, hidden) {
    elements(root, selector).forEach(function (element) {
      element.hidden = Boolean(hidden);
      if (element.style && typeof element.style.setProperty === 'function') {
        if (hidden) element.style.setProperty('display', 'none', 'important');
        else element.style.removeProperty('display');
      }
    });
  }

  function apply(root) {
    if (!root || !root.dataset) return;
    const mode = root.dataset.recordings2HeroMode || 'detail';
    const detail = mode === 'detail';
    const playback = mode === 'playback';
    const marks = mode === 'marks';
    const metadata = mode === 'metadata';

    setHidden(root, '.recordings2-detail-hero', !detail);
    setHidden(root, '.recordings2-detail-grid', !detail);
    setHidden(root, '.recordings2-hero-related', !detail);

    setHidden(root, '.recordings2-playback', !(playback || marks));
    setHidden(root, '.recordings2-actions', true);

    setHidden(root, '.recordings2-metadata-tabs', !metadata);
    setHidden(root, '.recordings2-metadata-panel', !metadata);
    setHidden(root, '.recordings2-metadata-assignment', !metadata);
    setHidden(root, '[data-recordings2-metadata-assignment-error]', !metadata);

    const back = root.querySelector && root.querySelector('.recordings2-hero-mode-back');
    if (back) back.hidden = detail;
  }

  function observe(root) {
    if (!root || typeof global.MutationObserver !== 'function') return;
    if (observers && observers.has(root)) return;
    const observer = new global.MutationObserver(function (mutations) {
      const relevant = mutations.some(function (mutation) {
        return mutation.type === 'attributes' || mutation.type === 'childList';
      });
      if (relevant) apply(root);
    });
    observer.observe(root, {
      attributes: true,
      attributeFilter: ['data-recordings2-hero-mode'],
      childList: true,
      subtree: true
    });
    if (observers) observers.set(root, observer);
  }

  function enhance(root, recording, backendId, metadata) {
    const result = owner.enhance(root, recording, backendId, metadata);
    apply(root);
    observe(root);
    return result;
  }

  global.VdrSuiteRecordings2HeroDetail = Object.freeze({
    enhance,
    __test: Object.freeze({apply})
  });
}(window));
