// Phase 65.D Slice 3 Recording timeline/continuity normalization.
//
// This adapter decorates the existing persistent Recording playback owner. It
// does not create MediaSessions, own transports, restart media, or classify
// failures. It normalizes the public absolute Recording position across the
// progressive/HLS transport boundary, exposes the presentation base alongside
// canonical lifecycle snapshots, and protects a user-owned progressive timeline
// preview from playback timeupdate until commit/cancel.
(function (global) {
  'use strict';

  const marker = '__vdrSuitePlaybackTimelineContinuityBound';
  if (!global || !global.document || global[marker] === true) return;

  const descriptor = Object.getOwnPropertyDescriptor(global, 'VdrSuiteRecordings2Playback');
  if (!descriptor || typeof descriptor.get !== 'function' || typeof descriptor.set !== 'function') return;

  function text(value) {
    return value === undefined || value === null ? '' : String(value);
  }

  function formatTime(value) {
    const seconds = Math.max(0, Math.floor(Number(value) || 0));
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const remaining = seconds % 60;
    return String(hours).padStart(2, '0') + ':' +
      String(minutes).padStart(2, '0') + ':' +
      String(remaining).padStart(2, '0');
  }

  function find(root, selector) {
    return root && typeof root.querySelector === 'function'
      ? root.querySelector(selector)
      : null;
  }

  function decoratePanel(panel) {
    if (!panel || !panel.element || typeof panel.position !== 'function' ||
        typeof panel.snapshot !== 'function' || typeof panel.subscribe !== 'function') return panel;
    if (panel.__vdrSuitePlaybackTimelineContinuityDecorated === true) return panel;

    const shell = panel.element;
    let previewOwned = false;
    let previewTargetSeconds = 0;
    let boundTimeline = null;
    let boundVideo = null;
    let observer = null;
    let disposed = false;

    function activeFallbackOwner() {
      const fallback = find(shell, '.recordings2-recording-fallback-shell');
      return fallback && fallback.__vdrSuiteRecordingFallbackOwner
        ? fallback.__vdrSuiteRecordingFallbackOwner
        : null;
    }

    function activePositionOwner() {
      const fallback = activeFallbackOwner();
      return fallback && typeof fallback.position === 'function' ? fallback : panel;
    }

    function currentVideo() {
      return find(shell, 'video');
    }

    function currentTimeline() {
      return find(shell, 'input[aria-label="Wiedergabeposition"]');
    }

    function position() {
      const owner = activePositionOwner();
      const value = Number(owner && owner.position());
      return Number.isFinite(value) && value >= 0 ? Math.floor(value) : 0;
    }

    function presentationBasePosition() {
      const video = currentVideo();
      const local = Number(video && video.currentTime);
      const localSeconds = Number.isFinite(local) && local > 0 ? Math.floor(local) : 0;
      return Math.max(0, position() - localSeconds);
    }

    function duration() {
      const fallback = activeFallbackOwner();
      const owner = fallback && typeof fallback.duration === 'function' ? fallback : panel;
      if (!owner || typeof owner.duration !== 'function') return 0;
      const value = Number(owner.duration());
      return Number.isFinite(value) && value > 0 ? Math.floor(value) : 0;
    }

    function timelineMaximum(timeline) {
      const maximum = Number(timeline && timeline.max);
      return Number.isFinite(maximum) && maximum >= 0 ? Math.floor(maximum) : null;
    }

    function clampPreview(value) {
      const target = Math.floor(Number(value));
      if (!Number.isFinite(target)) return null;
      const timeline = currentTimeline();
      const minimumValue = Number(timeline && timeline.min);
      const minimum = Number.isFinite(minimumValue) ? Math.floor(minimumValue) : 0;
      const maximum = timelineMaximum(timeline);
      return Math.max(minimum, maximum === null ? target : Math.min(maximum, target));
    }

    function reassertPreview() {
      if (!previewOwned || disposed || activeFallbackOwner()) return;
      const timeline = currentTimeline();
      const target = clampPreview(previewTargetSeconds);
      if (!timeline || timeline.disabled || target === null) return;
      previewTargetSeconds = target;
      timeline.value = String(target);
      const label = find(shell, '.recordings2-playback-position');
      if (label) {
        const total = duration();
        label.textContent = formatTime(target) + ' / ' +
          (total > 0 ? formatTime(total) : '--:--:--');
      }
    }

    function handlePlaybackTimeupdate() {
      if (!previewOwned || activeFallbackOwner()) return;
      reassertPreview();
      // Other already-installed owner decorators may also observe timeupdate.
      // Reassert after the full listener stack as well, before the next paint.
      Promise.resolve().then(function () {
        if (previewOwned && !disposed) reassertPreview();
      });
    }

    function bindTimeline() {
      const timeline = currentTimeline();
      if (timeline === boundTimeline) return false;
      boundTimeline = timeline || null;
      previewOwned = false;
      if (!timeline || activeFallbackOwner() || typeof timeline.addEventListener !== 'function') return true;

      timeline.addEventListener('input', function () {
        if (timeline.disabled) return;
        const target = clampPreview(timeline.value);
        if (target === null) return;
        previewOwned = true;
        previewTargetSeconds = target;
        reassertPreview();
      });
      timeline.addEventListener('change', function () {
        // The canonical owner registered its commit handler before this adapter.
        // Releasing ownership here lets committed playback updates take over.
        previewOwned = false;
      });
      timeline.addEventListener('pointercancel', function () {
        previewOwned = false;
      });
      return true;
    }

    function bindVideo() {
      const video = currentVideo();
      if (video === boundVideo) return false;
      boundVideo = video || null;
      if (!video || typeof video.addEventListener !== 'function') return true;
      video.addEventListener('timeupdate', handlePlaybackTimeupdate);
      return true;
    }

    function normalizedSnapshot(snapshot) {
      const source = snapshot && typeof snapshot === 'object' ? snapshot : {};
      return Object.freeze(Object.assign({}, source, {
        presentationBasePositionSeconds: presentationBasePosition()
      }));
    }

    function snapshot() {
      return normalizedSnapshot(panel.snapshot());
    }

    function subscribe(listener) {
      if (typeof listener !== 'function') return function () {};
      return panel.subscribe(function (ownerSnapshot) {
        listener(normalizedSnapshot(ownerSnapshot));
      });
    }

    function destroy() {
      disposed = true;
      previewOwned = false;
      if (observer && typeof observer.disconnect === 'function') observer.disconnect();
      observer = null;
      if (typeof panel.destroy === 'function') return panel.destroy();
    }

    bindTimeline();
    bindVideo();

    if (typeof global.MutationObserver === 'function') {
      observer = new global.MutationObserver(function () {
        // DOM movement is transport-local observation only. Rebinding to a
        // different element never publishes lifecycle/continuity state, and
        // reparenting the same element is therefore continuity-neutral.
        bindTimeline();
        bindVideo();
      });
      observer.observe(shell, {childList: true, subtree: true});
    }

    const wrapped = {};
    Object.keys(panel).forEach(function (key) { wrapped[key] = panel[key]; });
    wrapped.position = position;
    wrapped.presentationBasePosition = presentationBasePosition;
    wrapped.snapshot = snapshot;
    wrapped.subscribe = subscribe;
    wrapped.destroy = destroy;
    wrapped.__vdrSuitePlaybackTimelineContinuityDecorated = true;
    return Object.freeze(wrapped);
  }

  let cachedSource = null;
  let cachedDecorated = null;

  function decoratePlayback(value) {
    const source = value && typeof value === 'object' ? value : {};
    if (source === cachedSource && cachedDecorated) return cachedDecorated;
    if (typeof source.createPanel !== 'function' ||
        source.__vdrSuitePlaybackTimelineContinuityDecorated === true) {
      cachedSource = source;
      cachedDecorated = source;
      return source;
    }

    const decorated = {};
    Object.keys(source).forEach(function (key) { decorated[key] = source[key]; });
    const factory = source.createPanel;
    decorated.createPanel = function () {
      return decoratePanel(factory.apply(source, arguments));
    };
    decorated.__vdrSuitePlaybackTimelineContinuityDecorated = true;
    cachedSource = source;
    cachedDecorated = Object.freeze(decorated);
    return cachedDecorated;
  }

  Object.defineProperty(global, 'VdrSuiteRecordings2Playback', {
    configurable: descriptor.configurable !== false,
    enumerable: descriptor.enumerable !== false,
    get: function () { return decoratePlayback(descriptor.get.call(global)); },
    set: function (value) {
      cachedSource = null;
      cachedDecorated = null;
      descriptor.set.call(global, value);
    }
  });

  global[marker] = true;
  global.VdrSuitePlaybackTimelineContinuity = Object.freeze({
    formatTime: formatTime
  });
}(window));
