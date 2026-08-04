'use strict';

(function (global) {
  const STYLE_ID = 'vdr-suite-epg-detail-desktop-focus-style';
  const DESKTOP_QUERY = '(min-width: 1100px)';
  const EXPANDED_CLASS = 'epg-detail-expanded';
  const TIMELINE_CLASS = 'epg-timeline-foreground';

  function ensureStyles() {
    if (document.getElementById(STYLE_ID)) return;

    const style = document.createElement('style');
    style.id = STYLE_ID;
    style.textContent = [
      '.epg-metadata-tabs{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));width:100%;max-width:100%;gap:.26rem;overflow:visible;margin:.12rem 0 .72rem;padding:.14rem;border-radius:.62rem;box-sizing:border-box}',
      '.epg-metadata-tab{min-width:0;width:100%;padding:.42rem .28rem;border-radius:.48rem;font-size:.76rem;line-height:1.15;text-align:center;white-space:normal;overflow-wrap:anywhere}',
      '@media(max-width:380px){.epg-metadata-tabs{grid-template-columns:repeat(2,minmax(0,1fr))}}',
      '#detail-data.channels2-mount .channels2-detail.has-artwork>.channels2-artwork,#detail-data.channels2-mount .channels2-detail.epg-has-artwork>.epg-detail-artwork{grid-column:1;grid-row:1}',
      '#detail-data.channels2-mount .channels2-detail.has-artwork>.epg-detail-hero,#detail-data.channels2-mount .channels2-detail.epg-has-artwork>.epg-detail-hero{grid-column:2;grid-row:1;min-width:0}',
      '@media(max-width:720px){#detail-data.channels2-mount .channels2-detail.has-artwork>.channels2-artwork,#detail-data.channels2-mount .channels2-detail.epg-has-artwork>.epg-detail-artwork,#detail-data.channels2-mount .channels2-detail.has-artwork>.epg-detail-hero,#detail-data.channels2-mount .channels2-detail.epg-has-artwork>.epg-detail-hero{grid-column:1;grid-row:auto}}',
      '@media(min-width:1100px){',
      '.epg-workbench{position:relative;isolation:isolate;overflow:visible}',
      '.epg-workbench-main{position:relative;z-index:2;transition:filter .18s ease}',
      '.epg-detail-sidebar{position:relative;z-index:3;min-width:0}',
      '.epg-side-detail{width:100%;max-width:none;transform-origin:right top;transition:width .2s ease,box-shadow .2s ease,filter .2s ease}',
      '.epg-workbench.epg-detail-expanded .epg-detail-sidebar{z-index:20}',
      '.epg-workbench.epg-detail-expanded .epg-side-detail{position:absolute!important;right:0!important;left:auto!important;top:0!important;width:min(42rem,calc(100vw - 3rem));max-height:calc(100vh - 2rem);overflow:auto;overscroll-behavior:contain;filter:drop-shadow(0 1.1rem 2rem rgba(2,6,23,.72))}',
      '.epg-workbench.epg-detail-expanded .epg-event-detail{border-color:rgba(56,189,248,.7);box-shadow:0 0 0 1px rgba(56,189,248,.18),0 1rem 2.5rem rgba(2,6,23,.58)}',
      '.epg-workbench.epg-detail-expanded .epg-metadata-facts{grid-template-columns:repeat(3,minmax(0,1fr))}',
      '.epg-workbench.epg-detail-expanded .epg-metadata-cast{grid-template-columns:repeat(3,minmax(0,1fr))}',
      '.epg-workbench.epg-detail-expanded .epg-metadata-gallery-thumbs{grid-template-columns:repeat(4,minmax(0,1fr))}',
      '.epg-workbench.epg-timeline-foreground .epg-workbench-main{z-index:30}',
      '.epg-workbench.epg-timeline-foreground .epg-detail-sidebar{z-index:1}',
      '}',
      '@media(prefers-reduced-motion:reduce){.epg-workbench-main,.epg-side-detail{transition:none!important}}'
    ].join('');
    document.head.appendChild(style);
  }

  function closest(element, selector) {
    return element && typeof element.closest === 'function'
      ? element.closest(selector)
      : null;
  }

  function desktopMatches() {
    if (typeof global.matchMedia === 'function') {
      return global.matchMedia(DESKTOP_QUERY).matches;
    }
    return Number(global.innerWidth || 0) >= 1100;
  }

  function setExpanded(workbench, expanded) {
    if (!workbench || !desktopMatches()) return false;

    workbench.classList.toggle(EXPANDED_CLASS, expanded);
    workbench.classList.toggle(TIMELINE_CLASS, !expanded);
    workbench.dataset.epgDetailExpanded = expanded ? 'true' : 'false';
    return true;
  }

  function workbenchFromDetailTarget(target) {
    const detail = closest(target, '.epg-side-detail');
    return detail ? closest(detail, '.epg-workbench') : null;
  }

  function workbenchFromTimelineTarget(target) {
    const main = closest(target, '.epg-workbench-main');
    return main ? closest(main, '.epg-workbench') : null;
  }

  function handleDetailInteraction(event) {
    const workbench = workbenchFromDetailTarget(event.target);
    if (workbench) setExpanded(workbench, true);
  }

  function handleTimelineInteraction(event) {
    const workbench = workbenchFromTimelineTarget(event.target);
    if (workbench) setExpanded(workbench, false);
  }

  function handleKeydown(event) {
    if (event.key !== 'Escape') return;
    const workbench = closest(event.target, '.epg-workbench');
    if (workbench) setExpanded(workbench, false);
  }

  function resetForViewport() {
    if (desktopMatches() || typeof document.querySelectorAll !== 'function') return;
    document.querySelectorAll('.epg-workbench').forEach(function (workbench) {
      workbench.classList.remove(EXPANDED_CLASS, TIMELINE_CLASS);
      workbench.dataset.epgDetailExpanded = 'false';
    });
  }

  function install() {
    ensureStyles();
    document.addEventListener('pointerdown', handleDetailInteraction, true);
    document.addEventListener('focusin', handleDetailInteraction, true);
    document.addEventListener('pointerover', handleTimelineInteraction, true);
    document.addEventListener('focusin', handleTimelineInteraction, true);
    document.addEventListener('keydown', handleKeydown, true);

    if (typeof global.matchMedia === 'function') {
      const media = global.matchMedia(DESKTOP_QUERY);
      if (typeof media.addEventListener === 'function') {
        media.addEventListener('change', resetForViewport);
      }
    }

    document.documentElement.dataset.epgDetailDesktopFocus = 'true';
  }

  install();

  global.VdrSuiteEpgDetailDesktopFocus = Object.freeze({
    installed: function () {
      return document.documentElement.dataset.epgDetailDesktopFocus === 'true';
    },
    expand: function (workbench) {
      return setExpanded(workbench, true);
    },
    collapse: function (workbench) {
      return setExpanded(workbench, false);
    }
  });
}(window));