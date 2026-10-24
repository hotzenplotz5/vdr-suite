(function (global) {
  'use strict';

  const path = '/api/v1/addons/rectools/activation';

  function node(tag, text) {
    const result = global.document.createElement(tag);
    if (typeof text === 'string') result.textContent = text;
    return result;
  }

  function request(backendId, mutation, revision, enabled) {
    const session = global.VdrSuiteBrowserSession;
    const restore = session && typeof session.restore === 'function'
      ? Promise.resolve(session.restore()) : Promise.resolve();
    return restore.then(function () {
      const csrf = session && typeof session.csrfHeaders === 'function'
        ? session.csrfHeaders() : {};
      const url = mutation ? path : path + '?backendId=' + encodeURIComponent(backendId);
      const options = {
        credentials: 'same-origin', cache: 'no-store',
        headers: Object.assign({'Accept': 'application/json'}, mutation
          ? Object.assign({'Content-Type': 'application/json'}, csrf || {})
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

  function render(parent, backendId) {
    const card = node('article');
    // Never flash an administrator-only panel before the protected read.
    card.hidden = true;
    card.className = 'module-placeholder settings-card settings-addons-card';
    card.appendChild(node('h3', 'Media Tools'));
    card.appendChild(node('p',
      'Import und HEVC-Shrink benötigen einen geprüften Backend-Agent, ' +
      'Administrator-Freigabe und getrennte Benutzerberechtigungen.'));
    const status = node('p', 'Aktivierung wird geladen …');
    status.setAttribute('role', 'status');
    status.setAttribute('aria-live', 'polite');
    const details = node('p', '');
    const refresh = node('button', 'Status aktualisieren');
    const toggle = node('button', 'Aktivieren');
    refresh.type = 'button';
    toggle.type = 'button';
    toggle.disabled = true;
    card.appendChild(status);
    card.appendChild(details);
    card.appendChild(refresh);
    card.appendChild(toggle);
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
      toggle.disabled = busy || !current;
    }
    function reload() {
      busy = true; current = null; controls();
      status.textContent = 'Aktivierung wird geprüft …';
      return request(backendId, false).then(function (result) {
        current = result;
        card.hidden = false;
        status.textContent = result.desiredEnabled
          ? 'Administratorfreigabe eingeschaltet (Laufzeitstatus gesondert prüfen)'
          : 'Administratorfreigabe ausgeschaltet';
        details.textContent = 'Backend: ' + backendId +
          ' · Freigabe-Revision: ' + result.revision;
        toggle.textContent = result.desiredEnabled ? 'Deaktivieren' : 'Aktivieren';
      }).catch(function (error) {
        details.textContent = '';
        if (error.httpStatus === 403) {
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
    toggle.addEventListener('click', function () {
      if (!current || busy) return;
      const wanted = !current.desiredEnabled;
      const question = wanted
        ? 'Media Tools für Backend ' + backendId +
          ' aktivieren? Nur zuvor geprüfte Handler und vorhandene Benutzerrechte dürfen Aufträge ausführen.'
        : 'Media Tools für Backend ' + backendId + ' deaktivieren?';
      if (!global.confirm(question)) return;
      const expectedRevision = current.revision;
      busy = true; controls();
      status.textContent = 'Administratorfreigabe wird gespeichert …';
      request(backendId, true, expectedRevision, wanted).then(function () {
        return reload();
      }).catch(function (error) {
        status.textContent = 'Änderung abgelehnt: ' + error.message;
        return reload();
      }).finally(function () { busy = false; controls(); });
    });
    reload();
    return card;
  }

  global.VdrSuiteAddonSettings = Object.freeze({render: render});
})(window);
