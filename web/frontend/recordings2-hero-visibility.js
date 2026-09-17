// Keep Recordings 2 hero/detail visibility aligned with the real metadata-owned DOM.
(function (global) {
  'use strict';

  const owner = global.VdrSuiteRecordings2HeroDetail;
  if (!owner || typeof owner.enhance !== 'function') {
    console.error('VDR-Suite Recordings 2 hero detail owner is unavailable for visibility wiring');
    return;
  }

  const STYLE_ID = 'vdr-suite-recordings2-hero-nested-style';
  const observers = typeof WeakMap === 'function' ? new WeakMap() : null;

  function elements(root, selector) {
    if (!root || typeof root.querySelectorAll !== 'function') return [];
    return Array.from(root.querySelectorAll(selector));
  }

  function setElementHidden(element, hidden, shownDisplay) {
    if (!element) return;
    element.hidden = Boolean(hidden);
    if (element.style && typeof element.style.setProperty === 'function') {
      if (hidden) element.style.setProperty('display', 'none', 'important');
      else if (shownDisplay) element.style.setProperty('display', shownDisplay, 'important');
      else element.style.removeProperty('display');
    }
  }

  function releaseDisplay(element) {
    if (element && element.style && typeof element.style.removeProperty === 'function') {
      element.style.removeProperty('display');
    }
  }

  function setHidden(root, selector, hidden, shownDisplay) {
    elements(root, selector).forEach(function (element) {
      setElementHidden(element, hidden, shownDisplay);
    });
  }

  function recordingPanel(root) {
    return elements(root, '.recordings2-metadata-panel').find(function (panel) {
      return panel && typeof panel.querySelector === 'function' &&
        panel.querySelector('.recordings2-detail-hero');
    }) || null;
  }

  function selectedMetadataPanel(root, panels) {
    const tabs = elements(root, '.recordings2-metadata-tab');
    const selected = tabs.findIndex(function (tab) {
      return tab && typeof tab.getAttribute === 'function' &&
        tab.getAttribute('aria-selected') === 'true';
    });
    return selected >= 0 && selected < panels.length ? selected : 0;
  }

  function installNestedStyles() {
    if (!global.document || typeof global.document.getElementById !== 'function' ||
        global.document.getElementById(STYLE_ID)) return;
    const style = global.document.createElement('style');
    style.id = STYLE_ID;
    style.textContent = `
.recordings2-hero-page[data-recordings2-hero-mode="detail"]>.recordings2-metadata-panel.recordings2-hero-recording-panel{position:relative;z-index:2;margin:0!important;padding:0;border:0;background:transparent}
.recordings2-hero-page[data-recordings2-hero-mode="detail"]>.recordings2-metadata-panel.recordings2-hero-recording-panel>.recordings2-detail-hero{position:relative;z-index:2;display:grid;grid-template-columns:minmax(12rem,18rem) minmax(0,1fr);gap:clamp(1.5rem,3vw,3rem);align-items:end;min-height:min(78vh,58rem);margin:0;padding:clamp(6rem,11vw,10rem) clamp(1.25rem,5vw,5rem) clamp(3rem,6vw,5.5rem);overflow:hidden;border:0;border-radius:0;background:linear-gradient(90deg,rgba(7,9,12,.98) 0%,rgba(7,9,12,.9) 32%,rgba(7,9,12,.46) 63%,rgba(7,9,12,.2) 100%)}
.recordings2-hero-page[data-recordings2-hero-mode="detail"]>.recordings2-metadata-panel.recordings2-hero-recording-panel>.recordings2-detail-hero::before{content:'';position:absolute;inset:0;z-index:-3;background-image:var(--recordings2-hero-backdrop);background-size:cover;background-position:center 24%;opacity:.62;filter:saturate(.92);transform:scale(1.018)}
.recordings2-hero-page[data-recordings2-hero-mode="detail"]>.recordings2-metadata-panel.recordings2-hero-recording-panel>.recordings2-detail-hero::after{content:'';position:absolute;inset:0;z-index:-2;background:linear-gradient(90deg,rgba(7,9,12,.96) 0%,rgba(7,9,12,.82) 33%,rgba(7,9,12,.34) 68%,rgba(7,9,12,.2) 100%),linear-gradient(180deg,rgba(7,9,12,.08) 0%,rgba(7,9,12,.04) 58%,#090b0f 100%)}
.recordings2-hero-page[data-recordings2-hero-mode="detail"] .recordings2-detail-copy>.recordings2-hero-eyebrow{order:0}
.recordings2-hero-page[data-recordings2-hero-mode="detail"] .recordings2-detail-copy>h3{order:1}
.recordings2-hero-page[data-recordings2-hero-mode="detail"] .recordings2-detail-copy>.recordings2-subtitle{order:2}
.recordings2-hero-page[data-recordings2-hero-mode="detail"] .recordings2-detail-copy>.recordings2-hero-facts{order:3}
.recordings2-hero-page[data-recordings2-hero-mode="detail"] .recordings2-detail-copy>.recordings2-hero-actions{order:4}
.recordings2-hero-page[data-recordings2-hero-mode="detail"] .recordings2-detail-copy>.recordings2-detail-description{order:5}
.recordings2-hero-page[data-recordings2-hero-mode="detail"] .recordings2-detail-copy>.recordings2-hero-cast{order:6}
.recordings2-hero-page[data-recordings2-hero-mode="detail"]>.recordings2-volume-owner-shell{display:none!important}
.recordings2-hero-page[data-recordings2-hero-mode="playback"]>.recordings2-volume-owner-shell,.recordings2-hero-page[data-recordings2-hero-mode="marks"]>.recordings2-volume-owner-shell{position:relative;z-index:2;margin:5.5rem clamp(1.25rem,5vw,5rem) 1rem}
.recordings2-hero-page[data-recordings2-hero-mode="playback"]>.recordings2-volume-owner-shell>.recordings2-playback,.recordings2-hero-page[data-recordings2-hero-mode="marks"]>.recordings2-volume-owner-shell>.recordings2-playback{margin:0!important}
.recordings2-hero-page[data-recordings2-hero-mode="playback"]>.recordings2-metadata-panel.recordings2-hero-recording-panel,.recordings2-hero-page[data-recordings2-hero-mode="marks"]>.recordings2-metadata-panel.recordings2-hero-recording-panel{position:relative;z-index:2;margin:0 clamp(1.25rem,5vw,5rem)!important;padding:0;border:0;background:transparent}
.recordings2-hero-page[data-recordings2-hero-mode="playback"]>.recordings2-metadata-panel.recordings2-hero-recording-panel>.recordings2-detail-grid,.recordings2-hero-page[data-recordings2-hero-mode="marks"]>.recordings2-metadata-panel.recordings2-hero-recording-panel>.recordings2-detail-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(11rem,1fr));gap:.65rem;margin:0}
.recordings2-hero-page[data-recordings2-hero-mode="playback"] .recordings2-marks-detail,.recordings2-hero-page[data-recordings2-hero-mode="marks"] .recordings2-marks-detail{position:relative;z-index:2;margin:1rem clamp(1.25rem,5vw,5rem) 0}
@media(max-width:820px){.recordings2-hero-page[data-recordings2-hero-mode="detail"]>.recordings2-metadata-panel.recordings2-hero-recording-panel>.recordings2-detail-hero{grid-template-columns:9rem minmax(0,1fr);gap:1.2rem;min-height:auto;padding-top:6rem}}
@media(max-width:620px){.recordings2-hero-page[data-recordings2-hero-mode="detail"]>.recordings2-metadata-panel.recordings2-hero-recording-panel>.recordings2-detail-hero{grid-template-columns:1fr;padding-top:5.5rem}}
`;
    global.document.head.appendChild(style);
  }

  function apply(root) {
    if (!root || !root.dataset) return;
    const mode = root.dataset.recordings2HeroMode || 'detail';
    const detail = mode === 'detail';
    const playback = mode === 'playback';
    const marks = mode === 'marks';
    const metadata = mode === 'metadata';
    const playbackSurface = playback || marks;
    const panels = elements(root, '.recordings2-metadata-panel');
    const primary = recordingPanel(root);

    if (primary && primary.classList && typeof primary.classList.add === 'function') {
      primary.classList.add('recordings2-hero-recording-panel');
    }

    setHidden(root, '.recordings2-detail-hero', !(detail || metadata));
    setHidden(root, '.recordings2-detail-grid', !(playbackSurface || metadata));
    setHidden(root, '.recordings2-hero-related', !detail);
    setHidden(root, '.recordings2-playback', !playbackSurface);
    setHidden(root, '.recordings2-volume-owner-shell', !playbackSurface);
    setHidden(root, '.recordings2-marks-detail', !playbackSurface);
    setHidden(root, '.recordings2-actions', true);
    setHidden(root, '.recordings2-metadata-tabs', !metadata);
    setHidden(root, '.recordings2-metadata-assignment', !metadata);
    setHidden(root, '[data-recordings2-metadata-assignment-error]', !metadata);

    if (detail || playbackSurface) {
      panels.forEach(function (panel) {
        setElementHidden(panel, panel !== primary, panel === primary ? 'block' : '');
      });
    } else if (metadata) {
      const selected = selectedMetadataPanel(root, panels);
      panels.forEach(function (panel, index) {
        panel.hidden = index !== selected;
        releaseDisplay(panel);
      });
    } else {
      panels.forEach(function (panel) { setElementHidden(panel, true); });
    }

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
    installNestedStyles();
    apply(root);
    observe(root);
    return result;
  }

  global.VdrSuiteRecordings2HeroDetail = Object.freeze({
    enhance,
    __test: Object.freeze({apply, recordingPanel, selectedMetadataPanel})
  });
}(window));