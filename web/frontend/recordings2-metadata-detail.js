// Async metadata adapter for the Recordings 2 detail view.
(function (global) {
  'use strict';

  const shared = global.VdrSuiteRecordings2Shared;
  const metadataView = global.VdrSuiteRecordings2MetadataView;
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

  function enhance(root, recording, backendId) {
    if (!root || !recording || root.dataset.recordings2MetadataDetail === 'true') return root;
    root.dataset.recordings2MetadataDetail = 'true';
    const mounted = metadataView.mount(root, recording, backendId);
    if (!mounted) return root;
    fetchMetadata(recording, backendId)
      .then(mounted.setMetadata)
      .catch(mounted.setError);
    return root;
  }

  global.VdrSuiteRecordings2MetadataDetail = Object.freeze({
    enhance,
    fetchMetadata,
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
