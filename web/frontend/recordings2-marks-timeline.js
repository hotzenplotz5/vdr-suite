// Read-only native VDR cut-mark decoration for the canonical Recording timeline.
(function (global) {
  'use strict';

  const STYLE_ID = 'vdr-suite-recordings2-marks-timeline-style';
  const TIMELINE_SELECTOR = 'input[aria-label="Wiedergabeposition"]';
  const REPLACEABLE_FALLBACK_TRANSPORT_CLASS = 'recordings2-recording-fallback-transport';

  function text(value) {
    return value == null ? '' : String(value);
  }

  function node(tag, className) {
    const element = global.document.createElement(tag);
    if (className) element.className = className;
    return element;
  }

  function installStyles() {
    const document = global.document;
    if (!document || !document.head || document.getElementById(STYLE_ID)) return;
    const style = document.createElement('style');
    style.id = STYLE_ID;
    style.textContent = [
      '.recordings2-marks-timeline{position:relative;z-index:2;height:.8rem;margin-top:-.58rem;margin-bottom:.25rem;pointer-events:none}',
      '.recordings2-marks-timeline-marker{position:absolute;top:0;bottom:0;width:3px;border-radius:999px;background:#facc15;box-shadow:0 0 0 1px rgba(15,23,42,.82),0 0 5px rgba(250,204,21,.72);transform:translateX(-50%)}',
      '.recordings2-marks-timeline-marker::after{content:"";position:absolute;top:-.12rem;left:50%;width:.44rem;height:.44rem;border:1px solid rgba(15,23,42,.9);border-radius:50%;background:#fde047;transform:translate(-50%,-35%)}'
    ].join('');
    document.head.appendChild(style);
  }

  function hasClass(element, className) {
    if (!element) return false;
    if (element.classList && typeof element.classList.contains === 'function') {
      return element.classList.contains(className);
    }
    return text(element.className).split(/\s+/).filter(Boolean).indexOf(className) !== -1;
  }

  function insideReplaceableFallbackTransport(element, boundary) {
    let current = element && element.parentNode;
    while (current && current !== boundary) {
      if (hasClass(current, REPLACEABLE_FALLBACK_TRANSPORT_CLASS)) return true;
      current = current.parentNode;
    }
    return false;
  }

  function canonicalTimeline(root) {
    if (!root || typeof root.querySelector !== 'function') return null;
    const owner = root.__vdrSuiteRecordingPlaybackOwner;
    const ownerElement = owner && owner.element;
    const scope = ownerElement && typeof ownerElement.querySelectorAll === 'function'
      ? ownerElement
      : root;

    if (scope && typeof scope.querySelectorAll === 'function') {
      const timelines = scope.querySelectorAll(TIMELINE_SELECTOR);
      for (let index = 0; index < timelines.length; index += 1) {
        const timeline = timelines[index];
        if (!insideReplaceableFallbackTransport(timeline, scope)) return timeline;
      }
    }

    if (ownerElement && typeof ownerElement.querySelector === 'function') {
      const timeline = ownerElement.querySelector(TIMELINE_SELECTOR);
      if (timeline && !insideReplaceableFallbackTransport(timeline, ownerElement)) return timeline;
    }
    return root.querySelector(TIMELINE_SELECTOR);
  }

  function durationSeconds(recording, timeline) {
    const recordingDuration = Number(recording && recording.durationSeconds);
    if (Number.isFinite(recordingDuration) && recordingDuration > 0) return recordingDuration;
    const maximum = Number(timeline && timeline.max);
    return Number.isFinite(maximum) && maximum > 0 ? maximum : 0;
  }

  function attachRail(timeline, rail) {
    if (!timeline || !rail) return false;
    if (typeof timeline.insertAdjacentElement === 'function') {
      timeline.insertAdjacentElement('afterend', rail);
      return true;
    }
    if (!timeline.parentNode || typeof timeline.parentNode.appendChild !== 'function') return false;
    timeline.parentNode.appendChild(rail);
    return true;
  }

  function marksRevision(payload, marks) {
    const revision = text(payload && payload.marksRevision).trim();
    if (revision) return revision;
    return marks.map(function (mark) {
      return text(mark && mark.positionFrame) + ':' + text(mark && mark.positionSeconds);
    }).join('|');
  }

  function render(root, recording, payload) {
    if (!root || typeof root.querySelector !== 'function') return false;
    const timeline = canonicalTimeline(root);
    const marks = payload && Array.isArray(payload.marks) ? payload.marks : [];
    const duration = durationSeconds(recording, timeline);
    if (!timeline || !marks.length || !(duration > 0)) return false;

    const revision = marksRevision(payload, marks);
    const existing = root.querySelector('.recordings2-marks-timeline');
    if (existing && existing.parentNode === timeline.parentNode && timeline.dataset &&
        timeline.dataset.nativeMarksRevision === revision) {
      return true;
    }

    installStyles();
    const rail = node('div', 'recordings2-marks-timeline');
    rail.setAttribute('role', 'img');
    rail.setAttribute('aria-label', marks.length === 1
      ? '1 native Schnittmarke auf der Wiedergabe-Zeitlinie'
      : String(marks.length) + ' native Schnittmarken auf der Wiedergabe-Zeitlinie');
    rail.dataset.durationSeconds = String(duration);

    marks.forEach(function (mark, index) {
      const positionSeconds = Number(mark && mark.positionSeconds);
      if (!Number.isFinite(positionSeconds) || positionSeconds < 0) return;
      const marker = node('span', 'recordings2-marks-timeline-marker');
      marker.style.left = ((Math.min(duration, positionSeconds) / duration) * 100).toFixed(5) + '%';
      marker.setAttribute('aria-hidden', 'true');
      marker.dataset.positionSeconds = String(positionSeconds);
      marker.dataset.positionFrame = String(Number(mark && mark.positionFrame));
      const timecode = text(mark && mark.timecode).trim() || ('Marke ' + String(index + 1));
      const comment = text(mark && mark.comment).trim();
      marker.title = comment ? timecode + ' · ' + comment : timecode;
      rail.appendChild(marker);
    });

    if (!rail.children || rail.children.length === 0 || !attachRail(timeline, rail)) return false;
    if (timeline.dataset) {
      timeline.dataset.nativeMarksVisible = 'true';
      timeline.dataset.nativeMarksCount = String(rail.children.length);
      timeline.dataset.nativeMarksRevision = revision;
    }
    return true;
  }

  function release(root) {
    if (!root) return;
    const unsubscribe = root.__vdrSuiteRecordingMarksTimelineUnsubscribe;
    root.__vdrSuiteRecordingMarksTimelineUnsubscribe = null;
    if (typeof unsubscribe === 'function') {
      try { unsubscribe(); } catch (error) {}
    }
  }

  function bind(root, recording, payload) {
    const rendered = render(root, recording, payload);
    const owner = root && root.__vdrSuiteRecordingPlaybackOwner;
    release(root);
    if (!owner || typeof owner.subscribe !== 'function') return rendered;

    const unsubscribe = owner.subscribe(function (snapshot) {
      if (snapshot && snapshot.transition === 'destroyed') {
        release(root);
        return;
      }
      render(root, recording, payload);
    });
    root.__vdrSuiteRecordingMarksTimelineUnsubscribe =
      typeof unsubscribe === 'function' ? unsubscribe : null;
    return true;
  }

  global.VdrSuiteRecordings2MarksTimeline = Object.freeze({
    bind: bind,
    render: render,
    release: release
  });
}(window));
