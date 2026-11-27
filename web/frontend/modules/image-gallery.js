(function (global) {
  'use strict';

  const storageKey = 'vdr-suite.image-gallery.view';
  const mode = value => value === 'grid' ? 'grid' : 'carousel';
  const clamp = (value, size) => Math.max(0, Math.min(value, size - 1));
  const words = {
    de: {grid:'Raster', carousel:'Karussell', back:'Zurück', empty:'Keine Bilder',
      loading:'Bilder werden geladen …', error:'Bildquelle nicht erreichbar',
      sources:'Bildquellen', play:'Diashow', pause:'Pause', previous:'Vorheriges Bild',
      next:'Nächstes Bild', rotate:'Drehen', zoom:'Zoom', info:'Information', close:'Schließen'},
    en: {grid:'Grid', carousel:'Carousel', back:'Back', empty:'No pictures',
      loading:'Loading pictures …', error:'Picture source unavailable',
      sources:'Picture sources', play:'Slideshow', pause:'Pause', previous:'Previous picture',
      next:'Next picture', rotate:'Rotate', zoom:'Zoom', info:'Information', close:'Close'}
  };

  function createNavigation(storage) {
    let currentMode = 'carousel';
    try { currentMode = mode(storage.getItem(storageKey)); } catch (_) {}
    const state = {mode:currentMode, entries:[], selected:0, full:false, rotation:0, zoom:1, info:false};
    function resetPhoto() { state.rotation = 0; state.zoom = 1; state.info = false; }
    function setEntries(entries, selection) {
      state.entries = entries.slice();
      state.selected = clamp(selection || 0, entries.length);
      state.full = false;
      resetPhoto();
    }
    function toggleMode() {
      state.mode = state.mode === 'grid' ? 'carousel' : 'grid';
      try { storage.setItem(storageKey, state.mode); } catch (_) {}
    }
    function move(delta) {
      state.selected = clamp(state.selected + delta, state.entries.length);
    }
    function open() {
      const entry = state.entries[state.selected];
      if (!entry || entry.directory) return false;
      state.full = true;
      resetPhoto();
      return true;
    }
    function stepPhoto(delta, repeat) {
      if (!state.full || !delta) return false;
      const photos = state.entries.map((entry, i) => entry.directory ? -1 : i).filter(i => i >= 0);
      const at = photos.indexOf(state.selected);
      if (at < 0) return false;
      let next = at + (delta < 0 ? -1 : 1);
      if (repeat) next = (next + photos.length) % photos.length;
      if (next < 0 || next >= photos.length) return false;
      state.selected = photos[next];
      resetPhoto();
      return true;
    }
    return {state, setEntries, toggleMode, move, open, stepPhoto,
      close() { state.full = false; resetPhoto(); },
      rotate() { state.rotation = (state.rotation + 90) % 360; },
      zoom() { state.zoom = state.zoom === 1 ? 1.5 : state.zoom === 1.5 ? 2 : 1; }};
  }

  function installStyles(doc) {
    if (doc.getElementById('image-gallery-style')) return;
    const style = doc.createElement('style');
    style.id = 'image-gallery-style';
    style.textContent = `
      .image-gallery{grid-column:1/-1;width:100%;min-width:0;background:#090a0c;color:#f5f5f5;padding:1rem;box-sizing:border-box}
      .image-gallery-bar{display:flex;align-items:center;gap:.6rem;flex-wrap:wrap;margin-bottom:1rem}
      .image-gallery-bar span{flex:1;min-width:8rem}
      .image-gallery button{font:inherit;color:inherit;background:#202124;border:1px solid #43464b;border-radius:.45rem;cursor:pointer}
      .image-gallery-bar button,.image-gallery-full-controls button{padding:.55rem .8rem}
      .image-gallery button:focus-visible,.image-gallery-tile.is-selected{outline:3px solid #40e2f1;outline-offset:2px}
      .image-gallery-grid{display:flex;flex-wrap:wrap;gap:.55rem}
      .image-gallery-tile{position:relative;min-width:0;overflow:hidden;padding:0;aspect-ratio:4/3;flex:1 1 calc(25% - .55rem)}
      .image-gallery-tile img{display:block;width:100%;height:100%;object-fit:cover}
      .image-gallery-folder{position:absolute;inset:auto 0 0;padding:.6rem;background:linear-gradient(transparent,#000d);text-align:left;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
      .image-gallery-placeholder{display:grid;place-items:center;height:100%;font-size:2.8rem;color:#b8b9be}
      .image-gallery-carousel{position:relative;height:clamp(14rem,48vw,36rem);perspective:1100px;overflow:hidden;display:grid;place-items:center}
      .image-gallery-carousel .image-gallery-tile{position:absolute;width:43%;height:68%;aspect-ratio:auto;transition:transform .22s ease,opacity .22s ease;box-shadow:0 1rem 2rem #0008;-webkit-box-reflect:below 6px linear-gradient(transparent 77%,#ffffff15)}
      .image-gallery-carousel .image-gallery-tile img{object-fit:contain;background:#101114}
      .image-gallery-carousel .image-gallery-tile.is-selected{outline:2px solid #40e2f1}
      .image-gallery-full{position:fixed;inset:0;z-index:10000;background:#000;display:grid;place-items:center;overflow:hidden}
      .image-gallery-full img{max-width:100%;max-height:100%;object-fit:contain;transition:transform .18s ease}
      .image-gallery-full-controls{position:absolute;bottom:0;left:0;right:0;display:flex;justify-content:center;gap:.4rem;flex-wrap:wrap;padding:.7rem;background:linear-gradient(transparent,#000b)}
      .image-gallery-info{position:absolute;top:1rem;left:1rem;padding:.8rem;background:#111d;max-width:90%;overflow-wrap:anywhere}
      @media(max-width:700px){.image-gallery-tile{flex-basis:calc(50% - .55rem)}.image-gallery{padding:.55rem}}
      @media(prefers-reduced-motion:reduce){.image-gallery-tile,.image-gallery-full img{transition:none!important}}
    `;
    doc.head.appendChild(style);
  }

  // A view owner, not an HTTP owner. Its adapter must return already-authorized
  // data and same-origin image URLs from the actual Public-v1 client.
  function mount(target, adapter, options) {
    const doc = target.ownerDocument;
    const settings = options || {};
    const labels = words[settings.locale === 'en' ? 'en' : 'de'];
    const nav = createNavigation(global.localStorage);
    const s = nav.state;
    let source = null, directory = '', parents = [], sources = [];
    let disposed = false, epoch = 0, timer = null, status = '', root = null, overlay = null;
    const interval = Math.max(1000, Number(settings.intervalMs) || 5000);
    const repeat = settings.repeat === true;
    installStyles(doc);

    function node(tag, css, content) {
      const el = doc.createElement(tag);
      if (css) el.className = css;
      if (content !== undefined) el.textContent = content;
      return el;
    }
    function button(label, fn, css) {
      const el = node('button', css, label);
      el.type = 'button';
      el.addEventListener('click', fn);
      return el;
    }
    function stop() {
      if (timer !== null) { global.clearInterval(timer); timer = null; }
    }
    function step(delta, loop) {
      if (!nav.stepPhoto(delta, loop)) return false;
      render();
      return true;
    }
    function toggleSlide() {
      if (timer !== null) { stop(); render(); return; }
      if (!s.full && !nav.open()) return;
      timer = global.setInterval(() => {
        if (!step(1, repeat)) { stop(); render(); }
      }, interval);
      render();
    }
    function closePhoto() { stop(); nav.close(); render(true); }
    function choose(index) {
      s.selected = index;
      const entry = s.entries[index];
      if (!entry) return;
      if (!source) {
        source = sources[index];
        directory = '';
        parents = [{source:null, directory:'', selected:index}];
        loadFolder(0);
      } else if (entry.directory) {
        parents.push({source, directory, selected:index});
        directory = entry.relativePath;
        loadFolder(0);
      } else if (nav.open()) render(true);
    }
    function back() {
      if (s.full) { closePhoto(); return; }
      const parent = parents.pop();
      if (!parent) return;
      source = parent.source;
      directory = parent.directory;
      if (source) loadFolder(parent.selected);
      else { nav.setEntries(sources.map(x => ({name:x.title,directory:true})), parent.selected); status=''; render(true); }
    }
    async function loadFolder(selection) {
      const request = ++epoch;
      status = labels.loading;
      render();
      try {
        const entries = await adapter.browse(source.id, directory);
        if (disposed || request !== epoch) return;
        nav.setEntries(entries, selection);
        status = '';
      } catch (_) {
        if (disposed || request !== epoch) return;
        nav.setEntries([]);
        status = labels.error;
      }
      render(true);
    }
    function tile(entry, index) {
      const el = button('', () => choose(index), 'image-gallery-tile' + (s.selected === index ? ' is-selected' : ''));
      el.dataset.index = String(index);
      el.setAttribute('aria-label', entry.name);
      el.addEventListener('focus', () => { s.selected = index; });
      if (source && entry.cover) {
        const image = node('img');
        image.alt = '';
        image.loading = 'lazy';
        image.decoding = 'async';
        image.src = adapter.previewUrl(source.id, entry.cover);
        el.appendChild(image);
      } else {
        el.appendChild(node('span','image-gallery-placeholder', entry.directory ? '▰' : '▧'));
      }
      if (entry.directory) el.appendChild(node('span','image-gallery-folder',entry.name));
      return el;
    }
    function renderFull(focus) {
      if (overlay) overlay.remove();
      overlay = null;
      if (!s.full || !source) return;
      const entry = s.entries[s.selected];
      overlay = node('section','image-gallery image-gallery-full');
      overlay.setAttribute('role','dialog');
      overlay.setAttribute('aria-modal','true');
      const image = node('img');
      image.alt = entry.name;
      image.src = adapter.originalUrl(source.id, entry.relativePath);
      image.style.transform = 'rotate(' + s.rotation + 'deg) scale(' + s.zoom + ')';
      overlay.appendChild(image);
      if (s.info) overlay.appendChild(node('div','image-gallery-info',entry.name));
      const controls = node('div','image-gallery-full-controls');
      controls.append(
        button(labels.previous, () => step(-1,false)),
        button(labels.next, () => step(1,false)),
        button(timer === null ? labels.play : labels.pause,toggleSlide),
        button(labels.rotate, () => { nav.rotate(); render(); }),
        button(labels.zoom, () => { nav.zoom(); render(); }),
        button(labels.info, () => { s.info = !s.info; render(); }),
        button(labels.close,closePhoto));
      overlay.appendChild(controls);
      doc.body.appendChild(overlay);
      if (focus) controls.firstElementChild.focus();
    }
    function render(focus) {
      if (disposed) return;
      if (root) root.remove();
      root = node('section','image-gallery');
      const bar = node('div','image-gallery-bar');
      if (parents.length) bar.appendChild(button(labels.back,back));
      bar.appendChild(node('span','',source ? source.title + (directory ? ' / ' + directory : '') : labels.sources));
      bar.appendChild(button(s.mode === 'grid' ? labels.carousel : labels.grid, () => { nav.toggleMode(); render(true); }));
      if (source && s.entries.some(x => !x.directory)) bar.appendChild(button(labels.play, toggleSlide));
      root.appendChild(bar);
      if (status || !s.entries.length) root.appendChild(node('p','',status || labels.empty));
      if (!status) {
        const carousel = s.mode === 'carousel';
        const gallery = node('div', carousel ? 'image-gallery-carousel' : 'image-gallery-grid');
        s.entries.forEach((entry,i) => {
          const delta = i - s.selected;
          if (carousel && Math.abs(delta) > 2) return;
          const el = tile(entry,i);
          if (carousel) {
            el.style.transform = 'translateX(' + (delta * 55) + '%) translateZ(' + (-Math.abs(delta)*100) + 'px) rotateY(' + (-Math.sign(delta)*38) + 'deg)';
            el.style.zIndex = String(10 - Math.abs(delta));
            el.style.opacity = delta === 0 ? '1' : Math.abs(delta) === 1 ? '.76' : '.48';
          }
          gallery.appendChild(el);
        });
        root.appendChild(gallery);
      }
      target.appendChild(root);
      renderFull(focus);
      if (focus && !s.full) {
        const selected = root.querySelector('[data-index="' + s.selected + '"]');
        if (selected) selected.focus();
      }
    }
    function key(event) {
      const inside = root && (root.contains(event.target) || (overlay && overlay.contains(event.target)));
      if (!inside || event.altKey || event.ctrlKey || event.metaKey) return;
      let handled = true;
      const k = event.key;
      if (k === 'Escape' || k === 'Backspace' || k === 'BrowserBack') back();
      else if (k === '1' && !s.full) { nav.toggleMode(); render(true); }
      else if (s.full && (k === 'ArrowLeft' || k === 'ArrowRight')) step(k === 'ArrowLeft' ? -1 : 1,false);
      else if (!s.full && event.target.dataset.index !== undefined &&
          ['ArrowLeft','ArrowRight','ArrowUp','ArrowDown'].includes(k)) {
        const cols = root.clientWidth <= 700 ? 2 : 4;
        nav.move(k === 'ArrowLeft' ? -1 : k === 'ArrowRight' ? 1 :
          (k === 'ArrowUp' ? -1 : 1) * (s.mode === 'grid' ? cols : 1));
        render(true);
      } else handled = false;
      if (handled) { event.preventDefault(); event.stopPropagation(); }
    }
    doc.addEventListener('keydown',key,true);
    status = labels.loading;
    render();
    Promise.resolve().then(() => adapter.listSources()).then(result => {
      if (disposed) return;
      sources = result;
      nav.setEntries(sources.map(x => ({name:x.title,directory:true})));
      status = '';
      render(true);
    }).catch(() => { if (!disposed) {status=labels.error;render();} });
    return {dispose() {
      disposed = true; ++epoch; stop(); doc.removeEventListener('keydown',key,true);
      if (overlay) overlay.remove();
      if (root) root.remove();
    }};
  }
  const api = Object.freeze({createNavigation,mount});
  global.VdrSuiteImageGallery = api;
  if (global.VdrSuitePlatform) global.VdrSuitePlatform.registerModule('image-gallery',api);
})(window);
