(function (global) {
  'use strict';

  function createCard(event, channel) {
    if (typeof createEpgEventDetailCard !== 'function') {
      throw new Error('Der bestehende EPG-Detail-Renderer ist nicht verfügbar.');
    }
    return createEpgEventDetailCard(event, channel);
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
    open: open
  });
}(window));
