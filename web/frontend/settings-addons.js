(function (global) {
  'use strict';

  const path = '/api/v1/addons/rectools/activation';

  // Use the same bounded request correlation as the proven administrator API
  // mutation. This is diagnostic identity, never an authorization credential.
  function mutationCorrelationHeader() {
    if (!global.crypto || typeof global.crypto.getRandomValues !== 'function')
      return {};
    const bytes = new Uint8Array(12);
    global.crypto.getRandomValues(bytes);
    return {'X-Correlation-ID': 'addon-activation-' +
      Array.from(bytes, function (byte) {
        return byte.toString(16).padStart(2, '0');
      }).join('')};
  }

  function node(tag, text) {
    const result = global.document.createElement(tag);
    if (typeof text === 'string') result.textContent = text;
    return result;
  }

  function request(backendId, mutation, revision, enabled, moduleId) {
    const session = global.VdrSuiteBrowserSession;
    const restore = session && typeof session.restore === 'function'
      ? Promise.resolve(session.restore()) : Promise.resolve();
    return restore.then(function () {
      const csrf = session && typeof session.csrfHeaders === 'function'
        ? session.csrfHeaders() : {};
      const endpoint = moduleId === 'image' ? '/api/v1/addons/image/activation' : path;
      const url = mutation ? endpoint : endpoint + '?backendId=' + encodeURIComponent(backendId);
      const options = {
        credentials: 'same-origin', cache: 'no-store',
        headers: Object.assign({'Accept': 'application/json'}, mutation
          ? Object.assign({'Content-Type': 'application/json'},
              csrf || {}, mutationCorrelationHeader())
          : {})
      };
      if (mutation) {
        options.method = 'POST';
        options.body = JSON.stringify({
          backendId: backendId,
          desiredEnabled: enabled,
          expectedRevision: revision
        });
      }
      return global.fetch(url, options).then(function (response) {
        return response.text().then(function (text) {
          let body;
          try { body = text ? JSON.parse(text) : {}; }
          catch (error) { throw new Error('Ungültige Server-Antwort'); }
          if (!response.ok) {
            const message = body && typeof body.error === 'string'
              ? body.error : 'HTTP ' + response.status;
            const failure = new Error(message + ' (' + response.status + ')');
            failure.httpStatus = response.status;
            throw failure;
          }
          if (!body || body.backendId !== backendId ||
              typeof body.desiredEnabled !== 'boolean' ||
              !Number.isSafeInteger(body.revision) || body.revision < 0) {
            throw new Error('Ungültiger Aktivierungsstatus');
          }
          return body;
        });
      });
    });
  }

  // Tell the existing shell/Home owner when administrator intent changes.
  // This is a local UI notification, never an execution authorization.
  function notifyActivationChanged(backendId) {
    if (typeof global.dispatchEvent === 'function' &&
        typeof global.CustomEvent === 'function') {
      global.dispatchEvent(new global.CustomEvent(
        'vdr-suite:media-tools-activation-changed',
        {detail: {backendId: backendId}}));
    }
  }

  // One existing administrator login; one GET and one guarded POST.
  // The server owns the root file and rejects nonlocal/invalid directories.
  const sourcesPath = '/api/v1/addons/rectools/sources';
  function sourcePath(value) {
    if (typeof value !== 'string' || value.length < 2 ||
        value.length > 512 || !value.startsWith('/') ||
        value.endsWith('/') || /[\\\x00-\x1f\x7f]/.test(value)) return false;
    return value.slice(1).split('/').every(function (part) {
      return part && part !== '.' && part !== '..';
    });
  }
  function sourceName(value) {
    return typeof value === 'string' && value.length > 0 &&
      value.length <= 64 &&
      !/[/\\"\x00-\x1f\x7f]/.test(value);
  }
  function overlap(a, b) {
    return a === b || a.startsWith(b + '/') || b.startsWith(a + '/');
  }
  function sourceRequest(backendId, revision, sources, firstVideoRoot) {
    const session = global.VdrSuiteBrowserSession;
    const restoring = session && typeof session.restore === 'function'
      ? Promise.resolve(session.restore()) : Promise.resolve();
    const mutation = Array.isArray(sources);
    return restoring.then(function () {
      const headers = {'Accept': 'application/json'};
      const options = {credentials: 'same-origin', cache: 'no-store', headers};
      const url = mutation
        ? sourcesPath : sourcesPath + '?backendId=' + encodeURIComponent(backendId);
      if (mutation) {
        options.method = 'POST';
        headers['Content-Type'] = 'application/json';
        Object.assign(headers, session && typeof session.csrfHeaders === 'function'
          ? session.csrfHeaders() : {});
        const payload = {
          backendId, revision,
          entries: encodeURIComponent(sources.map(function (item) {
            return item.path + '\t' + item.name;
          }).join('\n'))
        };
        // Only an initially missing, not VDR-discovered recording root is
        // supplied by the administrator. Existing destinations are immutable.
        if (firstVideoRoot) payload.videoRoot = encodeURIComponent(firstVideoRoot);
        options.body = JSON.stringify(payload);
      }
      return global.fetch(url, options).then(function (response) {
        return response.text().then(function (text) {
          let data;
          try { data = JSON.parse(text); }
          catch (_) { throw new Error('Ungültige Serverantwort'); }
          if (!response.ok) {
            const code = data && data.error;
            const err = new Error(code === 'sources_changed'
              ? 'Die Einstellungen wurden zwischenzeitlich geändert. Neu laden.'
              : code === 'backend_is_not_local'
                ? 'Dieses Backend liegt auf einem anderen Server.'
                : code === 'import_in_progress'
                  ? 'Gerade läuft ein Import. Bitte danach erneut speichern.'
                  : code === 'video_root_unavailable'
                    ? 'Aufnahmeziel des lokalen VDR noch nicht ermittelt.'
                  : code === 'invalid_video_root'
                    ? 'VDR-Aufnahmeverzeichnis existiert nicht oder ist ungültig.'
                  : code === 'video_root_already_known'
                    ? 'VDR-Aufnahmeziel wurde inzwischen ermittelt. Bitte neu laden.'
                  : code === 'invalid_sources' || code === 'overlapping_roots'
                    ? 'Ordner ungültig, fehlen oder überschneiden sich.'
                    : 'Importverzeichnisse können nicht gespeichert oder geladen werden.');
            err.status = response.status;
            throw err;
          }
          if (!data || data.backendId !== backendId ||
              typeof data.videoRoot !== 'string' ||
              !/^[0-9a-f]{16}$/.test(data.revision || '') ||
              !Array.isArray(data.sources) ||
              (data.sources.length === 0 && data.initialSetup !== true) ||
              data.sources.length > 17 ||
              !data.sources.every(function (item) {
                return item && sourcePath(item.path) && sourceName(item.name);
              })) throw new Error('Ungültige Importverzeichnis-Antwort');
          return data;
        });
      });
    });
  }

  function appendSourceEditor(card, backendId) {
    const section = node('section');
    section.hidden = true;
    section.className = 'settings-media-import-source-editor';
    section.appendChild(node('h4', 'Importverzeichnisse'));
    section.appendChild(node('p',
      'Importordner hinzufügen, benennen oder entfernen. ' +
      'Der oberste Eintrag ist das Standardverzeichnis.'));
    const destination = node('p', '');
    const firstRootLabel = node('label', 'VDR-Aufnahmeverzeichnis (einmalig)');
    firstRootLabel.className = 'settings-media-import-source-row';
    const firstRootInput = node('input');
    firstRootInput.type = 'text';
    firstRootInput.placeholder = 'Tatsächliches VDR-Aufnahmeverzeichnis';
    firstRootLabel.appendChild(firstRootInput);
    firstRootLabel.hidden = true;
    const rowsContainer = node('div');
    rowsContainer.className = 'settings-media-import-source-list';
    const add = node('button', '+ Importordner hinzufügen');
    const save = node('button', 'Speichern');
    const status = node('p', 'Importverzeichnisse werden geladen …');
    status.setAttribute('role', 'status');
    add.type = 'button';
    save.type = 'button';
    section.appendChild(destination);
    section.appendChild(firstRootLabel);
    section.appendChild(rowsContainer);
    section.appendChild(add);
    section.appendChild(save);
    section.appendChild(status);
    card.appendChild(section);

    let state = null;
    let rows = [];
    let busy = false;

    function refreshControls() {
      add.disabled = busy || !state || rows.length >= 17;
      const target = state && (state.videoRoot ||
        (state.initialSetup === true ? firstRootInput.value : ''));
      save.disabled = busy || !state || !sourcePath(target) ||
        rows.length === 0;
    }
    firstRootInput.addEventListener('input', function () {
      status.textContent = 'Noch nicht gespeichert.';
      refreshControls();
    });
    function renderRows() {
      rowsContainer.replaceChildren();
      rows.forEach(function (entry, index) {
        const row = node('div');
        row.className = 'settings-media-import-source-row';
        const labelPath = node('label', 'Verzeichnis');
        const inputPath = node('input');
        inputPath.type = 'text';
        inputPath.value = entry.path;
        labelPath.appendChild(inputPath);
        const labelName = node('label', 'Anzeigename');
        const inputName = node('input');
        inputName.type = 'text';
        inputName.value = entry.name;
        labelName.appendChild(inputName);
        inputPath.addEventListener('input', function () {
          entry.path = inputPath.value;
          status.textContent = 'Noch nicht gespeichert.';
        });
        inputName.addEventListener('input', function () {
          entry.name = inputName.value;
          status.textContent = 'Noch nicht gespeichert.';
        });
        row.appendChild(labelPath);
        row.appendChild(labelName);
        if (index === 0) row.appendChild(node('strong', 'Standard'));
        else {
          const makeDefault = node('button', 'Als Standard festlegen');
          makeDefault.type = 'button';
          makeDefault.addEventListener('click', function () {
            rows.splice(index, 1);
            rows.unshift(entry);
            status.textContent = 'Noch nicht gespeichert.';
            renderRows();
          });
          row.appendChild(makeDefault);
        }
        if (rows.length > 1) {
          const remove = node('button', 'Entfernen');
          remove.type = 'button';
          remove.addEventListener('click', function () {
            rows.splice(index, 1);
            status.textContent = 'Noch nicht gespeichert.';
            renderRows();
          });
          row.appendChild(remove);
        }
        rowsContainer.appendChild(row);
      });
      refreshControls();
    }
    function load() {
      busy = true;
      refreshControls();
      return sourceRequest(backendId).then(function (value) {
        state = value;
        rows = value.sources.map(function (item) {
          return {path: item.path, name: item.name};
        });
        destination.textContent = value.videoRoot
          ? 'Aufnahmeziel: ' + value.videoRoot + ' (bleibt unverändert)'
          : 'VDR-Aufnahmeverzeichnis konnte nicht automatisch ermittelt werden. ' +
            'Bitte für die Ersteinrichtung das tatsächliche Aufnahmeziel eintragen.';
        firstRootLabel.hidden = !(value.initialSetup === true && !value.videoRoot);
        firstRootInput.value = '';
        section.hidden = false;
        renderRows();
        status.textContent = value.initialSetup === true
          ? 'Noch keine Importverzeichnisse eingerichtet. ' +
            'Bitte einen Importordner hinzufügen und speichern.'
          : 'Einstellungen geladen.';
      }).catch(function (error) {
        state = null;
        // A failed backend configuration read is visible to an already
        // authorized administrator; 401/403 must still hide the panel.
        section.hidden = error.status === 401 || error.status === 403;
        status.textContent = 'Quellen konnten nicht geladen werden: ' +
          error.message;
      }).finally(function () {
        busy = false;
        refreshControls();
      });
    }

    add.addEventListener('click', function () {
      if (busy || !state || rows.length >= 17) return;
      rows.push({path: '', name: ''});
      renderRows();
      status.textContent = 'Neuen Ordner eintragen und speichern.';
    });
    save.addEventListener('click', function () {
      if (busy || !state || rows.length === 0 || rows.length > 17) return;
      const videoRoot = state.videoRoot ||
        (state.initialSetup === true ? firstRootInput.value : '');
      if (!sourcePath(videoRoot)) {
        status.textContent = 'Bitte das tatsächliche VDR-Aufnahmeverzeichnis angeben.';
        return;
      }
      for (let i = 0; i < rows.length; ++i) {
        if (!sourcePath(rows[i].path) || !sourceName(rows[i].name) ||
            overlap(rows[i].path, videoRoot) ||
            rows.some(function (other, j) {
              return j !== i && overlap(rows[i].path, other.path);
            })) {
          status.textContent = 'Bitte gültige, getrennte Verzeichnisse und Namen eingeben.';
          return;
        }
      }
      busy = true;
      refreshControls();
      status.textContent = 'Importverzeichnisse werden gespeichert …';
      sourceRequest(backendId, state.revision, rows,
        state.videoRoot ? '' : videoRoot).then(function (result) {
        state = result;
        rows = result.sources.map(function (item) {
          return {path: item.path, name: item.name};
        });
        firstRootLabel.hidden = true;
        firstRootInput.value = '';
        destination.textContent = 'Aufnahmeziel: ' + result.videoRoot +
          ' (bleibt unverändert)';
        renderRows();
        status.textContent = 'Gespeichert. Der Backend-Agent übernimmt die ' +
          'Änderung beim nächsten Scan; kein Neustart erforderlich.';
      }).catch(function (error) {
        status.textContent = 'Nicht gespeichert: ' + error.message;
      }).finally(function () {
        busy = false;
        refreshControls();
      });
    });
    return {section: section, load: load};
  }

  function render(parent, backendId) {
    const card = node('article');
    // Never flash an administrator-only panel before the protected read.
    card.hidden = true;
    card.className = 'module-placeholder settings-card settings-addons-card';
    card.appendChild(node('h3', 'Media Tools'));
    card.appendChild(node('p',
      'Media Tools werden separat installiert. Diese Anzeige prüft ' +
      'den gespeicherten Aktivierungszustand. Die Verfügbarkeit der ' +
      'lokalen Worker wird bei Import- und Shrink-Aufträgen geprüft.'));
    const status = node('p', 'Aktivierung wird geladen …');
    status.setAttribute('role', 'status');
    status.setAttribute('aria-live', 'polite');
    const details = node('p', '');
    const refresh = node('button', 'Status aktualisieren');
    const toggle = node('button', 'Aktivieren');
    const renew = node('button', 'Agent-Freigabe erneuern');
    refresh.type = 'button';
    toggle.type = 'button';
    toggle.disabled = true;
    renew.type = 'button';
    renew.hidden = true;
    renew.disabled = true;
    card.appendChild(status);
    card.appendChild(details);
    card.appendChild(refresh);
    card.appendChild(toggle);
    card.appendChild(renew);
    const sourceEditor = appendSourceEditor(card, backendId);
    parent.appendChild(card);
    if (typeof backendId !== 'string' ||
        !/^[a-zA-Z0-9_.:-]{1,128}$/.test(backendId) ||
        backendId === '.' || backendId === '..') {
      card.hidden = false;
      status.textContent = 'Zuerst ein gültiges Backend auswählen.';
      refresh.disabled = true;
      return card;
    }

    let current = null;
    let busy = false;
    function controls() {
      refresh.disabled = busy;
      // The admin enable/disable preference must remain usable while the
      // worker is offline. Actual imports validate worker readiness later.
      const absent = current && current.installationState === 'absent';
      toggle.hidden = Boolean(absent);
      toggle.disabled = busy || !current || absent;
      toggle.title = '';
      const canRenew = current && !absent &&
        current.desiredEnabled === true &&
        current.providerVerified === true && current.agentReady === true &&
        current.activationBoundToProvider === false;
      renew.hidden = !canRenew;
      renew.disabled = busy || !canRenew;
    }
    function reload() {
      busy = true; current = null; controls();
      status.textContent = 'Aktivierung wird geprüft …';
      return request(backendId, false).then(function (result) {
        current = result;
        card.hidden = false;
        // Local file absence is a real package observation, unlike a
        // disconnected Agent. Unknown (remote/partial) is not "not installed".
        const absent = result.installationState === 'absent';
        if (!absent) sourceEditor.load();
        else sourceEditor.section.hidden = true;
        // A current, independently verified provider proves more than a
        // saved intent, but still says nothing about dpkg package ownership.
        const trusted = result.providerVerified === true;
        const ready = result.agentReady === true;
        const bound = result.activationBoundToProvider === true;
        status.textContent = absent ? 'Media Tools nicht installiert'
          : result.desiredEnabled
          ? (trusted && ready && bound
            ? 'Media Tools aktiviert und betriebsbereit'
            : (trusted && ready && !bound
              ? 'Media Tools aktiviert; Agent-Freigabe ist veraltet'
              : 'Media Tools aktiviert; Worker-Verfügbarkeit wird beim Auftrag geprüft'))
          : 'Media Tools deaktiviert';
        details.textContent = 'Backend: ' + backendId +
          ' · Freigabe-Revision: ' + result.revision +
          (result.installationState === 'present'
            ? ' · Add-on-Dateien vorhanden'
            : result.installationState === 'unknown'
              ? ' · Paketstatus nicht bestätigt' : '');
        toggle.textContent = result.desiredEnabled ? 'Deaktivieren' : 'Aktivieren';
      }).catch(function (error) {
        details.textContent = '';
        sourceEditor.section.hidden = true;
        if (error.httpStatus === 401 || error.httpStatus === 403) {
          // No role.admin grant: no visible administration controls.
          card.hidden = true;
          status.textContent = '';
          return;
        }
        card.hidden = false;
        status.textContent = 'Aktivierung nicht verfügbar: ' + error.message;
      }).finally(function () {
        busy = false; controls();
      });
    }

    refresh.addEventListener('click', reload);
    // An Agent reconnect changes its generation. Rebind an already-enabled
    // intent through the existing audited, CSRF-protected CAS endpoint.
    // Never disable Media Tools or mutate a provider binding from the browser.
    renew.addEventListener('click', function () {
      if (busy || !current || current.desiredEnabled !== true ||
          current.providerVerified !== true || current.agentReady !== true ||
          current.activationBoundToProvider !== false) return;
      if (!global.confirm('Media-Tools-Freigabe für den aktuellen Agenten auf ' +
          backendId + ' erneuern?')) return;
      const expectedRevision = current.revision;
      busy = true; controls();
      status.textContent = 'Aktuelle Agent-Freigabe wird geprüft und erneuert …';
      request(backendId, true, expectedRevision, true).then(function () {
        return reload().then(function () {
          notifyActivationChanged(backendId);
        });
      }).catch(function (error) {
        return reload().then(function () {
          if (!card.hidden)
            status.textContent = 'Freigabe konnte nicht erneuert werden: ' +
              error.message;
        });
      }).finally(function () { busy = false; controls(); });
    });
    toggle.addEventListener('click', function () {
      if (!current || busy || current.installationState === 'absent') return;
      const wanted = !current.desiredEnabled;
      const question = wanted
        ? 'Media Tools für Backend ' + backendId + ' aktivieren?'
        : 'Media Tools für Backend ' + backendId + ' deaktivieren?';
      if (!global.confirm(question)) return;
      const expectedRevision = current.revision;
      busy = true; controls();
      status.textContent = 'Administratorfreigabe wird gespeichert …';
      request(backendId, true, expectedRevision, wanted).then(function () {
        return reload().then(function () {
          notifyActivationChanged(backendId);
        });
      }).catch(function (error) {
        // Refresh the authoritative revision, then preserve the failure
        // message. Previously reload() replaced it immediately and left
        // operators with a misleading "deaktiviert" message.
        return reload().then(function () {
          if (!card.hidden) {
            status.textContent = 'Änderung abgelehnt: ' + error.message;
          }
        });
      }).finally(function () { busy = false; controls(); });
    });
    reload();
    return card;
  }

  function imageSourceRequest(backendId, sources) {
    const session = global.VdrSuiteBrowserSession;
    const mutation = Array.isArray(sources);
    const endpoint = '/api/v1/addons/image/sources';
    return Promise.resolve(session && typeof session.restore === 'function' ? session.restore() : null)
      .then(function () {
        const options = {credentials:'same-origin',cache:'no-store',method:mutation?'POST':'GET',
          headers:{Accept:'application/json'}};
        if (mutation) {
          options.headers = Object.assign(options.headers,{'Content-Type':'application/json'},
            session && typeof session.csrfHeaders === 'function' ? session.csrfHeaders() : {},
            mutationCorrelationHeader());
          options.body = JSON.stringify({backendId:backendId,entries:encodeURIComponent(
            sources.map(x => [x.id,x.title,x.root].join('\t')).join('\n'))});
        }
        return global.fetch(mutation ? endpoint : endpoint+'?backendId='+encodeURIComponent(backendId),options);
      }).then(async function (result) {
        if (!result.ok) { const error = new Error('HTTP '+result.status); error.httpStatus=result.status; throw error; }
        const body = JSON.parse(await result.text());
        if (body.backendId !== backendId || !Array.isArray(body.sources)) throw new Error('Invalid source response');
        return body.sources;
      });
  }

  function renderImageGallery(parent, backendId) {
    const en = global.document.documentElement.lang === 'en';
    const word = (de,english) => en ? english : de;
    const card = node('article');
    card.className = 'module-placeholder settings-card settings-addons-card';
    card.hidden = true;
    const status = node('p',word('Wird geladen …','Loading …'));
    status.setAttribute('role','status');
    const toggle = node('button',word('Aktivieren','Enable'));
    const rows = node('div');
    rows.className = 'settings-media-import-source-list';
    const add = node('button',word('Bildquelle hinzufügen','Add picture source'));
    const save = node('button',word('Bildquellen speichern','Save picture sources'));
    [toggle,add,save].forEach(x => x.type = 'button');
    card.appendChild(node('h3','Image Gallery'));
    card.appendChild(status);
    card.appendChild(toggle);
    card.appendChild(node('h4',word('Bildquellen','Picture sources')));
    card.appendChild(rows);
    card.appendChild(add);
    card.appendChild(save);
    card.appendChild(node('h4',word('Diashow auf diesem Gerät','Slideshow on this device')));
    const intervalLabel=node('label',word('Intervall (Sekunden)','Interval (seconds)'));
    const interval=node('input');interval.type='number';interval.min='1';interval.max='60';
    intervalLabel.appendChild(interval);card.appendChild(intervalLabel);
    const repeatLabel=node('label',word('Wiederholen','Repeat'));
    const repeat=node('input');repeat.type='checkbox';repeatLabel.appendChild(repeat);card.appendChild(repeatLabel);
    let preferences={};
    try { preferences=JSON.parse(global.localStorage.getItem('vdr-suite.image-gallery.slideshow')||'{}')||{}; } catch (_) {}
    interval.value=String(Math.max(1,Math.min(60,Number(preferences.intervalMs)/1000||5)));
    repeat.checked=preferences.repeat===true;
    const savePlayback=()=>{try {global.localStorage.setItem('vdr-suite.image-gallery.slideshow',JSON.stringify({
      intervalMs:Math.max(1,Math.min(60,Number(interval.value)||5))*1000,repeat:repeat.checked
    }));} catch (_) {}};
    interval.addEventListener('change',savePlayback);repeat.addEventListener('change',savePlayback);
    parent.appendChild(card);
    let current = null, busy = true, nextId = 0;
    const model = [];
    function controls() { [toggle,add,save].forEach(x => x.disabled = busy || !current); }
    function row(source) {
      const item = node('div'); item.className='settings-media-import-source-row';
      const title = node('input'); title.value=source.title; title.placeholder=word('Name','Name');
      title.setAttribute('aria-label',word('Name der Bildquelle','Picture source name'));
      const root = node('input'); root.value=source.root; root.placeholder='/srv/picture';
      root.setAttribute('aria-label',word('Bildverzeichnis','Picture directory'));
      const remove = node('button',word('Entfernen','Remove')); remove.type='button';
      const data = {id:source.id,title,root,item};
      remove.addEventListener('click',() => { if (!busy) {model.splice(model.indexOf(data),1);item.remove();} });
      item.appendChild(title);item.appendChild(root);item.appendChild(remove);
      model.push(data);rows.appendChild(item);
    }
    function notify() {
      if (typeof global.dispatchEvent === 'function' && typeof global.CustomEvent === 'function')
        global.dispatchEvent(new global.CustomEvent('vdr-suite:image-gallery-activation-changed',{detail:{backendId}}));
    }
    function describe() {
      toggle.textContent=current.desiredEnabled ? word('Deaktivieren','Disable') : word('Aktivieren','Enable');
      status.textContent=current.desiredEnabled ? word('Aktiviert · lokale Bilder','Enabled · local pictures') : word('Deaktiviert','Disabled');
    }
    add.addEventListener('click',() => {
      if (busy) return;
      let id;
      do {id='picture-'+Date.now().toString(36)+'-'+(++nextId).toString(36);} while(model.some(x=>x.id===id));
      row({id,title:'',root:''});
    });
    save.addEventListener('click',() => {
      if (busy || !current) return;
      const entries=model.map(x=>({id:x.id,title:x.title.value.trim(),root:x.root.value.trim()}));
      busy=true;controls();
      imageSourceRequest(backendId,entries).then(() => {
        status.textContent=word('Bildquellen gespeichert','Picture sources saved');notify();
      }).catch(error=>{status.textContent=word('Speichern fehlgeschlagen: ','Save failed: ')+error.message;})
        .finally(()=>{busy=false;controls();});
    });
    toggle.addEventListener('click',() => {
      if (busy || !current) return;
      busy=true;controls();
      request(backendId,true,current.revision,!current.desiredEnabled,'image').then(result=>{
        current=result;describe();notify();
      }).catch(error=>{status.textContent=word('Änderung fehlgeschlagen: ','Change failed: ')+error.message;})
        .finally(()=>{busy=false;controls();});
    });
    controls();
    Promise.all([request(backendId,false,undefined,undefined,'image'),imageSourceRequest(backendId)])
      .then(([activation,sources])=>{
        current=activation;sources.forEach(row);card.hidden=false;describe();
      }).catch(error=>{
        if ([401,403,404].includes(error.httpStatus)) return;
        card.hidden=false;status.textContent=word('Bildquellen nicht verfügbar: ','Picture sources unavailable: ')+error.message;
      }).finally(()=>{busy=false;controls();});
    return card;
  }

  // Read-only observation for the shared Home/nav owner. The server keeps
  // this admin-scoped; callers without the grant must handle its denial.
  function readActivationState(backendId) {
    return request(backendId, false);
  }
  global.VdrSuiteAddonSettings = Object.freeze({
    render: render,
    renderImageGallery: renderImageGallery,
    readActivationState: readActivationState
  });
})(window);
