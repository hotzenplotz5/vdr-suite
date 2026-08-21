// Compatibility bridge between the existing channel browser and the day programme extension.
(function(global) {
  'use strict';

  const SOURCE_SELECTOR = '.channel-browser-module';
  const SOURCE_CLASS = 'channel-browser-shell';
  const detailState = new WeakMap();

  function addSourceClass(element) {
    if (!element || !element.classList || !element.classList.contains('channel-browser-module')) {
      return false;
    }

    element.classList.add(SOURCE_CLASS);
    return true;
  }

  function markChannelBrowserShells(root) {
    const scope = root && typeof root.querySelectorAll === 'function' ? root : document;
    const shells = [];

    if (scope && typeof scope.matches === 'function' && scope.matches(SOURCE_SELECTOR)) {
      shells.push(scope);
    }

    if (scope && typeof scope.querySelectorAll === 'function') {
      scope.querySelectorAll(SOURCE_SELECTOR).forEach(shell => shells.push(shell));
    }

    let marked = 0;
    shells.forEach(shell => {
      if (addSourceClass(shell)) marked += 1;
    });
    return marked;
  }

  function normalizedActionLabel(value) {
    return String(value || '')
      .trim()
      .replace(/\u2026/g, '...')
      .replace(/\s+/g, ' ')
      .toLowerCase();
  }

  function normalizeDetailActions(scope) {
    if (!scope || typeof scope.querySelectorAll !== 'function') return;

    scope.querySelectorAll('.epg-detail-action').forEach(button => {
      const label = normalizedActionLabel(button.textContent);

      if (label === 'serie automatisch aufnehmen') {
        button.textContent = 'Serientimer';
        button.title = 'Serientimer als SearchTimer mit Treffer-Vorschau vorbereiten.';
      } else if (label === 'erweiterter searchtimer') {
        button.title = 'Vollständigen SearchTimer-Editor mit dieser Sendung öffnen.';
      }
    });
  }

  function installStyles() {
    if (document.getElementById('vdr-suite-channel-day-program-compat-style')) return;

    const style = document.createElement('style');
    style.id = 'vdr-suite-channel-day-program-compat-style';
    style.textContent = `
.channel-browser-shell[hidden]{display:none!important;}
.channel-day-program-view.channel-day-detail-mode .channel-day-date-controls,
.channel-day-program-view.channel-day-detail-mode .channel-day-heading,
.channel-day-program-view.channel-day-detail-mode .channel-day-status,
.channel-day-program-view.channel-day-detail-mode .channel-day-list{display:none!important;}
.channel-day-program-view.channel-day-detail-mode .channel-day-event-detail{margin:0;scroll-margin-top:.75rem;}
.channel-day-program-view.channel-day-detail-mode .channel-day-toolbar{align-items:flex-start;}
`;
    document.head.appendChild(style);
  }

  function activateDetailMode(row) {
    if (!row || typeof row.closest !== 'function') return;

    const view = row.closest('.channel-day-program-view');
    if (!view) return;

    global.setTimeout(() => {
      const wrapper = view.querySelector('.channel-day-event-detail');
      const channelHeader = view.querySelector('.channel-day-channel-head');
      if (!wrapper || !channelHeader) return;

      detailState.set(view, {
        row,
        windowY: global.scrollY || 0
      });

      view.classList.add('channel-day-detail-mode');
      channelHeader.insertAdjacentElement('afterend', wrapper);
      normalizeDetailActions(wrapper);
      wrapper.scrollIntoView({behavior: 'smooth', block: 'start'});
    }, 0);
  }

  function restoreProgrammeView(backButton) {
    if (!backButton || typeof backButton.closest !== 'function') return;

    const view = backButton.closest('.channel-day-program-view');
    if (!view) return;

    const saved = detailState.get(view) || null;
    global.setTimeout(() => {
      view.classList.remove('channel-day-detail-mode');
      detailState.delete(view);

      if (saved && saved.row && saved.row.isConnected) {
        global.scrollTo(0, saved.windowY);
        saved.row.focus({preventScroll: true});
      }
    }, 0);
  }

  function requestLateDayProgrammeRuntime(root) {
    if (!root || root.dataset.channelDayProgramBound === 'true') return;
    if (global.VdrSuiteChannels2 && global.VdrSuiteLivePlayback) return;
    if (root.dataset.channelDayProgramLateRequested === 'true') return;

    root.dataset.channelDayProgramLateRequested = 'true';

    const script = document.createElement('script');
    script.src = '/frontend/channel-day-program.js?late=' + String(Date.now());
    script.async = false;
    script.dataset.channelDayProgramLateRuntime = 'true';
    script.addEventListener('error', () => {
      root.dataset.channelDayProgramLateRequested = 'false';
    });
    document.head.appendChild(script);
  }

  function installRoot(root) {
    if (!root) return false;

    installStyles();
    markChannelBrowserShells(root);
    requestLateDayProgrammeRuntime(root);

    if (root.dataset.channelDayProgramCompatBound === 'true') return true;
    root.dataset.channelDayProgramCompatBound = 'true';

    root.addEventListener('pointerdown', event => {
      const target = event.target && typeof event.target.closest === 'function'
        ? event.target.closest(SOURCE_SELECTOR)
        : null;
      addSourceClass(target);
    }, true);

    root.addEventListener('click', event => {
      const target = event.target && typeof event.target.closest === 'function'
        ? event.target
        : null;
      if (!target) return;

      const row = target.closest('.channel-day-event');
      if (row) activateDetailMode(row);

      const back = target.closest('.channel-day-event-back');
      if (back) restoreProgrammeView(back);
    }, true);

    const observer = new MutationObserver(records => {
      records.forEach(record => {
        record.addedNodes.forEach(node => {
          if (!node || node.nodeType !== 1) return;
          markChannelBrowserShells(node);
          normalizeDetailActions(node);
        });
      });
    });
    observer.observe(root, {childList: true, subtree: true});
    return true;
  }

  function installWhenReady() {
    const root = document.getElementById('detail-data');
    if (installRoot(root)) return true;
    return false;
  }

  function watchDocument() {
    installWhenReady();

    if (!document.documentElement || document.documentElement.dataset.channelDayProgramDocumentWatch === 'true') {
      return;
    }

    document.documentElement.dataset.channelDayProgramDocumentWatch = 'true';
    const observer = new MutationObserver(() => installWhenReady());
    observer.observe(document.documentElement, {childList: true, subtree: true});
  }

  global.VdrSuiteChannelDayProgramCompat = Object.freeze({
    markChannelBrowserShells,
    normalizedActionLabel,
    installWhenReady
  });

  if (typeof document !== 'undefined') {
    if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', watchDocument);
    else watchDocument();
  }
})(window);

// Phase 65.D.1: persistent shell-level ownership for the accepted Live-TV
// adapter. It reuses the existing MediaSession/player and changes only browser
// ownership while the user navigates inside VDR-Suite.
(function(global) {
  'use strict';

  const already = global.VdrSuitePlaybackShell;
  if (already && typeof already.install === 'function') {
    already.install();
    return;
  }

  const doc = global.document || (typeof document !== 'undefined' ? document : null);
  const state = {
    owner: null,
    lastStopReason: '',
    deactivationDepth: 0,
    mini: null,
    slot: null,
    title: null,
    playPause: null,
    pip: null,
    fullscreen: null,
    playbackInstalled: false,
    baseGet: null,
    baseSet: null,
    baseValue: null,
    facade: null,
    wrappedChannels: null,
    sessionBound: false,
    sessionKnown: false,
    authenticated: false,
    backendBound: false,
    restoreObserver: null,
    restoreTimer: null
  };

  function text(value) { return value === undefined || value === null ? '' : String(value).trim(); }
  function channelId(channel) { return channel && typeof channel === 'object' ? text(channel.channelId || channel.id || channel.nativeId) : ''; }
  function channelName(channel) { return channel && typeof channel === 'object' ? text(channel.name || channel.channelName || channel.title || channelId(channel)) || 'Live-TV' : 'Live-TV'; }
  function selectedBackend() {
    const platform = global.VdrSuitePlatform;
    return platform && typeof platform.getSelectedBackendId === 'function' ? text(platform.getSelectedBackendId()) : '';
  }
  function selectedModule() {
    const platform = global.VdrSuitePlatform;
    return platform && typeof platform.getSelectedModule === 'function' ? text(platform.getSelectedModule()) : '';
  }
  function videoOf(element) {
    if (!element) return null;
    return String(element.tagName || '').toUpperCase() === 'VIDEO'
      ? element
      : (typeof element.querySelector === 'function' ? element.querySelector('video') : null);
  }
  function pipSupported(video) {
    return Boolean(doc && doc.pictureInPictureEnabled === true && video && typeof video.requestPictureInPicture === 'function');
  }
  function remove(element) {
    if (element && element.parentNode && typeof element.parentNode.removeChild === 'function') element.parentNode.removeChild(element);
  }

  function button(label, title, action) {
    const value = doc.createElement('button');
    value.type = 'button';
    value.textContent = label;
    value.title = title || label;
    value.addEventListener('click', action);
    return value;
  }

  function installStyles() {
    if (!doc || !doc.head || typeof doc.createElement !== 'function') return;
    if (typeof doc.getElementById === 'function' && doc.getElementById('vdr-suite-playback-shell-style')) return;
    const style = doc.createElement('style');
    style.id = 'vdr-suite-playback-shell-style';
    style.textContent = `
#vdr-suite-live-mini-player[hidden]{display:none!important}
#vdr-suite-live-mini-player{position:fixed;right:1rem;bottom:1rem;z-index:9000;width:min(28rem,calc(100vw - 2rem));padding:.65rem;border:1px solid rgba(56,189,248,.58);border-radius:1rem;background:rgba(2,6,23,.96);box-shadow:0 1rem 3rem rgba(2,6,23,.55);backdrop-filter:blur(12px)}
.vdr-suite-live-mini-head{display:grid;grid-template-columns:minmax(0,1fr) auto;align-items:center;gap:.55rem;margin-bottom:.5rem}.vdr-suite-live-mini-title{overflow:hidden;color:#f8fafc;font-size:.82rem;font-weight:800;white-space:nowrap;text-overflow:ellipsis}.vdr-suite-live-mini-actions{display:flex;flex-wrap:wrap;justify-content:flex-end;gap:.3rem}.vdr-suite-live-mini-actions button{min-height:2rem;padding:.3rem .48rem;border-radius:.55rem;font-size:.76rem}.vdr-suite-live-mini-slot{overflow:hidden;border-radius:.72rem;background:#000}.vdr-suite-live-mini-slot .recordings2-section-title,.vdr-suite-live-mini-slot .recordings2-playback-status{display:none!important}.vdr-suite-live-mini-slot video{display:block!important;width:100%!important;max-height:min(32vh,18rem)!important;background:#000}
@media(max-width:640px){#vdr-suite-live-mini-player{right:.5rem;bottom:.5rem;width:calc(100vw - 1rem)}.vdr-suite-live-mini-head{grid-template-columns:1fr}.vdr-suite-live-mini-actions{justify-content:flex-start}}
`;
    doc.head.appendChild(style);
  }

  function openLiveView() {
    if (!doc || typeof doc.querySelector !== 'function') return false;
    const target = doc.querySelector('[data-module="channels2"]');
    if (!target) return false;
    if (typeof target.click === 'function') target.click();
    else if (typeof target.onclick === 'function') target.onclick();
    return true;
  }

  function wireLiveBrandEntry() {
    if (!doc || typeof doc.querySelector !== 'function') return false;
    const label = doc.querySelector('[data-i18n="shell.liveTv"]');
    if (!label) return false;
    const entry = typeof label.closest === 'function' ? label.closest('.brand-feature') : label.parentNode && label.parentNode.parentNode;
    if (!entry) return false;
    if (entry.dataset && entry.dataset.vdrSuiteLiveEntryBound === 'true') return true;
    if (entry.dataset) {
      entry.dataset.brandModule = 'channels2';
      entry.dataset.vdrSuiteLiveEntryBound = 'true';
    }
    if (typeof entry.setAttribute === 'function') {
      entry.setAttribute('data-brand-module', 'channels2');
      entry.setAttribute('tabindex', '0');
      entry.setAttribute('role', 'button');
      entry.setAttribute('aria-label', 'Live TV');
      entry.setAttribute('data-i18n-aria-label', 'shell.liveTv');
    }
    const open = function(event) {
      if (event && typeof event.preventDefault === 'function') event.preventDefault();
      if (event && typeof event.stopImmediatePropagation === 'function') event.stopImmediatePropagation();
      openLiveView();
    };
    if (typeof entry.addEventListener === 'function') {
      entry.addEventListener('click', open);
      entry.addEventListener('keydown', function(event) {
        if (event && (event.key === 'Enter' || event.key === ' ')) open(event);
      });
    }
    return true;
  }

  function ownerVideo() { return state.owner ? state.owner.video : null; }
  function updateControls() {
    if (!state.owner) return;
    if (state.title) state.title.textContent = 'Live-TV · ' + state.owner.channelName;
    if (state.playPause && state.owner.video) state.playPause.textContent = state.owner.video.paused ? 'Play' : 'Pause';
    if (state.pip) state.pip.hidden = !pipSupported(state.owner.video);
    if (state.fullscreen) {
      state.fullscreen.hidden = !Boolean(
        state.owner.element && typeof state.owner.element.requestFullscreen === 'function' ||
        state.owner.video && typeof state.owner.video.requestFullscreen === 'function'
      );
    }
  }
  function hideMini() {
    if (state.mini) state.mini.hidden = true;
    if (state.owner) state.owner.miniVisible = false;
  }
  function ensureMini() {
    if (state.mini) return state.mini;
    if (!doc || !doc.body || typeof doc.createElement !== 'function') return null;
    installStyles();
    const root = doc.createElement('section');
    root.id = 'vdr-suite-live-mini-player';
    root.hidden = true;
    root.setAttribute('aria-label', 'Persistenter Live-TV Mini-Player');
    const head = doc.createElement('div');
    head.className = 'vdr-suite-live-mini-head';
    state.title = doc.createElement('div');
    state.title.className = 'vdr-suite-live-mini-title';
    const actions = doc.createElement('div');
    actions.className = 'vdr-suite-live-mini-actions';
    state.playPause = button('Pause', 'Wiedergabe pausieren oder fortsetzen', function() {
      const video = ownerVideo();
      if (!video) return;
      if (video.paused && typeof video.play === 'function') {
        const request = video.play();
        if (request && typeof request.catch === 'function') request.catch(function() {});
      } else if (typeof video.pause === 'function') video.pause();
      updateControls();
    });
    state.fullscreen = button('Vollbild', 'Live-TV im Vollbild anzeigen', function() {
      if (!state.owner) return;
      const target = state.owner.element && typeof state.owner.element.requestFullscreen === 'function' ? state.owner.element : state.owner.video;
      if (!target || typeof target.requestFullscreen !== 'function') return;
      const request = target.requestFullscreen();
      if (request && typeof request.catch === 'function') request.catch(function() {});
    });
    state.pip = button('PiP', 'Browser Picture-in-Picture starten', function() {
      const video = ownerVideo();
      if (!pipSupported(video)) return;
      const request = doc.pictureInPictureElement === video && typeof doc.exitPictureInPicture === 'function'
        ? doc.exitPictureInPicture()
        : video.requestPictureInPicture();
      if (request && typeof request.catch === 'function') request.catch(function() {});
    });
    [button('Live-TV', 'Zur Live-TV-Ansicht zurückkehren', openLiveView), state.playPause, state.fullscreen, state.pip, button('Beenden', 'Live-TV beenden', function() { stop('explicit_shell_stop'); })]
      .forEach(function(entry) { actions.appendChild(entry); });
    head.appendChild(state.title);
    head.appendChild(actions);
    state.slot = doc.createElement('div');
    state.slot.className = 'vdr-suite-live-mini-slot';
    root.appendChild(head);
    root.appendChild(state.slot);
    doc.body.appendChild(root);
    state.mini = root;
    return root;
  }

  function clearRestore() {
    if (state.restoreObserver && typeof state.restoreObserver.disconnect === 'function') state.restoreObserver.disconnect();
    if (state.restoreTimer !== null && typeof global.clearTimeout === 'function') global.clearTimeout(state.restoreTimer);
    state.restoreObserver = null;
    state.restoreTimer = null;
  }
  function mountTarget() {
    const platform = global.VdrSuitePlatform;
    if (platform && typeof platform.getMountTarget === 'function') {
      const target = platform.getMountTarget('channels2') || platform.getMountTarget('channels') || platform.getMountTarget('detail');
      if (target) return target;
    }
    return doc && typeof doc.getElementById === 'function' ? doc.getElementById('detail-data') : null;
  }
  function restoreIntoChannels() {
    if (!state.owner || state.owner.stopped || state.owner.yielded) return false;
    if (selectedModule() && selectedModule() !== 'channels2') return false;
    const root = mountTarget();
    if (!root || typeof root.querySelector !== 'function') return false;
    const start = root.querySelector('.channels2-live button');
    if (!start || start.disabled) return false;
    clearRestore();
    if (typeof start.click === 'function') start.click();
    else if (typeof start.onclick === 'function') start.onclick();
    return true;
  }
  function scheduleRestore() {
    if (!state.owner || state.owner.stopped || state.owner.yielded || restoreIntoChannels()) return;
    const root = mountTarget();
    if (!root || typeof global.MutationObserver !== 'function') return;
    clearRestore();
    state.restoreObserver = new global.MutationObserver(restoreIntoChannels);
    state.restoreObserver.observe(root, {childList: true, subtree: true});
    if (typeof global.setTimeout === 'function') state.restoreTimer = global.setTimeout(clearRestore, 2500);
  }

  function detach(playback) {
    const current = state.owner;
    if (!current || current.proxy !== playback || current.stopped || current.yielded || checkBackendBoundary()) return false;
    const mini = ensureMini();
    if (!mini || !state.slot) return false;
    state.slot.appendChild(current.element);
    current.miniVisible = true;
    updateControls();
    mini.hidden = false;
    return true;
  }
  function attach(playback) {
    if (!state.owner || state.owner.proxy !== playback || state.owner.stopped || state.owner.yielded) return false;
    hideMini();
    return true;
  }
  function release(playback, reason) {
    if (!state.owner || state.owner.proxy !== playback) return false;
    state.lastStopReason = text(reason) || state.lastStopReason;
    clearRestore();
    hideMini();
    if (state.slot && state.owner.element && state.owner.element.parentNode === state.slot) remove(state.owner.element);
    state.owner = null;
    return true;
  }
  function stop(reason) {
    if (!state.owner) return false;
    const current = state.owner;
    current.stopped = true;
    state.lastStopReason = text(reason) || 'explicit_shell_stop';
    clearRestore();
    hideMini();
    state.owner = null;
    if (state.slot && current.element && current.element.parentNode === state.slot) remove(current.element);
    if (current.actual && typeof current.actual.destroy === 'function') current.actual.destroy();
    return true;
  }
  function checkBackendBoundary() {
    if (!state.owner) return false;
    const selected = selectedBackend();
    if (!selected || !state.owner.backendId || selected === state.owner.backendId) return false;
    stop('backend_changed');
    return true;
  }

  function bindBackendBoundary() {
    if (state.backendBound || !doc || typeof global.MutationObserver !== 'function') return state.backendBound;
    const root = typeof doc.getElementById === 'function' ? doc.getElementById('backends') : null;
    if (!root) return false;
    const observer = new global.MutationObserver(function() {
      if (typeof global.setTimeout === 'function') global.setTimeout(checkBackendBoundary, 0);
      else checkBackendBoundary();
    });
    observer.observe(root, {attributes: true, attributeFilter: ['class'], childList: true, subtree: true});
    state.backendBound = true;
    return true;
  }
  function bindBrowserSession() {
    if (state.sessionBound) return true;
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.subscribe !== 'function') return false;
    state.sessionBound = true;
    session.subscribe(function(value) {
      const authenticated = Boolean(value && value.authenticated);
      if (!state.sessionKnown) {
        state.sessionKnown = true;
        state.authenticated = authenticated;
        return;
      }
      const previous = state.authenticated;
      state.authenticated = authenticated;
      if (previous && !authenticated && state.owner) stop('browser_session_lost');
    });
    return true;
  }

  function sameOwner(channel, backendId) {
    return Boolean(state.owner && !state.owner.stopped && !state.owner.yielded && state.owner.backendId === text(backendId || 'default') && state.owner.channelId === channelId(channel));
  }
  function proxyFor(actual, channel, backendId) {
    const proxy = {};
    let startPromise = null;
    let yieldedPromise = null;
    Object.keys(actual || {}).forEach(function(key) { proxy[key] = actual[key]; });
    proxy.element = actual.element;
    proxy.start = function() {
      attach(proxy);
      if (!startPromise) startPromise = Promise.resolve(actual.start()).then(function(id) {
        if (state.owner && state.owner.proxy === proxy) state.owner.sessionId = text(id);
        return id;
      });
      return startPromise;
    };
    proxy.destroy = function() {
      if (state.deactivationDepth > 0 && state.owner && state.owner.proxy === proxy) {
        detach(proxy);
        return;
      }
      if (state.owner && state.owner.proxy === proxy) {
        stop('explicit_live_stop');
        return;
      }
      if (typeof actual.destroy === 'function') actual.destroy();
    };
    proxy.relinquishForReplacement = function() {
      if (yieldedPromise) return yieldedPromise;
      if (typeof actual.relinquishForReplacement !== 'function') {
        proxy.destroy();
        return Promise.resolve('');
      }
      yieldedPromise = Promise.resolve(actual.relinquishForReplacement()).then(function(id) {
        const sessionId = text(id);
        if (!state.owner || state.owner.proxy !== proxy) return sessionId;
        if (!sessionId) {
          release(proxy, 'replacement_relinquish_failed');
          return '';
        }
        state.owner.yielded = true;
        state.owner.yieldedSessionId = sessionId;
        state.lastStopReason = 'replacement_relinquish';
        hideMini();
        return sessionId;
      });
      return yieldedPromise;
    };
    proxy.sessionId = function() {
      if (state.owner && state.owner.proxy === proxy && state.owner.sessionId) return state.owner.sessionId;
      return typeof actual.sessionId === 'function' ? actual.sessionId() : '';
    };
    return Object.freeze(proxy);
  }
  function adopt(actual, channel, backendId) {
    const proxy = proxyFor(actual, channel, backendId);
    const video = videoOf(actual.element);
    state.owner = {actual, proxy, element: actual.element, video, channelId: channelId(channel), channelName: channelName(channel), backendId: text(backendId || 'default'), yielded: false, yieldedSessionId: '', stopped: false, miniVisible: false, sessionId: ''};
    state.lastStopReason = '';
    hideMini();
    if (video && typeof video.addEventListener === 'function') {
      video.addEventListener('error', function() { if (state.owner && state.owner.proxy === proxy && !state.owner.yielded) stop('playback_error'); });
      video.addEventListener('ended', function() { if (state.owner && state.owner.proxy === proxy && !state.owner.yielded) stop('playback_ended'); });
      video.addEventListener('play', updateControls);
      video.addEventListener('pause', updateControls);
    }
    return proxy;
  }
  function createLive(rawFactory, channel, backendId, options) {
    const settings = options && typeof options === 'object' ? options : {};
    const backend = text(backendId || 'default');
    const replacement = text(settings.replacesSessionId);
    checkBackendBoundary();
    if (sameOwner(channel, backend) && !replacement) return state.owner.proxy;
    if (state.owner) {
      if (!state.owner.yielded || !replacement || replacement !== state.owner.yieldedSessionId) {
        throw new Error('Live-TV Replacement-Handoff fehlt oder ist ungültig.');
      }
      release(state.owner.proxy, 'replacement_handoff');
    }
    const actual = rawFactory(channel, backend, settings);
    if (!actual || !actual.element || typeof actual.start !== 'function' || typeof actual.destroy !== 'function') throw new Error('Live-TV Playback Adapter ist unvollständig.');
    return adopt(actual, channel, backend);
  }

  function basePlayback() { return state.baseGet ? state.baseGet.call(global) : state.baseValue; }
  function wrapPlayback(source) {
    if (!source || typeof source !== 'object' || typeof source.createLivePanel !== 'function') return null;
    const wrapped = {};
    Object.keys(source).forEach(function(key) { wrapped[key] = source[key]; });
    const rawFactory = source.createLivePanel;
    wrapped.createLivePanel = function(channel, backendId, options) { return createLive(rawFactory, channel, backendId, options); };
    return Object.freeze(wrapped);
  }
  function refreshPlayback() { state.facade = wrapPlayback(basePlayback()); return Boolean(state.facade); }
  function installPlayback() {
    if (state.playbackInstalled) return refreshPlayback();
    const descriptor = Object.getOwnPropertyDescriptor(global, 'VdrSuiteRecordings2Playback');
    if (descriptor && typeof descriptor.get === 'function' && typeof descriptor.set === 'function') {
      state.baseGet = descriptor.get;
      state.baseSet = descriptor.set;
    } else state.baseValue = global.VdrSuiteRecordings2Playback || null;
    if (!refreshPlayback()) return false;
    try {
      Object.defineProperty(global, 'VdrSuiteRecordings2Playback', {
        configurable: true,
        enumerable: true,
        get: function() { return state.facade; },
        set: function(value) {
          if (state.baseSet) state.baseSet.call(global, value); else state.baseValue = value;
          refreshPlayback();
        }
      });
      state.playbackInstalled = true;
    } catch (error) {
      global.VdrSuiteRecordings2Playback = state.facade;
    }
    return true;
  }
  function installChannels() {
    const source = global.VdrSuiteChannels2;
    if (!source || typeof source !== 'object') return false;
    if (source === state.wrappedChannels || source.__vdrSuitePlaybackShellWrapped === true) return true;
    const wrapped = {};
    Object.keys(source).forEach(function(key) { wrapped[key] = source[key]; });
    if (typeof source.deactivate === 'function') wrapped.deactivate = function() {
      state.deactivationDepth += 1;
      try { return source.deactivate.apply(source, arguments); }
      finally { state.deactivationDepth -= 1; }
    };
    ['activate', 'renderList', 'refresh'].forEach(function(name) {
      if (typeof source[name] !== 'function') return;
      wrapped[name] = function() {
        const result = source[name].apply(source, arguments);
        scheduleRestore();
        return result;
      };
    });
    wrapped.__vdrSuitePlaybackShellWrapped = true;
    state.wrappedChannels = Object.freeze(wrapped);
    global.VdrSuiteChannels2 = state.wrappedChannels;
    return true;
  }

  function snapshot() {
    const current = state.owner;
    return Object.freeze({
      active: Boolean(current && !current.stopped && !current.yielded),
      backendId: current ? current.backendId : '',
      channelId: current ? current.channelId : '',
      channelName: current ? current.channelName : '',
      sessionId: current ? current.sessionId || text(current.proxy.sessionId()) : '',
      miniVisible: Boolean(current && current.miniVisible && state.mini && state.mini.hidden === false),
      pipAvailable: Boolean(current && pipSupported(current.video)),
      lastStopReason: state.lastStopReason
    });
  }
  function install() {
    wireLiveBrandEntry();
    installPlayback();
    installChannels();
    bindBrowserSession();
    bindBackendBoundary();
    return true;
  }

  const api = Object.freeze({
    install,
    detach,
    attach,
    release,
    stop: function() { return stop('explicit_shell_stop'); },
    snapshot,
    __test: Object.freeze({pipSupported, checkBackendBoundary, bindBrowserSession, bindBackendBoundary, restoreIntoChannels, scheduleRestoreIntoChannels: scheduleRestore, wireLiveBrandEntry})
  });
  global.VdrSuitePlaybackShell = api;
  install();
  if (doc && doc.readyState === 'loading' && typeof doc.addEventListener === 'function') doc.addEventListener('DOMContentLoaded', install, {once: true});
  else if (typeof global.setTimeout === 'function') global.setTimeout(install, 0);
  if (typeof global.addEventListener === 'function') global.addEventListener('pageshow', function(event) {
    if (event && event.persisted && state.owner) stop('pagehide_cleanup');
  });
})(window);
