// Read-only native VDR cut-mark detail addon for the Recordings 2 browser owner.
(function (global) {
  'use strict';

  const shared = global.VdrSuiteRecordings2Shared;
  const browserOwner = global.VdrSuiteRecordings2BrowserView;
  const STYLE_ID = 'vdr-suite-recordings2-marks-detail-style';

  function text(value) {
    return value == null ? '' : String(value);
  }

  function node(tag, className, value) {
    const element = document.createElement(tag);
    if (className) element.className = className;
    if (value !== undefined && value !== null) element.textContent = String(value);
    return element;
  }

  function installStyles() {
    if (!document || !document.head || document.getElementById(STYLE_ID)) return;
    const style = document.createElement('style');
    style.id = STYLE_ID;
    style.textContent = [
      '.recordings2-marks-detail{display:grid;gap:.65rem}',
      '.recordings2-marks-summary{display:flex;flex-wrap:wrap;gap:.45rem .9rem;align-items:center}',
      '.recordings2-marks-list{display:grid;gap:.45rem;margin:0;padding:0;list-style:none}',
      '.recordings2-mark{display:grid;grid-template-columns:minmax(7rem,auto) minmax(0,1fr);gap:.25rem .8rem;padding:.55rem .7rem;border:1px solid rgba(148,163,184,.28);border-radius:.55rem}',
      '.recordings2-mark-time{font-weight:700}.recordings2-mark-meta{opacity:.78}',
      '.recordings2-marks-note{opacity:.78}',
      '.recordings2-marks-timeline{position:relative;z-index:2;height:.8rem;margin-top:-.58rem;margin-bottom:.25rem;pointer-events:none}',
      '.recordings2-marks-timeline-marker{position:absolute;top:0;bottom:0;width:3px;border-radius:999px;background:#facc15;box-shadow:0 0 0 1px rgba(15,23,42,.82),0 0 5px rgba(250,204,21,.72);transform:translateX(-50%)}',
      '.recordings2-marks-timeline-marker::after{content:"";position:absolute;top:-.12rem;left:50%;width:.44rem;height:.44rem;border:1px solid rgba(15,23,42,.9);border-radius:50%;background:#fde047;transform:translate(-50%,-35%)}'
    ].join('');
    document.head.appendChild(style);
  }

  function recordingId(recording) {
    return text(recording && (recording.recordingId || recording.id)).trim();
  }

  function backendId(recording, selectedBackendId) {
    return text(selectedBackendId || (recording && recording.backendId)).trim();
  }

  function fetchMarks(recording, selectedBackendId) {
    const client = global.VdrSuiteClientApi;
    const id = recordingId(recording);
    const backend = backendId(recording, selectedBackendId);
    if (!client || typeof client.requestJson !== 'function') {
      return Promise.reject(new Error('recording_marks_client_unavailable'));
    }
    if (!id || !backend) {
      return Promise.reject(new Error('recording_marks_public_identity_unavailable'));
    }
    return client.requestJson('/api/vdr/recordings/marks', {
      query: {
        backend: backend,
        recordingId: id
      },
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }

  function errorText(error) {
    const code = text(error && error.message ? error.message : error);
    if (code.indexOf('recording_marks_capability_unavailable') !== -1) {
      return 'Native Schnittmarken werden von diesem VDR derzeit nicht angeboten.';
    }
    if (code.indexOf('recording_native_state_stale') !== -1) {
      return 'Der Aufnahmestand hat sich geändert. Bitte die Aufnahmedetails neu laden.';
    }
    if (code.indexOf('recording_marks_unreadable') !== -1) {
      return 'Die nativen VDR-Schnittmarken konnten nicht gelesen werden.';
    }
    if (code.indexOf('recording_marks_transport_unavailable') !== -1) {
      return 'Die Verbindung zum nativen VDR-Schnittmarkenleser ist derzeit nicht verfügbar.';
    }
    if (code.indexOf('recording_not_found') !== -1) {
      return 'Die ausgewählte Aufnahme ist im aktuellen Recording-Stand nicht mehr vorhanden.';
    }
    return 'Schnittmarken konnten nicht geladen werden.';
  }

  function createPanel() {
    const section = node('section', 'recordings2-section recordings2-marks-detail');
    section.setAttribute('aria-label', 'Schnitt und Schnittmarken');
    const heading = node('div', 'recordings2-section-title');
    heading.appendChild(node('h4', '', 'Schnitt / Schnittmarken'));
    section.appendChild(heading);
    const body = node('div', 'recordings2-marks-body');
    body.setAttribute('role', 'status');
    body.setAttribute('aria-live', 'polite');
    body.appendChild(node('p', 'recordings2-marks-note', 'Native VDR-Schnittmarken werden geladen …'));
    section.appendChild(body);
    return {section: section, body: body};
  }

  function renderPayload(panel, payload) {
    const body = panel.body;
    body.replaceChildren();
    const marks = payload && Array.isArray(payload.marks) ? payload.marks : [];
    const sequenceCount = Number(payload && payload.sequenceCount || 0);
    const summary = node('div', 'recordings2-marks-summary');
    summary.appendChild(node(
      'strong',
      '',
      marks.length === 1 ? '1 Schnittmarke' : String(marks.length) + ' Schnittmarken'
    ));
    summary.appendChild(node(
      'span',
      '',
      sequenceCount === 1 ? '1 Schnittbereich' : String(sequenceCount) + ' Schnittbereiche'
    ));
    if (payload && payload.inUse === true) {
      summary.appendChild(node('span', '', 'Aufnahme wird aktuell von VDR verwendet'));
    }
    body.appendChild(summary);

    if (!marks.length) {
      body.appendChild(node('p', 'recordings2-marks-note', 'Keine nativen Schnittmarken vorhanden.'));
      return;
    }

    const list = node('ol', 'recordings2-marks-list');
    marks.forEach(function (mark, index) {
      const item = node('li', 'recordings2-mark');
      item.appendChild(node(
        'span',
        'recordings2-mark-time',
        text(mark && mark.timecode) || ('Marke ' + String(index + 1))
      ));
      const meta = [];
      const frame = Number(mark && mark.positionFrame);
      if (Number.isFinite(frame) && frame >= 0) meta.push('Frame ' + String(frame));
      const comment = text(mark && mark.comment).trim();
      if (comment) meta.push(comment);
      item.appendChild(node('span', 'recordings2-mark-meta', meta.join(' · ') || 'Native VDR-Marke'));
      list.appendChild(item);
    });
    body.appendChild(list);
  }

  function timelineDurationSeconds(recording, timeline) {
    const recordingDuration = Number(recording && recording.durationSeconds);
    if (Number.isFinite(recordingDuration) && recordingDuration > 0) return recordingDuration;
    const timelineMaximum = Number(timeline && timeline.max);
    return Number.isFinite(timelineMaximum) && timelineMaximum > 0 ? timelineMaximum : 0;
  }

  function attachTimelineRail(timeline, rail) {
    if (!timeline || !rail) return false;
    if (typeof timeline.insertAdjacentElement === 'function') {
      timeline.insertAdjacentElement('afterend', rail);
      return true;
    }
    if (!timeline.parentNode || typeof timeline.parentNode.appendChild !== 'function') return false;
    timeline.parentNode.appendChild(rail);
    return true;
  }

  function renderTimelineMarks(root, recording, payload) {
    if (!root || typeof root.querySelector !== 'function') return false;
    const timeline = root.querySelector('input[aria-label="Wiedergabeposition"]');
    const marks = payload && Array.isArray(payload.marks) ? payload.marks : [];
    const duration = timelineDurationSeconds(recording, timeline);
    if (!timeline || !marks.length || !(duration > 0)) return false;

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
      const bounded = Math.min(duration, positionSeconds);
      marker.style.left = ((bounded / duration) * 100).toFixed(5) + '%';
      marker.setAttribute('aria-hidden', 'true');
      marker.dataset.positionSeconds = String(positionSeconds);
      marker.dataset.positionFrame = String(Number(mark && mark.positionFrame));
      const timecode = text(mark && mark.timecode).trim() || ('Marke ' + String(index + 1));
      const comment = text(mark && mark.comment).trim();
      marker.title = comment ? timecode + ' · ' + comment : timecode;
      rail.appendChild(marker);
    });

    if (!rail.children || rail.children.length === 0 || !attachTimelineRail(timeline, rail)) return false;
    if (timeline.dataset) {
      timeline.dataset.nativeMarksVisible = 'true';
      timeline.dataset.nativeMarksCount = String(rail.children.length);
    }
    return true;
  }

  function renderError(panel, error) {
    panel.body.replaceChildren(node('p', 'recordings2-marks-note', errorText(error)));
    panel.body.children[0].setAttribute('role', 'status');
  }

  function enhance(root, recording, selectedBackendId) {
    if (!root || !root.dataset || root.dataset.recordings2MarksDetail === 'true') {
      return Promise.resolve(false);
    }
    root.dataset.recordings2MarksDetail = 'true';
    installStyles();
    const panel = createPanel();
    root.appendChild(panel.section);
    return fetchMarks(recording, selectedBackendId).then(function (payload) {
      if (!payload || payload.availability !== 'available') {
        throw new Error('recording_marks_invalid_payload');
      }
      panel.section.dataset.marksRevision = text(payload.marksRevision);
      renderPayload(panel, payload);
      renderTimelineMarks(root, recording, payload);
      return true;
    }).catch(function (error) {
      renderError(panel, error);
      return false;
    });
  }

  function enhanceCurrentDetail(options) {
    if (!shared || !options || typeof options.getState !== 'function') return;
    const state = options.getState();
    if (!state || !state.selectedRecording) return;
    const target = typeof shared.mountTarget === 'function' ? shared.mountTarget() : null;
    const root = target && typeof target.querySelector === 'function'
      ? target.querySelector('.recordings2-detail')
      : null;
    if (root) enhance(root, state.selectedRecording, state.backendId);
  }

  if (browserOwner && typeof browserOwner.create === 'function') {
    global.VdrSuiteRecordings2BrowserView = Object.freeze({
      create: function (options) {
        const view = browserOwner.create(options);
        return Object.freeze({
          renderLoading: view.renderLoading,
          renderError: view.renderError,
          renderFolder: view.renderFolder,
          renderDetail: function () {
            view.renderDetail();
            enhanceCurrentDetail(options);
          },
          destroy: view.destroy
        });
      },
      createRecordingCard: browserOwner.createRecordingCard
    });
  }

  global.VdrSuiteRecordings2MarksDetail = Object.freeze({enhance: enhance, fetchMarks: fetchMarks, renderPayload: renderPayload, renderTimelineMarks: renderTimelineMarks, errorText: errorText});
}(window));
