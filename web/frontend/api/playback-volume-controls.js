// Phase 65.D browser-local Volume/Mute controls.
//
// ADR-0053 keeps volume and mute as transient client-local player state. This
// decorator therefore owns only presentation controls around the existing
// playback owner and its currently active HTMLMediaElement. It never creates a
// MediaSession, media element, transport, restart path, or VDR volume command.
(function (global) {
  'use strict';

  const marker = '__vdrSuitePlaybackVolumeControlsBound';
  const STYLE_ID = 'vdr-suite-playback-volume-controls-style';
  const VOLUME_EPSILON = 0.005;
  if (!global || !global.document || global[marker] === true) return;

  const descriptor = Object.getOwnPropertyDescriptor(global, 'VdrSuiteRecordings2Playback');
  if (!descriptor || typeof descriptor.get !== 'function' || typeof descriptor.set !== 'function') return;

  // This is intentionally page-local, not persisted Suite domain state. It is
  // updated only from a real owned media element and lets a clean owner handoff
  // (for example Live channel replacement) retain the user's confirmed local
  // player state without turning Volume/Mute into a MediaSession mutation.
  const clientPreference = {
    initialized: false,
    volume: 1,
    muted: false
  };

  function finiteNumber(value, fallback) {
    const number = Number(value);
    return Number.isFinite(number) ? number : fallback;
  }

  function clamp(value, minimum, maximum) {
    return Math.min(maximum, Math.max(minimum, value));
  }

  function normalizeVolume(value) {
    return clamp(finiteNumber(value, 1), 0, 1);
  }

  function volumeFromPercent(value) {
    return clamp(finiteNumber(value, 100), 0, 100) / 100;
  }

  function volumeToPercent(value) {
    return Math.round(normalizeVolume(value) * 100);
  }

  function installStyles() {
    const document = global.document;
    if (!document || !document.head || typeof document.createElement !== 'function') return;
    if (typeof document.getElementById === 'function' && document.getElementById(STYLE_ID)) return;

    const style = document.createElement('style');
    style.id = STYLE_ID;
    style.textContent = `
.recordings2-volume-owner-shell{display:grid;gap:.65rem}
.recordings2-volume-controls{display:flex;align-items:center;gap:.65rem;flex-wrap:wrap;margin-top:.1rem;padding:.55rem .65rem;border:1px solid rgba(148,163,184,.22);border-radius:.72rem;background:rgba(15,23,42,.58)}
.recordings2-volume-mute{min-height:2.75rem;min-width:5.5rem;padding:.45rem .7rem}
.recordings2-volume-range-label{display:flex;align-items:center;gap:.55rem;flex:1 1 15rem;min-width:0;color:#cbd5e1;font-size:.86rem;font-weight:700}
.recordings2-volume-range{min-height:2.75rem;min-width:8rem;flex:1 1 12rem;touch-action:pan-y}
.recordings2-volume-output{min-width:3.7rem;color:#f8fafc;text-align:right;font-variant-numeric:tabular-nums}
.recordings2-volume-status{flex:1 0 100%;margin:0;color:#fbbf24;font-size:.78rem}
@media(max-width:520px){.recordings2-volume-controls{align-items:stretch}.recordings2-volume-range-label{flex-basis:100%}.recordings2-volume-mute{flex:0 0 auto}}
`;
    document.head.appendChild(style);
  }

  function firstVideo(root) {
    if (!root) return null;
    if (String(root.tagName || '').toUpperCase() === 'VIDEO') return root;
    return typeof root.querySelector === 'function' ? root.querySelector('video') : null;
  }

  function decoratePanel(panel) {
    if (!panel || !panel.element || panel.__vdrSuiteVolumeControlsDecorated === true) return panel;

    installStyles();
    const document = global.document;
    const shell = document.createElement('div');
    shell.className = 'recordings2-volume-owner-shell';
    shell.appendChild(panel.element);

    const controls = document.createElement('div');
    controls.className = 'recordings2-volume-controls';
    controls.setAttribute('role', 'group');
    controls.setAttribute('aria-label', 'Lautstärke');

    const muteButton = document.createElement('button');
    muteButton.type = 'button';
    muteButton.className = 'recordings2-volume-mute';
    controls.appendChild(muteButton);

    const rangeLabel = document.createElement('label');
    rangeLabel.className = 'recordings2-volume-range-label';
    rangeLabel.appendChild(document.createTextNode
      ? document.createTextNode('Lautstärke')
      : (function () {
          const textNode = document.createElement('span');
          textNode.textContent = 'Lautstärke';
          return textNode;
        }()));

    const range = document.createElement('input');
    range.type = 'range';
    range.className = 'recordings2-volume-range';
    range.min = '0';
    range.max = '100';
    range.step = '1';
    range.value = '100';
    range.inputMode = 'numeric';
    range.setAttribute('aria-label', 'Lautstärke in Prozent');
    rangeLabel.appendChild(range);

    const output = document.createElement('output');
    output.className = 'recordings2-volume-output';
    output.textContent = '100 %';
    rangeLabel.appendChild(output);
    controls.appendChild(rangeLabel);

    const status = document.createElement('p');
    status.className = 'recordings2-volume-status';
    status.setAttribute('role', 'status');
    status.hidden = true;
    controls.appendChild(status);
    shell.appendChild(controls);

    let disposed = false;
    let activeVideo = null;
    let volumeWritable = true;
    let mutedWritable = true;
    let observer = null;

    function setStatus(message) {
      status.textContent = message || '';
      status.hidden = !message;
    }

    function updateUiFromVideo() {
      const video = activeVideo;
      if (!video) {
        range.disabled = true;
        muteButton.disabled = true;
        return;
      }

      const currentVolume = normalizeVolume(video.volume);
      const currentMuted = Boolean(video.muted);
      range.value = String(volumeToPercent(currentVolume));
      output.textContent = String(volumeToPercent(currentVolume)) + ' %';
      range.disabled = !volumeWritable;
      muteButton.disabled = !mutedWritable;
      muteButton.textContent = currentMuted ? 'Ton an' : 'Stumm';
      muteButton.title = currentMuted ? 'Stummschaltung aufheben' : 'Ton stummschalten';
      muteButton.setAttribute('aria-label', muteButton.title);
      muteButton.setAttribute('aria-pressed', currentMuted ? 'true' : 'false');
    }

    function rememberFromVideo(video) {
      if (!video) return;
      clientPreference.volume = normalizeVolume(video.volume);
      clientPreference.muted = Boolean(video.muted);
      clientPreference.initialized = true;
    }

    function handleVolumeChange() {
      if (disposed || !activeVideo) return;
      rememberFromVideo(activeVideo);
      updateUiFromVideo();
    }

    function unbindVideo() {
      if (!activeVideo) return;
      if (typeof activeVideo.removeEventListener === 'function') {
        activeVideo.removeEventListener('volumechange', handleVolumeChange);
      }
      activeVideo = null;
    }

    function verifyVolume(target) {
      const actual = normalizeVolume(activeVideo && activeVideo.volume);
      if (Math.abs(actual - target) <= VOLUME_EPSILON) return true;
      volumeWritable = false;
      setStatus('Dieser Browser übernimmt die angeforderte Player-Lautstärke nicht; Systemlautstärke verwenden.');
      return false;
    }

    function verifyMuted(target) {
      const actual = Boolean(activeVideo && activeVideo.muted);
      if (actual === target) return true;
      mutedWritable = false;
      setStatus('Dieser Browser übernimmt die angeforderte Stummschaltung nicht.');
      return false;
    }

    function applyPreference(video) {
      if (!video || !clientPreference.initialized) return;
      volumeWritable = typeof video.volume === 'number';
      mutedWritable = typeof video.muted === 'boolean';

      if (volumeWritable) {
        try {
          video.volume = clientPreference.volume;
          verifyVolume(clientPreference.volume);
        } catch (error) {
          volumeWritable = false;
          setStatus('Dieser Browser erlaubt keine programmatische Player-Lautstärke; Systemlautstärke verwenden.');
        }
      }
      if (mutedWritable) {
        try {
          video.muted = clientPreference.muted;
          verifyMuted(clientPreference.muted);
        } catch (error) {
          mutedWritable = false;
          setStatus('Dieser Browser erlaubt keine programmatische Stummschaltung.');
        }
      }
    }

    function bindCurrentVideo() {
      if (disposed) return null;
      const next = firstVideo(panel.element);
      if (next === activeVideo) {
        updateUiFromVideo();
        return next;
      }

      unbindVideo();
      activeVideo = next;
      volumeWritable = Boolean(activeVideo && typeof activeVideo.volume === 'number');
      mutedWritable = Boolean(activeVideo && typeof activeVideo.muted === 'boolean');
      setStatus('');

      if (!activeVideo) {
        updateUiFromVideo();
        return null;
      }

      if (clientPreference.initialized) applyPreference(activeVideo);
      else rememberFromVideo(activeVideo);
      if (typeof activeVideo.addEventListener === 'function') {
        activeVideo.addEventListener('volumechange', handleVolumeChange);
      }
      rememberFromVideo(activeVideo);
      updateUiFromVideo();
      return activeVideo;
    }

    function setVolumePercent(percent) {
      const video = bindCurrentVideo();
      if (!video || !volumeWritable) return false;
      const target = volumeFromPercent(percent);
      try {
        video.volume = target;
      } catch (error) {
        volumeWritable = false;
        setStatus('Dieser Browser erlaubt keine programmatische Player-Lautstärke; Systemlautstärke verwenden.');
        updateUiFromVideo();
        return false;
      }
      const accepted = verifyVolume(target);
      rememberFromVideo(video);
      updateUiFromVideo();
      return accepted;
    }

    function setMuted(muted) {
      const video = bindCurrentVideo();
      if (!video || !mutedWritable) return false;
      const target = Boolean(muted);
      try {
        video.muted = target;
      } catch (error) {
        mutedWritable = false;
        setStatus('Dieser Browser erlaubt keine programmatische Stummschaltung.');
        updateUiFromVideo();
        return false;
      }
      const accepted = verifyMuted(target);
      rememberFromVideo(video);
      updateUiFromVideo();
      return accepted;
    }

    function dispose() {
      if (disposed) return;
      disposed = true;
      if (observer && typeof observer.disconnect === 'function') observer.disconnect();
      observer = null;
      unbindVideo();
    }

    range.addEventListener('input', function () {
      setVolumePercent(range.value);
    });
    muteButton.addEventListener('click', function () {
      const video = bindCurrentVideo();
      if (!video) return;
      setMuted(!Boolean(video.muted));
    });

    bindCurrentVideo();
    if (typeof global.MutationObserver === 'function') {
      observer = new global.MutationObserver(function () {
        // Observe only transport-owned DOM. The Volume/Mute UI itself updates
        // textContent, which is a childList mutation in real browsers; watching
        // the outer shell would therefore self-trigger indefinitely.
        if (firstVideo(panel.element) !== activeVideo) bindCurrentVideo();
      });
      observer.observe(panel.element, {childList: true, subtree: true});
    }

    const wrapped = {};
    Object.keys(panel).forEach(function (key) { wrapped[key] = panel[key]; });
    wrapped.element = shell;
    wrapped.__vdrSuiteVolumeControlsDecorated = true;

    if (typeof panel.destroy === 'function') {
      wrapped.destroy = function () {
        dispose();
        return panel.destroy.apply(panel, arguments);
      };
    }
    if (typeof panel.relinquishForReplacement === 'function') {
      wrapped.relinquishForReplacement = function () {
        let result;
        try {
          result = panel.relinquishForReplacement.apply(panel, arguments);
        } catch (error) {
          dispose();
          throw error;
        }
        return Promise.resolve(result).then(function (value) {
          dispose();
          return value;
        }, function (error) {
          dispose();
          throw error;
        });
      };
    }

    return Object.freeze(wrapped);
  }

  let cachedSource = null;
  let cachedDecorated = null;

  function decoratePlayback(source) {
    const value = source && typeof source === 'object' ? source : {};
    if (value === cachedSource && cachedDecorated) return cachedDecorated;
    if (value.__vdrSuitePlaybackVolumeDecorated === true) return value;
    if (typeof value.createPanel !== 'function' && typeof value.createLivePanel !== 'function') return value;

    const decorated = {};
    Object.keys(value).forEach(function (key) { decorated[key] = value[key]; });
    if (typeof value.createPanel === 'function') {
      const recordingFactory = value.createPanel;
      decorated.createPanel = function () {
        return decoratePanel(recordingFactory.apply(value, arguments));
      };
    }
    if (typeof value.createLivePanel === 'function') {
      const liveFactory = value.createLivePanel;
      decorated.createLivePanel = function () {
        return decoratePanel(liveFactory.apply(value, arguments));
      };
    }
    decorated.__vdrSuitePlaybackVolumeDecorated = true;
    cachedSource = value;
    cachedDecorated = Object.freeze(decorated);
    return cachedDecorated;
  }

  Object.defineProperty(global, 'VdrSuiteRecordings2Playback', {
    configurable: descriptor.configurable !== false,
    enumerable: descriptor.enumerable !== false,
    get: function () { return decoratePlayback(descriptor.get.call(global)); },
    set: function (value) {
      cachedSource = null;
      cachedDecorated = null;
      descriptor.set.call(global, value);
    }
  });

  global[marker] = true;
  global.VdrSuitePlaybackVolumeControls = Object.freeze({
    __test: Object.freeze({
      normalizeVolume: normalizeVolume,
      volumeFromPercent: volumeFromPercent,
      volumeToPercent: volumeToPercent,
      decoratePanel: decoratePanel,
      decoratePlayback: decoratePlayback
    })
  });
}(window));
