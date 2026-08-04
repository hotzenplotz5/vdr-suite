// Async metadata adapter for the Recordings 2 detail view.
(function (global) {
  'use strict';

  const shared = global.VdrSuiteRecordings2Shared;
  const metadataView = global.VdrSuiteRecordings2MetadataView;
  let assignmentRuntimePromise;
  if (!shared || !metadataView || typeof metadataView.mount !== 'function') {
    console.error('VDR-Suite Recordings 2 metadata runtime dependencies are unavailable');
    return;
  }

  function fetchMetadata(recording, backendId) {
    const backendNativeId = shared.text(shared.first(recording, ['backendNativeId'], ''));
    if (!backendNativeId) {
      return Promise.reject(new Error('Die Aufnahme besitzt keine stabile Backend-Identität.'));
    }
    const api = shared.clientApi();
    if (!api || typeof api.requestJson !== 'function') {
      return Promise.reject(new Error('Client API für Aufnahme-Metadaten ist nicht verfügbar.'));
    }
    return api.requestJson('/api/vdr/recordings/metadata', {
      query: {
        backend: shared.text(backendId) || 'default',
        backendNativeId: backendNativeId,
        _: String(Date.now())
      },
      cache: 'no-store',
      credentials: 'same-origin'
    });
  }

  function loadAssignmentRuntime() {
    if (global.VdrSuiteRecordings2MetadataAssignment) {
      return Promise.resolve(global.VdrSuiteRecordings2MetadataAssignment);
    }
    if (assignmentRuntimePromise) return assignmentRuntimePromise;

    assignmentRuntimePromise = new Promise(function (resolve, reject) {
      const existing = document.getElementById(
        'vdr-suite-recordings2-metadata-assignment-runtime'
      );
      const script = existing || document.createElement('script');
      function ready() {
        const runtime = global.VdrSuiteRecordings2MetadataAssignment;
        if (runtime && typeof runtime.mount === 'function') resolve(runtime);
        else reject(new Error('Manuelle Metadaten-Runtime wurde nicht initialisiert.'));
      }
      if (existing && global.VdrSuiteRecordings2MetadataAssignment) {
        ready();
        return;
      }
      script.id = 'vdr-suite-recordings2-metadata-assignment-runtime';
      script.async = false;
      script.addEventListener('load', ready, {once: true});
      script.addEventListener('error', function () {
        reject(new Error('Manuelle Metadaten-Runtime konnte nicht geladen werden.'));
      }, {once: true});
      if (!existing) {
        script.src = '/frontend/recordings2-metadata-assignment.js';
        document.head.appendChild(script);
      }
    }).catch(function (error) {
      assignmentRuntimePromise = null;
      throw error;
    });
    return assignmentRuntimePromise;
  }

  function enhance(root, recording, backendId) {
    if (!root || !recording || root.dataset.recordings2MetadataDetail === 'true') return root;
    root.dataset.recordings2MetadataDetail = 'true';
    const mounted = metadataView.mount(root, recording, backendId);
    if (!mounted) return root;
    fetchMetadata(recording, backendId)
      .then(function (metadata) {
        mounted.setMetadata(metadata);
        return loadAssignmentRuntime().then(function (assignmentRuntime) {
          assignmentRuntime.mount(root, recording, backendId, metadata);
        });
      })
      .catch(mounted.setError);
    return root;
  }

  global.VdrSuiteRecordings2MetadataDetail = Object.freeze({
    enhance,
    fetchMetadata,
    loadAssignmentRuntime,
    formatDate: metadataView.formatDate,
    isPublicMetadataImageUrl: metadataView.isPublicMetadataImageUrl,
    mediaTypeLabel: metadataView.mediaTypeLabel,
    orientationLabel: metadataView.orientationLabel,
    roleLabel: metadataView.roleLabel
  });

  const runtime = global.VdrSuiteRecordings2;
  if (runtime && typeof runtime.refreshDetailAddon === 'function') {
    runtime.refreshDetailAddon();
  }
}(window));
