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
      .image-gallery [hidden]{display:none!important}
      .image-gallery-grid{display:flex;flex-wrap:wrap;gap:.55rem}
      .image-gallery-tile{position:relative;min-width:0;overflow:hidden;padding:0;aspect-ratio:4/3;flex:1 1 calc(25% - .55rem);max-width:100%}
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
    let preferences = {};
    try { preferences = JSON.parse(global.localStorage.getItem('vdr-suite.image-gallery.slideshow') || '{}') || {}; } catch (_) {}
    const interval = Math.max(1000, Math.min(60000, Number(settings.intervalMs || preferences.intervalMs) || 5000));
    const repeat = settings.repeat === undefined ? preferences.repeat === true : settings.repeat === true;
    let bar, gallery, message, more, fullImage, fullInfo, slideButton, fullPath, preload;
    let renderedEntries, renderedMode, renderedStatus, renderedDirectory, renderedSource;
    let gridLimit = 80;
    const tiles = new Map();
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
      if (!s.full && !nav.open()) {
        const first = s.entries.findIndex(entry => !entry.directory);
        if (first < 0) return;
        s.selected = first;
        if (!nav.open()) return;
      }
      timer = global.setInterval(() => {
        if (!step(1, repeat)) { stop(); render(); }
      }, interval);
      render(true);
    }
    function closePhoto() { stop(); nav.close(); render(true); }
    function choose(index) {
      if (status) return;
      if (s.mode === 'carousel' && index !== s.selected) {
        s.selected = index;
        render(true);
        return;
      }
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
      ++epoch;
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
      el.addEventListener('focus', () => {
        if (s.mode === 'grid' && s.selected !== index) {
          s.selected = index;
          updateTiles(false);
        }
      });
      if (source && entry.cover) {
        const image = node('img');
        image.alt = '';
        image.loading = 'lazy';
        image.decoding = 'async';
        image.addEventListener('error', () => {
          image.hidden = true;
          el.insertBefore(node('span','image-gallery-placeholder',entry.directory ? '▰' : '▧'),image);
        }, {once:true});
        image.src = adapter.previewUrl(source.id, entry.cover);
        el.appendChild(image);
      } else {
        el.appendChild(node('span','image-gallery-placeholder', entry.directory ? '▰' : '▧'));
      }
      if (entry.directory) el.appendChild(node('span','image-gallery-folder',entry.name));
      return el;
    }
    function swipe(el, action) {
      let x = null, y = null;
      el.addEventListener('touchstart', event => {
        if (event.touches.length !== 1) { x = null; return; }
        x = event.touches[0].clientX; y = event.touches[0].clientY;
      }, {passive:true});
      el.addEventListener('touchend', event => {
        if (x === null || !event.changedTouches.length) return;
        const dx = event.changedTouches[0].clientX - x;
        const dy = event.changedTouches[0].clientY - y;
        x = null;
        if (Math.abs(dx) > 48 && Math.abs(dx) > Math.abs(dy) * 1.5) {
          action(dx < 0 ? 1 : -1);
          if (event.cancelable) event.preventDefault();
        }
      }, {passive:false});
    }
    function closeOverlay() {
      if (overlay) overlay.remove();
      overlay = null; fullPath = null; fullImage = null; preload = null;
    }
    function renderFull(focus) {
      if (!s.full || !source) { closeOverlay(); return; }
      const entry = s.entries[s.selected];
      const created = !overlay;
      if (created) {
        overlay = node('section','image-gallery image-gallery-full');
        overlay.setAttribute('role','dialog');
        overlay.setAttribute('aria-modal','true');
        fullImage = node('img');
        fullInfo = node('div','image-gallery-info');
        fullImage.addEventListener('load', () => updateInfo());
        const controls = node('div','image-gallery-full-controls');
        slideButton = button(labels.play,toggleSlide);
        controls.append(
          button(labels.previous, () => step(-1,false)),
          button(labels.next, () => step(1,false)),
          slideButton,
          button(labels.rotate, () => { nav.rotate(); renderFull(false); }),
          button(labels.zoom, () => { nav.zoom(); renderFull(false); }),
          button(labels.info, () => { s.info = !s.info; renderFull(false); }),
          button(labels.close,closePhoto));
        overlay.append(fullImage,fullInfo,controls);
        swipe(fullImage, delta => step(delta,false));
        doc.body.appendChild(overlay);
      }
      const url = adapter.originalUrl(source.id,entry.relativePath);
      if (url !== fullPath) {
        fullPath = url;
        fullImage.alt = entry.name;
        fullImage.src = url;
        const next = s.entries.slice(s.selected + 1).find(x => !x.directory) ||
          (repeat ? s.entries.find(x => !x.directory) : null);
        preload = null;
        if (next && next !== entry) {
          preload = node('img');
          preload.src = adapter.originalUrl(source.id,next.relativePath);
        }
      }
      const turned = s.rotation % 180 !== 0;
      fullImage.style.maxWidth = turned ? '100vh' : '100vw';
      fullImage.style.maxHeight = turned ? '100vw' : '100vh';
      fullImage.style.transform = 'rotate(' + s.rotation + 'deg) scale(' + s.zoom + ')';
      slideButton.textContent = timer === null ? labels.play : labels.pause;
      updateInfo();
      if (created || focus) overlay.querySelector('button').focus();
      function updateInfo() {
        if (!fullImage || !fullInfo) return;
        fullInfo.hidden = !s.info;
        fullInfo.textContent = entry.name + (fullImage.naturalWidth ?
          ' · ' + fullImage.naturalWidth + ' × ' + fullImage.naturalHeight : '');
      }
    }
    function updateTiles(focus) {
      if (!gallery || status) return;
      const carousel = s.mode === 'carousel';
      if (!carousel && s.selected >= gridLimit) gridLimit = Math.ceil((s.selected+1)/80)*80;
      const first = carousel ? Math.max(0,s.selected-3) : 0;
      const last = Math.min(s.entries.length,carousel ? s.selected+4 : gridLimit);
      for (const [i,el] of tiles) {
        if (i < first || i >= last) { el.remove(); tiles.delete(i); }
      }
      for (let i=first;i<last;i++) {
        let el=tiles.get(i);
        if (!el) {
          el=tile(s.entries[i],i);
          const next=tiles.get(i+1);
          gallery.insertBefore(el,next || null);
          tiles.set(i,el);
        }
        const delta=i-s.selected;
        el.classList.toggle('is-selected',delta===0);
        el.tabIndex=delta===0 ? 0 : -1;
        if (carousel) {
          el.style.transform='translateX('+(delta*55)+'%) translateZ('+(-Math.abs(delta)*100)+'px) rotateY('+(-Math.sign(delta)*38)+'deg)';
          el.style.zIndex=String(10-Math.abs(delta));
          el.style.opacity=delta===0?'1':Math.abs(delta)===1?'.76':Math.abs(delta)===2?'.48':'0';
          el.style.pointerEvents=Math.abs(delta)>2?'none':'auto';
          el.setAttribute('aria-hidden',Math.abs(delta)>2?'true':'false');
        }
      }
      more.hidden=carousel || last>=s.entries.length;
      if (focus && !s.full && tiles.has(s.selected)) tiles.get(s.selected).focus();
    }
    function render(focus) {
      if (disposed) return;
      if (!root) {
        root=node('section','image-gallery');
        bar=node('div','image-gallery-bar');
        message=node('p');
        gallery=node('div');
        more=button(settings.locale==='en'?'More pictures':'Weitere Bilder',()=>{
          gridLimit+=80;updateTiles(false);
        });
        root.append(bar,message,gallery,more);
        swipe(gallery,delta=>{if(s.mode==='carousel'&&!status){nav.move(delta);updateTiles(true);}});
        target.appendChild(root);
      }
      if (renderedEntries!==s.entries || renderedMode!==s.mode || renderedStatus!==status ||
          renderedDirectory!==directory || renderedSource!==source) {
        if (renderedEntries!==s.entries) gridLimit=80;
        renderedEntries=s.entries;renderedMode=s.mode;renderedStatus=status;
        renderedDirectory=directory;renderedSource=source;
        bar.replaceChildren();
        if (parents.length) bar.appendChild(button(labels.back,back));
        bar.appendChild(node('span','',source?source.title+(directory?' / '+directory:''):labels.sources));
        bar.appendChild(button(s.mode==='grid'?labels.carousel:labels.grid,()=>{nav.toggleMode();render(true);}));
        if (source&&!status&&s.entries.some(x=>!x.directory)) bar.appendChild(button(labels.play,toggleSlide));
        message.textContent=status||(!s.entries.length?labels.empty:'');
        message.hidden=!message.textContent;
        gallery.replaceChildren();tiles.clear();
        gallery.className=s.mode==='carousel'?'image-gallery-carousel':'image-gallery-grid';
        gallery.hidden=!!status;more.hidden=true;
      }
      updateTiles(focus);
      renderFull(focus);
      if (focus&&!s.full&&!tiles.has(s.selected)) {
        const fallback=bar.querySelector('button');
        if(fallback) fallback.focus();
      }
    }
    function key(event) {
      const inside = root && (root.contains(event.target) || (overlay && overlay.contains(event.target)));
      if (!inside || event.altKey || event.ctrlKey || event.metaKey) return;
      let handled = true;
      const k = event.key;
      if ((k === 'Escape' || k === 'Backspace' || k === 'BrowserBack') && (s.full || parents.length)) back();
      else if (k === '1' && !s.full) { nav.toggleMode(); render(true); }
      else if (s.full && (k === 'ArrowLeft' || k === 'ArrowRight')) step(k === 'ArrowLeft' ? -1 : 1,false);
      else if (s.full && (k === 'Info' || k === 'i')) { s.info=!s.info;renderFull(false); }
      else if (s.full && (k === 'MediaPlayPause' || k === 'p')) toggleSlide();
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
      closeOverlay();
      if (root) root.remove();
    }};
  }
  let client;
  function publicClient() {
    if (!client) client = global.VdrSuitePublicV1Client.createClient({
      baseUrl:global.location && global.location.origin || '',
      credentials:'same-origin'
    });
    return client;
  }
  function library(backendId) {
    return publicClient().getImageGalleryLibrary({backendId,cache:'no-store'});
  }
  function adapterFor(backendId) {
    const api = publicClient();
    const options = (sourceId,path) => ({backendId,sourceId,path,cache:'no-store'});
    return {
      listSources:() => library(backendId).then(data => data.sources || []),
      browse:(sourceId,path) => api.getImageGalleryFolder(options(sourceId,path)).then(data => data.entries),
      previewUrl:(sourceId,path) => api.imageGalleryPreviewUrl(options(sourceId,path)),
      originalUrl:(sourceId,path) => api.imageGalleryPhotoUrl(options(sourceId,path))
    };
  }
  const api = Object.freeze({createNavigation,mount,library,adapterFor});
  global.VdrSuiteImageGallery = api;
  if (global.VdrSuitePlatform) global.VdrSuitePlatform.registerModule('image-gallery',api);
})(window);
