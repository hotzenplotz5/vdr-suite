// Phase 65.D.1 dedicated Live-TV start view.
//
// This runtime intentionally loads before channel-day-program-compat.js. The
// compatibility file keeps an embedded fallback guarded by VdrSuiteLiveTvView,
// while this product runtime owns the installed path. Once a Live player is
// mounted in this view, ordinary EPG/status renders must never disconnect the
// exact HTMLMediaElement; navigation and sender replacement remain the only
// deliberate reparenting boundaries.
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
    hbbtvChannelId: '',
    hbbtvAvailable: false,
    hbbtvLoading: false,
    hbbtvApplicationCount: 0,
    hbbtvResult: '',
    hbbtvError: '',
    hbbtvApplications: [],
    hbbtvRequestSequence: 0,
    hbbtvSession: null,
    hbbtvSessionBusy: false,
    hbbtvSessionError: '',
    hbbtvSessionSequence: 0,
    hbbtvPresentationTimer: null,
    hbbtvStatusTimer: null,
    hbbtvFrameRevision: 0,
    hbbtvPresentationError: '',
    hbbtvInputTail: Promise.resolve(),
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

  function channelHasUsableCaids(channel) {
    if (!channel || typeof channel !== 'object') return false;
    const caids = channel.caids || channel.CAIDs || channel.caid || channel.CAID;
    if (Array.isArray(caids)) return caids.length > 0;
    return caids !== undefined && caids !== null && text(caids) !== '';
  }

  function channelHasEncryptionInfo(channel) {
    if (!channel || typeof channel !== 'object') return false;
    const keys = ['encrypted', 'scrambled', 'isEncrypted', 'isScrambled'];
    return keys.some(function(key) {
      return Object.prototype.hasOwnProperty.call(channel, key);
    }) || channelHasUsableCaids(channel);
  }

  function channelIsEncrypted(channel) {
    if (!channel || typeof channel !== 'object') return false;
    const keys = ['encrypted', 'scrambled', 'isEncrypted', 'isScrambled'];
    for (let index = 0; index < keys.length; index += 1) {
      const key = keys[index];
      if (Object.prototype.hasOwnProperty.call(channel, key)) {
        return boolValue(channel[key], false);
      }
    }
    return channelHasUsableCaids(channel);
  }

  function channelAvailabilityText(channel) {
    const prefix = channelNumber(channel) ? 'Kanal ' + channelNumber(channel) : 'TV';
    if (!channelIsEnabled(channel)) return prefix + ' · deaktiviert';
    if (channelIsEncrypted(channel)) return prefix + ' · verschlüsselt';
    if (channelHasEncryptionInfo(channel)) return prefix + ' · frei';
    return prefix + ' · verfügbar';
  }

  function liveErrorForChannel(error, channel, fallback) {
    const message = error && error.message ? text(error.message) : text(error);
    if (channelIsEncrypted(channel) && message.indexOf('live_source_receiver_unavailable') !== -1) {
      return (channelName(channel) ? channelName(channel) + ': ' : '') +
        'Dieser Sender ist verschlüsselt. VDR konnte aktuell keinen Live-Empfang dafür bereitstellen.';
    }
    return message || fallback || 'Live-TV konnte nicht gestartet werden.';
  }

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
  function prefersReducedMotion() {
    return typeof global.matchMedia === 'function' &&
      global.matchMedia('(prefers-reduced-motion: reduce)').matches === true;
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

  function hbbtvAvailabilityText() {
    if (state.hbbtvLoading) return 'HbbTV wird geprüft …';
    if (state.hbbtvError) return 'HbbTV Status unbekannt';
    if (state.hbbtvAvailable) {
      return state.hbbtvApplicationCount > 1
        ? 'HbbTV verfügbar · ' + state.hbbtvApplicationCount + ' Apps'
        : 'HbbTV verfügbar';
    }
    if (state.hbbtvResult === 'no_applications') return 'Kein HbbTV signalisiert';
    if (state.hbbtvResult === 'channel_mismatch') return 'HbbTV Senderkontext nicht aktiv';
    if (state.hbbtvResult === 'receiver_inactive' ||
        state.hbbtvResult === 'no_live_service') return 'HbbTV Empfänger nicht aktiv';
    if (state.hbbtvChannelId) return 'HbbTV nicht verfügbar';
    return 'HbbTV';
  }

  function applyHbbtvIndicator(element) {
    if (!element) return;
    element.textContent = hbbtvAvailabilityText();
    const status = state.hbbtvLoading
      ? 'loading'
      : (state.hbbtvError ? 'unknown' : (state.hbbtvAvailable ? 'available' : 'unavailable'));
    element.setAttribute('data-hbbtv-state', status);
    element.setAttribute(
      'aria-label',
      state.hbbtvAvailable
        ? 'HbbTV ist für den aktuellen Sender verfügbar'
        : hbbtvAvailabilityText()
    );
  }

  function updateHbbtvIndicator() {
    const mount = mountTarget();
    if (!mount || typeof mount.querySelector !== 'function') return;
    applyHbbtvIndicator(
      mount.querySelector('.vdr-suite-hbbtv-availability')
    );
    updateHbbtvSessionUi();
  }

  function resetHbbtvAvailability() {
    state.hbbtvRequestSequence += 1;
    state.hbbtvChannelId = '';
    state.hbbtvAvailable = false;
    state.hbbtvLoading = false;
    state.hbbtvApplicationCount = 0;
    state.hbbtvApplications = [];
    state.hbbtvResult = '';
    state.hbbtvError = '';
    updateHbbtvIndicator();
  }

  function loadHbbtvAttempt(channel, sequence, attempt) {
    const id = channelId(channel);
    const client = clientApi();
    if (!state.active || !id || state.liveChannelId !== id ||
        sequence !== state.hbbtvRequestSequence) {
      return Promise.resolve(null);
    }
    if (!client || typeof client.fetchClientHbbtvApplications !== 'function') {
      state.hbbtvLoading = false;
      state.hbbtvError = 'hbbtv_client_unavailable';
      updateHbbtvIndicator();
      return Promise.resolve(null);
    }

    return client.fetchClientHbbtvApplications({
      query: {
        backend: state.backendId || selectedBackend(),
        channel: id
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(function(data) {
      if (!state.active || state.liveChannelId !== id ||
          sequence !== state.hbbtvRequestSequence) return null;

      const applications = list(data, 'applications');
      state.hbbtvResult = text(data && data.result);
      state.hbbtvAvailable =
        Boolean(data && data.available === true) && applications.length > 0;
      state.hbbtvApplicationCount = applications.length;
      state.hbbtvApplications = applications.slice();
      state.hbbtvError = '';

      if (!state.hbbtvAvailable &&
          state.hbbtvResult === 'no_applications' &&
          attempt < 4 &&
          typeof global.setTimeout === 'function') {
        state.hbbtvLoading = true;
        updateHbbtvIndicator();
        return new Promise(function(resolve) {
          global.setTimeout(function() {
            resolve(loadHbbtvAttempt(channel, sequence, attempt + 1));
          }, 1000);
        });
      }

      state.hbbtvLoading = false;
      updateHbbtvIndicator();
      return data;
    }).catch(function(error) {
      if (!state.active || state.liveChannelId !== id ||
          sequence !== state.hbbtvRequestSequence) return null;
      state.hbbtvLoading = false;
      state.hbbtvAvailable = false;
      state.hbbtvApplicationCount = 0;
      state.hbbtvApplications = [];
      state.hbbtvError =
        error && error.message ? error.message : 'hbbtv_discovery_failed';
      updateHbbtvIndicator();
      return null;
    });
  }

  function beginHbbtvAvailability(channel) {
    const id = channelId(channel);
    if (!state.active || !id || state.liveChannelId !== id) {
      return Promise.resolve(null);
    }

    const sequence = ++state.hbbtvRequestSequence;
    state.hbbtvChannelId = id;
    state.hbbtvAvailable = false;
    state.hbbtvLoading = true;
    state.hbbtvApplicationCount = 0;
    state.hbbtvResult = '';
    state.hbbtvError = '';
    updateHbbtvIndicator();
    return loadHbbtvAttempt(channel, sequence, 0);
  }


  function hbbtvControlCode(application) {
    const control = application && application.control;
    const value = Number(
      control && control.code !== undefined
        ? control.code
        : pick(application, ['controlCode'], 0)
    );
    return Number.isFinite(value) ? Math.floor(value) : 0;
  }

  function hbbtvApplicationRef(application) {
    return application && application.ref && typeof application.ref === 'object'
      ? application.ref
      : {};
  }

  function launchableHbbtvApplication() {
    const candidates = (state.hbbtvApplications || []).filter(function(application) {
      const ref = hbbtvApplicationRef(application);
      return hbbtvControlCode(application) === 1 &&
        Number(ref.applicationId) > 0 &&
        Number(ref.descriptorRevision) > 0;
    });
    return candidates.length === 1 ? candidates[0] : null;
  }

  function hbbtvSessionId() {
    return text(state.hbbtvSession && state.hbbtvSession.sessionId);
  }

  function hbbtvSessionState() {
    return text(state.hbbtvSession && state.hbbtvSession.state).toLowerCase();
  }

  function hbbtvSessionBackendId() {
    return text(state.hbbtvSession && state.hbbtvSession.backendId) ||
      state.backendId || selectedBackend();
  }

  function hbbtvSessionInputActions() {
    const capabilities = state.hbbtvSession && state.hbbtvSession.capabilities;
    return capabilities && Array.isArray(capabilities.inputActions)
      ? capabilities.inputActions.map(function(action) {
          return text(action).toLowerCase();
        }).filter(Boolean)
      : [];
  }

  function hbbtvCanInput(action) {
    return hbbtvSessionState() === 'active' &&
      hbbtvSessionInputActions().indexOf(text(action).toLowerCase()) !== -1;
  }

  function clearHbbtvTimer(name) {
    if (state[name] !== null && typeof global.clearTimeout === 'function') {
      global.clearTimeout(state[name]);
    }
    state[name] = null;
  }

  function pauseHbbtvPolling() {
    clearHbbtvTimer('hbbtvPresentationTimer');
    clearHbbtvTimer('hbbtvStatusTimer');
  }

  function hbbtvCanvas() {
    const mount = mountTarget();
    return mount && typeof mount.querySelector === 'function'
      ? mount.querySelector('.vdr-suite-hbbtv-overlay')
      : null;
  }

  function hbbtvPlayerSlot() {
    const mount = mountTarget();
    return mount && typeof mount.querySelector === 'function'
      ? mount.querySelector('.vdr-suite-live-tv-player-slot')
      : null;
  }

  function hbbtvVideo() {
    const element = state.playback && state.playback.element;
    return element && typeof element.querySelector === 'function'
      ? element.querySelector('video')
      : null;
  }

  function alignHbbtvCanvas() {
    const canvas = hbbtvCanvas();
    const slot = hbbtvPlayerSlot();
    const video = hbbtvVideo();
    if (!canvas || !slot || !video ||
        typeof slot.getBoundingClientRect !== 'function' ||
        typeof video.getBoundingClientRect !== 'function') return false;

    const slotRect = slot.getBoundingClientRect();
    const videoRect = video.getBoundingClientRect();
    if (!videoRect || !(videoRect.width > 0) || !(videoRect.height > 0)) {
      return false;
    }

    canvas.style.left = Math.max(0, videoRect.left - slotRect.left) + 'px';
    canvas.style.top = Math.max(0, videoRect.top - slotRect.top) + 'px';
    canvas.style.width = videoRect.width + 'px';
    canvas.style.height = videoRect.height + 'px';
    return true;
  }

  function clearHbbtvOverlay() {
    const canvas = hbbtvCanvas();
    if (!canvas) return;
    if (typeof canvas.getContext === 'function') {
      const context = canvas.getContext('2d');
      if (context && typeof context.clearRect === 'function') {
        context.clearRect(0, 0, canvas.width || 0, canvas.height || 0);
      }
    }
    canvas.hidden = true;
    if (canvas.dataset) {
      canvas.dataset.hbbtvRevision = '';
      canvas.dataset.hbbtvInteractive = 'false';
    }
  }

  function hbbtvSessionStatusText() {
    if (state.hbbtvSessionError) return state.hbbtvSessionError;
    if (state.hbbtvPresentationError) return state.hbbtvPresentationError;

    switch (hbbtvSessionState()) {
      case 'starting': return 'HbbTV startet …';
      case 'active': return 'HbbTV aktiv · Bedienung im Browser';
      case 'degraded': return 'HbbTV aktiv · Darstellung eingeschränkt';
      case 'suspended': return 'HbbTV pausiert · Senderkontext geändert';
      case 'closing': return 'HbbTV wird geschlossen …';
      case 'closed': return 'HbbTV geschlossen';
      case 'expired': return 'HbbTV Sitzung abgelaufen';
      case 'failed': return 'HbbTV Start fehlgeschlagen';
      default:
        if (state.hbbtvAvailable && !launchableHbbtvApplication()) {
          return 'Keine eindeutig startbare HbbTV-Anwendung';
        }
        return '';
    }
  }

  function applyHbbtvSessionUi(toggle, status, remote, canvas) {
    const sessionId = hbbtvSessionId();
    const sessionState = hbbtvSessionState();
    const launchable = launchableHbbtvApplication();

    if (toggle) {
      toggle.hidden = !sessionId && !state.hbbtvAvailable;
      toggle.disabled = Boolean(
        state.hbbtvSessionBusy ||
        (!sessionId && !launchable)
      );
      if (state.hbbtvSessionBusy) {
        toggle.textContent = sessionId ? 'HbbTV schließt …' : 'HbbTV startet …';
      } else if (sessionId) {
        toggle.textContent = sessionState === 'closing'
          ? 'HbbTV schließt …'
          : 'HbbTV schließen';
      } else {
        toggle.textContent = 'HbbTV starten';
      }
    }

    if (status) {
      const value = hbbtvSessionStatusText();
      status.textContent = value;
      status.hidden = !value;
      status.classList.toggle(
        'error',
        Boolean(state.hbbtvSessionError || state.hbbtvPresentationError)
      );
    }

    if (remote) {
      remote.hidden = !sessionId;
      if (typeof remote.querySelectorAll === 'function') {
        remote.querySelectorAll('[data-hbbtv-action]').forEach(function(control) {
          control.disabled = !hbbtvCanInput(control.dataset.hbbtvAction);
        });
      }
    }

    if (canvas && canvas.dataset) {
      canvas.dataset.hbbtvInteractive =
        sessionState === 'active' ? 'true' : 'false';
    }
  }

  function updateHbbtvSessionUi() {
    const mount = mountTarget();
    if (!mount || typeof mount.querySelector !== 'function') return;

    applyHbbtvSessionUi(
      mount.querySelector('.vdr-suite-hbbtv-session-toggle'),
      mount.querySelector('.vdr-suite-hbbtv-session-status'),
      mount.querySelector('.vdr-suite-hbbtv-remote'),
      mount.querySelector('.vdr-suite-hbbtv-overlay')
    );
  }

  function resetHbbtvSessionState(options) {
    const settings = options && typeof options === 'object' ? options : {};
    pauseHbbtvPolling();
    state.hbbtvSessionSequence += 1;
    state.hbbtvSession = null;
    state.hbbtvSessionBusy = false;
    state.hbbtvFrameRevision = 0;
    state.hbbtvPresentationError = '';
    state.hbbtvInputTail = Promise.resolve();
    if (!settings.keepError) state.hbbtvSessionError = '';
    clearHbbtvOverlay();
    updateHbbtvSessionUi();
  }

  function validateHbbtvSessionIdentity(session, application) {
    if (!session || typeof session !== 'object') return false;
    const ref = hbbtvApplicationRef(application);
    return Boolean(
      text(session.sessionId) &&
      text(session.backendId) === (state.backendId || selectedBackend()) &&
      text(session.channelId) === state.liveChannelId &&
      Number(session.applicationId) === Number(ref.applicationId) &&
      Number(session.descriptorRevision) === Number(ref.descriptorRevision)
    );
  }

  function drawHbbtvPresentation(frame) {
    const canvas = hbbtvCanvas();
    const decoder = global.VdrSuiteQoi;
    if (!canvas || !frame || frame.status !== 200 ||
        !decoder || typeof decoder.decode !== 'function') {
      return false;
    }

    const decoded = decoder.decode(frame.bytes);
    if ((frame.width && frame.width !== decoded.width) ||
        (frame.height && frame.height !== decoded.height)) {
      throw new Error('hbbtv_overlay_dimension_mismatch');
    }

    if (typeof canvas.getContext !== 'function') {
      throw new Error('hbbtv_overlay_canvas_unavailable');
    }

    const context = canvas.getContext('2d');
    if (!context || typeof context.createImageData !== 'function' ||
        typeof context.putImageData !== 'function') {
      throw new Error('hbbtv_overlay_canvas_unavailable');
    }

    canvas.width = decoded.width;
    canvas.height = decoded.height;
    const image = context.createImageData(decoded.width, decoded.height);
    image.data.set(decoded.pixels);
    context.clearRect(0, 0, decoded.width, decoded.height);
    context.putImageData(image, 0, 0);
    canvas.hidden = false;
    if (canvas.dataset) {
      canvas.dataset.hbbtvRevision = String(frame.revision || 0);
    }
    state.hbbtvFrameRevision = Number(frame.revision) || 0;
    alignHbbtvCanvas();
    updateHbbtvSessionUi();
    return true;
  }

  function scheduleHbbtvPresentation(sequence, delay) {
    clearHbbtvTimer('hbbtvPresentationTimer');
    if (!state.active || sequence !== state.hbbtvSessionSequence ||
        !hbbtvSessionId() || typeof global.setTimeout !== 'function') return;

    state.hbbtvPresentationTimer = global.setTimeout(function() {
      pollHbbtvPresentation(sequence);
    }, Math.max(0, Number(delay) || 0));
  }

  function pollHbbtvPresentation(sequence) {
    if (!state.active || sequence !== state.hbbtvSessionSequence ||
        !hbbtvSessionId()) return Promise.resolve(null);

    const client = clientApi();
    if (!client || typeof client.fetchClientHbbtvPresentation !== 'function') {
      state.hbbtvPresentationError = 'HbbTV Darstellung ist nicht verfügbar.';
      updateHbbtvSessionUi();
      return Promise.resolve(null);
    }

    return client.fetchClientHbbtvPresentation({
      query: {
        backend: hbbtvSessionBackendId(),
        session: hbbtvSessionId(),
        revision: String(state.hbbtvFrameRevision || 0)
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(function(frame) {
      if (!state.active || sequence !== state.hbbtvSessionSequence) return null;

      if (frame && frame.status === 200) {
        drawHbbtvPresentation(frame);
      } else if (frame && frame.status === 204 && Number(frame.revision) > 0) {
        state.hbbtvFrameRevision = Number(frame.revision);
      }

      state.hbbtvPresentationError = '';
      updateHbbtvSessionUi();
      scheduleHbbtvPresentation(
        sequence,
        doc && doc.hidden ? 1000 : 250
      );
      return frame;
    }).catch(function(error) {
      if (!state.active || sequence !== state.hbbtvSessionSequence) return null;
      state.hbbtvPresentationError =
        error && error.message
          ? error.message
          : 'HbbTV Darstellung konnte nicht geladen werden.';
      updateHbbtvSessionUi();

      if (error && (error.status === 403 ||
                    error.status === 404 ||
                    error.status === 409)) {
        clearHbbtvOverlay();
        return null;
      }

      scheduleHbbtvPresentation(sequence, 1000);
      return null;
    });
  }

  function startHbbtvPresentationPolling(sequence) {
    clearHbbtvTimer('hbbtvPresentationTimer');
    if (!hbbtvSessionId()) return;
    scheduleHbbtvPresentation(
      sequence === undefined ? state.hbbtvSessionSequence : sequence,
      0
    );
  }

  function scheduleHbbtvStatus(sequence, delay) {
    clearHbbtvTimer('hbbtvStatusTimer');
    if (!state.active || sequence !== state.hbbtvSessionSequence ||
        !hbbtvSessionId() || typeof global.setTimeout !== 'function') return;

    state.hbbtvStatusTimer = global.setTimeout(function() {
      pollHbbtvSessionStatus(sequence);
    }, Math.max(0, Number(delay) || 0));
  }

  function pollHbbtvSessionStatus(sequence) {
    if (!state.active || sequence !== state.hbbtvSessionSequence ||
        !hbbtvSessionId()) return Promise.resolve(null);

    const client = clientApi();
    if (!client || typeof client.fetchClientHbbtvSessionStatus !== 'function') {
      state.hbbtvSessionError = 'HbbTV Status ist nicht verfügbar.';
      updateHbbtvSessionUi();
      return Promise.resolve(null);
    }

    const expectedId = hbbtvSessionId();
    const expectedBackend = hbbtvSessionBackendId();
    return client.fetchClientHbbtvSessionStatus({
      payload: {
        backendId: expectedBackend,
        sessionId: expectedId
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(function(session) {
      if (!state.active || sequence !== state.hbbtvSessionSequence) return null;
      if (!session || text(session.sessionId) !== expectedId ||
          text(session.backendId) !== expectedBackend) {
        throw new Error('hbbtv_session_identity_mismatch');
      }

      state.hbbtvSession = session;
      state.hbbtvSessionError = '';
      updateHbbtvSessionUi();

      const currentState = hbbtvSessionState();
      if (currentState === 'closed') {
        resetHbbtvSessionState();
        return session;
      }
      if (currentState === 'failed' ||
          currentState === 'expired' ||
          currentState === 'suspended') {
        clearHbbtvTimer('hbbtvPresentationTimer');
        clearHbbtvOverlay();
        return session;
      }

      if (currentState === 'active') {
        startHbbtvPresentationPolling(sequence);
      }
      scheduleHbbtvStatus(
        sequence,
        currentState === 'starting' || currentState === 'closing'
          ? 350
          : 2000
      );
      return session;
    }).catch(function(error) {
      if (!state.active || sequence !== state.hbbtvSessionSequence) return null;
      state.hbbtvSessionError =
        error && error.message ? error.message : 'HbbTV Status konnte nicht gelesen werden.';
      updateHbbtvSessionUi();
      if (error && (error.status === 403 ||
                    error.status === 404 ||
                    error.status === 409)) {
        clearHbbtvTimer('hbbtvPresentationTimer');
        clearHbbtvOverlay();
        return null;
      }
      scheduleHbbtvStatus(sequence, 1000);
      return null;
    });
  }

  function startHbbtvStatusPolling(sequence) {
    clearHbbtvTimer('hbbtvStatusTimer');
    if (!hbbtvSessionId()) return;
    scheduleHbbtvStatus(
      sequence === undefined ? state.hbbtvSessionSequence : sequence,
      250
    );
  }

  function openHbbtvSession() {
    const application = launchableHbbtvApplication();
    const client = clientApi();
    if (!state.active || !state.liveChannelId || state.hbbtvSessionBusy ||
        hbbtvSessionId() || !application) {
      return Promise.resolve(null);
    }

    if (!client || typeof client.fetchClientHbbtvSessionLaunch !== 'function') {
      state.hbbtvSessionError = 'HbbTV Start ist nicht verfügbar.';
      updateHbbtvSessionUi();
      return Promise.resolve(null);
    }

    pauseHbbtvPolling();
    clearHbbtvOverlay();
    state.hbbtvSessionSequence += 1;
    const sequence = state.hbbtvSessionSequence;
    const ref = hbbtvApplicationRef(application);
    state.hbbtvSessionBusy = true;
    state.hbbtvSessionError = '';
    state.hbbtvPresentationError = '';
    state.hbbtvFrameRevision = 0;
    updateHbbtvSessionUi();

    return client.fetchClientHbbtvSessionLaunch({
      payload: {
        backendId: state.backendId || selectedBackend(),
        channelId: state.liveChannelId,
        applicationId: Number(ref.applicationId),
        descriptorRevision: Number(ref.descriptorRevision)
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(function(session) {
      if (sequence !== state.hbbtvSessionSequence) return null;
      if (!validateHbbtvSessionIdentity(session, application)) {
        throw new Error('hbbtv_session_identity_mismatch');
      }

      state.hbbtvSession = session;
      state.hbbtvSessionBusy = false;
      state.hbbtvSessionError = '';
      updateHbbtvSessionUi();
      startHbbtvPresentationPolling(sequence);
      startHbbtvStatusPolling(sequence);
      return session;
    }).catch(function(error) {
      if (sequence !== state.hbbtvSessionSequence) return null;
      state.hbbtvSessionBusy = false;
      state.hbbtvSession = null;
      state.hbbtvSessionError =
        error && error.message ? error.message : 'HbbTV konnte nicht gestartet werden.';
      clearHbbtvOverlay();
      updateHbbtvSessionUi();
      return null;
    });
  }

  function closeHbbtvSession() {
    const sessionId = hbbtvSessionId();
    const client = clientApi();
    if (!sessionId || state.hbbtvSessionBusy) return Promise.resolve(false);

    if (!client || typeof client.fetchClientHbbtvSessionClose !== 'function') {
      state.hbbtvSessionError = 'HbbTV Schließen ist nicht verfügbar.';
      updateHbbtvSessionUi();
      return Promise.resolve(false);
    }

    const sequence = state.hbbtvSessionSequence;
    const backendId = hbbtvSessionBackendId();
    state.hbbtvSessionBusy = true;
    state.hbbtvSessionError = '';
    clearHbbtvTimer('hbbtvPresentationTimer');
    clearHbbtvOverlay();
    updateHbbtvSessionUi();

    return client.fetchClientHbbtvSessionClose({
      payload: {
        backendId: backendId,
        sessionId: sessionId
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(function(session) {
      if (sequence !== state.hbbtvSessionSequence) return false;
      if (!session || text(session.sessionId) !== sessionId ||
          text(session.backendId) !== backendId) {
        throw new Error('hbbtv_session_identity_mismatch');
      }

      state.hbbtvSession = session;
      state.hbbtvSessionBusy = false;
      state.hbbtvSessionError = '';
      updateHbbtvSessionUi();
      startHbbtvStatusPolling(sequence);
      return true;
    }).catch(function(error) {
      if (sequence !== state.hbbtvSessionSequence) return false;
      state.hbbtvSessionBusy = false;
      state.hbbtvSessionError =
        error && error.message ? error.message : 'HbbTV konnte nicht geschlossen werden.';
      updateHbbtvSessionUi();
      return false;
    });
  }

  function releaseHbbtvSessionBestEffort() {
    const sessionId = hbbtvSessionId();
    const backendId = hbbtvSessionBackendId();
    const client = clientApi();

    resetHbbtvSessionState();
    if (!sessionId || !client ||
        typeof client.fetchClientHbbtvSessionClose !== 'function') {
      return Promise.resolve(false);
    }

    return client.fetchClientHbbtvSessionClose({
      payload: {
        backendId: backendId,
        sessionId: sessionId
      },
      cache: 'no-store',
      credentials: 'same-origin'
    }).then(function() {
      return true;
    }).catch(function() {
      return false;
    });
  }

  function sendHbbtvInput(action) {
    const semantic = text(action).toLowerCase();
    const sessionId = hbbtvSessionId();
    const backendId = hbbtvSessionBackendId();
    const client = clientApi();

    if (!sessionId || !hbbtvCanInput(semantic) ||
        !client || typeof client.fetchClientHbbtvSessionInput !== 'function') {
      return Promise.resolve(false);
    }

    state.hbbtvInputTail = Promise.resolve(state.hbbtvInputTail)
      .catch(function() {})
      .then(function() {
        if (!hbbtvSessionId() || !hbbtvCanInput(semantic)) return false;
        return client.fetchClientHbbtvSessionInput({
          payload: {
            backendId: backendId,
            sessionId: sessionId,
            action: semantic
          },
          cache: 'no-store',
          credentials: 'same-origin'
        }).then(function(session) {
          if (hbbtvSessionId() !== sessionId) return false;
          if (session && text(session.sessionId) === sessionId) {
            state.hbbtvSession = session;
          }
          state.hbbtvSessionError = '';
          updateHbbtvSessionUi();
          return true;
        });
      })
      .catch(function(error) {
        if (hbbtvSessionId() === sessionId) {
          state.hbbtvSessionError =
            error && error.message ? error.message : 'HbbTV Eingabe fehlgeschlagen.';
          updateHbbtvSessionUi();
        }
        return false;
      });

    return state.hbbtvInputTail;
  }

  function keyboardHbbtvAction(event) {
    const key = event && text(event.key);
    const mapping = {
      ArrowUp: 'up',
      ArrowDown: 'down',
      ArrowLeft: 'left',
      ArrowRight: 'right',
      Enter: 'ok',
      Escape: 'back',
      Backspace: 'back',
      r: 'red',
      R: 'red',
      g: 'green',
      G: 'green',
      y: 'yellow',
      Y: 'yellow',
      b: 'blue',
      B: 'blue',
      '0': '0',
      '1': '1',
      '2': '2',
      '3': '3',
      '4': '4',
      '5': '5',
      '6': '6',
      '7': '7',
      '8': '8',
      '9': '9',
      MediaPlay: 'play',
      MediaPause: 'pause',
      MediaStop: 'stop',
      MediaTrackNext: 'fast_forward',
      MediaTrackPrevious: 'rewind'
    };
    return mapping[key] || '';
  }

  function createHbbtvRemote() {
    const remote = doc.createElement('div');
    remote.className = 'vdr-suite-hbbtv-remote';
    remote.setAttribute('role', 'group');
    remote.setAttribute('aria-label', 'HbbTV Fernbedienung');

    const controls = [
      ['up', '↑'], ['left', '←'], ['ok', 'OK'], ['right', '→'], ['down', '↓'],
      ['back', 'Zurück'],
      ['red', 'Rot'], ['green', 'Grün'], ['yellow', 'Gelb'], ['blue', 'Blau'],
      ['0', '0'], ['1', '1'], ['2', '2'], ['3', '3'], ['4', '4'],
      ['5', '5'], ['6', '6'], ['7', '7'], ['8', '8'], ['9', '9'],
      ['play', '▶'], ['pause', '⏸'], ['stop', '■'],
      ['rewind', '⏪'], ['fast_forward', '⏩']
    ];

    controls.forEach(function(entry) {
      const control = button(entry[1], 'vdr-suite-hbbtv-remote-button');
      control.dataset.hbbtvAction = entry[0];
      if (entry[0] === 'red' || entry[0] === 'green' ||
          entry[0] === 'yellow' || entry[0] === 'blue') {
        control.dataset.hbbtvColor = entry[0];
      }
      control.addEventListener('click', function() {
        sendHbbtvInput(entry[0]);
      });
      remote.appendChild(control);
    });

    remote.hidden = true;
    return remote;
  }

  function handleHbbtvOverlayKey(event) {
    const action = keyboardHbbtvAction(event);
    if (!action || !hbbtvCanInput(action)) return;
    if (event && typeof event.preventDefault === 'function') event.preventDefault();
    if (event && typeof event.stopPropagation === 'function') event.stopPropagation();
    sendHbbtvInput(action);
  }

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
.vdr-suite-live-tv-player{display:grid;grid-column:1/-1;gap:.65rem;padding:.75rem;border:1px solid rgba(34,211,238,.42);border-radius:1rem;background:rgba(8,47,73,.42)}.vdr-suite-live-tv-player-head{display:flex;align-items:center;justify-content:space-between;gap:.75rem;flex-wrap:wrap}.vdr-suite-live-tv-player-title{display:grid;gap:.15rem;color:#f8fafc;font-weight:850}.vdr-suite-live-tv-player-title span{color:#a5f3fc;font-size:.82rem;font-weight:650}.vdr-suite-hbbtv-availability{display:inline-flex;align-items:center;min-height:2.4rem;padding:.42rem .68rem;border:1px solid rgba(148,163,184,.38);border-radius:.68rem;background:rgba(15,23,42,.72);color:#cbd5e1;font-size:.82rem;font-weight:800}.vdr-suite-hbbtv-availability[data-hbbtv-state="available"]{border-color:rgba(248,113,113,.72);color:#fecaca}.vdr-suite-hbbtv-availability[data-hbbtv-state="loading"]{color:#bae6fd}.vdr-suite-live-tv-stop{min-height:2.5rem;padding:.5rem .8rem;border:1px solid rgba(248,113,113,.62)!important;border-radius:.68rem;background:transparent!important;color:#fecaca!important}.vdr-suite-live-tv-player-slot{position:relative;overflow:hidden;border-radius:.85rem;background:#000}.vdr-suite-live-tv-player-slot video{display:block!important;width:100%!important;max-height:min(64vh,42rem)!important;background:#000}
.vdr-suite-hbbtv-session-toggle{min-height:2.4rem;padding:.42rem .7rem;border:1px solid rgba(248,113,113,.72);border-radius:.68rem;background:rgba(127,29,29,.28);color:#fecaca;font-weight:850}.vdr-suite-hbbtv-session-toggle:disabled{opacity:.55;cursor:not-allowed}.vdr-suite-hbbtv-session-status{margin:0;color:#bae6fd;font-size:.8rem;font-weight:700}.vdr-suite-hbbtv-session-status.error{color:#fecaca}
.vdr-suite-hbbtv-overlay{position:absolute;z-index:12;display:block;box-sizing:border-box;max-width:none;max-height:none;outline:none;background:transparent;image-rendering:auto;pointer-events:none}.vdr-suite-hbbtv-overlay[hidden]{display:none}.vdr-suite-hbbtv-overlay[data-hbbtv-interactive="true"]{pointer-events:auto}.vdr-suite-hbbtv-overlay[data-hbbtv-interactive="true"]:focus-visible{outline:2px solid rgba(34,211,238,.9);outline-offset:-2px}
.vdr-suite-hbbtv-remote{display:flex;grid-column:1/-1;align-items:center;gap:.35rem;padding:.55rem .1rem;overflow-x:auto;scrollbar-width:thin}.vdr-suite-hbbtv-remote[hidden]{display:none}.vdr-suite-hbbtv-remote-button{flex:0 0 auto;min-width:2.45rem;min-height:2.35rem;padding:.35rem .55rem;border:1px solid rgba(148,163,184,.35);border-radius:.55rem;background:rgba(15,23,42,.82);color:#e2e8f0;font-weight:850}.vdr-suite-hbbtv-remote-button:disabled{opacity:.42}.vdr-suite-hbbtv-remote-button[data-hbbtv-color="red"]{border-color:rgba(248,113,113,.8)}.vdr-suite-hbbtv-remote-button[data-hbbtv-color="green"]{border-color:rgba(74,222,128,.8)}.vdr-suite-hbbtv-remote-button[data-hbbtv-color="yellow"]{border-color:rgba(250,204,21,.8)}.vdr-suite-hbbtv-remote-button[data-hbbtv-color="blue"]{border-color:rgba(96,165,250,.8)}
.vdr-suite-live-tv-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(13.5rem,1fr));gap:.8rem}.vdr-suite-live-tv-channel{position:relative;display:grid;grid-template-columns:5.25rem minmax(0,1fr);align-items:center;gap:.75rem;min-height:7rem;padding:.7rem;overflow:hidden;border:1px solid rgba(96,165,250,.28);border-radius:1rem;background:rgba(15,23,42,.82);color:#f8fafc;text-align:left;cursor:pointer;isolation:isolate}.vdr-suite-live-tv-channel:hover,.vdr-suite-live-tv-channel:focus-visible{border-color:#38bdf8;outline:none;box-shadow:0 .9rem 2rem rgba(2,132,199,.18);transform:translateY(-1px)}.vdr-suite-live-tv-channel.active{border-color:rgba(34,211,238,.8);background:rgba(8,47,73,.68)}.vdr-suite-live-tv-channel:disabled{cursor:not-allowed;opacity:.55}
.vdr-suite-live-tv-logo{display:grid;place-items:center;width:5.25rem;height:3.3rem;padding:.25rem;border-radius:.62rem;background:rgba(248,250,252,.96);overflow:hidden}.vdr-suite-live-tv-logo .channel-logo-frame,.vdr-suite-live-tv-logo img,.vdr-suite-live-tv-logo .channel-logo{width:100%!important;height:100%!important;max-width:100%!important;max-height:100%!important;object-fit:contain!important}.vdr-suite-live-tv-copy{display:grid;gap:.18rem;min-width:0}.vdr-suite-live-tv-name{overflow:hidden;color:#f8fafc;font-weight:900;white-space:nowrap;text-overflow:ellipsis}.vdr-suite-live-tv-meta{color:#94a3b8;font-size:.78rem}.vdr-suite-live-tv-now{overflow:hidden;color:#bae6fd;font-size:.82rem;font-weight:750;white-space:nowrap;text-overflow:ellipsis}
.vdr-suite-live-tv-preview{position:absolute;z-index:3;inset:0;display:grid;align-content:end;gap:.2rem;padding:.85rem;opacity:0;pointer-events:none;transform:translateY(.35rem);transition:opacity .16s ease,transform .16s ease;background-position:center;background-size:cover;color:#fff}.vdr-suite-live-tv-preview::before{content:"";position:absolute;z-index:-1;inset:0;background:linear-gradient(180deg,rgba(2,6,23,.12),rgba(2,6,23,.94) 68%)}.vdr-suite-live-tv-channel:hover .vdr-suite-live-tv-preview,.vdr-suite-live-tv-channel:focus-visible .vdr-suite-live-tv-preview{opacity:1;transform:translateY(0)}.vdr-suite-live-tv-preview-title{font-size:1rem;font-weight:900;text-shadow:0 1px 4px #000}.vdr-suite-live-tv-preview-meta{color:#bae6fd;font-size:.78rem;font-weight:750}.vdr-suite-live-tv-preview-subtitle{overflow:hidden;color:#e2e8f0;font-size:.78rem;white-space:nowrap;text-overflow:ellipsis}
@media(min-width:72rem){.vdr-suite-live-tv-grid{grid-template-columns:repeat(auto-fill,minmax(16.5rem,1fr))}.vdr-suite-live-tv-channel{min-height:8rem}}
@media(hover:none){.vdr-suite-live-tv-grid{grid-template-columns:repeat(2,minmax(0,1fr));gap:.55rem}.vdr-suite-live-tv-channel{grid-template-columns:1fr;align-content:start;min-height:11.5rem;padding:.55rem}.vdr-suite-live-tv-logo{width:100%;height:4rem}.vdr-suite-live-tv-copy{gap:.12rem}.vdr-suite-live-tv-preview{position:relative;inset:auto;min-height:4.3rem;margin:.3rem -.55rem -.55rem;padding:.55rem;opacity:1;transform:none;background-position:center 35%}.vdr-suite-live-tv-preview::before{background:linear-gradient(180deg,rgba(2,6,23,.35),rgba(2,6,23,.94))}.vdr-suite-live-tv-preview-title{font-size:.82rem}.vdr-suite-live-tv-preview-subtitle{display:none}}
@media(prefers-reduced-motion:reduce){.vdr-suite-live-tv-channel:hover,.vdr-suite-live-tv-channel:focus-visible{transform:none}.vdr-suite-live-tv-preview{transition:none;transform:none}.vdr-suite-live-tv-channel:hover .vdr-suite-live-tv-preview,.vdr-suite-live-tv-channel:focus-visible .vdr-suite-live-tv-preview{transform:none}}
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
    const meta = addText(doc.createElement('span'), channelAvailabilityText(channel));
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
    if (state.playback && state.liveChannelId === snapshot.channelId) {
      if (state.active && state.hbbtvChannelId !== snapshot.channelId) {
        const current = state.channels.find(function(entry) {
          return channelId(entry) === snapshot.channelId;
        }) || {
          id: snapshot.channelId,
          name: snapshot.channelName || snapshot.channelId,
          enabled: true
        };
        beginHbbtvAvailability(current);
      }
      return;
    }
    const playback = playbackApi();
    if (!playback || typeof playback.createLivePanel !== 'function') return;
    const channel = state.channels.find(function(entry) { return channelId(entry) === snapshot.channelId; }) || {id: snapshot.channelId, name: snapshot.channelName || snapshot.channelId, enabled: true};
    try {
      state.playback = playback.createLivePanel(channel, snapshot.backendId || selectedBackend(), {});
      state.liveChannelId = snapshot.channelId;
      if (state.hbbtvChannelId !== snapshot.channelId) {
        beginHbbtvAvailability(channel);
      }
    } catch (error) {
      state.liveError = error && error.message ? error.message : String(error || '');
    }
  }

  function playbackMountedIn(mount) {
    const element = state.playback && state.playback.element;
    return Boolean(element && mount && typeof mount.contains === 'function' && mount.contains(element));
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
    const teletext = global.VdrSuiteTeletextView;
    if (currentChannel && teletext && typeof teletext.createLauncher === 'function') {
      const teletextButton = teletext.createLauncher(
        currentChannel,
        snapshot && snapshot.backendId ? snapshot.backendId : selectedBackend()
      );
      if (teletextButton) head.appendChild(teletextButton);
    }
    const hbbtvIndicator = doc.createElement('span');
    hbbtvIndicator.className = 'vdr-suite-hbbtv-availability';
    hbbtvIndicator.setAttribute('aria-live', 'polite');
    applyHbbtvIndicator(hbbtvIndicator);
    head.appendChild(hbbtvIndicator);

    const hbbtvToggle = button(
      hbbtvSessionId() ? 'HbbTV schließen' : 'HbbTV starten',
      'vdr-suite-hbbtv-session-toggle'
    );
    hbbtvToggle.addEventListener('click', function() {
      if (hbbtvSessionId()) closeHbbtvSession();
      else openHbbtvSession();
    });
    head.appendChild(hbbtvToggle);

    const hbbtvStatus = doc.createElement('span');
    hbbtvStatus.className = 'vdr-suite-hbbtv-session-status';
    hbbtvStatus.setAttribute('aria-live', 'polite');
    head.appendChild(hbbtvStatus);

    const stopButton = button('Live-TV beenden', 'vdr-suite-live-tv-stop');
    stopButton.addEventListener('click', stop);
    head.appendChild(stopButton);
    const slot = doc.createElement('div');
    slot.className = 'vdr-suite-live-tv-player-slot';
    slot.appendChild(state.playback.element);

    const hbbtvOverlay = doc.createElement('canvas');
    hbbtvOverlay.className = 'vdr-suite-hbbtv-overlay';
    hbbtvOverlay.hidden = true;
    hbbtvOverlay.tabIndex = 0;
    hbbtvOverlay.setAttribute('role', 'application');
    hbbtvOverlay.setAttribute(
      'aria-label',
      'HbbTV Anwendung · Pfeiltasten, Enter, Escape und Farbtasten verwenden'
    );
    hbbtvOverlay.dataset.hbbtvInteractive = 'false';
    hbbtvOverlay.addEventListener('click', function() {
      if (typeof hbbtvOverlay.focus === 'function') hbbtvOverlay.focus();
    });
    hbbtvOverlay.addEventListener('keydown', handleHbbtvOverlayKey);
    slot.appendChild(hbbtvOverlay);

    const hbbtvRemote = createHbbtvRemote();

    box.appendChild(slot);
    box.appendChild(hbbtvRemote);
    box.appendChild(head);
    root.appendChild(box);

    applyHbbtvSessionUi(
      hbbtvToggle,
      hbbtvStatus,
      hbbtvRemote,
      hbbtvOverlay
    );
  }

  function render() {
    if (!state.active || !doc) return;
    const mount = mountTarget();
    if (!mount || typeof mount.replaceChildren !== 'function') return;
    installStyles();
    synchronizePlaybackState();

    // Do not tear down/reinsert a loading or playing HTMLMediaElement merely to
    // refresh EPG/status data. Chromium may abort the media resource when the
    // element is disconnected, which violates the persistent-player contract.
    if (playbackMountedIn(mount)) return;

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

    if (hbbtvSessionId() &&
        text(state.hbbtvSession && state.hbbtvSession.channelId) === state.liveChannelId) {
      state.hbbtvFrameRevision = 0;
      const sequence = state.hbbtvSessionSequence;
      startHbbtvPresentationPolling(sequence);
      startHbbtvStatusPolling(sequence);
      alignHbbtvCanvas();
    }
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
    player.scrollIntoView({behavior: prefersReducedMotion() ? 'auto' : 'smooth', block: 'nearest'});
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
      state.liveError = liveErrorForChannel(error, channel, 'Live-TV konnte nicht gestartet werden.');
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
    resetHbbtvAvailability();
    render();
    return Promise.resolve(created.start()).then(function() {
      if (state.active && sequence === state.switchSequence) {
        scrollPlayerIntoView();
        beginHbbtvAvailability(channel);
      }
      return created;
    }).catch(function(error) {
      if (sequence !== state.switchSequence) return null;
      if (state.playback === created && typeof created.destroy === 'function') created.destroy();
      state.playback = null;
      state.liveChannelId = '';
      state.liveSwitching = false;
      state.liveError = liveErrorForChannel(error, channel, 'Live-TV konnte nicht gestartet werden.');
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
    if (hbbtvSessionId() && state.liveChannelId !== channelId(channel)) {
      releaseHbbtvSessionBestEffort();
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
    releaseHbbtvSessionBestEffort();
    const current = state.playback;
    state.playback = null;
    state.liveChannelId = '';
    state.liveSwitching = false;
    state.liveError = '';
    state.switchSequence += 1;
    resetHbbtvAvailability();
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
    state.hbbtvRequestSequence += 1;
    state.hbbtvChannelId = '';
    state.hbbtvLoading = false;
    state.liveSwitching = false;
    pauseHbbtvPolling();
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
    if (hbbtvSessionId() &&
        text(state.hbbtvSession && state.hbbtvSession.channelId) === state.liveChannelId) {
      startHbbtvPresentationPolling(state.hbbtvSessionSequence);
      startHbbtvStatusPolling(state.hbbtvSessionSequence);
    }
    if (changedBackend || state.channels.length === 0) load();
    else loadPrograms(++state.requestSequence);
    const mount = mountTarget();
    if (mount && typeof mount.scrollIntoView === 'function') mount.scrollIntoView({behavior: prefersReducedMotion() ? 'auto' : 'smooth', block: 'start'});
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
    return Object.freeze({
      active: state.active,
      backendId: state.backendId,
      channelCount: state.channels.length,
      eventCount: state.events.length,
      liveChannelId: state.liveChannelId,
      liveSwitching: state.liveSwitching,
      hbbtvChannelId: state.hbbtvChannelId,
      hbbtvAvailable: state.hbbtvAvailable,
      hbbtvApplicationCount: state.hbbtvApplicationCount,
      hbbtvResult: state.hbbtvResult,
      hbbtvSessionId: hbbtvSessionId(),
      hbbtvSessionState: hbbtvSessionState(),
      hbbtvFrameRevision: state.hbbtvFrameRevision,
      dataError: state.dataError,
      programError: state.programError
    });
  }
  const api = Object.freeze({
    open,
    deactivate,
    refresh,
    startChannel,
    stop,
    snapshot,
    __test: Object.freeze({channelId, channelName, channelIsRadio, channelHasEncryptionInfo, channelIsEncrypted, channelAvailabilityText, liveErrorForChannel, currentEventForChannel, eventArtwork, applyChannels, applyPrograms, render, synchronizePlaybackState, playbackMountedIn, prefersReducedMotion, scrollPlayerIntoView, hbbtvControlCode, launchableHbbtvApplication, keyboardHbbtvAction, alignHbbtvCanvas, setActive: function(value) { state.active = Boolean(value); }})
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
