// Read-only native VDR cut-mark detail addon for the Recordings 2 browser owner.
(function (global) {
  'use strict';

  const shared = global.VdrSuiteRecordings2Shared;
  const browserOwner = global.VdrSuiteRecordings2BrowserView;
  const STYLE_ID = 'vdr-suite-recordings2-marks-detail-style';
  let timelineRuntimePromise = null;

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
      '.recordings2-marks-note{opacity:.78}'
    ].join('');
    document.head.appendChild(style);
  }

  function timelineRuntime() {
    const runtime = global.VdrSuiteRecordings2MarksTimeline;
    return runtime && typeof runtime.bind === 'function' ? runtime : null;
  }

  function ensureTimelineRuntime() {
    const ready = timelineRuntime();
    if (ready) return Promise.resolve(ready);
    if (timelineRuntimePromise) return timelineRuntimePromise;
    if (typeof global.loadVdrSuiteDeferredRuntime !== 'function') return Promise.resolve(null);

    timelineRuntimePromise = global.loadVdrSuiteDeferredRuntime(
      'vdr-suite-recordings2-marks-timeline-runtime',
      '/frontend/recordings2-marks-timeline.js',
      function () { return Boolean(timelineRuntime()); }
    ).then(function () {
      return timelineRuntime();
    }).catch(function (error) {
      timelineRuntimePromise = null;
      if (global.console && typeof global.console.error === 'function') {
        global.console.error('VDR-Suite Recording marks timeline runtime failed', error);
      }
      return null;
    });
    return timelineRuntimePromise;
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
      ensureTimelineRuntime().then(function (timeline) {
        if (timeline) timeline.bind(root, recording, payload);
      });
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

  global.VdrSuiteRecordings2MarksDetail = Object.freeze({
    enhance: enhance,
    fetchMarks: fetchMarks,
    renderPayload: renderPayload,
    errorText: errorText
  });
}(window));
