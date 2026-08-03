(function (global) {
  'use strict';

  function firstValue(object, keys, fallback) {
    for (const key of keys) {
      if (object && object[key] !== undefined && object[key] !== null && object[key] !== '') {
        return object[key];
      }
    }
    return fallback;
  }

  function selectedBackendId(event) {
    const explicit = String(firstValue(event, ['backendId', 'backend'], '')).trim();
    if (explicit) return explicit;
    const platform = global.VdrSuitePlatform;
    if (platform && typeof platform.getSelectedBackendId === 'function') {
      const selected = String(platform.getSelectedBackendId() || '').trim();
      if (selected) return selected;
    }
    return 'default';
  }

  function eventChannelId(event, channel) {
    return String(firstValue(
      event,
      ['channelId', 'channel', 'channel_id'],
      firstValue(channel, ['id', 'channelId', 'nativeId'], '')
    )).trim();
  }

  function eventId(event) {
    return String(firstValue(event, ['eventId', 'id', 'nativeId'], '')).trim();
  }

  function isPersistentArtworkUrl(value) {
    return String(value || '').trim().startsWith('/api/epg/cache/artwork?');
  }

  function resolvePublicUrl(value) {
    const url = String(value || '').trim();
    if (!url) return '';

    const publicUrl = global.VdrSuitePublicUrl;
    if (!publicUrl || typeof publicUrl.resolvePath !== 'function') return url;

    try {
      return publicUrl.resolvePath(url);
    } catch (error) {
      return '';
    }
  }

  function persistentArtworkUrl(event, channel) {
    const artwork = event && event.artwork && typeof event.artwork === 'object'
      ? event.artwork
      : {};
    const explicit = String(artwork.url || '').trim();
    if (artwork.available === true && isPersistentArtworkUrl(explicit)) return explicit;

    const channelId = eventChannelId(event, channel);
    const nativeEventId = eventId(event);
    if (!channelId || !nativeEventId) return '';

    return '/api/epg/cache/artwork?' + new URLSearchParams({
      backend: selectedBackendId(event),
      channelId: channelId,
      eventId: nativeEventId,
      probe: String(Date.now())
    }).toString();
  }

  function attachPersistentArtwork(detail, event, channel) {
    if (!detail || detail.querySelector('.epg-detail-artwork')) return;
    const url = resolvePublicUrl(persistentArtworkUrl(event, channel));
    if (!url || typeof global.Image !== 'function') return;

    const probe = new global.Image();
    probe.onload = function () {
      if (detail.querySelector('.epg-detail-artwork')) return;
      const hero = detail.querySelector('.epg-detail-hero');
      if (!hero) return;
      const image = document.createElement('div');
      image.className = 'epg-detail-artwork';
      image.setAttribute('role', 'img');
      image.setAttribute('aria-label', 'Gespeichertes Bild zu ' + String(firstValue(event, ['title'], 'Sendung')));
      image.style.backgroundImage = 'url("' + url.replace(/["\\\r\n]/g, '') + '")';
      detail.classList.add('epg-has-artwork');
      detail.insertBefore(image, hero);
    };
    probe.src = url;
  }

  function createCard(event, channel) {
    if (typeof createEpgEventDetailCard !== 'function') {
      throw new Error('Der bestehende EPG-Detail-Renderer ist nicht verfügbar.');
    }
    const detail = createEpgEventDetailCard(event, channel);
    attachPersistentArtwork(detail, event, channel);
    return detail;
  }

  function mountTarget() {
    const platform = global.VdrSuitePlatform;
    if (platform && typeof platform.getMountTarget === 'function') {
      return platform.getMountTarget('genres') ||
        platform.getMountTarget('detail') ||
        document.getElementById('detail-data');
    }
    return document.getElementById('detail-data');
  }

  function open(event, channel, options) {
    const target = mountTarget();
    if (!target) return false;
    const config = options && typeof options === 'object' ? options : {};
    const root = document.createElement('section');
    root.className = 'genres-owned-detail genres-owned-epg-detail';

    const back = document.createElement('button');
    back.type = 'button';
    back.className = 'genres-detail-back';
    back.textContent = config.backLabel || '← Zurück zum Genre';
    back.addEventListener('click', function () {
      if (typeof config.onClose === 'function') config.onClose();
    });
    root.appendChild(back);
    root.appendChild(createCard(event, channel));

    target.replaceChildren(root);
    return true;
  }

  global.VdrSuiteEpgDetailOwner = Object.freeze({
    createCard: createCard,
    open: open,
    persistentArtworkUrl: persistentArtworkUrl
  });
}(window));
