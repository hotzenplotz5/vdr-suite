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

// Phase 65.D.1 product entry: dedicated Live-TV start view. The detailed
// Channels2 browser remains a separate programme/channel browser; this view
// owns the fast "pick a channel and watch" path while reusing the same
// persistent shell-level MediaSession/player.
(function(global) {
  'use strict';

  if (global.VdrSuiteLiveTvView) return;

  const doc = global.document || (typeof document !== 'undefined' ? document : null);
  const state = {
    active: false,
    backendId: '',
    channels: [],
    events: [],
    playback: null,
    liveChannelId: '',
    liveSwitching: false,
    liveError: '',
    loadingChannels: false,
    loadingPrograms: false,
    dataError: '',
    programError: '',
    requestSequence: 0,
    switchSequence: 0,
    hiddenTab: null,
    navigationBound: false
  };

  function text(value) {
    return value === undefined || value === null ? '' : String(value).trim();
  }

  function pick(object, keys, fallback) {
    for (let index = 0; index < keys.length; index += 1) {
      const key = keys[index];
      if (object && object[key] !== undefined && object[key] !== null && object[key] !== '') return object[key];
    }
    return fallback === undefined ? '' : fallback;
  }

  function list(data, key) {
    if (Array.isArray(data)) return data;
    if (data && Array.isArray(data[key])) return data[key];
    if (data && Array.isArray(data.items)) return data.items;
    if (data && Array.isArray(data.results)) return data.results;
    return [];
  }

  function channelId(channel) { return text(pick(channel, ['channelId', 'id', 'nativeId'])); }
  function channelName(channel) { return text(pick(channel, ['name', 'channelName', 'title', 'displayName'], channelId(channel) || 'Kanal')); }
  function channelNumber(channel) {
    const value = Number(pick(channel, ['number', 'channelNumber', 'position'], 0));
    return Number.isFinite(value) && value > 0 ? value : 0;
  }

  function boolValue(value, fallback) {
    if (value === true || value === 1 || value === '1') return true;
    if (value === false || value === 0 || value === '0') return false;
    const normalized = text(value).toLowerCase();
    if (normalized === 'true' || normalized === 'yes' || normalized === 'ja' || normalized === 'on') return true;
    if (normalized === 'false' || normalized === 'no' || normalized === 'nein' || normalized === 'off') return false;
    return Boolean(fallback);
  }

  function channelIsRadio(channel) { return boolValue(pick(channel, ['radio', 'isRadio'], false), false); }
  function channelIsEnabled(channel) { return boolValue(pick(channel, ['enabled', 'active'], true), true); }

  function epoch(value) {
    const number = Number(value);
    if (Number.isFinite(number) && number > 0) return number > 1e11 ? Math.floor(number / 1000) : Math.floor(number);
    const parsed = Date.parse(String(value || ''));
    return Number.isFinite(parsed) ? Math.floor(parsed / 1000) : 0;
  }

  function eventStart(event) { return epoch(pick(event, ['startTime', 'start', 'beginTime'], 0)); }
  function eventEnd(event) {
    const start = eventStart(event);
    const explicit = epoch(pick(event, ['endTime', 'end', 'stopTime'], 0));
    const duration = Number(pick(event, ['durationSeconds', 'duration'], 0));
    if (explicit > start) return explicit;
    return start + (Number.isFinite(duration) && duration > 0 ? duration : 0);
  }
  function eventTitle(event) { return text(pick(event, ['title', 'name', 'eventTitle'], 'Keine Programminformation')); }
  function eventSubtitle(event) { return text(pick(event, ['subtitle', 'shortText', 'short_text'])); }

  function resolvePublicUrl(value) {
    const url = text(value);
    const publicUrl = global.VdrSuitePublicUrl;
    if (url && publicUrl && typeof publicUrl.resolvePath === 'function' && url.charAt(0) === '/') return publicUrl.resolvePath(url);
    return url;
  }

  function eventArtwork(event) {
    const artwork = event && event.artwork;
    if (artwork && artwork.available === true && text(artwork.url)) return resolvePublicUrl(artwork.url);
    return resolvePublicUrl(pick(event, ['bannerUrl', 'imageUrl', 'posterUrl', 'artworkUrl', 'image', 'poster', 'banner'], ''));
  }

  function currentEventForChannel(channel, events, nowValue) {
    const id = channelId(channel);
    const now = Number.isFinite(Number(nowValue)) ? Number(nowValue) : Math.floor(Date.now() / 1000);
    const matches = (events || []).filter(function(event) {
      return text(pick(event, ['channelId', 'channel', 'channel_id'])) === id;
    }).sort(function(left, right) { return eventStart(left) - eventStart(right); });
    for (let index = 0; index < matches.length; index += 1) {
      const start = eventStart(matches[index]);
      const end = eventEnd(matches[index]);
      if (start > 0 && start <= now && (end === 0 || end > now)) return matches[index];
    }
    return channel && (channel.currentEvent || channel.now || channel.currentProgram) || null;
  }

  function formatClock(value) {
    const seconds = epoch(value);
    if (!seconds) return '';
    return new Date(seconds * 1000).toLocaleTimeString('de-DE', {hour: '2-digit', minute: '2-digit'});
  }
  function eventTime(event) {
    if (!event) return '';
    const start = formatClock(eventStart(event));
    const end = formatClock(eventEnd(event));
    return start && end ? start + '–' + end : start;
  }

  function platform() { return global.VdrSuitePlatform || null; }
  function clientApi() {
    const value = platform();
    return value && typeof value.getClientApi === 'function' ? value.getClientApi() : global.VdrSuiteClientApi;
  }
  function selectedBackend() {
    const value = platform();
    const backend = value && typeof value.getSelectedBackendId === 'function' ? text(value.getSelectedBackendId()) : '';
    return backend || state.backendId || 'default';
  }
  function mountTarget() {
    const value = platform();
    if (value && typeof value.getMountTarget === 'function') {
      const target = value.getMountTarget('livetv') || value.getMountTarget('detail');
      if (target) return target;
    }
    return doc && typeof doc.getElementById === 'function' ? doc.getElementById('detail-data') : null;
  }
  function playbackShell() { return global.VdrSuitePlaybackShell || null; }
  function playbackApi() { return global.VdrSuiteRecordings2Playback || null; }
  function addText(element, value) { element.textContent = String(value); return element; }
  function button(label, className) {
    const value = doc.createElement('button');
    value.type = 'button';
    value.textContent = label;
    if (className) value.className = className;
    return value;
  }

  function installStyles() {
    if (!doc || !doc.head || typeof doc.createElement !== 'function') return;
    if (typeof doc.getElementById === 'function' && doc.getElementById('vdr-suite-live-tv-view-style')) return;
    const style = doc.createElement('style');
    style.id = 'vdr-suite-live-tv-view-style';
    style.textContent = `
.vdr-suite-live-tv-view{display:grid;grid-column:1/-1;width:100%;gap:1rem}
.vdr-suite-live-tv-header{display:flex;align-items:flex-start;justify-content:space-between;gap:1rem;flex-wrap:wrap}.vdr-suite-live-tv-header h3,.vdr-suite-live-tv-header p{margin:0}.vdr-suite-live-tv-header h3{color:#f8fafc;font-size:clamp(1.35rem,3vw,2.15rem)}.vdr-suite-live-tv-header p{margin-top:.28rem;color:#94a3b8}
.vdr-suite-live-tv-status{padding:.75rem .9rem;border:1px solid rgba(148,163,184,.25);border-radius:.8rem;background:rgba(15,23,42,.72);color:#cbd5e1}.vdr-suite-live-tv-status.error{border-color:rgba(248,113,113,.5);color:#fecaca}
.vdr-suite-live-tv-player{display:grid;grid-column:1/-1;gap:.65rem;padding:.75rem;border:1px solid rgba(34,211,238,.42);border-radius:1rem;background:rgba(8,47,73,.42)}.vdr-suite-live-tv-player-head{display:flex;align-items:center;justify-content:space-between;gap:.75rem;flex-wrap:wrap}.vdr-suite-live-tv-player-title{display:grid;gap:.15rem;color:#f8fafc;font-weight:850}.vdr-suite-live-tv-player-title span{color:#a5f3fc;font-size:.82rem;font-weight:650}.vdr-suite-live-tv-stop{min-height:2.5rem;padding:.5rem .8rem;border:1px solid rgba(248,113,113,.62)!important;border-radius:.68rem;background:transparent!important;color:#fecaca!important}.vdr-suite-live-tv-player-slot{overflow:hidden;border-radius:.85rem;background:#000}.vdr-suite-live-tv-player-slot video{display:block!important;width:100%!important;max-height:min(64vh,42rem)!important;background:#000}
.vdr-suite-live-tv-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(13.5rem,1fr));gap:.8rem}.vdr-suite-live-tv-channel{position:relative;display:grid;grid-template-columns:5.25rem minmax(0,1fr);align-items:center;gap:.75rem;min-height:7rem;padding:.7rem;overflow:hidden;border:1px solid rgba(96,165,250,.28);border-radius:1rem;background:rgba(15,23,42,.82);color:#f8fafc;text-align:left;cursor:pointer;isolation:isolate}.vdr-suite-live-tv-channel:hover,.vdr-suite-live-tv-channel:focus-visible{border-color:#38bdf8;outline:none;box-shadow:0 .9rem 2rem rgba(2,132,199,.18);transform:translateY(-1px)}.vdr-suite-live-tv-channel.active{border-color:rgba(34,211,238,.8);background:rgba(8,47,73,.68)}.vdr-suite-live-tv-channel:disabled{cursor:not-allowed;opacity:.55}
.vdr-suite-live-tv-logo{display:grid;place-items:center;width:5.25rem;height:3.3rem;padding:.25rem;border-radius:.62rem;background:rgba(248,250,252,.96);overflow:hidden}.vdr-suite-live-tv-logo .channel-logo-frame,.vdr-suite-live-tv-logo img,.vdr-suite-live-tv-logo .channel-logo{width:100%!important;height:100%!important;max-width:100%!important;max-height:100%!important;object-fit:contain!important}.vdr-suite-live-tv-copy{display:grid;gap:.18rem;min-width:0}.vdr-suite-live-tv-name{overflow:hidden;color:#f8fafc;font-weight:900;white-space:nowrap;text-overflow:ellipsis}.vdr-suite-live-tv-meta{color:#94a3b8;font-size:.78rem}.vdr-suite-live-tv-now{overflow:hidden;color:#bae6fd;font-size:.82rem;font-weight:750;white-space:nowrap;text-overflow:ellipsis}
.vdr-suite-live-tv-preview{position:absolute;z-index:3;inset:0;display:grid;align-content:end;gap:.2rem;padding:.85rem;opacity:0;pointer-events:none;transform:translateY(.35rem);transition:opacity .16s ease,transform .16s ease;background-position:center;background-size:cover;color:#fff}.vdr-suite-live-tv-preview::before{content:"";position:absolute;z-index:-1;inset:0;background:linear-gradient(180deg,rgba(2,6,23,.12),rgba(2,6,23,.94) 68%)}.vdr-suite-live-tv-channel:hover .vdr-suite-live-tv-preview,.vdr-suite-live-tv-channel:focus-visible .vdr-suite-live-tv-preview{opacity:1;transform:translateY(0)}.vdr-suite-live-tv-preview-title{font-size:1rem;font-weight:900;text-shadow:0 1px 4px #000}.vdr-suite-live-tv-preview-meta{color:#bae6fd;font-size:.78rem;font-weight:750}.vdr-suite-live-tv-preview-subtitle{overflow:hidden;color:#e2e8f0;font-size:.78rem;white-space:nowrap;text-overflow:ellipsis}
@media(min-width:72rem){.vdr-suite-live-tv-grid{grid-template-columns:repeat(auto-fill,minmax(16.5rem,1fr))}.vdr-suite-live-tv-channel{min-height:8rem}}
@media(hover:none){.vdr-suite-live-tv-grid{grid-template-columns:repeat(2,minmax(0,1fr));gap:.55rem}.vdr-suite-live-tv-channel{grid-template-columns:1fr;align-content:start;min-height:11.5rem;padding:.55rem}.vdr-suite-live-tv-logo{width:100%;height:4rem}.vdr-suite-live-tv-copy{gap:.12rem}.vdr-suite-live-tv-preview{position:relative;inset:auto;min-height:4.3rem;margin:.3rem -.55rem -.55rem;padding:.55rem;opacity:1;transform:none;background-position:center 35%}.vdr-suite-live-tv-preview::before{background:linear-gradient(180deg,rgba(2,6,23,.35),rgba(2,6,23,.94))}.vdr-suite-live-tv-preview-title{font-size:.82rem}.vdr-suite-live-tv-preview-subtitle{display:none}}
@media(max-width:420px){.vdr-suite-live-tv-grid{grid-template-columns:1fr}.vdr-suite-live-tv-channel{grid-template-columns:5rem minmax(0,1fr);min-height:7rem}.vdr-suite-live-tv-logo{width:5rem;height:3.2rem}.vdr-suite-live-tv-preview{grid-column:1/-1}}
`;
    doc.head.appendChild(style);
  }

  function fallbackLogo(channel) {
    const frame = doc.createElement('div');
    frame.className = 'channel-logo-frame';
    frame.appendChild(addText(doc.createElement('div'), channelName(channel).charAt(0).toUpperCase() || '?'));
    return frame;
  }
  function createLogo(channel) {
    const wrapper = doc.createElement('div');
    wrapper.className = 'vdr-suite-live-tv-logo';
    const logo = typeof global.createChannelLogoElement === 'function' ? global.createChannelLogoElement(channelName(channel), channelId(channel)) : fallbackLogo(channel);
    wrapper.appendChild(logo);
    return wrapper;
  }
  function createPreview(event) {
    const preview = doc.createElement('span');
    preview.className = 'vdr-suite-live-tv-preview';
    const artwork = eventArtwork(event);
    if (artwork) preview.style.backgroundImage = 'url("' + artwork.replace(/"/g, '%22') + '")';
    const title = addText(doc.createElement('span'), event ? eventTitle(event) : 'Keine Programminformation');
    title.className = 'vdr-suite-live-tv-preview-title';
    preview.appendChild(title);
    const timing = addText(doc.createElement('span'), event ? eventTime(event) : '');
    timing.className = 'vdr-suite-live-tv-preview-meta';
    preview.appendChild(timing);
    const subtitle = event ? eventSubtitle(event) : '';
    if (subtitle) {
      const value = addText(doc.createElement('span'), subtitle);
      value.className = 'vdr-suite-live-tv-preview-subtitle';
      preview.appendChild(value);
    }
    return preview;
  }

  function createChannelTile(channel) {
    const tile = button('', 'vdr-suite-live-tv-channel');
    const id = channelId(channel);
    const event = currentEventForChannel(channel, state.events);
    tile.dataset.channelId = id;
    tile.disabled = state.liveSwitching || !channelIsEnabled(channel);
    if (state.liveChannelId === id && state.playback) tile.classList.add('active');
    tile.setAttribute('aria-label', channelName(channel) + ' live ansehen');
    tile.appendChild(createLogo(channel));
    const copy = doc.createElement('span');
    copy.className = 'vdr-suite-live-tv-copy';
    const name = addText(doc.createElement('span'), channelName(channel));
    name.className = 'vdr-suite-live-tv-name';
    copy.appendChild(name);
    const meta = addText(doc.createElement('span'), (channelNumber(channel) ? 'Kanal ' + channelNumber(channel) : 'TV') + (channelIsEnabled(channel) ? ' · verfügbar' : ' · deaktiviert'));
    meta.className = 'vdr-suite-live-tv-meta';
    copy.appendChild(meta);
    const now = addText(doc.createElement('span'), event ? eventTitle(event) : (state.loadingPrograms ? 'Programm wird geladen …' : 'Keine Programminformation'));
    now.className = 'vdr-suite-live-tv-now';
    copy.appendChild(now);
    tile.appendChild(copy);
    tile.appendChild(createPreview(event));
    tile.addEventListener('click', function() { startChannel(channel); });
    return tile;
  }

  function shellSnapshot() {
    const shell = playbackShell();
    return shell && typeof shell.snapshot === 'function' ? shell.snapshot() : null;
  }
  function synchronizePlaybackState() {
    const shell = playbackShell();
    const snapshot = shellSnapshot();
    if (!shell || !snapshot) return;
    if (!snapshot.active) {
      if (!state.liveSwitching) {
        state.playback = null;
        state.liveChannelId = '';
      }
      return;
    }
    if (snapshot.backendId && snapshot.backendId !== selectedBackend()) return;
    if (state.playback && state.liveChannelId === snapshot.channelId) return;
    const playback = playbackApi();
    if (!playback || typeof playback.createLivePanel !== 'function') return;
    const channel = state.channels.find(function(entry) { return channelId(entry) === snapshot.channelId; }) || {id: snapshot.channelId, name: snapshot.channelName || snapshot.channelId, enabled: true};
    try {
      state.playback = playback.createLivePanel(channel, snapshot.backendId || selectedBackend(), {});
      state.liveChannelId = snapshot.channelId;
    } catch (error) {
      state.liveError = error && error.message ? error.message : String(error || '');
    }
  }

  function renderPlayer(root) {
    synchronizePlaybackState();
    if (!state.playback || !state.liveChannelId) return;
    const snapshot = shellSnapshot();
    if (snapshot && !snapshot.active) return;
    const shell = playbackShell();
    if (shell && typeof shell.attach === 'function') shell.attach(state.playback);
    const box = doc.createElement('section');
    box.className = 'vdr-suite-live-tv-player';
    const head = doc.createElement('div');
    head.className = 'vdr-suite-live-tv-player-head';
    const title = doc.createElement('div');
    title.className = 'vdr-suite-live-tv-player-title';
    const currentChannel = state.channels.find(function(channel) { return channelId(channel) === state.liveChannelId; });
    title.appendChild(addText(doc.createElement('strong'), 'Live-TV · ' + (currentChannel ? channelName(currentChannel) : (snapshot && snapshot.channelName ? snapshot.channelName : state.liveChannelId))));
    title.appendChild(addText(doc.createElement('span'), 'Dieselbe VDR-Suite MediaSession bleibt bei interner Navigation aktiv.'));
    head.appendChild(title);
    const stopButton = button('Live-TV beenden', 'vdr-suite-live-tv-stop');
    stopButton.addEventListener('click', stop);
    head.appendChild(stopButton);
    box.appendChild(head);
    const slot = doc.createElement('div');
    slot.className = 'vdr-suite-live-tv-player-slot';
    slot.appendChild(state.playback.element);
    box.appendChild(slot);
    root.appendChild(box);
  }

  function render() {
    if (!state.active || !doc) return;
    const mount = mountTarget();
    if (!mount || typeof mount.replaceChildren !== 'function') return;
    installStyles();
    synchronizePlaybackState();
    mount.replaceChildren();
    if (mount.classList) {
      mount.classList.remove('channels2-mount');
      mount.classList.remove('recordings2-mount');
    }
    const root = doc.createElement('section');
    root.className = 'vdr-suite-live-tv-view';
    const header = doc.createElement('div');
    header.className = 'vdr-suite-live-tv-header';
    const copy = doc.createElement('div');
    copy.appendChild(addText(doc.createElement('h3'), 'Live TV'));
    copy.appendChild(addText(doc.createElement('p'), 'Sender wählen – die Wiedergabe startet sofort.'));
    header.appendChild(copy);
    root.appendChild(header);
    if (state.dataError) {
      const error = addText(doc.createElement('div'), state.dataError);
      error.className = 'vdr-suite-live-tv-status error';
      root.appendChild(error);
    } else if (state.loadingChannels && state.channels.length === 0) {
      const loading = addText(doc.createElement('div'), 'Sender werden geladen …');
      loading.className = 'vdr-suite-live-tv-status';
      root.appendChild(loading);
    }
    if (state.liveError) {
      const playbackError = addText(doc.createElement('div'), state.liveError);
      playbackError.className = 'vdr-suite-live-tv-status error';
      root.appendChild(playbackError);
    }
    if (state.channels.length > 0) {
      const grid = doc.createElement('section');
      grid.className = 'vdr-suite-live-tv-grid';
      grid.setAttribute('aria-label', 'Live-TV Sender');
      let playerRendered = false;
      state.channels.forEach(function(channel) {
        grid.appendChild(createChannelTile(channel));
        if (state.playback && state.liveChannelId === channelId(channel)) {
          renderPlayer(grid);
          playerRendered = true;
        }
      });
      if (state.playback && !playerRendered) renderPlayer(grid);
      root.appendChild(grid);
    } else if (state.playback) {
      renderPlayer(root);
    }
    if (state.programError) {
      const warning = addText(doc.createElement('div'), state.programError);
      warning.className = 'vdr-suite-live-tv-status';
      root.appendChild(warning);
    }
    mount.appendChild(root);
  }

  function applyChannels(data) {
    state.channels = list(data, 'channels').filter(function(channel) { return !channelIsRadio(channel); }).slice().sort(function(left, right) {
      return channelNumber(left) - channelNumber(right) || channelName(left).localeCompare(channelName(right), 'de-DE');
    });
  }
  function applyPrograms(data) { state.events = list(data, 'events').slice(); }

  function loadPrograms(sequence) {
    const client = clientApi();
    if (!client || typeof client.fetchClientEpgCacheWindow !== 'function' || state.channels.length === 0) {
      state.loadingPrograms = false;
      render();
      return Promise.resolve(null);
    }
    const ids = state.channels.map(channelId).filter(Boolean);
    if (ids.length === 0) {
      state.loadingPrograms = false;
      render();
      return Promise.resolve(null);
    }
    const now = Math.floor(Date.now() / 1000);
    state.loadingPrograms = true;
    state.programError = '';
    render();
    return client.fetchClientEpgCacheWindow({
      query: {backend: state.backendId, channelIds: ids.join(','), fromTime: String(now - 21600), untilTime: String(now + 21600), limit: '0', _: String(Date.now())},
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(function(data) {
      if (!state.active || sequence !== state.requestSequence) return null;
      applyPrograms(data);
      state.loadingPrograms = false;
      state.programError = '';
      render();
      return data;
    }).catch(function() {
      if (!state.active || sequence !== state.requestSequence) return null;
      state.events = [];
      state.loadingPrograms = false;
      state.programError = 'Aktuelle Programminformationen sind vorübergehend nicht verfügbar.';
      render();
      return null;
    });
  }

  function load() {
    const client = clientApi();
    state.backendId = selectedBackend();
    state.dataError = '';
    state.programError = '';
    if (!client || typeof client.fetchClientChannels !== 'function') {
      state.loadingChannels = false;
      state.dataError = 'Senderliste ist derzeit nicht verfügbar.';
      render();
      return Promise.resolve(null);
    }
    const sequence = ++state.requestSequence;
    state.loadingChannels = true;
    render();
    return client.fetchClientChannels({query: {backend: state.backendId, _: String(Date.now())}, cache: 'no-store', credentials: 'same-origin'}).then(function(data) {
      if (!state.active || sequence !== state.requestSequence) return null;
      applyChannels(data);
      state.loadingChannels = false;
      render();
      return loadPrograms(sequence);
    }).catch(function(error) {
      if (!state.active || sequence !== state.requestSequence) return null;
      state.loadingChannels = false;
      state.channels = [];
      state.events = [];
      state.dataError = error && error.message ? error.message : 'Senderliste konnte nicht geladen werden.';
      render();
      return null;
    });
  }

  function scrollPlayerIntoView() {
    const mount = mountTarget();
    if (!mount || typeof mount.querySelector !== 'function') return false;
    const player = mount.querySelector('.vdr-suite-live-tv-player');
    if (!player || typeof player.scrollIntoView !== 'function') return false;
    player.scrollIntoView({behavior: 'smooth', block: 'nearest'});
    return true;
  }

  function createPlayback(channel, replacesSessionId, sequence) {
    const playback = playbackApi();
    if (!playback || typeof playback.createLivePanel !== 'function') {
      state.liveSwitching = false;
      state.liveError = 'Live-TV Playback ist derzeit nicht verfügbar.';
      render();
      return Promise.resolve(null);
    }
    let created;
    try {
      created = playback.createLivePanel(channel, state.backendId || selectedBackend(), {replacesSessionId: replacesSessionId || ''});
    } catch (error) {
      state.liveSwitching = false;
      state.liveError = error && error.message ? error.message : String(error || 'Live-TV konnte nicht gestartet werden.');
      render();
      return Promise.resolve(null);
    }
    if (!created || !created.element || typeof created.start !== 'function') {
      state.liveSwitching = false;
      state.liveError = 'Live-TV Playback Adapter ist unvollständig.';
      render();
      return Promise.resolve(null);
    }
    if (!state.active || sequence !== state.switchSequence) {
      if (typeof created.destroy === 'function') created.destroy();
      return Promise.resolve(null);
    }
    state.playback = created;
    state.liveChannelId = channelId(channel);
    state.liveSwitching = false;
    state.liveError = '';
    render();
    return Promise.resolve(created.start()).then(function() {
      if (state.active && sequence === state.switchSequence) {
        render();
        scrollPlayerIntoView();
      }
      return created;
    }).catch(function(error) {
      if (sequence !== state.switchSequence) return null;
      if (state.playback === created && typeof created.destroy === 'function') created.destroy();
      state.playback = null;
      state.liveChannelId = '';
      state.liveSwitching = false;
      state.liveError = error && error.message ? error.message : String(error || 'Live-TV konnte nicht gestartet werden.');
      render();
      return null;
    });
  }

  function startChannel(channel) {
    if (!state.active || !channel || !channelIsEnabled(channel) || state.liveSwitching) return Promise.resolve(null);
    state.backendId = selectedBackend();
    synchronizePlaybackState();
    if (state.playback && state.liveChannelId === channelId(channel)) {
      const shell = playbackShell();
      if (shell && typeof shell.attach === 'function') shell.attach(state.playback);
      render();
      return Promise.resolve(state.playback.start()).then(function() {
        scrollPlayerIntoView();
        return state.playback;
      });
    }
    const previous = state.playback;
    const sequence = ++state.switchSequence;
    state.liveSwitching = true;
    state.liveError = '';
    render();
    if (!previous) return createPlayback(channel, '', sequence);
    if (typeof previous.relinquishForReplacement !== 'function') {
      if (typeof previous.destroy === 'function') previous.destroy();
      state.playback = null;
      state.liveChannelId = '';
      return createPlayback(channel, '', sequence);
    }
    return Promise.resolve(previous.relinquishForReplacement()).then(function(replacesSessionId) {
      if (!state.active || sequence !== state.switchSequence) {
        if (typeof previous.destroy === 'function') previous.destroy();
        return null;
      }
      state.playback = null;
      state.liveChannelId = '';
      return createPlayback(channel, text(replacesSessionId), sequence);
    }).catch(function(error) {
      if (sequence !== state.switchSequence) return null;
      state.playback = null;
      state.liveChannelId = '';
      state.liveSwitching = false;
      state.liveError = error && error.message ? error.message : String(error || 'Senderwechsel fehlgeschlagen.');
      render();
      return null;
    });
  }

  function stop() {
    const current = state.playback;
    state.playback = null;
    state.liveChannelId = '';
    state.liveSwitching = false;
    state.liveError = '';
    state.switchSequence += 1;
    if (current && typeof current.destroy === 'function') current.destroy();
    else {
      const shell = playbackShell();
      if (shell && typeof shell.stop === 'function') shell.stop();
    }
    render();
    return Boolean(current);
  }

  function deactivate() {
    if (!state.active) return false;
    state.active = false;
    state.requestSequence += 1;
    state.liveSwitching = false;
    synchronizePlaybackState();
    const shell = playbackShell();
    const snapshot = shellSnapshot();
    if (state.playback && shell && snapshot && snapshot.active && typeof shell.detach === 'function') shell.detach(state.playback);
    else if (state.playback && !shell && typeof state.playback.destroy === 'function') {
      state.playback.destroy();
      state.playback = null;
      state.liveChannelId = '';
    } else if (shell && snapshot && !snapshot.active) {
      state.playback = null;
      state.liveChannelId = '';
    }
    return true;
  }

  function clearVisibleModuleTabs() {
    if (!doc || typeof doc.querySelectorAll !== 'function') return;
    doc.querySelectorAll('.module-tab').forEach(function(tab) { if (tab.classList) tab.classList.remove('active'); });
  }
  function open() {
    const nextBackend = selectedBackend();
    const changedBackend = Boolean(state.backendId && state.backendId !== nextBackend);
    state.active = true;
    state.backendId = nextBackend;
    state.dataError = '';
    state.liveError = '';
    clearVisibleModuleTabs();
    synchronizePlaybackState();
    render();
    if (changedBackend || state.channels.length === 0) load();
    else loadPrograms(++state.requestSequence);
    const mount = mountTarget();
    if (mount && typeof mount.scrollIntoView === 'function') mount.scrollIntoView({behavior: 'smooth', block: 'start'});
    return true;
  }
  function refresh() { if (!state.active) return false; load(); return true; }

  function installHiddenModuleTab() {
    if (!doc || typeof doc.getElementById !== 'function' || typeof doc.createElement !== 'function') return false;
    if (state.hiddenTab && state.hiddenTab.parentNode) return true;
    const nav = doc.getElementById('module-nav');
    if (!nav) return false;
    let tab = typeof doc.querySelector === 'function' ? doc.querySelector('[data-module="livetv"]') : null;
    if (!tab) {
      tab = doc.createElement('button');
      tab.type = 'button';
      tab.className = 'module-tab vdr-suite-live-tv-module-tab';
      tab.dataset.module = 'livetv';
      tab.hidden = true;
      tab.setAttribute('aria-hidden', 'true');
      tab.setAttribute('tabindex', '-1');
      tab.textContent = 'Live TV';
      nav.appendChild(tab);
    }
    state.hiddenTab = tab;
    return true;
  }
  function refreshBrandIdentity() {
    if (!doc || typeof doc.querySelector !== 'function') return false;
    const label = doc.querySelector('[data-i18n="shell.liveTv"]');
    const entry = label && typeof label.closest === 'function' ? label.closest('.brand-feature') : null;
    if (!entry) return false;
    if (entry.dataset) entry.dataset.brandModule = 'livetv';
    if (typeof entry.setAttribute === 'function') entry.setAttribute('data-brand-module', 'livetv');
    return true;
  }
  function liveBrandEntry() {
    if (!doc || typeof doc.querySelector !== 'function') return null;
    const label = doc.querySelector('[data-i18n="shell.liveTv"]');
    return label && typeof label.closest === 'function' ? label.closest('.brand-feature') : null;
  }
  function contains(parent, child) { return Boolean(parent && child && (parent === child || (typeof parent.contains === 'function' && parent.contains(child)))); }
  function openThroughApp() {
    installHiddenModuleTab();
    if (state.hiddenTab && typeof state.hiddenTab.click === 'function') state.hiddenTab.click();
    open();
  }
  function isMiniReturn(target) {
    if (!target || typeof target.closest !== 'function') return false;
    const buttonNode = target.closest('#vdr-suite-live-mini-player button');
    if (!buttonNode) return false;
    return buttonNode.title === 'Zur Live-TV-Ansicht zurückkehren' || text(buttonNode.textContent) === 'Live-TV';
  }
  function navigationTarget(target) {
    if (!target || typeof target.closest !== 'function') return null;
    return target.closest('.module-tab,[data-brand-module],.backend-card');
  }
  function installNavigation() {
    if (state.navigationBound || !doc || typeof doc.addEventListener !== 'function') return false;
    state.navigationBound = true;
    doc.addEventListener('click', function(event) {
      const target = event && event.target;
      const entry = liveBrandEntry();
      if (contains(entry, target) || isMiniReturn(target)) {
        if (event && typeof event.preventDefault === 'function') event.preventDefault();
        if (event && typeof event.stopImmediatePropagation === 'function') event.stopImmediatePropagation();
        openThroughApp();
        return;
      }
      if (state.active && navigationTarget(target)) deactivate();
    }, true);
    doc.addEventListener('keydown', function(event) {
      if (!event || (event.key !== 'Enter' && event.key !== ' ')) return;
      const entry = liveBrandEntry();
      if (!contains(entry, event.target)) return;
      if (typeof event.preventDefault === 'function') event.preventDefault();
      if (typeof event.stopImmediatePropagation === 'function') event.stopImmediatePropagation();
      openThroughApp();
    }, true);
    return true;
  }
  function installRefreshBoundary() {
    if (!doc || typeof doc.addEventListener !== 'function') return false;
    doc.addEventListener('click', function(event) {
      if (!state.active || !event || !event.target) return;
      const refreshButton = typeof event.target.closest === 'function' ? event.target.closest('#refresh-detail') : null;
      if (!refreshButton) return;
      if (typeof event.preventDefault === 'function') event.preventDefault();
      if (typeof event.stopImmediatePropagation === 'function') event.stopImmediatePropagation();
      refresh();
    }, true);
    return true;
  }

  function snapshot() {
    return Object.freeze({active: state.active, backendId: state.backendId, channelCount: state.channels.length, eventCount: state.events.length, liveChannelId: state.liveChannelId, liveSwitching: state.liveSwitching, dataError: state.dataError, programError: state.programError});
  }
  const api = Object.freeze({
    open,
    deactivate,
    refresh,
    startChannel,
    stop,
    snapshot,
    __test: Object.freeze({channelId, channelName, channelIsRadio, currentEventForChannel, eventArtwork, applyChannels, applyPrograms, render, synchronizePlaybackState, setActive: function(value) { state.active = Boolean(value); }})
  });

  global.VdrSuiteLiveTvView = api;
  installStyles();
  installHiddenModuleTab();
  installNavigation();
  installRefreshBoundary();
  if (doc && doc.readyState === 'loading' && typeof doc.addEventListener === 'function') {
    doc.addEventListener('DOMContentLoaded', function() {
      installHiddenModuleTab();
      refreshBrandIdentity();
    }, {once: true});
  } else refreshBrandIdentity();
})(window);
