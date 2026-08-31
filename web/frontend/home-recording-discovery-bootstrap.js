// Phase 66.5 follow-up: deferred recording discovery plus shared Home mouse-drag affordance.
//
// The recording rails are rendered lazily, so the mouse interaction is delegated
// from this eager Home bootstrap. It is presentation-only: discovery rails move
// their existing scroll position and the Live-TV Hero delegates a completed drag
// to its existing browse-only selectOffset() API. Touch/swipe, playback ownership,
// recording identity and navigation owners remain unchanged.
(function (global) {
  'use strict';

  const doc = global && global.document ? global.document : null;
  const RAIL_SELECTOR = '.media-home-discovery-rail, .media-home-series-season-rail';
  const HERO_SELECTOR = '.media-home-hero.media-home-live-hero-active[data-home-zone="hero"]';
  const DRAG_CLASS = 'media-home-mouse-dragging';
  const START_THRESHOLD = 8;
  const HERO_SWITCH_THRESHOLD = 48;
  const HORIZONTAL_DOMINANCE = 1.15;
  const CLICK_SUPPRESS_MS = 420;

  let documentBound = false;
  let activeDrag = null;
  let suppressedTarget = null;
  let suppressClickUntil = 0;

  function discoveryReady() {
    return Boolean(
      global.VdrSuiteHomeRecordingDiscovery &&
      typeof global.VdrSuiteHomeRecordingDiscovery.install === 'function'
    );
  }

  function closestTarget(target, selector) {
    return target && typeof target.closest === 'function' ? target.closest(selector) : null;
  }

  function dragTarget(event) {
    const target = event && event.target;
    const rail = closestTarget(target, RAIL_SELECTOR);
    if (rail) return {kind: 'rail', element: rail};
    const hero = closestTarget(target, HERO_SELECTOR);
    if (hero) return {kind: 'hero', element: hero};
    return null;
  }

  function addDraggingClass(element) {
    if (element && element.classList && typeof element.classList.add === 'function') {
      element.classList.add(DRAG_CLASS);
    }
  }

  function removeDraggingClass(element) {
    if (element && element.classList && typeof element.classList.remove === 'function') {
      element.classList.remove(DRAG_CLASS);
    }
  }

  function capturePointer(element, pointerId) {
    if (!element || typeof element.setPointerCapture !== 'function') return;
    try { element.setPointerCapture(pointerId); } catch (_) {}
  }

  function releasePointer(element, pointerId) {
    if (!element || typeof element.releasePointerCapture !== 'function') return;
    try { element.releasePointerCapture(pointerId); } catch (_) {}
  }

  function resetDrag(release) {
    if (!activeDrag) return;
    const current = activeDrag;
    activeDrag = null;
    removeDraggingClass(current.element);
    if (release) releasePointer(current.element, current.pointerId);
  }

  function horizontalGesture(deltaX, deltaY, threshold) {
    return Math.abs(deltaX) >= threshold &&
      Math.abs(deltaX) > Math.abs(deltaY) * HORIZONTAL_DOMINANCE;
  }

  function suppressNextClick(element) {
    suppressedTarget = element;
    suppressClickUntil = Date.now() + CLICK_SUPPRESS_MS;
  }

  function handlePointerDown(event) {
    if (!event || event.pointerType !== 'mouse' || Number(event.button) !== 0) return;
    const target = dragTarget(event);
    if (!target) return;
    activeDrag = {
      kind: target.kind,
      element: target.element,
      pointerId: event.pointerId,
      startX: Number(event.clientX),
      startY: Number(event.clientY),
      startScrollLeft: Number(target.element.scrollLeft) || 0,
      dragging: false
    };
  }

  function handlePointerMove(event) {
    if (!activeDrag || !event || event.pointerId !== activeDrag.pointerId) return;
    const deltaX = Number(event.clientX) - activeDrag.startX;
    const deltaY = Number(event.clientY) - activeDrag.startY;

    if (!activeDrag.dragging) {
      if (Math.abs(deltaX) < START_THRESHOLD && Math.abs(deltaY) < START_THRESHOLD) return;
      if (!horizontalGesture(deltaX, deltaY, START_THRESHOLD)) {
        resetDrag(false);
        return;
      }
      activeDrag.dragging = true;
      addDraggingClass(activeDrag.element);
      capturePointer(activeDrag.element, activeDrag.pointerId);
    }

    if (activeDrag.kind === 'rail') {
      activeDrag.element.scrollLeft = activeDrag.startScrollLeft - deltaX;
    }
    if (typeof event.preventDefault === 'function') event.preventDefault();
  }

  function finishPointer(event, cancelled) {
    if (!activeDrag || !event || event.pointerId !== activeDrag.pointerId) return;
    const current = activeDrag;
    const deltaX = Number(event.clientX) - current.startX;
    const deltaY = Number(event.clientY) - current.startY;
    const dragged = current.dragging;
    resetDrag(true);

    if (!dragged || cancelled) return;
    suppressNextClick(current.element);
    if (typeof event.preventDefault === 'function') event.preventDefault();

    if (current.kind === 'hero' && horizontalGesture(deltaX, deltaY, HERO_SWITCH_THRESHOLD)) {
      const hero = global.VdrSuiteHomeLiveHero;
      if (hero && typeof hero.selectOffset === 'function') {
        hero.selectOffset(deltaX < 0 ? 1 : -1);
      }
    }
  }

  function handlePointerUp(event) { finishPointer(event, false); }
  function handlePointerCancel(event) { finishPointer(event, true); }

  function handleClickCapture(event) {
    if (!suppressedTarget || Date.now() > suppressClickUntil) {
      suppressedTarget = null;
      suppressClickUntil = 0;
      return;
    }
    const target = event && event.target;
    const rail = closestTarget(target, RAIL_SELECTOR);
    const hero = closestTarget(target, HERO_SELECTOR);
    if (rail !== suppressedTarget && hero !== suppressedTarget) return;
    suppressedTarget = null;
    suppressClickUntil = 0;
    if (typeof event.preventDefault === 'function') event.preventDefault();
    if (typeof event.stopImmediatePropagation === 'function') event.stopImmediatePropagation();
    else if (typeof event.stopPropagation === 'function') event.stopPropagation();
  }

  function installStyles() {
    if (!doc || !doc.head || typeof doc.createElement !== 'function') return false;
    if (typeof doc.getElementById === 'function' && doc.getElementById('vdr-suite-home-mouse-drag-style')) return true;
    const style = doc.createElement('style');
    style.id = 'vdr-suite-home-mouse-drag-style';
    style.textContent = [
      '.media-home-discovery-rail,.media-home-series-season-rail{scrollbar-width:none;-ms-overflow-style:none;cursor:grab}',
      '.media-home-discovery-rail::-webkit-scrollbar,.media-home-series-season-rail::-webkit-scrollbar{display:none;width:0;height:0}',
      '.media-home-live-hero-active{cursor:grab}',
      '.media-home-mouse-dragging{cursor:grabbing!important;user-select:none!important;scroll-snap-type:none!important}',
      '.media-home-mouse-dragging *{cursor:grabbing!important;user-select:none!important}'
    ].join('');
    doc.head.appendChild(style);
    return true;
  }

  function installMouseDrag() {
    if (documentBound || !doc || typeof doc.addEventListener !== 'function') return false;
    documentBound = true;
    installStyles();
    doc.addEventListener('pointerdown', handlePointerDown);
    doc.addEventListener('pointermove', handlePointerMove, {passive: false});
    doc.addEventListener('pointerup', handlePointerUp);
    doc.addEventListener('pointercancel', handlePointerCancel);
    doc.addEventListener('click', handleClickCapture, true);
    return true;
  }

  function load() {
    if (discoveryReady()) return Promise.resolve(true);
    if (typeof global.loadVdrSuiteDeferredRuntime !== 'function') {
      return Promise.resolve(false);
    }
    return Promise.resolve(global.loadVdrSuiteDeferredRuntime(
      'vdr-suite-home-recording-discovery-runtime',
      '/frontend/home-recording-discovery.js',
      discoveryReady
    )).then(discoveryReady).catch(function (error) {
      if (global.console && typeof global.console.error === 'function') {
        global.console.error('VDR-Suite Home Recording Discovery runtime failed', error);
      }
      return false;
    });
  }

  global.VdrSuiteHomeRecordingDiscoveryBootstrap = Object.freeze({
    load: load,
    installMouseDrag: installMouseDrag
  });
  installMouseDrag();
  load();
}(window));
