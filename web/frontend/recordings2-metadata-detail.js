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

  function assignmentRuntimePath() {
    const path = '/frontend/recordings2-metadata-assignment.js';
    const resolver = global.VdrSuitePublicUrl;
    return resolver && typeof resolver.resolvePath === 'function'
      ? resolver.resolvePath(path)
      : path;
  }

  function repairMetadataImagePaths(root) {
    const resolver = global.VdrSuitePublicUrl;
    if (!root || typeof root.querySelectorAll !== 'function' ||
        !resolver || typeof resolver.resolvePath !== 'function') return;
    Array.from(root.querySelectorAll(
      '.recordings2-metadata-image img,.recordings2-person-image'
    )).forEach(function (image) {
      const path = image && typeof image.getAttribute === 'function'
        ? image.getAttribute('src') : '';
      if (metadataView.isPublicMetadataImageUrl(path)) {
        image.src = resolver.resolvePath(path);
      }
    });
  }

  function versionedManualImageUrl(url, revision) {
    const value = shared.text(url);
    if (!value || revision <= 0 ||
        !metadataView.isPublicMetadataImageUrl(value) ||
        /[?&]assignmentRevision=/.test(value)) return value;
    return value + (value.indexOf('?') >= 0 ? '&' : '?') +
      'assignmentRevision=' + String(revision);
  }

  function versionManualMetadataArtwork(value) {
    if (!value || typeof value !== 'object') return value;
    const manual = value.manualAssignment;
    const revision = manual && manual.active === true
      ? Math.max(0, Number(manual.revision) || 0)
      : 0;
    if (revision <= 0) return value;

    const result = Object.assign({}, value);
    if (value.preferredArtwork && typeof value.preferredArtwork === 'object') {
      result.preferredArtwork = Object.assign({}, value.preferredArtwork, {
        url: versionedManualImageUrl(value.preferredArtwork.url, revision)
      });
    }
    if (Array.isArray(value.images)) {
      result.images = value.images.map(function (entry) {
        if (!entry || !entry.image || typeof entry.image !== 'object') return entry;
        return Object.assign({}, entry, {
          image: Object.assign({}, entry.image, {
            url: versionedManualImageUrl(entry.image.url, revision)
          })
        });
      });
    }
    if (Array.isArray(value.people)) {
      result.people = value.people.map(function (person) {
        if (!person || !person.image || typeof person.image !== 'object') return person;
        return Object.assign({}, person, {
          image: Object.assign({}, person.image, {
            url: versionedManualImageUrl(person.image.url, revision)
          })
        });
      });
    }
    return result;
  }

  function prioritizeDetailPoster(root) {
    if (!root || typeof root.querySelector !== 'function') return;
    const image = root.querySelector('.recordings2-detail-poster img');
    if (!image) return;
    image.loading = 'eager';
    image.decoding = 'async';
    image.fetchPriority = 'high';
    if (typeof image.setAttribute === 'function') {
      image.setAttribute('fetchpriority', 'high');
    }
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
      const id = 'vdr-suite-recordings2-metadata-assignment-runtime';
      const existing = document.getElementById(id);
      const script = existing || document.createElement('script');
      function ready() {
        const runtime = global.VdrSuiteRecordings2MetadataAssignment;
        if (runtime && typeof runtime.mount === 'function') resolve(runtime);
        else reject(new Error('Manuelle Metadaten-Runtime wurde nicht initialisiert.'));
      }
      if (existing && global.VdrSuiteRecordings2MetadataAssignment) return ready();
      script.id = id;
      script.async = false;
      script.addEventListener('load', ready, {once: true});
      script.addEventListener('error', function () {
        reject(new Error('Manuelle Metadaten-Runtime konnte nicht geladen werden.'));
      }, {once: true});
      if (!existing) {
        script.src = assignmentRuntimePath();
        document.head.appendChild(script);
      }
    }).catch(function (error) {
      assignmentRuntimePromise = null;
      throw error;
    });
    return assignmentRuntimePromise;
  }

  function renderAssignmentLoadError(root, error) {
    if (!root || root.querySelector('[data-recordings2-metadata-assignment-error]')) return;
    const box = document.createElement('section');
    const title = document.createElement('strong');
    const message = document.createElement('p');
    box.className = 'recordings2-status error';
    box.dataset.recordings2MetadataAssignmentError = 'true';
    box.setAttribute('data-recordings2-metadata-assignment-error', 'true');
    box.setAttribute('role', 'alert');
    title.textContent = 'Metadatenkorrektur konnte nicht geladen werden';
    message.textContent = error && error.message
      ? error.message
      : String(error || 'Unbekannter Fehler');
    box.append(title, message);
    root.appendChild(box);
  }

  function enhance(root, recording, backendId) {
    if (!root || !recording || root.dataset.recordings2MetadataDetail === 'true') return root;
    root.dataset.recordings2MetadataDetail = 'true';
    const mounted = metadataView.mount(root, recording, backendId);
    if (!mounted) return root;
    fetchMetadata(recording, backendId).then(function (metadata) {
      const presentedMetadata = versionManualMetadataArtwork(metadata);
      mounted.setMetadata(presentedMetadata);
      repairMetadataImagePaths(root);
      metadataView.applyToDetail(root, presentedMetadata);
      prioritizeDetailPoster(root);
      loadAssignmentRuntime().then(function (runtime) {
        runtime.mount(root, recording, backendId, presentedMetadata);
      }).catch(function (error) {
        renderAssignmentLoadError(root, error);
      });
    }).catch(mounted.setError);
    return root;
  }

  global.VdrSuiteRecordings2MetadataDetail = Object.freeze({
    enhance,
    fetchMetadata,
    loadAssignmentRuntime,
    assignmentRuntimePath,
    repairMetadataImagePaths,
    versionManualMetadataArtwork,
    prioritizeDetailPoster,
    preferredArtworkUrl: metadataView.preferredArtworkUrl,
    applyMetadataToDetail: metadataView.applyToDetail,
    formatDate: metadataView.formatDate,
    isPublicMetadataImageUrl: metadataView.isPublicMetadataImageUrl,
    mediaTypeLabel: metadataView.mediaTypeLabel,
    orientationLabel: metadataView.orientationLabel,
    roleLabel: metadataView.roleLabel
  });

  const runtime = global.VdrSuiteRecordings2;
  if (runtime && typeof runtime.refreshDetailAddon === 'function') runtime.refreshDetailAddon();
}(window));
