(function (global) {
  'use strict';

  const doc = global.document;
  const ENDPOINT = '/api/media/continue-watching';

  function text(value) { return value == null ? '' : String(value); }
  function positiveInt(value) {
    const number = Number(value);
    return Number.isFinite(number) && number > 0 ? Math.floor(number) : 0;
  }
  function selectedBackendId() {
    const selected = doc && doc.querySelector ? doc.querySelector('#backends .backend-card.selected, #backends [aria-selected="true"]') : null;
    return text(selected && selected.dataset && selected.dataset.backendId) || 'default';
  }
  function normalizeItem(value) {
    if (!value || typeof value !== 'object') return null;
    const backendId = text(value.backendId);
    const recordingId = text(value.recordingId);
    const position = positiveInt(value.resumePositionSeconds);
    const durationKnown = value.durationKnown === true;
    const duration = durationKnown ? positiveInt(value.durationSeconds) : 0;
    if (!backendId || !recordingId || !position) return null;
    if (durationKnown && duration > 0 && position >= duration) return null;
    return {
      backendId,
      recordingId,
      title: text(value.title) || 'Aufnahme',
      subtitle: text(value.subtitle),
      resumePositionSeconds: position,
      durationKnown,
      durationSeconds: duration,
      lastActivityAt: text(value.lastActivityAt)
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
  function ensureRecordings2() {
    if (recordings2Ready()) return Promise.resolve(true);
    const runtimes = global.VdrSuiteDeferredFrontendRuntimes;
    if (!runtimes || typeof runtimes.loadRecordings2 !== 'function') return Promise.resolve(false);
    return Promise.resolve(runtimes.loadRecordings2()).then(recordings2Ready).catch(function () { return false; });
  }
  function openItem(item, resume) {
    const normalized = normalizeItem(item);
    if (!normalized) return Promise.resolve(false);
    releasePreview();
    return ensureRecordings2().then(function (ready) {
      if (!ready) return false;
      global.VdrSuiteRecordings2.openRecording({
        id: normalized.recordingId,
        backendId: normalized.backendId,
        title: normalized.title
      }, {
        autoStartPlayback: true,
        playbackStartPositionSeconds: resume ? normalized.resumePositionSeconds : 0,
        continueWatching: true
      });
      return true;
    });
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
      host.appendChild(section);
    }
    section.innerHTML = '';
    const heading = doc.createElement('div');
    heading.className = 'media-home-section-heading';
    const title = doc.createElement('h3');
    title.textContent = 'Weiterschauen';
    heading.appendChild(title);
    section.appendChild(heading);
    const rail = doc.createElement('div');
    rail.className = 'media-home-continue-rail';
    valid.forEach(function (item) {
      const card = doc.createElement('article');
      card.className = 'media-home-continue-card';
      card.dataset.recordingId = item.recordingId;
      card.dataset.backendId = item.backendId;
      const artwork = doc.createElement('div');
      artwork.className = 'media-home-continue-artwork';
      artwork.textContent = item.title.slice(0, 1).toUpperCase();
      card.appendChild(artwork);
      const copy = doc.createElement('div');
      copy.className = 'media-home-continue-copy';
      const name = doc.createElement('h4'); name.textContent = item.title; copy.appendChild(name);
      if (item.subtitle) { const subtitle = doc.createElement('p'); subtitle.textContent = item.subtitle; copy.appendChild(subtitle); }
      const progress = progressModel(item);
      const progressText = doc.createElement('p');
      progressText.className = 'media-home-continue-progress-text';
      progressText.textContent = progress.hasPercent
        ? formatTime(progress.positionSeconds) + ' / ' + formatTime(progress.durationSeconds)
        : 'Fortsetzen bei ' + formatTime(progress.positionSeconds);
      copy.appendChild(progressText);
      if (progress.hasPercent) {
        const meter = doc.createElement('progress');
        meter.max = 100; meter.value = progress.percent;
        meter.setAttribute('aria-label', 'Wiedergabefortschritt ' + progress.percent + ' Prozent');
        copy.appendChild(meter);
      }
      const actions = doc.createElement('div'); actions.className = 'media-home-continue-actions';
      const resume = doc.createElement('button'); resume.type = 'button'; resume.textContent = 'Fortsetzen'; resume.setAttribute('data-home-continue-action', 'continue');
      const restart = doc.createElement('button'); restart.type = 'button'; restart.textContent = 'Von vorn'; restart.setAttribute('data-home-continue-action', 'restart');
      resume.addEventListener('click', function () { openItem(item, true); });
      restart.addEventListener('click', function () { openItem(item, false); });
      actions.appendChild(resume); actions.appendChild(restart); copy.appendChild(actions);
      card.appendChild(copy); rail.appendChild(card);
    });
    section.appendChild(rail);
    return true;
  }
  function installStyles() {
    if (!doc || !doc.head || doc.getElementById('vdr-suite-continue-watching-style')) return;
    const style = doc.createElement('style');
    style.id = 'vdr-suite-continue-watching-style';
    style.textContent = '.media-home-continue-watching{min-width:0}.media-home-continue-rail{display:grid;grid-auto-flow:column;grid-auto-columns:minmax(17rem,24rem);gap:.85rem;overflow-x:auto;padding:0 .25rem 1rem;scroll-snap-type:x proximity}.media-home-continue-card{scroll-snap-align:start;display:grid;grid-template-columns:6rem 1fr;gap:.8rem;padding:.8rem;border:1px solid rgba(148,163,184,.24);border-radius:1rem;background:rgba(15,23,42,.78)}.media-home-continue-artwork{display:grid;place-items:center;min-height:6rem;border-radius:.75rem;background:linear-gradient(135deg,#1e293b,#334155);font-size:2rem;font-weight:700}.media-home-continue-copy{min-width:0}.media-home-continue-copy h4,.media-home-continue-copy p{margin:.1rem 0 .45rem}.media-home-continue-copy progress{width:100%}.media-home-continue-actions{display:flex;flex-wrap:wrap;gap:.45rem;margin-top:.65rem}.media-home-continue-actions button{min-height:2.75rem;padding:.55rem .75rem;border-radius:.65rem}@media(max-width:46rem){.media-home-continue-rail{grid-auto-columns:minmax(80vw,20rem)}.media-home-continue-card{grid-template-columns:5rem 1fr}}';
    doc.head.appendChild(style);
  }
  function refresh() {
    const backendId = selectedBackendId();
    return post({operation: 'list', backendId}).then(function (payload) {
      render(payload && payload.items);
      return true;
    }).catch(function () {
      render([]);
      return false;
    });
  }
  function install() {
    installStyles();
    refresh();
    if (doc && typeof doc.addEventListener === 'function') {
      doc.addEventListener('click', function (event) {
        const target = event && event.target;
        if (target && typeof target.closest === 'function' && target.closest('[data-brand-module="overview"], .module-tab[data-module="overview"], #backends')) {
          global.setTimeout(refresh, 0);
        }
      });
    }
    return true;
  }

  global.VdrSuiteHomeContinueWatching = Object.freeze({
    install,
    refresh,
    _test: Object.freeze({normalizeItem, progressModel, openItem, formatTime, post, ensureRecordings2, releasePreview})
  });
  if (doc) {
    if (doc.readyState === 'loading') doc.addEventListener('DOMContentLoaded', install, {once: true});
    else install();
  }
}(window));