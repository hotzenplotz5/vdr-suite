// Phase 66.2/66.7: browse-only Live-TV Hero projection for Media Home.
//
// This module deliberately owns only Home selection/projection state. It reads
// the existing Channel/EPG client APIs and delegates explicit playback to the
// canonical VdrSuiteLiveTvView owner. Explicit EPG timer creation and programme
// details delegate to the existing canonical EPG owners. Selection, keyboard
// navigation and touch swipes never create media/session work. Slice 66.7 adds
// presentation-only accessibility, reduced-motion and responsive polish without
// changing owners.
(function (global) {
  'use strict';

  if (!global || global.VdrSuiteHomeLiveHero) return;

  const doc = global.document || (typeof document !== 'undefined' ? document : null);
  const PROGRAMME_RAIL_LIMIT = 24;
  const state = {
    active: false,
    backendId: '',
    channels: [],
    events: [],
    selectedIndex: 0,
    loadingChannels: false,
    loadingPrograms: false,
    dataError: '',
    programError: '',
    actionError: '',
    requestSequence: 0,
    syncScheduled: false,
    observer: null,
    navigationBound: false,
    touchStartX: null,
    touchStartY: null
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

  function boolValue(value, fallback) {
    if (value === true || value === 1 || value === '1') return true;
    if (value === false || value === 0 || value === '0') return false;
    const normalized = text(value).toLowerCase();
    if (normalized === 'true' || normalized === 'yes' || normalized === 'ja' || normalized === 'on') return true;
    if (normalized === 'false' || normalized === 'no' || normalized === 'nein' || normalized === 'off') return false;
    return Boolean(fallback);
  }

  function channelId(channel) { return text(pick(channel, ['channelId', 'id', 'nativeId'])); }
  function channelName(channel) { return text(pick(channel, ['name', 'channelName', 'title', 'displayName'], channelId(channel) || 'Kanal')); }
  function channelNumber(channel) {
    const value = Number(pick(channel, ['number', 'channelNumber', 'position'], 0));
    return Number.isFinite(value) && value > 0 ? value : 0;
  }
  function channelIsRadio(channel) { return boolValue(pick(channel, ['radio', 'isRadio'], false), false); }
  function channelIsEnabled(channel) { return boolValue(pick(channel, ['enabled', 'active'], true), true); }

  function orderedChannels(data) {
    return list(data, 'channels').filter(channel => !channelIsRadio(channel)).slice().sort((left, right) => {
      const leftNumber = channelNumber(left) || 999999;
      const rightNumber = channelNumber(right) || 999999;
      return leftNumber - rightNumber || channelName(left).localeCompare(channelName(right), 'de-DE');
    });
  }

  function epoch(value) {
    const number = Number(value);
    if (Number.isFinite(number) && number > 0) return number > 100000000000 ? Math.floor(number / 1000) : Math.floor(number);
    const parsed = Date.parse(String(value || ''));
    return Number.isFinite(parsed) ? Math.floor(parsed / 1000) : 0;
  }

  function eventChannelId(event) { return text(pick(event, ['channelId', 'channel', 'channel_id'])); }
  function eventStart(event) { return epoch(pick(event, ['startTime', 'start', 'beginTime'], 0)); }
  function eventEnd(event) {
    const start = eventStart(event);
    const explicit = epoch(pick(event, ['endTime', 'end', 'stopTime'], 0));
    if (explicit > start) return explicit;
    const duration = Number(pick(event, ['durationSeconds', 'duration'], 0));
    return start + (Number.isFinite(duration) && duration > 0 ? duration : 0);
  }
  function eventTitle(event) { return text(pick(event, ['title', 'name', 'eventTitle'], 'Keine Programminformation')); }
  function eventSubtitle(event) { return text(pick(event, ['subtitle', 'shortText', 'short_text', 'description'], '')); }

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

  function channelEvents(channel, events) {
    const id = channelId(channel);
    return (Array.isArray(events) ? events : []).filter(event => eventChannelId(event) === id).slice().sort((left, right) => eventStart(left) - eventStart(right));
  }

  function currentEventForChannel(channel, events, nowValue) {
    const now = Number.isFinite(Number(nowValue)) ? Number(nowValue) : Math.floor(Date.now() / 1000);
    const matches = channelEvents(channel, events);
    for (let index = 0; index < matches.length; index += 1) {
      const start = eventStart(matches[index]);
      const end = eventEnd(matches[index]);
      if (start > 0 && start <= now && (end === 0 || now < end)) return matches[index];
    }
    return channel && (channel.currentEvent || channel.now || channel.currentProgram) || null;
  }

  function nextEventForChannel(channel, events, nowValue) {
    const now = Number.isFinite(Number(nowValue)) ? Number(nowValue) : Math.floor(Date.now() / 1000);
    const current = currentEventForChannel(channel, events, now);
    const currentEnd = current ? eventEnd(current) : now;
    const matches = channelEvents(channel, events);
    for (let index = 0; index < matches.length; index += 1) {
      const start = eventStart(matches[index]);
      if (start > 0 && start >= currentEnd && matches[index] !== current) return matches[index];
    }
    return null;
  }

  function formatClock(value) {
    const seconds = epoch(value);
    if (!seconds) return '--:--';
    return new Date(seconds * 1000).toLocaleTimeString('de-DE', {hour: '2-digit', minute: '2-digit'});
  }

  function eventTime(event) {
    if (!event) return '';
    const start = formatClock(eventStart(event));
    const end = formatClock(eventEnd(event));
    return start + (end !== '--:--' ? '–' + end : '');
  }

  function progressPercent(event, nowValue) {
    if (!event) return 0;
    const now = Number.isFinite(Number(nowValue)) ? Number(nowValue) : Math.floor(Date.now() / 1000);
    const start = eventStart(event);
    const end = eventEnd(event);
    if (start <= 0 || end <= start) return 0;
    return Math.max(0, Math.min(100, ((now - start) / (end - start)) * 100));
  }

  function prefersReducedMotion() {
    try {
      return typeof global.matchMedia === 'function' && global.matchMedia('(prefers-reduced-motion: reduce)').matches === true;
    } catch (_) {
      return false;
    }
  }

  function platform() { return global.VdrSuitePlatform || null; }
  function clientApi() {
    const value = platform();
    if (value && typeof value.getClientApi === 'function') {
      const client = value.getClientApi();
      if (client) return client;
    }
    return global.VdrSuiteClientApi || null;
  }

  function selectedBackendId() {
    const value = platform();
    if (value && typeof value.getSelectedBackendId === 'function') return text(value.getSelectedBackendId()) || 'default';
    return state.backendId || 'default';
  }

  function selectedModule() {
    const value = platform();
    if (value && typeof value.getSelectedModule === 'function') return text(value.getSelectedModule());
    if (!doc || typeof doc.querySelector !== 'function') return '';
    return doc.querySelector('.module-tab.active[data-module="overview"]') ? 'overview' : '';
  }

  function homeIsActive() { return selectedModule() === 'overview'; }
  function heroRoot() {
    if (!doc || typeof doc.querySelector !== 'function') return null;
    return doc.querySelector('.media-home-hero[data-home-zone="hero"]');
  }

  function focusHeroRoot(root) {
    const target = root || heroRoot();
    if (!target || typeof target.focus !== 'function') return false;
    try { target.focus({preventScroll: true}); } catch (_) { target.focus(); }
    return true;
  }

  function currentChannel() {
    if (state.channels.length === 0) return null;
    return state.channels[Math.max(0, Math.min(state.selectedIndex, state.channels.length - 1))] || null;
  }

  function neighborChannel(delta) {
    if (state.channels.length === 0) return null;
    const length = state.channels.length;
    return state.channels[(state.selectedIndex + delta + length) % length] || null;
  }

  function addTextNode(element, value) {
    element.textContent = String(value === undefined || value === null ? '' : value);
    return element;
  }

  function createButton(label, className) {
    const button = doc.createElement('button');
    button.type = 'button';
    button.className = className || '';
    button.textContent = label;
    return button;
  }

  function appendChannelLogo(parent, channel, className) {
    const title = channelName(channel);
    const id = channelId(channel);
    if (typeof createChannelLogoElement === 'function') {
      const logo = createChannelLogoElement(title, id);
      if (logo.classList && className) logo.classList.add(className);
      parent.appendChild(logo);
      return;
    }
    const fallback = addTextNode(doc.createElement('div'), channelLogoInitial(title));
    fallback.className = className || 'media-home-live-logo-fallback';
    parent.appendChild(fallback);
  }

  function createNeighbor(channel, direction) {
    const control = createButton('', 'media-home-live-neighbor media-home-live-neighbor-' + direction);
    control.setAttribute('aria-label', (direction === 'previous' ? 'Vorheriger Sender: ' : 'Nächster Sender: ') + channelName(channel));
    control.dataset.channelId = channelId(channel);
    appendChannelLogo(control, channel, 'media-home-live-neighbor-logo');
    const copy = doc.createElement('span');
    copy.className = 'media-home-live-neighbor-copy';
    copy.appendChild(addTextNode(doc.createElement('strong'), channelName(channel)));
    const event = currentEventForChannel(channel, state.events);
    copy.appendChild(addTextNode(doc.createElement('span'), event ? eventTitle(event) : 'Keine Programminformation'));
    control.appendChild(copy);
    control.addEventListener('click', () => {
      selectOffset(direction === 'previous' ? -1 : 1);
      focusHeroRoot();
    });
    return control;
  }

  function createProgramCard(label, event, current) {
    const card = doc.createElement('div');
    card.className = 'media-home-live-program' + (current ? ' current' : '');
    card.appendChild(addTextNode(doc.createElement('span'), label)).className = 'media-home-live-program-label';
    card.appendChild(addTextNode(doc.createElement('strong'), event ? eventTitle(event) : 'Keine Programminformation')).className = 'media-home-live-program-title';
    const subtitle = event ? eventSubtitle(event) : '';
    const meta = [event ? eventTime(event) : '', subtitle].filter(Boolean).join(' · ');
    card.appendChild(addTextNode(doc.createElement('span'), meta || 'EPG-Daten nicht verfügbar')).className = 'media-home-live-program-meta';
    if (current && event) {
      const percent = progressPercent(event);
      const progress = doc.createElement('progress');
      progress.className = 'media-home-live-progress';
      progress.max = 100;
      progress.value = Number(percent.toFixed(1));
      progress.setAttribute('aria-label', 'Fortschritt der laufenden Sendung: ' + String(Math.round(percent)) + ' Prozent');
      card.appendChild(progress);
    }
    return card;
  }

  function programmeHost() {
    if (!doc || typeof doc.querySelector !== 'function') return null;
    return doc.querySelector('[data-home-zone="additional-sections"]');
  }

  function programmeEntries(kind) {
    const now = Math.floor(Date.now() / 1000);
    const next = kind === 'next';
    return state.channels.map(channel => ({
      channel,
      event: next
        ? nextEventForChannel(channel, state.events, now)
        : currentEventForChannel(channel, state.events, now)
    })).filter(entry => Boolean(entry.event)).slice(0, PROGRAMME_RAIL_LIMIT);
  }

  function createProgrammeArtwork(channel, event) {
    const artwork = doc.createElement('div');
    artwork.className = 'media-home-live-guide-artwork';
    const fallback = () => {
      artwork.classList.add('is-fallback');
      artwork.replaceChildren();
      appendChannelLogo(artwork, channel, 'media-home-live-guide-logo');
    };
    const url = eventArtwork(event);
    if (!url) {
      fallback();
      return artwork;
    }
    const image = doc.createElement('img');
    image.src = url;
    image.alt = '';
    image.loading = 'lazy';
    image.decoding = 'async';
    image.fetchPriority = 'low';
    image.addEventListener('error', fallback);
    artwork.appendChild(image);
    return artwork;
  }

  function showProgrammeActionError(feedback, message) {
    if (!feedback || typeof feedback.replaceChildren !== 'function') return false;
    feedback.replaceChildren();
    const status = doc.createElement('p');
    status.className = 'media-home-live-guide-action-error';
    status.textContent = message;
    feedback.appendChild(status);
    return true;
  }

  function createProgrammeTimer(entry, button, feedback) {
    if (typeof createEpgTimerFromDetail !== 'function') {
      showProgrammeActionError(feedback, 'Timer-Erstellung ist derzeit nicht verfügbar.');
      return false;
    }
    if (feedback && typeof feedback.replaceChildren === 'function') feedback.replaceChildren();
    createEpgTimerFromDetail(feedback, entry.event, entry.channel, button);
    return true;
  }

  function programmeDetailOwnerReady() {
    return Boolean(
      global.VdrSuiteEpgDetailOwner &&
      typeof global.VdrSuiteEpgDetailOwner.createCard === 'function'
    );
  }

  function loadProgrammeDetailOwner() {
    if (programmeDetailOwnerReady()) return Promise.resolve(global.VdrSuiteEpgDetailOwner);
    const runtimes = global.VdrSuiteDeferredFrontendRuntimes;
    const epgDetailLoad = runtimes && typeof runtimes.loadEpgDetail === 'function'
      ? Promise.resolve(runtimes.loadEpgDetail())
      : Promise.resolve();

    return epgDetailLoad.then(() => {
      if (programmeDetailOwnerReady()) return global.VdrSuiteEpgDetailOwner;
      if (typeof global.loadVdrSuiteDeferredRuntime !== 'function') return null;
      return Promise.resolve(global.loadVdrSuiteDeferredRuntime(
        'vdr-suite-epg-detail-owner-runtime',
        '/frontend/epg-detail-owner.js',
        programmeDetailOwnerReady
      )).then(() => programmeDetailOwnerReady() ? global.VdrSuiteEpgDetailOwner : null);
    }).catch(() => null);
  }

  function clearProgrammeDetails(focusCard) {
    const host = programmeHost();
    if (!host || typeof host.querySelectorAll !== 'function') return false;
    host.querySelectorAll('[data-home-live-guide-detail="true"]').forEach(element => {
      if (typeof element.remove === 'function') element.remove();
    });
    host.querySelectorAll('.media-home-live-guide-card.detail-selected').forEach(element => {
      if (element.classList) element.classList.remove('detail-selected');
      if (typeof element.setAttribute === 'function') element.setAttribute('aria-expanded', 'false');
    });
    if (focusCard && typeof focusCard.focus === 'function') {
      try { focusCard.focus({preventScroll: true}); } catch (_) { focusCard.focus(); }
    }
    return true;
  }

  function openProgrammeDetail(entry, card) {
    const section = card && typeof card.closest === 'function'
      ? card.closest('[data-home-live-guide]')
      : null;
    if (!section || !entry || !entry.event || !entry.channel) return Promise.resolve(false);

    const alreadyOpen = Boolean(card.classList && card.classList.contains('detail-selected'));
    clearProgrammeDetails(null);
    if (alreadyOpen) {
      if (typeof card.focus === 'function') card.focus();
      return Promise.resolve(true);
    }

    if (card.classList) card.classList.add('detail-selected');
    card.setAttribute('aria-expanded', 'true');

    const shell = doc.createElement('section');
    shell.className = 'media-home-live-guide-detail';
    shell.dataset.homeLiveGuideDetail = 'true';
    shell.setAttribute('aria-label', 'Sendungsdetails: ' + eventTitle(entry.event));
    shell.addEventListener('click', event => {
      if (event && typeof event.stopPropagation === 'function') event.stopPropagation();
    });

    const heading = doc.createElement('div');
    heading.className = 'media-home-live-guide-detail-head';
    const name = doc.createElement('h4');
    name.textContent = 'Sendungsdetails';
    heading.appendChild(name);
    const close = createButton('Schließen', 'media-home-live-guide-detail-close');
    close.setAttribute('aria-label', 'Sendungsdetails schließen');
    close.addEventListener('click', event => {
      if (event && typeof event.stopPropagation === 'function') event.stopPropagation();
      clearProgrammeDetails(card);
    });
    heading.appendChild(close);
    shell.appendChild(heading);

    const status = doc.createElement('p');
    status.className = 'media-home-live-guide-detail-state';
    status.textContent = 'Sendungsdetails werden geladen …';
    shell.appendChild(status);
    section.appendChild(shell);
    if (typeof shell.scrollIntoView === 'function') {
      shell.scrollIntoView({behavior: prefersReducedMotion() ? 'auto' : 'smooth', block: 'nearest'});
    }

    return loadProgrammeDetailOwner().then(owner => {
      if (!shell.parentNode) return false;
      if (!owner || typeof owner.createCard !== 'function') {
        status.classList.add('error');
        status.textContent = 'Sendungsdetails sind derzeit nicht verfügbar.';
        return false;
      }
      try {
        const detail = owner.createCard(entry.event, entry.channel);
        shell.replaceChildren(heading, detail);
        return true;
      } catch (error) {
        status.classList.add('error');
        status.textContent = error && error.message
          ? String(error.message)
          : 'Sendungsdetails konnten nicht geöffnet werden.';
        return false;
      }
    });
  }

  function createProgrammeGuideCard(entry, current) {
    const card = doc.createElement('article');
    card.className = 'media-home-live-guide-card' + (current ? ' current' : '');
    card.dataset.channelId = channelId(entry.channel);
    card.tabIndex = 0;
    card.setAttribute('aria-label', eventTitle(entry.event) + ' – Sendungsdetails öffnen');
    card.setAttribute('aria-expanded', 'false');
    card.appendChild(createProgrammeArtwork(entry.channel, entry.event));
    const copy = doc.createElement('div');
    copy.className = 'media-home-live-guide-copy';
    const channel = doc.createElement('span');
    channel.className = 'media-home-live-guide-channel';
    channel.textContent = channelName(entry.channel) + ' · ' + eventTime(entry.event);
    copy.appendChild(channel);
    const title = doc.createElement('strong');
    title.textContent = eventTitle(entry.event);
    copy.appendChild(title);
    const subtitle = eventSubtitle(entry.event);
    if (subtitle) {
      const detail = doc.createElement('span');
      detail.className = 'media-home-live-guide-subtitle';
      detail.textContent = subtitle;
      copy.appendChild(detail);
    }
    if (current) {
      const percent = progressPercent(entry.event);
      const progress = doc.createElement('progress');
      progress.className = 'media-home-live-guide-progress';
      progress.max = 100;
      progress.value = Number(percent.toFixed(1));
      progress.setAttribute('aria-label', 'Fortschritt von ' + eventTitle(entry.event) + ': ' + String(Math.round(percent)) + ' Prozent');
      copy.appendChild(progress);
    }
    card.appendChild(copy);

    const actions = doc.createElement('div');
    actions.className = 'media-home-live-guide-actions' + (current ? '' : ' single');
    if (current) {
      const live = createButton('Live TV', 'media-home-live-guide-action primary');
      live.setAttribute('aria-label', channelName(entry.channel) + ' live ansehen');
      live.addEventListener('click', event => {
        if (event && typeof event.stopPropagation === 'function') event.stopPropagation();
        watchChannel(entry.channel);
      });
      actions.appendChild(live);
    }
    const timer = createButton('Timer erstellen', 'media-home-live-guide-action');
    timer.setAttribute('aria-label', 'Timer für ' + eventTitle(entry.event) + ' erstellen');
    const feedback = doc.createElement('div');
    feedback.className = 'media-home-live-guide-feedback';
    feedback.setAttribute('role', 'status');
    feedback.setAttribute('aria-live', 'polite');
    timer.addEventListener('click', event => {
      if (event && typeof event.stopPropagation === 'function') event.stopPropagation();
      createProgrammeTimer(entry, timer, feedback);
    });
    actions.appendChild(timer);
    card.appendChild(actions);
    card.appendChild(feedback);
    card.addEventListener('click', event => {
      if (event && typeof event.stopPropagation === 'function') event.stopPropagation();
      openProgrammeDetail(entry, card);
    });
    card.addEventListener('keydown', event => {
      if (!event || (event.key !== 'Enter' && event.key !== ' ')) return;
      if (typeof event.preventDefault === 'function') event.preventDefault();
      if (typeof event.stopPropagation === 'function') event.stopPropagation();
      openProgrammeDetail(entry, card);
    });
    return card;
  }

  function renderProgrammeRail(kind, title, current) {
    const host = programmeHost();
    if (!host || typeof host.querySelector !== 'function') return false;
    let section = host.querySelector('[data-home-live-guide="' + kind + '"]');
    const ready = !state.loadingChannels && !state.loadingPrograms && !state.dataError && !state.programError;
    const entries = ready ? programmeEntries(kind) : [];
    if (!entries.length) {
      if (section && typeof section.remove === 'function') section.remove();
      return true;
    }
    if (!section) {
      section = doc.createElement('section');
      section.dataset.homeLiveGuide = kind;
      host.appendChild(section);
    }
    section.className = 'media-home-live-guide media-home-live-guide-' + kind;
    section.setAttribute('aria-labelledby', 'media-home-live-guide-' + kind + '-title');
    section.replaceChildren();
    const heading = doc.createElement('div');
    heading.className = 'media-home-section-heading';
    const name = doc.createElement('h3');
    name.id = 'media-home-live-guide-' + kind + '-title';
    name.textContent = title;
    heading.appendChild(name);
    section.appendChild(heading);
    const rail = doc.createElement('div');
    rail.className = 'media-home-live-guide-rail';
    rail.setAttribute('aria-label', title);
    entries.forEach(entry => rail.appendChild(createProgrammeGuideCard(entry, current)));
    section.appendChild(rail);
    return true;
  }

  function renderProgrammeRails() {
    renderProgrammeRail('now', 'Was läuft jetzt', true);
    renderProgrammeRail('next', 'Was läuft danach', false);
  }

  function statusHero(title, message, error) {
    const root = heroRoot();
    if (!root || typeof root.replaceChildren !== 'function') return false;
    root.classList.add('media-home-live-hero-active');
    root.replaceChildren();
    const box = doc.createElement('div');
    box.className = 'media-home-live-status' + (error ? ' error' : '');
    box.appendChild(addTextNode(doc.createElement('p'), 'Live-TV')).className = 'media-home-eyebrow';
    box.appendChild(addTextNode(doc.createElement('h3'), title));
    box.appendChild(addTextNode(doc.createElement('p'), message));
    root.appendChild(box);
    return true;
  }

  function watchChannel(channel) {
    if (!channel || !channelIsEnabled(channel)) {
      state.actionError = 'Dieser Sender kann derzeit nicht gestartet werden.';
      render();
      return Promise.resolve(null);
    }
    const liveOwner = global.VdrSuiteLiveTvView;
    const liveEntry = doc && typeof doc.querySelector === 'function'
      ? (doc.querySelector('[data-brand-module="livetv"]') || doc.querySelector('[data-brand-module="channels2"]')) : null;
    if (!liveOwner || typeof liveOwner.startChannel !== 'function' || !liveEntry || typeof liveEntry.click !== 'function') {
      state.actionError = 'Live-TV Navigation ist derzeit nicht verfügbar.';
      render();
      return Promise.resolve(null);
    }
    state.actionError = '';
    liveEntry.click();
    try {
      return Promise.resolve(liveOwner.startChannel(channel)).catch(error => {
        state.actionError = error && error.message ? error.message : 'Live-TV konnte nicht gestartet werden.';
        return null;
      });
    } catch (error) {
      state.actionError = error && error.message ? error.message : 'Live-TV konnte nicht gestartet werden.';
      return Promise.resolve(null);
    }
  }

  function watchLive() {
    return watchChannel(currentChannel());
  }

  function openEpg() {
    const entry = doc && typeof doc.querySelector === 'function' ? doc.querySelector('[data-brand-module="epg"]') : null;
    if (!entry || typeof entry.click !== 'function') {
      state.actionError = 'EPG Navigation ist derzeit nicht verfügbar.';
      render();
      return false;
    }
    state.actionError = '';
    entry.click();
    return true;
  }

  function render() {
    if (!state.active || !doc) return false;
    const root = heroRoot();
    if (!root || typeof root.replaceChildren !== 'function') return false;
    installStyles();
    bindHeroInteractions(root);
    renderProgrammeRails();
    if (state.loadingChannels && state.channels.length === 0) return statusHero('Sender werden geladen …', 'Media Home lädt die vorhandene VDR-Kanalliste. Browsing bleibt von Playback getrennt.', false);
    if (state.dataError) return statusHero('Live-TV ist vorübergehend nicht verfügbar', state.dataError, true);
    if (state.channels.length === 0) return statusHero('Keine TV-Sender gefunden', 'Die kanonische Kanalliste enthält für dieses Backend derzeit keine TV-Sender.', false);

    const channel = currentChannel();
    const previous = neighborChannel(-1);
    const next = neighborChannel(1);
    const currentEvent = currentEventForChannel(channel, state.events);
    const nextEvent = nextEventForChannel(channel, state.events);
    root.classList.add('media-home-live-hero-active');
    root.replaceChildren();

    const artwork = doc.createElement('div');
    artwork.className = 'media-home-live-artwork';
    artwork.setAttribute('aria-hidden', 'true');
    const artworkUrl = eventArtwork(currentEvent);
    if (artworkUrl) artwork.style.backgroundImage = 'url("' + artworkUrl.replace(/"/g, '%22') + '")';
    root.appendChild(artwork);

    const carousel = doc.createElement('div');
    carousel.className = 'media-home-live-carousel';
    if (previous) carousel.appendChild(createNeighbor(previous, 'previous'));

    const focus = doc.createElement('article');
    focus.className = 'media-home-live-focus';
    focus.dataset.channelId = channelId(channel);
    const channelHead = doc.createElement('div');
    channelHead.className = 'media-home-live-channel-head';
    appendChannelLogo(channelHead, channel, 'media-home-live-channel-logo');
    const channelCopy = doc.createElement('div');
    channelCopy.className = 'media-home-live-channel-copy';
    channelCopy.appendChild(addTextNode(doc.createElement('p'), 'Live-TV · Kanal ' + String(channelNumber(channel) || state.selectedIndex + 1))).className = 'media-home-eyebrow';
    channelCopy.appendChild(addTextNode(doc.createElement('h3'), channelName(channel)));
    channelCopy.appendChild(addTextNode(doc.createElement('p'), 'Sender ' + String(state.selectedIndex + 1) + ' von ' + String(state.channels.length) + ' · Auswahl startet keine Wiedergabe.')).className = 'media-home-live-position';
    channelHead.appendChild(channelCopy);
    focus.appendChild(channelHead);

    const programmes = doc.createElement('div');
    programmes.className = 'media-home-live-programmes';
    programmes.appendChild(createProgramCard('Jetzt', currentEvent, true));
    programmes.appendChild(createProgramCard('Als nächstes', nextEvent, false));
    focus.appendChild(programmes);

    const actions = doc.createElement('div');
    actions.className = 'media-home-live-actions';
    const watch = createButton('Live ansehen', 'media-home-live-action primary');
    watch.disabled = !channelIsEnabled(channel);
    watch.setAttribute('data-home-live-action', 'watch');
    watch.setAttribute('aria-label', channelName(channel) + ' live ansehen');
    watch.addEventListener('click', watchLive);
    actions.appendChild(watch);
    const epg = createButton('EPG', 'media-home-live-action');
    epg.setAttribute('data-home-live-action', 'epg');
    epg.setAttribute('aria-label', 'EPG für ' + channelName(channel) + ' öffnen');
    epg.addEventListener('click', openEpg);
    actions.appendChild(epg);
    focus.appendChild(actions);

    const noticeText = state.actionError || state.programError || (state.loadingPrograms ? 'Aktuelle Programminformationen werden geladen …' : 'Mit ←/→ oder Wischen Sender wechseln.');
    const notice = addTextNode(doc.createElement('p'), noticeText);
    notice.className = 'media-home-live-notice' + (state.actionError ? ' error' : '');
    focus.appendChild(notice);
    carousel.appendChild(focus);
    if (next) carousel.appendChild(createNeighbor(next, 'next'));
    root.appendChild(carousel);
    return true;
  }

  function selectIndex(index) {
    if (state.channels.length === 0) return false;
    const length = state.channels.length;
    state.selectedIndex = ((Number(index) || 0) % length + length) % length;
    state.actionError = '';
    render();
    return true;
  }
  function selectOffset(delta) { return selectIndex(state.selectedIndex + Number(delta || 0)); }

  function applyChannels(data) {
    const selectedId = channelId(currentChannel());
    state.channels = orderedChannels(data);
    let nextIndex = selectedId ? state.channels.findIndex(channel => channelId(channel) === selectedId) : -1;
    if (nextIndex < 0) nextIndex = Math.min(state.selectedIndex, Math.max(0, state.channels.length - 1));
    state.selectedIndex = Math.max(0, nextIndex);
  }
  function applyPrograms(data) { state.events = list(data, 'events').slice(); }

  function loadPrograms(sequence) {
    const client = clientApi();
    if (!client || typeof client.fetchClientEpgCacheWindow !== 'function' || state.channels.length === 0) {
      state.loadingPrograms = false;
      state.programError = state.channels.length === 0 ? '' : 'Aktuelle Programminformationen sind vorübergehend nicht verfügbar.';
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
      cache: 'no-store', credentials: 'same-origin'
    }).then(data => {
      if (!state.active || sequence !== state.requestSequence) return null;
      applyPrograms(data);
      state.loadingPrograms = false;
      state.programError = '';
      render();
      return data;
    }).catch(() => {
      if (!state.active || sequence !== state.requestSequence) return null;
      state.events = [];
      state.loadingPrograms = false;
      state.programError = 'Aktuelle Programminformationen sind vorübergehend nicht verfügbar.';
      render();
      return null;
    });
  }

  function load(force) {
    if (!state.active) return Promise.resolve(null);
    const client = clientApi();
    const nextBackend = selectedBackendId();
    const backendChanged = state.backendId !== nextBackend;
    state.backendId = nextBackend;
    state.dataError = '';
    state.programError = '';
    if (!force && !backendChanged && state.channels.length > 0) {
      render();
      return loadPrograms(++state.requestSequence);
    }
    if (!client || typeof client.fetchClientChannels !== 'function') {
      state.loadingChannels = false;
      state.dataError = 'Senderliste ist derzeit nicht verfügbar.';
      render();
      return Promise.resolve(null);
    }
    const sequence = ++state.requestSequence;
    state.loadingChannels = true;
    render();
    return client.fetchClientChannels({query: {backend: state.backendId, _: String(Date.now())}, cache: 'no-store', credentials: 'same-origin'}).then(data => {
      if (!state.active || sequence !== state.requestSequence) return null;
      applyChannels(data);
      state.loadingChannels = false;
      render();
      return loadPrograms(sequence);
    }).catch(error => {
      if (!state.active || sequence !== state.requestSequence) return null;
      state.loadingChannels = false;
      state.channels = [];
      state.events = [];
      state.dataError = error && error.message ? error.message : 'Senderliste konnte nicht geladen werden.';
      render();
      return null;
    });
  }

  function sync(force) {
    const active = homeIsActive();
    if (!active) {
      state.active = false;
      state.requestSequence += 1;
      const root = heroRoot();
      if (root && root.classList) root.classList.remove('media-home-live-hero-active');
      return Promise.resolve(null);
    }
    const becameActive = !state.active;
    state.active = true;
    const backendChanged = state.backendId !== selectedBackendId();
    if (becameActive || backendChanged || state.channels.length === 0 || force) return load(Boolean(force || backendChanged));
    render();
    return Promise.resolve(null);
  }

  function scheduleSync(force) {
    if (state.syncScheduled) return;
    state.syncScheduled = true;
    const schedule = typeof global.setTimeout === 'function' ? global.setTimeout : setTimeout;
    schedule(() => {
      state.syncScheduled = false;
      sync(Boolean(force));
    }, 0);
  }

  function moveVertical(direction) {
    if (!doc) return false;
    const target = direction > 0 ? (typeof doc.getElementById === 'function' ? doc.getElementById('detail-data') : null)
      : (typeof doc.querySelector === 'function' ? doc.querySelector('.module-tab.active') : null);
    if (!target) return false;
    if (typeof target.setAttribute === 'function' && !(typeof target.hasAttribute === 'function' && target.hasAttribute('tabindex'))) target.setAttribute('tabindex', '-1');
    if (typeof target.focus === 'function') target.focus({preventScroll: true});
    if (typeof target.scrollIntoView === 'function') target.scrollIntoView({behavior: prefersReducedMotion() ? 'auto' : 'smooth', block: 'nearest'});
    return true;
  }

  function bindHeroInteractions(root) {
    if (!root || !root.dataset || root.dataset.homeLiveHeroBound === 'true') return;
    root.dataset.homeLiveHeroBound = 'true';
    root.setAttribute('tabindex', '0');
    root.setAttribute('role', 'region');
    root.setAttribute('aria-label', 'Live-TV Senderkarussell');
    root.setAttribute('aria-roledescription', 'Senderkarussell');
    root.addEventListener('click', event => {
      const target = event && event.target;
      if (target && String(target.tagName || '').toUpperCase() === 'BUTTON') return;
      focusHeroRoot(root);
    });
    root.addEventListener('keydown', event => {
      if (!event) return;
      if (event.key === 'ArrowLeft') { event.preventDefault(); selectOffset(-1); focusHeroRoot(root); }
      else if (event.key === 'ArrowRight') { event.preventDefault(); selectOffset(1); focusHeroRoot(root); }
      else if (event.key === 'ArrowDown') { event.preventDefault(); moveVertical(1); }
      else if (event.key === 'ArrowUp') { event.preventDefault(); moveVertical(-1); }
    });
    root.addEventListener('touchstart', event => {
      const touch = event && event.touches && event.touches[0];
      if (!touch) return;
      state.touchStartX = Number(touch.clientX);
      state.touchStartY = Number(touch.clientY);
    }, {passive: true});
    root.addEventListener('touchend', event => {
      const touch = event && event.changedTouches && event.changedTouches[0];
      if (!touch || state.touchStartX === null || state.touchStartY === null) return;
      const deltaX = Number(touch.clientX) - state.touchStartX;
      const deltaY = Number(touch.clientY) - state.touchStartY;
      state.touchStartX = null;
      state.touchStartY = null;
      if (Math.abs(deltaX) < 48 || Math.abs(deltaX) <= Math.abs(deltaY) * 1.15) return;
      selectOffset(deltaX < 0 ? 1 : -1);
    }, {passive: true});
  }

  function installStyles() {
    if (!doc || !doc.head || typeof doc.createElement !== 'function') return false;
    if (typeof doc.getElementById === 'function' && doc.getElementById('vdr-suite-home-live-hero-style')) return true;
    const style = doc.createElement('style');
    style.id = 'vdr-suite-home-live-hero-style';
    style.textContent = `
.media-home-live-hero-active{min-height:24rem;display:grid;isolation:isolate;background:#020617}.media-home-live-artwork{position:absolute;z-index:-2;inset:0;background-position:center 28%;background-size:cover;opacity:.3;filter:saturate(.9)}.media-home-live-hero-active::before{content:"";position:absolute;z-index:-1;inset:0;background:linear-gradient(90deg,rgba(2,6,23,.99) 0%,rgba(2,6,23,.91) 43%,rgba(2,6,23,.62) 72%,rgba(2,6,23,.88) 100%)}
.media-home-live-carousel{box-sizing:border-box;display:grid;grid-template-columns:minmax(7rem,.44fr) minmax(0,2.2fr) minmax(7rem,.44fr);align-items:stretch;gap:clamp(.5rem,1.5vw,1.15rem);width:100%;min-width:0;padding:clamp(1rem,3.5vw,2.3rem)}.media-home-live-focus{display:grid;align-content:center;gap:1rem;min-width:0;padding:clamp(1rem,2.5vw,2rem);border:1px solid rgba(125,211,252,.36);border-radius:1.3rem;background:linear-gradient(145deg,rgba(2,6,23,.9),rgba(15,23,42,.76));box-shadow:0 1.4rem 3.2rem rgba(2,6,23,.35)}.media-home-live-hero-active:focus-visible .media-home-live-focus{outline:3px solid rgba(125,211,252,.96);outline-offset:3px}.media-home-live-channel-head{display:flex;align-items:center;gap:1rem;min-width:0}.media-home-live-channel-logo{flex:0 0 clamp(5.2rem,9vw,8rem);width:clamp(5.2rem,9vw,8rem);height:clamp(3.2rem,5vw,4.8rem);padding:.35rem;border-radius:.8rem;background:rgba(248,250,252,.96);overflow:hidden}.media-home-live-channel-logo img{width:100%;height:100%;object-fit:contain}.media-home-live-channel-copy{min-width:0}.media-home-live-channel-copy h3{max-width:none;font-size:clamp(2rem,4.2vw,4.25rem);white-space:nowrap;overflow:hidden;text-overflow:ellipsis}.media-home-live-position{margin:.45rem 0 0!important;color:#a8b6c8!important;font-size:.82rem!important}
.media-home-live-programmes{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:.7rem}.media-home-live-program{display:grid;gap:.28rem;min-width:0;padding:.8rem .9rem;border:1px solid rgba(148,163,184,.18);border-radius:.9rem;background:rgba(15,23,42,.68)}.media-home-live-program.current{border-color:rgba(56,189,248,.34)}.media-home-live-program-label{color:#7dd3fc;font-size:.7rem;font-weight:850;letter-spacing:.1em;text-transform:uppercase}.media-home-live-program-title{overflow:hidden;color:#f8fafc;font-size:clamp(.96rem,1.6vw,1.18rem);white-space:nowrap;text-overflow:ellipsis}.media-home-live-program-meta{overflow:hidden;color:#a8b6c8;font-size:.78rem;white-space:nowrap;text-overflow:ellipsis}.media-home-live-progress{display:block;width:100%;height:.26rem;margin-top:.25rem;border:0;border-radius:999px;background:rgba(148,163,184,.2);overflow:hidden;appearance:none}.media-home-live-progress::-webkit-progress-bar{border-radius:999px;background:rgba(148,163,184,.2)}.media-home-live-progress::-webkit-progress-value{border-radius:999px;background:#38bdf8}.media-home-live-progress::-moz-progress-bar{border-radius:999px;background:#38bdf8}
.media-home-live-actions{display:flex;gap:.65rem;flex-wrap:wrap}.media-home-live-action{min-height:2.9rem;padding:.58rem 1rem;border:1px solid rgba(125,211,252,.36);border-radius:.75rem;background:rgba(15,23,42,.78);color:#e0f2fe;font-weight:800;cursor:pointer}.media-home-live-action.primary{border-color:rgba(56,189,248,.74);background:#0369a1;color:#fff}.media-home-live-action:focus-visible{outline:3px solid rgba(125,211,252,.96);outline-offset:2px}.media-home-live-action:disabled{cursor:not-allowed;opacity:.5}.media-home-live-notice{margin:0!important;color:#a8b6c8!important;font-size:.78rem!important}.media-home-live-notice.error{color:#fecaca!important}
.media-home-live-neighbor{display:grid;align-content:center;gap:.55rem;min-width:2.75rem;min-height:2.75rem;padding:.65rem;border:1px solid rgba(148,163,184,.14);border-radius:1rem;background:rgba(15,23,42,.48);color:#cbd5e1;text-align:left;cursor:pointer;opacity:.64;transition:opacity .16s ease,border-color .16s ease,transform .16s ease,box-shadow .16s ease}.media-home-live-neighbor:hover{opacity:.88;border-color:rgba(125,211,252,.42);transform:translateY(-1px)}.media-home-live-neighbor:focus-visible{opacity:1;border-color:rgba(125,211,252,.74);outline:3px solid rgba(125,211,252,.96);outline-offset:2px;box-shadow:0 0 0 1px rgba(2,6,23,.92);transform:translateY(-1px)}.media-home-live-neighbor-logo{width:100%;height:3.2rem;padding:.3rem;border-radius:.65rem;background:rgba(248,250,252,.94);overflow:hidden}.media-home-live-neighbor-logo img{width:100%;height:100%;object-fit:contain}.media-home-live-neighbor-copy{display:grid;gap:.16rem;min-width:0}.media-home-live-neighbor-copy strong,.media-home-live-neighbor-copy span{overflow:hidden;white-space:nowrap;text-overflow:ellipsis}.media-home-live-neighbor-copy strong{color:#f8fafc;font-size:.82rem}.media-home-live-neighbor-copy span{color:#a8b6c8;font-size:.7rem}.media-home-live-status{display:grid;align-content:center;gap:.7rem;min-height:20rem;padding:clamp(1.5rem,5vw,4rem)}.media-home-live-status h3{max-width:18ch}.media-home-live-status.error{color:#fecaca}
#detail:has(.module-tab.active[data-module="overview"]) .media-home-additional-sections{display:flex;flex-direction:column}.media-home-additional-sections>*{order:70}.media-home-live-guide-now{order:10}.media-home-live-guide-next{order:20}.media-home-continue-watching{order:30}.media-home-discovery[data-home-discovery-rail="newly"]{order:40}.media-home-discovery[data-home-discovery-rail="series"]{order:45}.media-home-discovery[data-home-discovery-rail="genres"]{order:50}.media-home-discovery[data-home-discovery-rail="folders"]{order:60}.media-home-recently-watched{order:80}
.media-home-live-guide{min-width:0}.media-home-live-guide-rail{display:grid;grid-auto-flow:column;grid-auto-columns:minmax(11rem,15rem);gap:.8rem;overflow-x:auto;padding:0 1rem 1.1rem;scroll-snap-type:x proximity;overscroll-behavior-inline:contain}.media-home-live-guide-card{scroll-snap-align:start;min-width:0;overflow:hidden;border:1px solid rgba(148,163,184,.2);border-radius:.95rem;background:rgba(15,23,42,.76);color:#e2e8f0;cursor:pointer;transition:border-color .16s ease,box-shadow .16s ease,transform .16s ease}.media-home-live-guide-card:hover,.media-home-live-guide-card.detail-selected{border-color:rgba(125,211,252,.62);box-shadow:0 0 0 1px rgba(56,189,248,.16) inset}.media-home-live-guide-card:hover{transform:translateY(-1px)}.media-home-live-guide-card:focus-visible{outline:3px solid rgba(125,211,252,.96);outline-offset:2px}.media-home-live-guide-artwork{display:grid;place-items:center;width:100%;aspect-ratio:2/3;overflow:hidden;background:linear-gradient(135deg,#172033,#26364d)}.media-home-live-guide-artwork img{display:block;width:100%;height:100%;object-fit:cover}.media-home-live-guide-artwork.is-fallback{background:radial-gradient(circle at 70% 22%,rgba(56,189,248,.18),transparent 32%),linear-gradient(145deg,#172033,#26364d)}.media-home-live-guide-logo{box-sizing:border-box;width:72%;height:4rem;padding:.35rem;border-radius:.65rem;background:rgba(248,250,252,.96);overflow:hidden}.media-home-live-guide-logo img{width:100%;height:100%;object-fit:contain}.media-home-live-guide-copy{display:grid;gap:.28rem;padding:.7rem}.media-home-live-guide-channel,.media-home-live-guide-subtitle{overflow:hidden;color:#94a3b8;font-size:.78rem;white-space:nowrap;text-overflow:ellipsis}.media-home-live-guide-copy strong{overflow:hidden;color:#f8fafc;white-space:nowrap;text-overflow:ellipsis}.media-home-live-guide-progress{display:block;width:100%;height:.26rem;margin-top:.2rem;border:0;border-radius:999px;background:rgba(148,163,184,.2);overflow:hidden;appearance:none}.media-home-live-guide-progress::-webkit-progress-bar{border-radius:999px;background:rgba(148,163,184,.2)}.media-home-live-guide-progress::-webkit-progress-value{border-radius:999px;background:#38bdf8}.media-home-live-guide-progress::-moz-progress-bar{border-radius:999px;background:#38bdf8}.media-home-live-guide-actions{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:.42rem;padding:0 .7rem .7rem}.media-home-live-guide-actions.single{grid-template-columns:1fr}.media-home-live-guide-action{min-width:0;min-height:2.35rem;padding:.42rem .5rem;border:1px solid rgba(125,211,252,.32);border-radius:.62rem;background:rgba(15,23,42,.9);color:#e0f2fe;font-size:.76rem;font-weight:800;cursor:pointer}.media-home-live-guide-action.primary{border-color:rgba(56,189,248,.68);background:#075985;color:#fff}.media-home-live-guide-action:focus-visible{outline:3px solid rgba(125,211,252,.96);outline-offset:2px}.media-home-live-guide-action:disabled{cursor:not-allowed;opacity:.58}.media-home-live-guide-feedback:empty{display:none}.media-home-live-guide-feedback{padding:0 .7rem .7rem}.media-home-live-guide-action-error{margin:0;color:#fecaca;font-size:.74rem}.media-home-live-guide-feedback .epg-timer-status{margin:0;padding:.55rem .6rem;border-radius:.62rem;font-size:.74rem}.media-home-live-guide-feedback .epg-timer-status h4,.media-home-live-guide-feedback .epg-timer-status p{margin:.1rem 0}
.media-home-live-guide-detail{box-sizing:border-box;margin:0 1rem 1.25rem;padding:.8rem;border:1px solid rgba(125,211,252,.28);border-radius:1rem;background:linear-gradient(145deg,rgba(15,23,42,.9),rgba(2,6,23,.96));box-shadow:0 1rem 2.4rem rgba(2,6,23,.24)}.media-home-live-guide-detail-head{display:flex;align-items:center;justify-content:space-between;gap:.75rem;margin:0 0 .7rem}.media-home-live-guide-detail-head h4{margin:0;color:#f8fafc;font-size:1rem}.media-home-live-guide-detail-close{min-height:2.35rem;padding:.42rem .7rem;border:1px solid rgba(148,163,184,.26);border-radius:.62rem;background:rgba(15,23,42,.9);color:#e2e8f0;font-weight:750;cursor:pointer}.media-home-live-guide-detail-close:focus-visible{outline:3px solid rgba(125,211,252,.96);outline-offset:2px}.media-home-live-guide-detail-state{margin:0;padding:.8rem;border:1px solid rgba(148,163,184,.14);border-radius:.72rem;color:#94a3b8;background:rgba(15,23,42,.5)}.media-home-live-guide-detail-state.error{border-color:rgba(239,68,68,.4);color:#fecaca}.media-home-live-guide-detail>.epg-event-detail{margin:0}
@media(min-width:120rem){.media-home-live-hero-active{min-height:30rem}.media-home-live-carousel{max-width:150rem;margin-inline:auto;padding:clamp(2rem,3vw,4rem)}.media-home-live-focus{padding:clamp(2rem,2vw,3rem)}.media-home-live-channel-copy h3{font-size:clamp(3.2rem,3.4vw,5.2rem)}.media-home-live-program-title{font-size:clamp(1.15rem,1vw,1.45rem)}}
@media(max-width:72rem){.media-home-live-carousel{grid-template-columns:minmax(5.4rem,.34fr) minmax(0,2.2fr) minmax(5.4rem,.34fr);padding:1rem}.media-home-live-neighbor-copy span{display:none}}
@media(max-width:46rem){.media-home-live-hero-active{min-height:25rem}.media-home-live-carousel{grid-template-columns:minmax(2.75rem,.18fr) minmax(0,1fr) minmax(2.75rem,.18fr);gap:.35rem;padding:.72rem .35rem}.media-home-live-focus{padding:1rem .8rem;border-radius:1.05rem}.media-home-live-channel-head{align-items:flex-start;flex-direction:column;gap:.65rem}.media-home-live-channel-logo{width:5.5rem;height:3.4rem}.media-home-live-channel-copy h3{font-size:clamp(2rem,10vw,3.25rem)}.media-home-live-programmes{grid-template-columns:1fr}.media-home-live-program{padding:.68rem .72rem}.media-home-live-program:nth-child(2){background:rgba(15,23,42,.5)}.media-home-live-actions{display:grid;grid-template-columns:1fr 1fr}.media-home-live-action{width:100%;padding:.58rem .55rem}.media-home-live-neighbor{padding:.3rem;border-color:rgba(148,163,184,.12);background:rgba(15,23,42,.3)}.media-home-live-neighbor-logo{height:2.35rem;padding:.18rem}.media-home-live-neighbor-copy{display:none}.media-home-live-guide-rail{grid-auto-columns:minmax(42vw,11rem);padding:0 .78rem 1rem}.media-home-live-guide-actions{padding-left:.62rem;padding-right:.62rem}.media-home-live-guide-feedback{padding-left:.62rem;padding-right:.62rem}.media-home-live-guide-detail{margin-left:.78rem;margin-right:.78rem;padding:.68rem}}
@media(max-height:34rem) and (min-width:40rem) and (max-width:64rem){.media-home-live-hero-active{min-height:16rem}.media-home-live-carousel{grid-template-columns:4.2rem minmax(0,1fr) 4.2rem;padding:.55rem}.media-home-live-focus{grid-template-columns:minmax(0,1.25fr) minmax(0,1fr);gap:.55rem;padding:.7rem}.media-home-live-channel-head{grid-column:1}.media-home-live-programmes{grid-column:2;grid-row:1 / span 2;grid-template-columns:1fr}.media-home-live-actions{grid-column:1}.media-home-live-notice{grid-column:1}.media-home-live-neighbor-copy{display:none}}
@media(prefers-reduced-motion:reduce){.media-home-live-neighbor,.media-home-live-guide-card{transition:none}.media-home-live-neighbor:hover,.media-home-live-neighbor:focus-visible,.media-home-live-guide-card:hover{transform:none}.media-home-live-carousel,.media-home-live-guide-rail{scroll-behavior:auto}}
`;
    doc.head.appendChild(style);
    return true;
  }

  function bindNavigation() {
    if (state.navigationBound || !doc || typeof doc.addEventListener !== 'function') return;
    state.navigationBound = true;
    doc.addEventListener('click', () => scheduleSync(false));
  }

  function installObserver() {
    if (state.observer || typeof global.MutationObserver !== 'function' || !doc) return;
    const nav = typeof doc.getElementById === 'function' ? doc.getElementById('module-nav') : null;
    const backends = typeof doc.getElementById === 'function' ? doc.getElementById('backends') : null;
    if (!nav && !backends) return;
    state.observer = new global.MutationObserver(() => scheduleSync(false));
    if (nav) state.observer.observe(nav, {subtree: true, attributes: true, attributeFilter: ['class']});
    if (backends) state.observer.observe(backends, {subtree: true, childList: true, attributes: true, attributeFilter: ['class', 'aria-selected']});
  }

  function install() {
    if (!doc) return;
    installStyles();
    bindNavigation();
    installObserver();
    scheduleSync(false);
  }

  function snapshot() {
    const channel = currentChannel();
    const current = channel ? currentEventForChannel(channel, state.events) : null;
    const next = channel ? nextEventForChannel(channel, state.events) : null;
    return Object.freeze({
      active: state.active,
      backendId: state.backendId,
      channelCount: state.channels.length,
      eventCount: state.events.length,
      selectedIndex: state.selectedIndex,
      selectedChannelId: channelId(channel),
      currentEventTitle: current ? eventTitle(current) : '',
      nextEventTitle: next ? eventTitle(next) : '',
      loadingChannels: state.loadingChannels,
      loadingPrograms: state.loadingPrograms,
      dataError: state.dataError,
      programError: state.programError,
      actionError: state.actionError
    });
  }

  const api = Object.freeze({
    refresh: () => sync(true),
    snapshot,
    selectOffset,
    watchLive,
    openEpg,
    __test: Object.freeze({
      orderedChannels,
      currentEventForChannel,
      nextEventForChannel,
      progressPercent,
      prefersReducedMotion,
      programmeEntries,
      createProgrammeTimer,
      loadProgrammeDetailOwner,
      openProgrammeDetail,
      clearProgrammeDetails,
      renderProgrammeRails,
      applyChannels,
      applyPrograms,
      render,
      bindHeroInteractions,
      setActive(value) { state.active = Boolean(value); },
      setBackendId(value) { state.backendId = text(value); },
      selectIndex,
      load,
      sync
    })
  });

  global.VdrSuiteHomeLiveHero = api;
  if (doc) {
    if (doc.readyState === 'loading' && typeof doc.addEventListener === 'function') doc.addEventListener('DOMContentLoaded', install, {once: true});
    else install();
  }
}(window));