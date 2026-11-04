(function (global) {
  'use strict';

  function browserSessionAuthenticated() {
    const session = global.VdrSuiteBrowserSession;
    if (!session) return false;
    if (typeof session.isAuthenticated === 'function') {
      return session.isAuthenticated() === true;
    }
    if (typeof session.snapshot === 'function') {
      const state = session.snapshot();
      return Boolean(state && state.authenticated);
    }
    return false;
  }

  function backendSelectionReady() {
    if (!doc || typeof doc.querySelector !== 'function') return false;
    return Boolean(doc.querySelector(
      '#backends .backend-card.selected, #backends [aria-selected="true"]'
    ));
  }

  const doc = global.document;
  const ENDPOINT = '/api/media/continue-watching';
  const HOME_RESUME_EVENT = 'vdr-suite:home-resume';
  let refreshGeneration = 0;
  let liveSource = null;
  let liveSequence = 0;
  let liveConnectionSequence = 0;
  let liveRefreshPending = false;
  let liveRefreshScheduled = false;

  function text(value) { return value == null ? '' : String(value); }
  function positiveInt(value) {
    const number = Number(value);
    return Number.isFinite(number) && number > 0 ? Math.floor(number) : 0;
  }
  function homePreview(path) {
    const helpers = global.VdrSuiteFrontendHelpers;
    return helpers && typeof helpers.homeArtworkPreviewUrl === 'function'
      ? helpers.homeArtworkPreviewUrl(path) : path;
  }
  function publicPath(path) {
    const value = text(path);
    const resolver = global.VdrSuitePublicUrl;
    return value && resolver && typeof resolver.resolvePath === 'function'
      ? resolver.resolvePath(value)
      : value;
  }
  function selectedBackendId() {
    const selected = doc && doc.querySelector ? doc.querySelector('#backends .backend-card.selected, #backends [aria-selected="true"]') : null;
    return text(selected && selected.dataset && selected.dataset.backendId) || 'default';
  }
  function normalizeItem(value) {
    if (!value || typeof value !== 'object') return null;
    const backendId = text(value.backendId);
    const recordingId = text(value.recordingId);
    const backendNativeId = text(value.backendNativeId);
    const position = positiveInt(value.resumePositionSeconds);
    const durationKnown = value.durationKnown === true;
    const duration = durationKnown ? positiveInt(value.durationSeconds) : 0;
    if (!backendId || !recordingId || !position) return null;
    if (durationKnown && duration > 0 && position >= duration) return null;
    const projected = value.recording && typeof value.recording === 'object'
      ? Object.assign({}, value.recording)
      : {};
    projected.id = text(projected.id) || recordingId;
    projected.recordingId = text(projected.recordingId) || recordingId;
    projected.backendId = text(projected.backendId) || backendId;
    projected.backendNativeId = text(projected.backendNativeId) || backendNativeId;
    projected.title = text(projected.title) || text(value.title) || 'Aufnahme';
    return {
      backendId,
      recordingId,
      backendNativeId: projected.backendNativeId,
      title: text(value.title) || projected.title || 'Aufnahme',
      subtitle: text(value.subtitle),
      posterUrl: text(value.posterUrl),
      resumePositionSeconds: position,
      durationKnown,
      durationSeconds: duration,
      lastActivityAt: text(value.lastActivityAt),
      recording: projected
    };
  }
  function progressModel(item) {
    const position = positiveInt(item && item.resumePositionSeconds);
    const duration = item && item.durationKnown === true ? positiveInt(item.durationSeconds) : 0;
    if (!duration) return {hasPercent: false, percent: null, positionSeconds: position, durationSeconds: 0};
    return {
      hasPercent: true,
      percent: Math.max(0, Math.min(100, Math.floor((position / duration) * 100))),
      positionSeconds: position,
      durationSeconds: duration
    };
  }
  function formatTime(seconds) {
    const total = Math.max(0, Math.floor(Number(seconds) || 0));
    const hours = Math.floor(total / 3600);
    const minutes = Math.floor((total % 3600) / 60);
    const secs = total % 60;
    return hours > 0
      ? hours + ':' + String(minutes).padStart(2, '0') + ':' + String(secs).padStart(2, '0')
      : minutes + ':' + String(secs).padStart(2, '0');
  }
  function useFallback(artwork, fallback) {
    artwork.classList.add('is-fallback');
    artwork.textContent = fallback;
  }
  function createArtwork(item) {
    const artwork = doc.createElement('div');
    artwork.className = 'media-home-continue-artwork';
    const fallback = text(item && item.title).slice(0, 1).toUpperCase() || '▶';
    const posterUrl = text(item && item.posterUrl);
    if (!posterUrl) {
      useFallback(artwork, fallback);
      return artwork;
    }
    const image = doc.createElement('img');
    image.src = publicPath(homePreview(posterUrl));
    image.alt = 'Poster zu ' + (text(item && item.title) || 'Aufnahme');
    image.loading = 'lazy';
    image.decoding = 'async';
    image.fetchPriority = 'low';
    image.addEventListener('error', function () {
      image.remove();
      useFallback(artwork, fallback);
    });
    artwork.appendChild(image);
    return artwork;
  }
  function csrfHeaders() {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.csrfHeaders !== 'function') return {};
    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object' ? headers : {};
  }
  function post(body) {
    const fetcher = global.fetch || fetch;
    return fetcher(ENDPOINT, {
      method: 'POST',
      credentials: 'same-origin',
      headers: Object.assign({'Content-Type': 'application/json'}, csrfHeaders()),
      body: JSON.stringify(body)
    }).then(function (response) {
      if (!response || !response.ok) throw new Error('Continue Watching unavailable');
      return response.json();
    });
  }
  function releasePreview() {
    const preview = global.VdrSuiteHomeLivePreview;
    if (preview && typeof preview.cancel === 'function') {
      preview.cancel('Continue Watching geöffnet');
      return true;
    }
    return false;
  }
  function recordings2Ready() {
    return Boolean(global.VdrSuiteRecordings2 && typeof global.VdrSuiteRecordings2.openRecording === 'function');
  }
  function continueWatchingPlaybackReady() {
    const playback = global.VdrSuiteRecordings2Playback;
    const sync = global.VdrSuiteContinueWatchingSync;
    return Boolean(
      playback && typeof playback.createPanel === 'function' &&
      sync && typeof sync.clear === 'function'
    );
  }
  function selectShellModule(moduleName) {
    if (typeof global.selectModule !== 'function') return false;
    global.selectModule(moduleName);
    return true;
  }
  function refreshHomeAfterPersistence() {
    const sync = global.VdrSuiteContinueWatchingSync;
    const pendingWrites = sync && typeof sync.flush === 'function'
      ? sync.flush()
      : Promise.resolve(true);
    return Promise.resolve(pendingWrites).catch(function () { return false; }).then(function () {
      const refreshes = [refresh()];
      const recentlyWatched = global.VdrSuiteHomeRecentlyWatched;
      if (recentlyWatched && typeof recentlyWatched.refresh === 'function') {
        refreshes.push(recentlyWatched.refresh());
      }
      return Promise.all(refreshes);
    });
  }
  function returnHome() {
    selectShellModule('overview');
    if (typeof global.setTimeout === 'function') global.setTimeout(refreshHomeAfterPersistence, 0);
  }
  function ensureContinueWatchingPlaybackRuntime() {
    if (continueWatchingPlaybackReady()) return Promise.resolve(true);
    if (typeof global.loadVdrSuiteDeferredRuntime !== 'function') return Promise.resolve(false);
    return Promise.resolve(global.loadVdrSuiteDeferredRuntime(
      'vdr-suite-session-frontend-sync-runtime',
      '/frontend/api/session-frontend-sync.js',
      function () { return Boolean(global.VdrSuiteRecordingFastPlayback && global.VdrSuiteLivePlayback); }
    )).then(function () {
      return global.loadVdrSuiteDeferredRuntime(
        'vdr-suite-recordings2-playback-runtime',
        '/frontend/recordings2-playback.js',
        continueWatchingPlaybackReady
      );
    }).then(continueWatchingPlaybackReady).catch(function () { return false; });
  }
  function ensureRecordings2() {
    const ready = recordings2Ready()
      ? Promise.resolve(true)
      : (function () {
          const runtimes = global.VdrSuiteDeferredFrontendRuntimes;
          if (!runtimes || typeof runtimes.loadRecordings2 !== 'function') return Promise.resolve(false);
          return Promise.resolve(runtimes.loadRecordings2()).then(recordings2Ready).catch(function () { return false; });
        }());
    return ready.then(function (recordingsReady) {
      return recordingsReady ? ensureContinueWatchingPlaybackRuntime() : false;
    });
  }
  function clearBeforeRestart(item) {
    const sync = global.VdrSuiteContinueWatchingSync;
    if (!sync || typeof sync.clear !== 'function') return false;
    Promise.resolve(sync.clear(item.backendId, item.recordingId)).catch(function () {});
    return true;
  }
  function openItem(item, resume) {
    const normalized = normalizeItem(item);
    if (!normalized) return Promise.resolve(false);
    releasePreview();
    return ensureRecordings2().then(function (ready) {
      if (!ready) return false;
      selectShellModule('recordings2');
      if (!resume) clearBeforeRestart(normalized);
      const recording = Object.assign({}, normalized.recording, {
        id: normalized.recordingId,
        recordingId: normalized.recordingId,
        backendId: normalized.backendId,
        backendNativeId: normalized.backendNativeId,
        title: normalized.recording.title || normalized.title
      });
      global.VdrSuiteRecordings2.openRecording(recording, {
        backendId: normalized.backendId,
        autoStartPlayback: true,
        playbackStartPositionSeconds: resume ? normalized.resumePositionSeconds : 0,
        continueWatching: true,
        backLabel: '← Zurück zu Home',
        onClose: returnHome
      });
      return true;
    });
  }

  // Keep cards, poster nodes and active scroll in place on every SSE readback.
  function createContinueCard(item) {
    const card = doc.createElement('article');
    card.className = 'media-home-continue-card';
    card.dataset.recordingId = item.recordingId;
    card.dataset.backendId = item.backendId;
    const artwork = createArtwork(item);
    card.appendChild(artwork);
    const copy = doc.createElement('div');
    copy.className = 'media-home-continue-copy';
    const name = doc.createElement('h4');
    const subtitle = doc.createElement('p');
    const progressText = doc.createElement('p');
    progressText.className = 'media-home-continue-progress-text';
    const meter = doc.createElement('progress');
    meter.max = 100;
    const actions = doc.createElement('div');
    actions.className = 'media-home-continue-actions';
    const resume = doc.createElement('button');
    resume.type = 'button';
    resume.textContent = 'Fortsetzen';
    resume.setAttribute('data-home-continue-action', 'continue');
    const restart = doc.createElement('button');
    restart.type = 'button';
    restart.textContent = 'Von vorn';
    restart.setAttribute('data-home-continue-action', 'restart');
    resume.addEventListener('click', function () {
      openItem(card.__vdrSuiteContinueItem, true);
    });
    restart.addEventListener('click', function () {
      openItem(card.__vdrSuiteContinueItem, false);
    });
    actions.appendChild(resume);
    actions.appendChild(restart);
    copy.appendChild(name);
    copy.appendChild(subtitle);
    copy.appendChild(progressText);
    copy.appendChild(meter);
    copy.appendChild(actions);
    card.appendChild(copy);
    card.__vdrSuiteContinueView = {
      artwork: artwork, name: name, subtitle: subtitle,
      progressText: progressText, meter: meter,
      resume: resume, restart: restart
    };
    updateContinueCard(card, item);
    return card;
  }

  function updateContinueCard(card, item) {
    const view = card.__vdrSuiteContinueView;
    if (!view) return false;
    const previous = card.__vdrSuiteContinueItem;
    card.__vdrSuiteContinueItem = item;
    const oldArtwork = previous && text(previous.posterUrl);
    if (previous && oldArtwork !== text(item.posterUrl)) {
      const artwork = createArtwork(item);
      card.insertBefore(artwork, view.artwork);
      view.artwork.remove();
      view.artwork = artwork;
    } else if (previous && previous.title !== item.title) {
      // A title correction alone must not detach or refetch an unchanged poster.
      const image = Array.from(view.artwork.children || []).find(function (node) {
        return node.tagName === 'IMG';
      });
      if (image) image.alt = 'Poster zu ' + item.title;
      else view.artwork.textContent = item.title.slice(0, 1).toUpperCase() || '▶';
    }
    if (view.name.textContent !== item.title) view.name.textContent = item.title;
    const subtitle = text(item.subtitle);
    if (view.subtitle.textContent !== subtitle) view.subtitle.textContent = subtitle;
    view.subtitle.hidden = !subtitle;
    view.resume.setAttribute('aria-label', item.title + ' fortsetzen');
    view.restart.setAttribute('aria-label', item.title + ' von vorn abspielen');
    const progress = progressModel(item);
    const progressLabel = progress.hasPercent
      ? formatTime(progress.positionSeconds) + ' / ' + formatTime(progress.durationSeconds)
      : 'Fortsetzen bei ' + formatTime(progress.positionSeconds);
    if (view.progressText.textContent !== progressLabel) {
      view.progressText.textContent = progressLabel;
    }
    view.meter.hidden = !progress.hasPercent;
    if (progress.hasPercent) {
      view.meter.value = progress.percent;
      view.meter.setAttribute('aria-label',
        'Wiedergabefortschritt ' + progress.percent + ' Prozent');
    }
    return true;
  }

  function reconcileContinueCards(rail, cards) {
    const old = Array.from(rail.children);
    if (old.length === cards.length && old.every(function (card, index) {
      return cards[index] === card;
    })) return;
    const scroll = rail.scrollLeft;
    const focused = doc.activeElement;
    const retained = new Set(cards);
    old.forEach(function (card) {
      if (!retained.has(card)) card.remove();
    });
    cards.forEach(function (card, index) {
      if (rail.children[index] !== card) {
        rail.insertBefore(card, rail.children[index] || null);
      }
    });
    if (focused && cards.includes(focused) && doc.activeElement !== focused &&
        typeof focused.focus === 'function') {
      focused.focus({preventScroll: true});
    }
    if (Number.isFinite(scroll)) rail.scrollLeft = scroll;
  }

  function render(items) {
    if (!doc || typeof doc.querySelector !== 'function') return false;
    const host = doc.querySelector('[data-home-zone="additional-sections"]');
    if (!host) return false;
    const valid = (Array.isArray(items) ? items : []).map(normalizeItem).filter(Boolean);
    let section = host.querySelector ? host.querySelector('[data-home-continue-watching]') : null;
    if (!valid.length) {
      if (section && typeof section.remove === 'function') section.remove();
      return true;
    }
    if (!section) {
      section = doc.createElement('section');
      section.className = 'media-home-continue-watching';
      section.setAttribute('data-home-continue-watching', '');
      section.setAttribute('aria-labelledby', 'media-home-continue-title');
      const heading = doc.createElement('div');
      heading.className = 'media-home-section-heading';
      const title = doc.createElement('h3');
      title.id = 'media-home-continue-title';
      title.textContent = 'Weiterschauen';
      heading.appendChild(title);
      section.appendChild(heading);
      const rail = doc.createElement('div');
      rail.className = 'media-home-continue-rail';
      rail.setAttribute('aria-label', 'Fortsetzbare Aufnahmen');
      section.appendChild(rail);
      host.appendChild(section);
    }
    const rail = Array.from(section.children).find(function (element) {
      return element.className === 'media-home-continue-rail';
    });
    if (!rail) return false;
    const existing = new Map(Array.from(rail.children).map(function (card) {
      return [card.dataset.backendId + '\u001f' + card.dataset.recordingId, card];
    }));
    const nextCards = valid.map(function (item) {
      const key = item.backendId + '\u001f' + item.recordingId;
      const card = existing.get(key);
      if (card) {
        updateContinueCard(card, item);
        return card;
      }
      return createContinueCard(item);
    });
    reconcileContinueCards(rail, nextCards);
    return true;
  }
  function installStyles() {
    if (!doc || !doc.head || doc.getElementById('vdr-suite-continue-watching-style')) return;
    const style = doc.createElement('style');
    style.id = 'vdr-suite-continue-watching-style';
    style.textContent = '#detail:has(.module-tab.active[data-module="overview"]){display:flex;flex-direction:column}#detail:has(.module-tab.active[data-module="overview"]) [data-home-zone="additional-sections"]{order:40}#detail:has(.module-tab.active[data-module="overview"]) [data-home-zone="primary-rail"]{order:50}#detail:has(.module-tab.active[data-module="overview"]) #detail-data{order:60}.media-home-continue-watching{min-width:0}.media-home-continue-rail{display:grid;grid-auto-flow:column;grid-auto-columns:minmax(17rem,24rem);gap:.85rem;overflow-x:auto;padding:0 .25rem 1rem;scroll-snap-type:x proximity}.media-home-continue-card{scroll-snap-align:start;display:grid;grid-template-columns:6rem 1fr;gap:.8rem;padding:.8rem;border:1px solid rgba(148,163,184,.26);border-radius:1rem;background:linear-gradient(145deg,rgba(15,23,42,.86),rgba(15,23,42,.7));box-shadow:0 .8rem 1.8rem rgba(2,6,23,.14)}.media-home-continue-artwork{display:grid;place-items:center;align-self:start;width:100%;aspect-ratio:2/3;overflow:hidden;border-radius:.75rem;background:linear-gradient(145deg,#1e293b,#334155);font-size:2rem;font-weight:800}.media-home-continue-artwork.is-fallback{position:relative;background:radial-gradient(circle at 70% 22%,rgba(56,189,248,.2),transparent 32%),linear-gradient(145deg,#172033,#26364d);color:#dbeafe;text-shadow:0 .12rem .5rem rgba(2,6,23,.75)}.media-home-continue-artwork.is-fallback::after{content:"";position:absolute;inset:.45rem;border:1px solid rgba(148,163,184,.18);border-radius:.55rem;pointer-events:none}.media-home-continue-artwork img{display:block;width:100%;height:100%;object-fit:cover}.media-home-continue-copy{min-width:0}.media-home-continue-copy h4,.media-home-continue-copy p{margin:.1rem 0 .45rem}.media-home-continue-copy progress{width:100%;accent-color:#38bdf8}.media-home-continue-actions{display:flex;flex-wrap:wrap;gap:.45rem;margin-top:.65rem}.media-home-continue-actions button{min-width:2.75rem;min-height:2.75rem;padding:.55rem .75rem;border-radius:.65rem;transition:transform .14s ease,border-color .14s ease}.media-home-continue-actions button:hover{transform:translateY(-1px)}.media-home-continue-actions button:focus-visible{outline:3px solid rgba(125,211,252,.96);outline-offset:2px}@media(min-width:120rem){.media-home-continue-rail{grid-auto-columns:minmax(22rem,29rem);gap:1.15rem}.media-home-continue-card{grid-template-columns:7.5rem 1fr;padding:1rem}}@media(min-width:46.01rem){.media-home-continue-rail{grid-auto-columns:minmax(11rem,15rem);gap:.8rem;padding:0 1rem 1.1rem}.media-home-continue-card{grid-template-columns:1fr;gap:0;padding:0;overflow:hidden}.media-home-continue-artwork{border-radius:0}.media-home-continue-copy{display:grid;gap:.25rem;padding:.7rem}.media-home-continue-actions{display:grid;grid-template-columns:1fr;gap:.45rem}.media-home-continue-actions button{width:100%}}@media(max-width:46rem){.media-home-continue-rail{grid-auto-columns:minmax(80vw,20rem)}.media-home-continue-card{grid-template-columns:5rem 1fr}}@media(prefers-reduced-motion:reduce){.media-home-continue-actions button{transition:none}.media-home-continue-actions button:hover{transform:none}.media-home-continue-rail{scroll-behavior:auto}}';
    doc.head.appendChild(style);
  }

  function homeIsActive() {
    const platform = global.VdrSuitePlatform;
    return !platform || typeof platform.getSelectedModule !== 'function' ||
      text(platform.getSelectedModule()) === 'overview';
  }
  function liveReady() {
    return browserSessionAuthenticated() && backendSelectionReady() &&
      homeIsActive() && !(doc && doc.hidden);
  }
  function stopLive() {
    if (liveSource) {
      liveSource.close();
      liveSource = null;
    }
    liveRefreshPending = false;
    liveSequence = 0;
    liveConnectionSequence = 0;
    ++refreshGeneration; // An old actor/backend read must not repaint this rail.
  }
  function scheduleLiveRefresh() {
    liveRefreshPending = true;
    if (liveRefreshScheduled || !liveReady()) return;
    liveRefreshScheduled = true;
    Promise.resolve().then(function () {
      liveRefreshScheduled = false;
      if (!liveRefreshPending || !liveReady()) return;
      liveRefreshPending = false;
      refresh({retainVisible: true});
    });
  }
  function subscribeLive() {
    if (liveSource || !liveReady()) return false;
    const platform = global.VdrSuitePlatform;
    const client = platform && typeof platform.getClientApi === 'function'
      ? platform.getClientApi()
      : global.VdrSuiteClientApi;
    if (!client || typeof client.createClientLiveUpdateSource !== 'function') return false;
    const source = client.createClientLiveUpdateSource();
    if (!source) return false;
    liveSource = source;
    source.onopen = function () {
      liveConnectionSequence = 0;
    };
    source.addEventListener('update', function (event) {
      if (liveSource !== source || !liveReady()) return;
      let data = null;
      try { data = JSON.parse(event.data); } catch (_) { return; }
      const sequence = Number(data && data.sequenceNumber);
      if (!Number.isSafeInteger(sequence) || sequence < 1) return;
      liveConnectionSequence = Math.max(liveConnectionSequence, sequence);
      if (sequence <= liveSequence) return;
      liveSequence = sequence;
      if (text(data.backendId || 'default') !== selectedBackendId() ||
          !Array.isArray(data.changedDomains)) return;
      if (data.changedDomains.includes('continueWatching') ||
          data.changedDomains.includes('recordings')) {
        scheduleLiveRefresh();
      }
    });
    source.onerror = function () {
      if (liveSource !== source) return;
      // The existing feed is finite. Normal completion is not a change.
      // A lower sequence after daemon restart requires one canonical read.
      if (liveConnectionSequence < liveSequence) {
        liveSequence = liveConnectionSequence;
        scheduleLiveRefresh();
      }
    };
    return true;
  }
  function refresh(options) {
    if (!browserSessionAuthenticated()) return Promise.resolve(false);
    const config = options && typeof options === 'object' ? options : {};
    const backendId = selectedBackendId();
    const generation = ++refreshGeneration;
    return post({operation: 'list', backendId}).then(function (payload) {
      if (!browserSessionAuthenticated() ||
          generation !== refreshGeneration || selectedBackendId() !== backendId) return false;
      render(payload && payload.items);
      return true;
    }).catch(function () {
      if (!browserSessionAuthenticated() ||
          generation !== refreshGeneration || selectedBackendId() !== backendId) return false;
      if (config.retainVisible !== true) render([]);
      return false;
    });
  }
  function install() {
    installStyles();
    if (browserSessionAuthenticated() && backendSelectionReady() &&
        typeof global.setTimeout === 'function') {
      global.setTimeout(function () {
        if (liveReady()) subscribeLive();
        refresh({retainVisible: true});
      }, 0);
    }
    if (doc && typeof doc.addEventListener === 'function') {
      doc.addEventListener(HOME_RESUME_EVENT, function () {
        if (liveReady()) subscribeLive();
        refresh({retainVisible: true});
      });
      doc.addEventListener('visibilitychange', function () {
        if (!liveReady()) stopLive();
        else {
          subscribeLive();
          refresh({retainVisible: true});
        }
      });
      doc.addEventListener('click', function (event) {
        const target = event && event.target;
        if (!target || typeof target.closest !== 'function') return;
        const tab = target.closest('.module-tab[data-module], [data-brand-module]');
        const moduleName = text(tab && tab.dataset &&
          (tab.dataset.module || tab.dataset.brandModule));
        if (moduleName && moduleName !== 'overview') stopLive();
        if (target.closest('#backends')) {
          stopLive();
          // The canonical Home-resume event owns the backend readback.
        }
      });
    }
    return true;
  }

  global.VdrSuiteHomeContinueWatching = Object.freeze({
    install,
    refresh,
    _test: Object.freeze({normalizeItem, progressModel, openItem, formatTime, post, ensureRecordings2, ensureContinueWatchingPlaybackRuntime, continueWatchingPlaybackReady, releasePreview, clearBeforeRestart, selectShellModule, refreshHomeAfterPersistence, returnHome, publicPath, createArtwork, render, subscribeLive, stopLive, scheduleLiveRefresh})
  });
  if (doc) {
    if (doc.readyState === 'loading') doc.addEventListener('DOMContentLoaded', install, {once: true});
    else install();
  }
}(window));