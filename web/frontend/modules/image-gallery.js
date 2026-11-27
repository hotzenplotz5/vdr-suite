(function (global) {
  'use strict';

  // Approved artwork from modules/image/assets/gallery-camera.webp.
  // Embed the small WebP in this already served optional module so the
  // installed daemon route inventory and PWA asset composition stay unchanged.
  const galleryCamera = 'data:image/webp;base64,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';
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
    let photos = [], positions = new Map();
    function resetPhoto() { state.rotation = 0; state.zoom = 1; state.info = false; }
    function setEntries(entries, selection) {
      state.entries = entries.slice();
      photos = []; positions = new Map();
      state.entries.forEach((entry,index) => {
        if (!entry.directory) { positions.set(index,photos.length); photos.push(index); }
      });
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
      const at = positions.get(state.selected);
      if (at === undefined) return false;
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
      .image-gallery-pages{display:flex;align-items:center;justify-content:center;gap:1rem;padding:.7rem}
      .image-gallery-pages button{padding:.45rem .8rem}
      .image-gallery-pages button:disabled{opacity:.45;cursor:default}
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
      .image-gallery-home-rail .image-gallery-home-card{flex:0 0 clamp(9rem,20vw,16rem);min-width:0;aspect-ratio:4/3;overflow:hidden;position:relative;padding:0;border-radius:.55rem}
      .image-gallery-home-rail .image-gallery-home-card img{width:100%;height:100%;object-fit:cover;display:block}
      /* Other Home owners use flex order 40–60 on the detail shell.
         The gallery is a separate final sibling, not part of their reorderable rails. */
      #detail:has(.module-tab.active[data-module="overview"]) > [data-home-zone="image-gallery"]{order:100;min-width:0}
      .image-gallery-home-rail .image-gallery-home-link{display:grid;grid-template-rows:minmax(0,1fr) auto;align-items:center;justify-items:center;gap:.2rem;padding:.35rem .45rem .5rem;text-align:center;background:linear-gradient(145deg,#173a52,#0b2033);color:#f1f5f9;border:1px solid #3983a5}
      .image-gallery-home-rail .image-gallery-home-link .image-gallery-home-icon{width:100%;height:100%;min-height:0;object-fit:contain}
      .image-gallery-home-link span{font-weight:700;line-height:1.2;font-size:clamp(.85rem,1.4vw,1.1rem)}
      .image-gallery-home-rail{display:flex;gap:.6rem;overflow-x:auto;padding:.25rem 1rem .65rem;scroll-snap-type:x proximity}
      .image-gallery-home-card{scroll-snap-align:start}
      .image-gallery-home-card:focus-visible{outline:2px solid #40e2f1;outline-offset:2px}
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
    let bar, gallery, message, pages, previousPage, nextPage, pageInfo, fullImage, fullInfo, slideButton, fullPath, preload;
    let renderedEntries, renderedMode, renderedStatus, renderedDirectory, renderedSource;
    const gridPageSize = 80; // divisible by both TV (4) and mobile (2) columns
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
    async function loadFolder(selection, openPhoto = false) {
      const request = ++epoch;
      status = labels.loading;
      render();
      try {
        const entries = await adapter.browse(source.id, directory);
        if (disposed || request !== epoch) return;
        const index = typeof selection === 'string' ? entries.findIndex(x => x.relativePath === selection) : selection;
        nav.setEntries(entries, Math.max(0,index || 0));
        if (openPhoto && index >= 0) nav.open();
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
        fullImage.addEventListener('load', () => {
          if (!fullImage || !fullInfo || !s.full) return;
          const current = s.entries[s.selected];
          fullInfo.textContent = current.name + (fullImage.naturalWidth ?
            ' · ' + fullImage.naturalWidth + ' × ' + fullImage.naturalHeight : '');
        });
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
      const first = carousel ? Math.max(0,s.selected-3) : Math.floor(s.selected/gridPageSize)*gridPageSize;
      const last = Math.min(s.entries.length,carousel ? s.selected+4 : first+gridPageSize);
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
      pages.hidden=carousel || s.entries.length<=gridPageSize;
      previousPage.disabled=first===0;
      nextPage.disabled=last>=s.entries.length;
      pageInfo.textContent=(first+1)+'–'+last+' / '+s.entries.length;
      if (focus && !s.full && tiles.has(s.selected)) tiles.get(s.selected).focus();
    }
    function render(focus) {
      if (disposed) return;
      if (!root) {
        root=node('section','image-gallery');
        bar=node('div','image-gallery-bar');
        message=node('p');
        gallery=node('div');
        pages=node('div','image-gallery-pages');
        previousPage=button(settings.locale==='en'?'Previous pictures':'Vorherige Bilder',()=>{
          nav.move(-gridPageSize);updateTiles(true);
        });
        nextPage=button(settings.locale==='en'?'More pictures':'Weitere Bilder',()=>{
          nav.move(gridPageSize);updateTiles(true);
        });
        pageInfo=node('span');
        pages.append(previousPage,pageInfo,nextPage);
        root.append(bar,message,gallery,pages);
        swipe(gallery,delta=>{if(s.mode==='carousel'&&!status){nav.move(delta);updateTiles(true);}});
        target.appendChild(root);
      }
      if (renderedEntries!==s.entries || renderedMode!==s.mode || renderedStatus!==status ||
          renderedDirectory!==directory || renderedSource!==source) {
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
        gallery.hidden=!!status;pages.hidden=true;
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
      else if (s.full && (k === 'MediaPlayPause' || k === 'p' || k === 'ColorF1Green')) toggleSlide();
      else if (!s.full && source && !status && k === 'ColorF1Green') toggleSlide();
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
      const initial = settings.initialPhoto;
      const sourceIndex = initial && sources.findIndex(x => x.id === initial.sourceId);
      if (initial && sourceIndex >= 0 && typeof initial.relativePath === 'string') {
        source = sources[sourceIndex];
        const parts = initial.relativePath.split('/');
        parts.pop();
        parents = [{source:null,directory:'',selected:sourceIndex}];
        directory = '';
        for (const part of parts) {
          const child = directory ? directory+'/'+part : part;
          parents.push({source,directory,selected:child});
          directory = child;
        }
        loadFolder(initial.relativePath,true);
      } else render(true);
    }).catch(() => { if (!disposed) {status=labels.error;render();} });
    return {dispose() {
      disposed = true; ++epoch; stop(); doc.removeEventListener('keydown',key,true);
      closeOverlay();
      if (root) root.remove();
    }};
  }
  // Independent Home projection: one sample per Home visit; no timers.
  function mountHome(host,adapter,options) {
    const doc = host.ownerDocument, en = options.locale === 'en';
    let disposed = false;
    installStyles(doc);
    const heading = doc.createElement('div');
    heading.className = 'media-home-section-heading';
    const title = doc.createElement('h3');
    title.textContent = en ? 'Discover pictures' : 'Bilder entdecken';
    heading.appendChild(title);
    const rail = doc.createElement('div');
    rail.className = 'media-home-discovery-rail image-gallery-home-rail';
    function card(label,photo) {
      const button = doc.createElement('button');
      button.type = 'button';
      button.className = 'media-home-discovery-card image-gallery-home-card';
      button.setAttribute('aria-label',label);
      button.title = label;
      button.addEventListener('click',()=>{if (!disposed) options.open(photo);});
      return button;
    }
    const shortcut = card(en ? 'Open gallery and folders' : 'Galerie und Ordner öffnen',null);
    shortcut.className += ' image-gallery-home-link';
    const icon = doc.createElement('img');
    icon.className = 'image-gallery-home-icon';
    icon.alt = '';
    icon.setAttribute('aria-hidden','true');
    icon.src = galleryCamera;
    const label = doc.createElement('span');
    label.textContent = en ? 'My pictures' : 'Meine Bilder';
    shortcut.append(icon,label);
    rail.appendChild(shortcut);
    host.replaceChildren(heading,rail);
    host.hidden = false;
    rail.addEventListener('keydown',event=>{
      if (!['ArrowLeft','ArrowRight'].includes(event.key)) return;
      const cards = Array.from(rail.children);
      const at = cards.indexOf(event.target);
      const next = cards[at+(event.key==='ArrowLeft'?-1:1)];
      if (at < 0 || !next) return;
      event.preventDefault();event.stopPropagation();next.focus();
    });
    Promise.resolve().then(()=>adapter.sample()).then(photos=>{
      if (disposed) return;
      const seen = new Set();
      for (const photo of photos || []) {
        if (!photo || typeof photo.sourceId !== 'string' || typeof photo.relativePath !== 'string') continue;
        const key = JSON.stringify([photo.sourceId,photo.relativePath]);
        if (seen.has(key)) continue;
        seen.add(key);
        const button = card(photo.name || photo.relativePath,photo);
        const img = doc.createElement('img');
        img.alt = photo.name || photo.relativePath;
        img.loading = 'lazy';img.decoding = 'async';
        img.src = adapter.previewUrl(photo.sourceId,photo.relativePath);
        button.appendChild(img);
        rail.appendChild(button);
        if (seen.size === 9) break;
      }
    }).catch(()=>{}); // The folder shortcut remains usable when discovery fails.
    return {dispose(){disposed=true;host.hidden=true;host.replaceChildren();}};
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
      sample:() => api.getImageGalleryLibrary({backendId,cache:'no-store',sample:true}).then(data=>data.sample || []),
      browse:(sourceId,path) => api.getImageGalleryFolder(options(sourceId,path)).then(data => data.entries),
      previewUrl:(sourceId,path) => api.imageGalleryPreviewUrl(options(sourceId,path)),
      originalUrl:(sourceId,path) => api.imageGalleryPhotoUrl(options(sourceId,path))
    };
  }
  const api = Object.freeze({createNavigation,mount,mountHome,library,adapterFor});
  global.VdrSuiteImageGallery = api;
  if (global.VdrSuitePlatform) global.VdrSuitePlatform.registerModule('image-gallery',api);
})(window);
