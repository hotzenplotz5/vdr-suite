(function (global) {
  'use strict';

  if (!global || global.VdrSuiteHomeRecentlyWatched) return;

  const doc = global.document;
  const LIMIT = 12;
  const state = {generation: 0, placementObserver: null};

  function text(value) {
    return value === undefined || value === null ? '' : String(value).trim();
  }
  function selectedBackendId() {
    const platform = global.VdrSuitePlatform;
    if (platform && typeof platform.getSelectedBackendId === 'function') {
      return text(platform.getSelectedBackendId()) || 'default';
    }
    return 'default';
  }
  function homeIsActive() {
    const platform = global.VdrSuitePlatform;
    if (platform && typeof platform.getSelectedModule === 'function') {
      return text(platform.getSelectedModule()) === 'overview';
    }
    return true;
  }
  function host() {
    return doc && doc.querySelector
      ? doc.querySelector('[data-home-zone="additional-sections"]')
      : null;
  }
  function csrfHeaders() {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.csrfHeaders !== 'function') return {};
    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object' ? headers : {};
  }
  function post(body) {
    const fetcher = global.fetch || fetch;
    return fetcher('/api/media/recently-watched', {
      method: 'POST',
      credentials: 'same-origin',
      cache: 'no-store',
      headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
      body: JSON.stringify(body)
    }).then(function (response) {
      if (!response || !response.ok) throw new Error('recently watched unavailable');
      return response.json();
    });
  }
  function normalizeItem(item, backendId) {
    if (!item || typeof item !== 'object') return null;
    const recording = item.recording && typeof item.recording === 'object'
      ? item.recording
      : null;
    const recordingId = text(item.recordingId || (recording && (recording.recordingId || recording.id)));
    const scopedBackend = text(item.backendId || (recording && recording.backendId));
    if (!recording || !recordingId || scopedBackend !== backendId) return null;
    return {
      backendId: scopedBackend,
      recordingId: recordingId,
      recording: recording,
      posterUrl: text(item.posterUrl),
      positionKnown: item.positionKnown === true,
      positionSeconds: Math.max(0, Math.floor(Number(item.positionSeconds) || 0)),
      completionKnown: item.completionKnown === true,
      completed: item.completed === true,
      resumeRelevanceKnown: item.resumeRelevanceKnown === true,
      resumeRelevant: item.resumeRelevant === true,
      sourceEvidence: text(item.sourceEvidence),
      lastActivityAt: text(item.lastActivityAt)
    };
  }
  function recordingMetadata(recording) {
    return recording && recording.metadata && typeof recording.metadata === 'object'
      ? recording.metadata : {};
  }
  function presentation(recording) {
    const value = recordingMetadata(recording).presentation;
    return value && typeof value === 'object' ? value : {};
  }
  function title(item) {
    return text(presentation(item.recording).title || item.recording.title) || 'Aufnahme';
  }
  function subtitle(item) {
    return text(presentation(item.recording).subtitle);
  }
  function posterUrl(item) {
    const metadata = recordingMetadata(item.recording);
    const artwork = metadata.artwork && typeof metadata.artwork === 'object' ? metadata.artwork : {};
    return text(presentation(item.recording).posterUrl || artwork.preferredUrl || item.posterUrl);
  }
  function publicPath(path) {
    const resolver = global.VdrSuitePublicUrl;
    return path && resolver && typeof resolver.resolvePath === 'function'
      ? resolver.resolvePath(path) : path;
  }
  function formatTime(seconds) {
    const value = Math.max(0, Math.floor(Number(seconds) || 0));
    const hours = Math.floor(value / 3600);
    const minutes = Math.floor((value % 3600) / 60);
    const rest = value % 60;
    return hours > 0
      ? hours + ':' + String(minutes).padStart(2, '0') + ':' + String(rest).padStart(2, '0')
      : minutes + ':' + String(rest).padStart(2, '0');
  }
  function activityLabel(item) {
    if (item.completionKnown && item.completed) return 'Angesehen';
    if (item.resumeRelevanceKnown && item.resumeRelevant) {
      return item.positionKnown ? 'Fortsetzbar · ' + formatTime(item.positionSeconds) : 'Fortsetzbar';
    }
    if (item.completionKnown) return 'Teilweise angesehen';
    return 'Zuletzt angesehen';
  }
  function section() {
    const target = host();
    if (!target) return null;
    let value = target.querySelector
      ? target.querySelector('[data-home-recently-watched]') : null;
    if (!value) {
      value = doc.createElement('section');
      value.className = 'media-home-discovery media-home-recently-watched';
      value.setAttribute('data-home-recently-watched', 'true');
      target.appendChild(value);
    } else if (target.lastElementChild !== value) {
      target.appendChild(value);
    }
    return value;
  }
  function clear() {
    const target = host();
    const value = target && target.querySelector
      ? target.querySelector('[data-home-recently-watched]') : null;
    if (value && typeof value.remove === 'function') value.remove();
  }
  function createArtwork(item) {
    const artwork = doc.createElement('div');
    artwork.className = 'media-home-discovery-artwork';
    const fallback = title(item).slice(0, 1).toUpperCase() || '▶';
    const url = posterUrl(item);
    if (!url) {
      artwork.textContent = fallback;
      return artwork;
    }
    const image = doc.createElement('img');
    image.src = publicPath(url);
    image.alt = 'Poster zu ' + title(item);
    image.loading = 'lazy';
    image.addEventListener('error', function () {
      image.remove();
      artwork.textContent = fallback;
    });
    artwork.appendChild(image);
    return artwork;
  }
  function ensureRecordings2() {
    if (global.VdrSuiteRecordings2 && typeof global.VdrSuiteRecordings2.openRecording === 'function') {
      return Promise.resolve(true);
    }
    const runtimes = global.VdrSuiteDeferredFrontendRuntimes;
    if (!runtimes || typeof runtimes.loadRecordings2 !== 'function') return Promise.resolve(false);
    return Promise.resolve(runtimes.loadRecordings2()).then(function () {
      return Boolean(global.VdrSuiteRecordings2 && typeof global.VdrSuiteRecordings2.openRecording === 'function');
    }).catch(function () { return false; });
  }
  function releasePreview() {
    const preview = global.VdrSuiteHomeLivePreview;
    if (preview && typeof preview.cancel === 'function') preview.cancel('Recently Watched geöffnet');
  }
  function returnHome() {
    if (typeof global.selectModule === 'function') global.selectModule('overview');
    if (typeof global.setTimeout === 'function') global.setTimeout(refresh, 0);
  }
  function openItem(item) {
    releasePreview();
    return ensureRecordings2().then(function (ready) {
      if (!ready || typeof global.selectModule !== 'function') return false;
      global.selectModule('recordings2');
      global.VdrSuiteRecordings2.openRecording(item.recording, {
        backendId: item.backendId,
        backLabel: '← Zurück zu Home',
        onClose: returnHome
      });
      return true;
    });
  }
  function render(items) {
    if (!items.length) {
      clear();
      return true;
    }
    const target = section();
    if (!target) return false;
    target.replaceChildren();
    const heading = doc.createElement('div');
    heading.className = 'media-home-section-heading';
    const name = doc.createElement('h3');
    name.textContent = 'Zuletzt angesehen';
    heading.appendChild(name);
    target.appendChild(heading);
    const rail = doc.createElement('div');
    rail.className = 'media-home-discovery-rail';
    items.forEach(function (item) {
      const card = doc.createElement('button');
      card.type = 'button';
      card.className = 'media-home-discovery-card recording recently-watched';
      card.dataset.recordingId = item.recordingId;
      card.dataset.backendId = item.backendId;
      card.dataset.historyEvidence = item.sourceEvidence;
      card.appendChild(createArtwork(item));
      const copy = doc.createElement('span');
      copy.className = 'media-home-discovery-copy';
      const label = doc.createElement('strong');
      label.textContent = title(item);
      copy.appendChild(label);
      const detailText = subtitle(item);
      if (detailText) {
        const detail = doc.createElement('span');
        detail.textContent = detailText;
        copy.appendChild(detail);
      }
      const activity = doc.createElement('span');
      activity.textContent = activityLabel(item);
      copy.appendChild(activity);
      card.appendChild(copy);
      card.addEventListener('click', function () { openItem(item); });
      rail.appendChild(card);
    });
    target.appendChild(rail);
    const owner = host();
    if (owner && owner.lastElementChild !== target) owner.appendChild(target);
    return true;
  }
  function refresh() {
    if (!homeIsActive()) return Promise.resolve(false);
    const backendId = selectedBackendId();
    const generation = ++state.generation;
    return post({operation: 'list', backendId: backendId}).then(function (payload) {
      if (generation !== state.generation || backendId !== selectedBackendId() || !homeIsActive()) return false;
      const raw = payload && Array.isArray(payload.items) ? payload.items : [];
      const items = raw.map(function (item) { return normalizeItem(item, backendId); }).filter(Boolean).slice(0, LIMIT);
      return render(items);
    }).catch(function () {
      if (generation === state.generation) clear();
      return false;
    });
  }
  function installPlacementObserver() {
    const target = host();
    if (!target || state.placementObserver || typeof global.MutationObserver !== 'function') return;
    state.placementObserver = new global.MutationObserver(function () {
      const current = target.querySelector ? target.querySelector('[data-home-recently-watched]') : null;
      if (current && target.lastElementChild !== current) target.appendChild(current);
    });
    state.placementObserver.observe(target, {childList: true});
  }
  function install() {
    if (!doc) return false;
    installPlacementObserver();
    if (typeof doc.addEventListener === 'function') {
      doc.addEventListener('click', function (event) {
        const target = event && event.target;
        if (target && typeof target.closest === 'function' &&
            target.closest('[data-brand-module="overview"], .module-tab[data-module="overview"], #backends')) {
          global.setTimeout(refresh, 0);
        }
      });
    }
    if (typeof global.setTimeout === 'function') global.setTimeout(refresh, 0);
    return true;
  }

  global.VdrSuiteHomeRecentlyWatched = Object.freeze({
    install: install,
    refresh: refresh,
    _test: Object.freeze({normalizeItem: normalizeItem, activityLabel: activityLabel, openItem: openItem, post: post})
  });

  if (doc) {
    if (doc.readyState === 'loading') doc.addEventListener('DOMContentLoaded', install, {once: true});
    else install();
  }
}(window));
