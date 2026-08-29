// Phase 66.3: deferred Live-TV preview for Media Home.
//
// Preview is an intent controller, not a playback owner. It waits for a stable
// Home selection, then asks the existing VdrSuiteRecordings2Playback facade for
// the canonical Live-TV adapter. The Phase-65 playback shell remains the only
// MediaSession/player lifecycle authority.
(function (global) {
  'use strict';

  if (!global || global.VdrSuiteHomeLivePreview) return;

  const doc = global.document || (typeof document !== 'undefined' ? document : null);
  const previewSettleMs = 650; // private UX tuning value; not a public contract
  const state = {
    focusToken: 0,
    pendingTimer: null,
    previewPlayback: null,
    previewStarting: null,
    previewChannelId: '',
    previewBackendId: '',
    lastChannelId: '',
    lastBackendId: '',
    failedToken: -1,
    rootObserver: null,
    shellObserver: null,
    inputBound: false,
    syncScheduled: false,
    host: null,
    status: ''
  };

  function text(value) {
    return value === undefined || value === null ? '' : String(value).trim();
  }

  function heroSnapshot() {
    const hero = global.VdrSuiteHomeLiveHero;
    if (!hero || typeof hero.snapshot !== 'function') return null;
    try { return hero.snapshot(); } catch (_) { return null; }
  }

  function shellSnapshot() {
    const shell = global.VdrSuitePlaybackShell;
    if (!shell || typeof shell.snapshot !== 'function') return null;
    try { return shell.snapshot(); } catch (_) { return null; }
  }

  function playbackApi() {
    return global.VdrSuiteRecordings2Playback || null;
  }

  function heroRoot() {
    if (!doc || typeof doc.querySelector !== 'function') return null;
    return doc.querySelector('.media-home-hero[data-home-zone="hero"]');
  }

  function cssEscape(value) {
    if (global.CSS && typeof global.CSS.escape === 'function') return global.CSS.escape(String(value));
    return String(value).replace(/[^a-zA-Z0-9_-]/g, function (character) {
      return '\\' + character.charCodeAt(0).toString(16) + ' ';
    });
  }

  function selectedChannelDescriptor(snapshot) {
    const value = snapshot || heroSnapshot();
    const id = text(value && value.selectedChannelId);
    if (!id) return null;
    let name = id;
    const root = heroRoot();
    if (root && typeof root.querySelector === 'function') {
      const focus = root.querySelector('.media-home-live-focus[data-channel-id="' + cssEscape(id) + '"]');
      const heading = focus && typeof focus.querySelector === 'function' ? focus.querySelector('h3') : null;
      if (heading && text(heading.textContent)) name = text(heading.textContent);
    }
    return {channelId: id, id: id, name: name};
  }

  function removeHost() {
    const host = state.host;
    if (host && host.parentNode && typeof host.parentNode.removeChild === 'function') {
      host.parentNode.removeChild(host);
    }
  }

  function ensureHost() {
    if (!doc || typeof doc.createElement !== 'function') return null;
    const root = heroRoot();
    const focus = root && typeof root.querySelector === 'function'
      ? root.querySelector('.media-home-live-focus')
      : null;
    if (!focus) return null;
    if (!state.host) {
      state.host = doc.createElement('section');
      state.host.className = 'media-home-live-preview';
      state.host.setAttribute('aria-label', 'Live-TV Vorschau');
      state.host.setAttribute('aria-live', 'polite');
    }
    if (state.host.parentNode !== focus) focus.appendChild(state.host);
    return state.host;
  }

  function setPreviewStatus(message, error) {
    state.status = text(message);
    const host = ensureHost();
    if (!host || !doc || typeof doc.createElement !== 'function') return;
    const playback = state.previewPlayback || (state.previewStarting && state.previewStarting.playback);
    const element = playback && playback.element;
    if (element && element.parentNode === host) return;
    if (typeof host.replaceChildren === 'function') host.replaceChildren();
    const status = doc.createElement('div');
    status.className = 'media-home-live-preview-status' + (error ? ' error' : '');
    status.textContent = state.status || 'Live-Vorschau';
    host.appendChild(status);
  }

  function preparePreviewElement(playback) {
    const element = playback && playback.element;
    if (!element || typeof element.querySelector !== 'function') return element || null;
    const video = element.querySelector('video');
    if (video) {
      video.muted = true;
      video.controls = false;
      video.autoplay = true;
      video.playsInline = true;
      if (typeof video.setAttribute === 'function') video.setAttribute('muted', '');
    }
    return element;
  }

  function mountPreview(playback) {
    const host = ensureHost();
    const element = preparePreviewElement(playback);
    if (!host || !element) return false;
    if (element.parentNode !== host) {
      if (typeof host.replaceChildren === 'function') host.replaceChildren(element);
      else host.appendChild(element);
    }
    state.status = 'Live-Vorschau aktiv';
    return true;
  }

  function cancelPendingPreview() {
    if (state.pendingTimer === null) return false;
    if (typeof global.clearTimeout === 'function') global.clearTimeout(state.pendingTimer);
    state.pendingTimer = null;
    return true;
  }

  function destroyActivePreview(reason) {
    const playback = state.previewPlayback;
    state.previewPlayback = null;
    state.previewChannelId = '';
    state.previewBackendId = '';
    if (playback && typeof playback.destroy === 'function') {
      try { playback.destroy(); } catch (_) {}
    }
    removeHost();
    state.status = text(reason);
    return Boolean(playback);
  }

  function cancelStartingPreview(reason) {
    if (!state.previewStarting) return false;
    state.previewStarting.cancelled = true;
    state.previewStarting.cancelReason = text(reason) || 'superseded';
    removeHost();
    return true;
  }

  function cancelPreview(reason) {
    cancelPendingPreview();
    cancelStartingPreview(reason);
    destroyActivePreview(reason);
  }

  function currentIntentMatches(token, backendId, channelId) {
    const snapshot = heroSnapshot();
    return Boolean(
      snapshot && snapshot.active === true &&
      token === state.focusToken &&
      text(snapshot.backendId) === text(backendId) &&
      text(snapshot.selectedChannelId) === text(channelId)
    );
  }

  function ownsShellPreview(snapshot) {
    const shell = snapshot || shellSnapshot();
    if (!shell || shell.active !== true) return false;
    if (state.previewPlayback) {
      return text(shell.backendId) === state.previewBackendId && text(shell.channelId) === state.previewChannelId;
    }
    if (state.previewStarting) {
      return text(shell.backendId) === state.previewStarting.backendId && text(shell.channelId) === state.previewStarting.channelId;
    }
    return false;
  }

  function schedulePreview() {
    cancelPendingPreview();
    const snapshot = heroSnapshot();
    if (!snapshot || snapshot.active !== true) return false;
    const backendId = text(snapshot.backendId);
    const channelId = text(snapshot.selectedChannelId);
    if (!backendId || !channelId || state.failedToken === state.focusToken) return false;

    if (state.previewPlayback && state.previewBackendId === backendId && state.previewChannelId === channelId) {
      mountPreview(state.previewPlayback);
      return true;
    }

    // A cancelled in-flight request still owns the canonical shell until its
    // asynchronous session creation resolves and can be destroyed. Never start
    // a competing preview while that handoff is unresolved.
    if (state.previewStarting) return true;

    const token = state.focusToken;
    const schedule = typeof global.setTimeout === 'function' ? global.setTimeout : setTimeout;
    state.pendingTimer = schedule(function () {
      state.pendingTimer = null;
      startPreview(token, backendId, channelId);
    }, previewSettleMs);
    return true;
  }

  function startPreview(token, backendId, channelId) {
    if (!currentIntentMatches(token, backendId, channelId) || state.previewStarting) {
      return Promise.resolve('');
    }

    const shell = shellSnapshot();
    if (shell && shell.active === true && !ownsShellPreview(shell)) {
      state.failedToken = token;
      setPreviewStatus('Live-Vorschau pausiert · explizite Wiedergabe ist aktiv.', false);
      return Promise.resolve('');
    }

    const api = playbackApi();
    const channel = selectedChannelDescriptor();
    if (!api || typeof api.createLivePanel !== 'function' || !channel || text(channel.channelId) !== channelId) {
      state.failedToken = token;
      setPreviewStatus('Live-Vorschau ist derzeit nicht verfügbar.', true);
      return Promise.resolve('');
    }

    let playback = null;
    try {
      playback = api.createLivePanel(channel, backendId, {ownerIntent: 'preview'});
    } catch (error) {
      state.failedToken = token;
      setPreviewStatus(error && error.message ? error.message : 'Live-Vorschau konnte nicht vorbereitet werden.', true);
      return Promise.resolve('');
    }

    preparePreviewElement(playback);
    const request = {
      token: token,
      backendId: backendId,
      channelId: channelId,
      playback: playback,
      cancelled: false,
      promoted: false,
      cancelReason: ''
    };
    state.previewStarting = request;
    setPreviewStatus('Live-Vorschau wird vorbereitet …', false);

    let startRequest = null;
    try {
      startRequest = playback && typeof playback.start === 'function' ? playback.start() : '';
    } catch (error) {
      startRequest = Promise.reject(error);
    }

    return Promise.resolve(startRequest).then(function (sessionId) {
      if (state.previewStarting === request) state.previewStarting = null;

      if (request.promoted) {
        removeHost();
        return text(sessionId);
      }

      const stale = request.cancelled || !currentIntentMatches(token, backendId, channelId);
      if (stale) {
        if (playback && typeof playback.destroy === 'function') {
          try { playback.destroy(); } catch (_) {}
        }
        removeHost();
        const latest = heroSnapshot();
        if (latest && latest.active === true) schedulePreview();
        return '';
      }

      if (!text(sessionId)) {
        if (playback && typeof playback.destroy === 'function') {
          try { playback.destroy(); } catch (_) {}
        }
        state.failedToken = token;
        setPreviewStatus('Live-Vorschau konnte nicht gestartet werden.', true);
        return '';
      }

      state.previewPlayback = playback;
      state.previewChannelId = channelId;
      state.previewBackendId = backendId;
      mountPreview(playback);
      return text(sessionId);
    }).catch(function (error) {
      if (state.previewStarting === request) state.previewStarting = null;
      if (!request.promoted && playback && typeof playback.destroy === 'function') {
        try { playback.destroy(); } catch (_) {}
      }
      if (!request.cancelled && currentIntentMatches(token, backendId, channelId)) {
        state.failedToken = token;
        setPreviewStatus(error && error.message ? error.message : 'Live-Vorschau konnte nicht gestartet werden.', true);
      } else {
        const latest = heroSnapshot();
        if (latest && latest.active === true) schedulePreview();
      }
      return '';
    });
  }

  function promotePreviewToFull() {
    cancelPendingPreview();
    state.focusToken += 1;
    if (state.previewStarting) {
      state.previewStarting.promoted = true;
      state.previewStarting = null;
    }
    state.previewPlayback = null;
    state.previewChannelId = '';
    state.previewBackendId = '';
    removeHost();
    state.status = 'Explizite Live-Wiedergabe übernimmt den kanonischen Owner.';
  }

  function selectionChanged(snapshot) {
    return text(snapshot && snapshot.backendId) !== state.lastBackendId ||
      text(snapshot && snapshot.selectedChannelId) !== state.lastChannelId;
  }

  function sync() {
    state.syncScheduled = false;
    const snapshot = heroSnapshot();
    if (!snapshot || snapshot.active !== true) {
      if (state.lastBackendId || state.lastChannelId || state.previewPlayback || state.previewStarting || state.pendingTimer !== null) {
        state.focusToken += 1;
        cancelPreview('Home verlassen');
      }
      state.lastBackendId = '';
      state.lastChannelId = '';
      state.failedToken = -1;
      return false;
    }

    if (selectionChanged(snapshot)) {
      state.focusToken += 1;
      cancelPreview('Senderauswahl geändert');
      state.lastBackendId = text(snapshot.backendId);
      state.lastChannelId = text(snapshot.selectedChannelId);
      state.failedToken = -1;
      schedulePreview();
      return true;
    }

    if (state.previewPlayback) {
      mountPreview(state.previewPlayback);
      return true;
    }

    if (!state.previewStarting && state.pendingTimer === null && state.failedToken !== state.focusToken) {
      schedulePreview();
    }
    return true;
  }

  function scheduleSync() {
    if (state.syncScheduled) return;
    state.syncScheduled = true;
    const schedule = typeof global.setTimeout === 'function' ? global.setTimeout : setTimeout;
    schedule(sync, 0);
  }

  function preemptForBrowse(event) {
    const key = event && event.key;
    if (key !== 'ArrowLeft' && key !== 'ArrowRight') return;
    const root = heroRoot();
    if (!root || (event.target !== root && !(root.contains && root.contains(event.target)))) return;
    state.focusToken += 1;
    cancelPreview('Senderauswahl bewegt');
  }

  function captureAction(event) {
    const target = event && event.target;
    if (!target || typeof target.closest !== 'function') return;
    if (target.closest('[data-home-live-action="watch"]')) {
      promotePreviewToFull();
      return;
    }
    const epg = target.closest('[data-home-live-action="epg"]');
    const neighbor = target.closest('.media-home-live-neighbor');
    if (epg || neighbor) {
      state.focusToken += 1;
      cancelPreview(epg ? 'EPG geöffnet' : 'Senderauswahl bewegt');
    }
  }

  function bindInput() {
    if (state.inputBound || !doc || typeof doc.addEventListener !== 'function') return;
    state.inputBound = true;
    doc.addEventListener('keydown', preemptForBrowse, true);
    doc.addEventListener('click', captureAction, true);
    // The Hero applies swipe selection on touchend. Schedule after bubbling so
    // the updated focus token/channel can be observed without cancelling taps.
    doc.addEventListener('touchend', scheduleSync, {passive: true});
  }

  function installStyles() {
    if (!doc || !doc.head || typeof doc.createElement !== 'function') return;
    if (typeof doc.getElementById === 'function' && doc.getElementById('vdr-suite-home-live-preview-style')) return;
    const style = doc.createElement('style');
    style.id = 'vdr-suite-home-live-preview-style';
    style.textContent = `
.media-home-live-preview{min-width:0;overflow:hidden;border:1px solid rgba(125,211,252,.26);border-radius:1rem;background:rgba(2,6,23,.86);box-shadow:0 .9rem 2.2rem rgba(2,6,23,.28)}
.media-home-live-preview-status{display:grid;place-items:center;min-height:8rem;padding:.8rem;color:#94a3b8;font-size:.8rem;text-align:center}.media-home-live-preview-status.error{color:#fecaca}
.media-home-live-preview .recordings2-playback{margin:0!important;padding:0!important;border:0!important;background:#000!important;box-shadow:none!important}.media-home-live-preview .recordings2-section-title,.media-home-live-preview .recordings2-playback-status{display:none!important}.media-home-live-preview video{display:block!important;width:100%!important;max-height:18rem!important;aspect-ratio:16/9;object-fit:contain;background:#000}
@media(min-width:46.01rem){.media-home-live-focus:has(.media-home-live-preview){grid-template-columns:minmax(0,1.2fr) minmax(16rem,.8fr);align-items:center}.media-home-live-focus:has(.media-home-live-preview)>.media-home-live-channel-head,.media-home-live-focus:has(.media-home-live-preview)>.media-home-live-programmes,.media-home-live-focus:has(.media-home-live-preview)>.media-home-live-actions,.media-home-live-focus:has(.media-home-live-preview)>.media-home-live-notice{grid-column:1}.media-home-live-focus:has(.media-home-live-preview)>.media-home-live-preview{grid-column:2;grid-row:1/span 4;align-self:stretch;display:grid;align-items:center}}
@media(max-width:46rem){.media-home-live-preview{order:2}.media-home-live-programmes{order:3}.media-home-live-actions{order:4}.media-home-live-notice{order:5}.media-home-live-preview video{max-height:14rem!important}}
`;
    doc.head.appendChild(style);
  }

  function installObservers() {
    if (!doc || typeof global.MutationObserver !== 'function') return;
    const root = heroRoot();
    if (root && !state.rootObserver) {
      state.rootObserver = new global.MutationObserver(scheduleSync);
      state.rootObserver.observe(root, {childList: true, subtree: true});
    }
    if (!state.shellObserver) {
      const nav = typeof doc.getElementById === 'function' ? doc.getElementById('module-nav') : null;
      const backends = typeof doc.getElementById === 'function' ? doc.getElementById('backends') : null;
      if (nav || backends) {
        state.shellObserver = new global.MutationObserver(scheduleSync);
        if (nav) state.shellObserver.observe(nav, {subtree: true, attributes: true, attributeFilter: ['class']});
        if (backends) state.shellObserver.observe(backends, {subtree: true, childList: true, attributes: true, attributeFilter: ['class', 'aria-selected']});
      }
    }
  }

  function install() {
    if (!doc) return false;
    installStyles();
    bindInput();
    installObservers();
    scheduleSync();
    return true;
  }

  function snapshot() {
    return Object.freeze({
      focusToken: state.focusToken,
      pending: state.pendingTimer !== null,
      starting: Boolean(state.previewStarting),
      active: Boolean(state.previewPlayback),
      backendId: state.previewBackendId,
      channelId: state.previewChannelId,
      status: state.status
    });
  }

  global.VdrSuiteHomeLivePreview = Object.freeze({
    install: install,
    sync: sync,
    snapshot: snapshot,
    __test: Object.freeze({
      previewSettleMs: previewSettleMs,
      schedulePreview: schedulePreview,
      startPreview: startPreview,
      cancelPendingPreview: cancelPendingPreview,
      cancelPreview: cancelPreview,
      promotePreviewToFull: promotePreviewToFull,
      currentIntentMatches: currentIntentMatches,
      ownsShellPreview: ownsShellPreview
    })
  });

  if (doc) {
    if (doc.readyState === 'loading' && typeof doc.addEventListener === 'function') {
      doc.addEventListener('DOMContentLoaded', install, {once: true});
    } else {
      install();
    }
  }
}(window));
