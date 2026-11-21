(function(global) {
  'use strict';
  const names = Object.freeze({
    'channels.view': 'Sender ansehen',
    'recordings.view': 'Aufnahmen ansehen',
    'epg.view': 'EPG ansehen',
    'timers.view': 'Aufnahmetimer ansehen',
    'timers.create': 'Einzelaufnahmen programmieren',
    'timers.modify': 'Aufnahmetimer bearbeiten',
    'timers.delete': 'Aufnahmetimer löschen',
    'searchtimers.view': 'Suchtimer ansehen',
    'searchtimers.create': 'Suchtimer anlegen',
    'searchtimers.modify': 'Suchtimer bearbeiten',
    'searchtimers.execute': 'Suchtimer aktivieren',
    'searchtimers.delete': 'Suchtimer löschen',
    'media.live.play': 'Live-TV wiedergeben',
    'media.recording.play': 'Aufnahmen wiedergeben'
  });
  function text(tag, value, className) {
    const node = document.createElement(tag);
    node.textContent = String(value === undefined || value === null ? '' : value);
    if (className) node.className = className;
    return node;
  }
  function button(value, click, className) {
    const node = text('button', value, className);
    node.type = 'button';
    node.addEventListener('click', click);
    return node;
  }
  function option(select, value, label) {
    const node = text('option', label);
    node.value = value;
    select.appendChild(node);
  }
  function errorText(error) {
    if (error && error.status === 403)
      return 'Keine Berechtigung für Geräteverwaltung. Administrator: devices.grants.view / modify.';
    if (error && error.status === 401)
      return 'Bitte als Administrator im Browser anmelden.';
    if (error && error.status === 412)
      return 'Berechtigungsstand hat sich geändert. Daten neu geladen – bitte erneut prüfen.';
    return 'Geräteverwaltung aktuell nicht verfügbar (Backend oder Verbindung prüfen).';
  }
  function isDeviceId(id) {
    return typeof id === 'string' && /^[A-Za-z0-9_-]{1,128}$/.test(id);
  }
  function isBackendId(id) {
    return typeof id === 'string' && /^[A-Za-z0-9_.-]{1,128}$/.test(id);
  }
  function render(parent) {
    const api = global.VdrSuiteDeviceAdminClientApi;
    if (!api || !parent || typeof api.listDevices !== 'function') {
      return Promise.resolve(null);
    }
    const card = document.createElement('article');
    card.className = 'module-placeholder settings-card settings-account-admin';
    // Pending pairing requests are *not* registered devices. Render them
    // independently so a Device-grants failure cannot hide admin approval.
    const pendingSection = document.createElement('section');
    pendingSection.className = 'settings-device-pairing-pending';
    pendingSection.appendChild(text('h3', 'Wartende Geräte'));
    pendingSection.appendChild(text('p',
      'Hier erscheinen offene Kopplungsanfragen. Prüfe Gerätetyp und Ablaufzeit am Fernseher. ' +
      'Der TV-Kopplungscode wird aus Sicherheitsgründen nicht an die Administrator-Liste übertragen. ' +
      'Listen aktualisieren sich automatisch alle 10 Sekunden, solange Einstellungen geöffnet sind.',
      'settings-account-admin-meta'));
    const pendingRefresh = button('Anfragen aktualisieren', function() {
      loadPending(false);
    });
    pendingSection.appendChild(pendingRefresh);
    const pendingStatus = text('p', 'Offene Kopplungen werden geladen …',
      'settings-account-admin-status');
    pendingStatus.setAttribute('role', 'status');
    pendingStatus.setAttribute('aria-live', 'polite');
    pendingSection.appendChild(pendingStatus);
    const pendingList = document.createElement('div');
    pendingList.className = 'settings-account-admin-list';
    pendingSection.appendChild(pendingList);
    card.appendChild(pendingSection);
    const pending = {busy: false, loading: false, cursor: '', items: []};

    function pendingError(error) {
      if (error && error.status === 403)
        return 'Keine Berechtigung: device.pairing.view / decide (global).';
      if (error && error.status === 401)
        return 'Bitte als Administrator anmelden.';
      if (error && (error.status === 409 || error.status === 410 ||
                    error.status === 412 || error.status === 428))
        return 'Kopplung abgelaufen, bereits entschieden oder geändert. Bitte Anfragen aktualisieren.';
      return 'Kopplungsanfragen derzeit nicht verfügbar (Server/API prüfen).';
    }
    function validPendingItem(item) {
      return item && typeof item.pairingRequestId === 'string' &&
        /^dpr_[0-9a-f]{32}$/.test(item.pairingRequestId) &&
        item.status === 'pending' &&
        typeof item.expiresAt === 'string' && item.expiresAt.length > 0 &&
        item.client && typeof item.client.displayName === 'string' &&
        item.client.displayName.length > 0 &&
        typeof item.client.clientKind === 'string' &&
        item.client.clientKind.length > 0;
    }
    function decidePending(request, decision) {
      if (pending.busy || pending.loading || !validPendingItem(request)) return;
      pending.busy = true;
      pendingStatus.textContent = 'Anfrage wird vor Freigabe erneut geprüft …';
      // Read a fresh administrator item and its server-issued strong ETag.
      // Never approve based on stale list metadata or a bootstrap token.
      api.loadPendingPairing(request.pairingRequestId).then(function(reply) {
        if (!reply || !reply.etag || !/^"[^"]{1,256}"$/.test(reply.etag) ||
            !reply.data || !validPendingItem(reply.data) ||
            reply.data.pairingRequestId !== request.pairingRequestId ||
            reply.data.client.displayName !== request.client.displayName ||
            reply.data.client.clientKind !== request.client.clientKind ||
            reply.data.expiresAt !== request.expiresAt)
          throw new Error('Pairing request changed');
        if (!global.confirm || !global.confirm(
          (decision === 'approve' ? 'Kopplung genehmigen' : 'Kopplung ablehnen') +
          '?\nGerät: ' + reply.data.client.displayName +
          '\nTyp: ' + reply.data.client.clientKind +
          '\nGültig bis: ' + reply.data.expiresAt +
          '\nNur bestätigen, wenn du die Kopplung selbst am TV gestartet hast.'))
          return null;
        return api.decidePairing(request.pairingRequestId, reply.etag, decision);
      }).then(function(result) {
        if (result) pendingStatus.textContent =
          decision === 'approve'
            ? 'Genehmigt. Jetzt auf dem TV „Status prüfen“ und „Gerät aktivieren“ wählen.'
            : 'Kopplungsanfrage abgelehnt.';
        return loadPending(false, Boolean(result));
      }).catch(function(error) {
        pendingStatus.textContent = pendingError(error);
      }).finally(function() {
        pending.busy = false;
        pendingRefresh.disabled = pending.loading;
        renderPending();
      });
    }
    function renderPending() {
      pendingList.replaceChildren();
      pending.items.forEach(function(request) {
        const row = document.createElement('div');
        row.className = 'settings-account-admin-item';
        row.appendChild(text('strong', request.client.displayName));
        row.appendChild(text('p',
          'Typ: ' + request.client.clientKind +
          ' · Gültig bis: ' + request.expiresAt,
          'settings-account-admin-meta'));
        const approve = button('Genehmigen', function() {
          decidePending(request, 'approve');
        });
        const reject = button('Ablehnen', function() {
          decidePending(request, 'reject');
        });
        approve.disabled = pending.busy || pending.loading;
        reject.disabled = pending.busy || pending.loading;
        row.appendChild(approve);
        row.appendChild(reject);
        pendingList.appendChild(row);
      });
      if (pending.cursor) {
        const more = button('Weitere Anfragen laden', function() {
          loadPending(true);
        });
        more.disabled = pending.busy || pending.loading;
        pendingList.appendChild(more);
      }
    }
    function loadPending(append, preserveStatus, silent) {
      if (pending.loading || (pending.busy && !preserveStatus)) return Promise.resolve();
      // A manual multi-page review must not be replaced by a background first-page fetch.
      if (silent && (pending.cursor || pending.items.length > 25)) return Promise.resolve();
      pending.loading = true;
      pendingRefresh.disabled = true;
      if (!preserveStatus && !silent) pendingStatus.textContent = 'Wartende Geräte werden gelesen …';
      let redraw = !silent;
      return api.listPendingPairings(append ? pending.cursor : '').then(function(result) {
        if (!result || !Array.isArray(result.items) ||
            !result.page || typeof result.page.hasMore !== 'boolean' ||
            (result.page.hasMore &&
             (typeof result.page.nextCursor !== 'string' ||
              !/^dp1_[A-Za-z0-9_-]{1,1024}$/.test(result.page.nextCursor))) ||
            !result.items.every(validPendingItem))
          throw new Error('Invalid pending pairing collection');
        const updated = append ? pending.items.concat(result.items) : result.items;
        const changed = JSON.stringify(pending.items) !== JSON.stringify(updated);
        pending.items = updated;
        pending.cursor = result.page.hasMore ? result.page.nextCursor : '';
        if (!preserveStatus && (!silent || changed)) pendingStatus.textContent = pending.items.length
          ? pending.items.length + ' offene Kopplungsanfrage(n).'
          : 'Keine wartenden Geräte.';
        redraw = changed || !silent;
      }).catch(function(error) {
        pendingStatus.textContent = pendingError(error);
        // Do not display stale actionable requests on a failed refresh.
        pending.items = [];
        pending.cursor = '';
        redraw = true;
      }).finally(function() {
        pending.loading = false;
        pendingRefresh.disabled = pending.busy;
        // Restore action availability only after pending.loading has cleared.
        if (redraw) renderPending();
      });
    }
    loadPending(false);
    card.appendChild(text('h3', 'Gekoppelte Geräte (TV & Apps)'));
    card.appendChild(text('p',
      'Unabhängig von Benutzerkonten. Nur vom Server registrierte Geräte werden angezeigt.',
      'settings-account-admin-meta'));
    const status = text('p', 'Geräte werden geladen …', 'settings-account-admin-status');
    status.setAttribute('role', 'status');
    status.setAttribute('aria-live', 'polite');
    card.appendChild(status);
    const layout = document.createElement('div');
    layout.className = 'settings-account-admin-layout';
    const list = document.createElement('div');
    list.className = 'settings-account-admin-list';
    const detail = document.createElement('div');
    detail.className = 'settings-account-admin-detail';
    layout.appendChild(list); layout.appendChild(detail);
    card.appendChild(layout);
    parent.appendChild(card);

    const state = {items: [], cursor: '', selected: '', selectedDevice: null,
      backends: [], grants: null, revision: '', loading: false, listing: false,
      showRevoked: false, serial: 0};
    function setError(error) { status.textContent = errorText(error); }
    function markSelected() {
      Array.from(list.querySelectorAll('button[data-device-id]')).forEach(function(node) {
        node.setAttribute('aria-current',
          node.dataset.deviceId === state.selected ? 'true' : 'false');
      });
    }
    function loadGrants(id) {
      const ticket = ++state.serial;
      state.grants = null; state.revision = '';
      detail.replaceChildren(text('p', 'Geräteberechtigungen werden geladen …'));
      return api.loadDevice(id).then(function(reply) {
        if (ticket !== state.serial || state.selected !== id) return;
        if (!reply || !reply.etag || !reply.data ||
            reply.data.deviceId !== id ||
            !Array.isArray(reply.data.items) ||
            !Array.isArray(reply.data.supportedPermissions))
          throw new Error('Invalid Device Grant server response');
        state.grants = reply.data; state.revision = reply.etag;
        renderDetail();
      }).catch(function(error) {
        if (ticket !== state.serial || state.selected !== id) return;
        detail.replaceChildren(text('p', errorText(error)));
      });
    }
    function act(permission, backendId, active) {
      if (!state.selected || !state.revision || state.loading ||
          !isDeviceId(state.selected) || !state.selectedDevice ||
          !state.selectedDevice.active || state.selectedDevice.revoked) return;
      if (!state.grants.supportedPermissions.includes(permission)) return;
      // Removed backends may still have historical grants that an admin must
      // be able to revoke; never allow a new grant for an unknown backend.
      const knownBackend = state.backends.some(function(x) {
        return x.backendId === backendId;
      });
      const existingGrant = state.grants.items.some(function(x) {
        return x.permission === permission && x.backendId === backendId;
      });
      if (active ? !knownBackend : !existingGrant) return;
      if (['timers.create', 'searchtimers.create'].includes(permission) &&
          backendId === '*') return;
      const target = state.selected;
      const question = (active ? 'Freigeben: ' : 'Entziehen: ') +
        (names[permission] || permission) + ' für Backend ' + backendId + '?';
      if (typeof global.confirm !== 'function' || !global.confirm(question)) return;
      state.loading = true;
      status.textContent = 'Berechtigung wird gespeichert …';
      api.setGrant(target, state.revision, permission, backendId, active)
        .then(function() {
          status.textContent = 'Berechtigung gespeichert.';
          if (state.selected === target) return loadGrants(target);
        }).catch(function(error) {
          setError(error);
          // A 412 is not a retry authorization. Reload ETag for deliberate next click.
          if (error && error.status === 412 && state.selected === target)
            return loadGrants(target);
        }).finally(function() {
          state.loading = false;
        });
    }
    function retireDevice() {
      const device = state.selectedDevice;
      if (!device || !device.active || device.revoked || !isDeviceId(device.deviceId) ||
          state.loading || state.listing) return;
      const id = device.deviceId;
      state.loading = true;
      status.textContent = 'Prüfe Gerätestatus vor dem Entfernen …';
      // Lifecycle ETag belongs to the Device, not its grant set. Always read
      // a fresh one before the explicit, irreversible revoke confirmation.
      api.loadLifecycle(id).then(function(reply) {
        if (!reply || !/^"[^"]{1,256}"$/.test(reply.etag || '') ||
            !reply.data || reply.data.deviceId !== id ||
            reply.data.active !== true || reply.data.revoked !== false)
          throw new Error('Device lifecycle changed');
        if (state.selected !== id || !global.confirm ||
            !global.confirm('Gerät wirklich entfernen (widerrufen)?' +
              '\nName: ' + (device.displayName || 'Unbenanntes Gerät') +
              '\nDevice-ID: ' + id +
              '\nAlle Anmeldungen dieses Geräts werden ungültig. ' +
              'Das lässt sich nicht rückgängig machen.'))
          return null;
        return api.revokeDevice(id, reply.etag);
      }).then(function(reply) {
        if (!reply) {
          status.textContent = 'Gerät nicht entfernt.';
          return;
        }
        if (!reply.data || reply.data.deviceId !== id ||
            reply.data.active !== false || reply.data.revoked !== true)
          throw new Error('Unexpected Device revoke response');
        state.items = state.items.map(function(item) {
          return item.deviceId === id
            ? Object.assign({}, item, {active: false, revoked: true}) : item;
        });
        if (state.selected === id) {
          ++state.serial;
          state.selected = '';
          state.selectedDevice = null;
          state.grants = null;
          state.revision = '';
          detail.replaceChildren(text('p', 'Gerät widerrufen. Andere Geräte bleiben unverändert.'));
        }
        renderList();
        status.textContent = 'Gerät widerrufen und aus der aktiven Liste ausgeblendet.';
      }).catch(function(error) {
        status.textContent = error && error.status === 412
          ? 'Gerätestatus wurde geändert. Keine Wiederholung ohne neue Prüfung.'
          : error && error.status === 403
            ? 'Entfernen nicht erlaubt: Admin-Berechtigung devices.revoke@* erforderlich.'
            : errorText(error);
      }).finally(function() {
        state.loading = false;
        // Read-only synchronization, never an automatic mutation retry.
        loadDevices(false, true);
      });
    }
    function renderDetail() {
      const item = state.selectedDevice, grant = state.grants;
      if (!item || !grant) return;
      detail.replaceChildren();
      const title = text('h4', item.displayName || 'Unbenanntes Gerät');
      detail.appendChild(title);
      detail.appendChild(text('p',
        item.revoked ? 'Widerrufen – keine Rechteänderung möglich' :
        !item.active ? 'Inaktiv – keine Rechteänderung möglich' :
        'Geräteidentität aktiv (Verbindungsstatus unbekannt)', 'settings-account-admin-meta'));
      const technical = document.createElement('details');
      technical.appendChild(text('summary', 'Gerätekennung (technisch)'));
      technical.appendChild(text('p', 'Device-ID: ' + item.deviceId));
      technical.appendChild(text('p', 'Service-Actor: ' + item.actorId));
      detail.appendChild(technical);
      if (item.active && !item.revoked) {
        const retire = button('Gerät entfernen', retireDevice);
        retire.setAttribute('aria-label', 'Gerät entfernen: ' + item.deviceId);
        detail.appendChild(retire);
        detail.appendChild(text('p',
          'Widerruft alle Zugänge dieses Geräts. Nicht mehr genutzte Geräte ' +
          'werden ausgeblendet; die Server-Historie bleibt erhalten.',
          'settings-account-admin-meta'));
      }
      detail.appendChild(text('h4', 'Vorhandene Berechtigungen'));
      if (!grant.items.length)
        detail.appendChild(text('p', 'Noch keine Freigaben für dieses Gerät.'));
      grant.items.forEach(function(entry) {
        const row = document.createElement('div');
        row.className = 'settings-account-admin-item';
        row.appendChild(text('strong', names[entry.permission] || entry.permission));
        row.appendChild(text('p', 'Backend: ' + entry.backendId,
          'settings-account-admin-meta'));
        if (item.active && !item.revoked)
          row.appendChild(button('Entziehen', function() {
            act(entry.permission, entry.backendId, false);
          }));
        detail.appendChild(row);
      });
      if (!item.active || item.revoked) return;
      detail.appendChild(text('h4', 'Berechtigung hinzufügen'));
      detail.appendChild(text('p',
        'Rechte werden nur nach ausdrücklicher Bestätigung vergeben.',
        'settings-account-admin-meta'));
      const controls = document.createElement('div');
      controls.className = 'settings-account-admin-lifecycle settings-account-admin-grant-picker';
      const permission = document.createElement('select');
      permission.setAttribute('aria-label', 'Berechtigung');
      permission.className = 'settings-account-admin-grant-permission-input';
      const allow = grant.supportedPermissions.filter(function(p) {
        return typeof p === 'string' && Object.prototype.hasOwnProperty.call(names, p);
      });
      allow.forEach(function(p) { option(permission, p, names[p]); });
      const backend = document.createElement('select');
      backend.setAttribute('aria-label', 'VDR-Backend');
      backend.className = 'settings-account-admin-grant-backend-input';
      state.backends.filter(function(b) {
        return isBackendId(b.backendId);
      }).forEach(function(b) {
        option(backend, b.backendId, b.name || b.backendId);
      });
      const save = button('Zugriff hinzufügen', function() {
        act(permission.value, backend.value, true);
      });
      save.disabled = !allow.length || !backend.options.length;
      controls.appendChild(permission); controls.appendChild(backend);
      controls.appendChild(save);
      detail.appendChild(controls);
    }
    function selectDevice(device) {
      if (!isDeviceId(device.deviceId)) return Promise.resolve();
      state.selected = device.deviceId;
      state.selectedDevice = device;
      markSelected();
      return loadGrants(device.deviceId);
    }
    function renderList() {
      list.replaceChildren();
      const revokedCount = state.items.filter(function(device) {
        return device && device.revoked;
      }).length;
      if (revokedCount) {
        const toggle = button(state.showRevoked
          ? 'Widerrufene ausblenden' : 'Widerrufene anzeigen (' + revokedCount + ')',
          function() { state.showRevoked = !state.showRevoked; renderList(); });
        list.appendChild(toggle);
      }
      state.items.forEach(function(device) {
        if (device && device.revoked && !state.showRevoked) return;
        if (!device || !isDeviceId(device.deviceId)) return;
        const entry = button(device.displayName || 'Unbenanntes Gerät',
          function() { selectDevice(device); }, 'settings-account-admin-account');
        entry.dataset.deviceId = device.deviceId;
        entry.appendChild(text('span',
          (device.revoked ? 'widerrufen' : device.active ? 'aktiv' : 'inaktiv') +
          ' · Device-ID: ' + device.deviceId,
          'settings-account-admin-meta'));
        list.appendChild(entry);
      });
      if (state.cursor) {
        const more = button('Weitere Geräte laden', function() { loadDevices(true); });
        more.disabled = state.listing || state.loading;
        list.appendChild(more);
      }
      markSelected();
    }
    function loadDevices(append, silent) {
      if (state.listing || state.loading) return Promise.resolve();
      // Preserve a deliberately extended paginated list; silent first-page
      // replacement would discard pages and the administrator's selection.
      if (silent && state.items.length > 25) return Promise.resolve();
      state.listing = true;
      const cursor = append ? state.cursor : '';
      if (!silent) status.textContent = 'Gekoppelte Geräte werden gelesen …';
      return api.listDevices(cursor).then(function(result) {
        if (!result || !Array.isArray(result.items) ||
            !result.page || typeof result.page.hasMore !== 'boolean' ||
            (result.page.hasMore && !/^dv1_[A-Za-z0-9_.-]{1,128}$/.test(
              result.page.nextCursor || '')))
          throw new Error('Invalid Device collection response');
        const valid = result.items.every(function(d) {
          return d && isDeviceId(d.deviceId) &&
            typeof d.displayName === 'string' &&
            typeof d.active === 'boolean' && typeof d.revoked === 'boolean';
        });
        if (!valid) throw new Error('Invalid Device item');
        const updated = append ? state.items.concat(result.items) : result.items;
        const changed = JSON.stringify(state.items) !== JSON.stringify(updated);
        const oldSelected = state.selectedDevice;
        state.items = updated;
        state.cursor = result.page.hasMore ? result.page.nextCursor : '';
        if (changed || !silent) renderList();
        if (!silent || changed) status.textContent = state.items.length
          ? '' : 'Noch kein gekoppeltes Gerät vorhanden.';
        const selected = state.items.find(function(d) {
          return d.deviceId === state.selected;
        });
        if (selected) {
          state.selectedDevice = selected;
          if (oldSelected &&
              (oldSelected.active !== selected.active ||
               oldSelected.revoked !== selected.revoked)) return loadGrants(selected.deviceId);
          if (changed && oldSelected && oldSelected.displayName !== selected.displayName &&
              state.grants) renderDetail();
        } else if (state.selected) {
          state.serial += 1; // discard any in-flight obsolete grant result
          state.selected = '';
          state.selectedDevice = null;
          state.grants = null;
          state.revision = '';
          detail.replaceChildren(text('p', 'Gerät nicht mehr in der Liste.'));
        }
        if (!state.selected && state.items.length) {
          const firstVisible = state.items.find(function(d) {
            return !d.revoked || state.showRevoked;
          });
          if (firstVisible) return selectDevice(firstVisible);
        }
      }).catch(setError).finally(function() {
        state.listing = false;
        if (!silent) renderList();
      });
    }

    // One short-lived refresh owner per Settings card. A card removed by the
    // Settings renderer is never polled or rescheduled, even on tab changes.
    // No global listeners, background requests, navigation or page reload.
    const AUTO_REFRESH_MS = 10000;
    let refreshTimer = null;
    function scheduleRefresh() {
      if (refreshTimer !== null || typeof global.setTimeout !== 'function') return;
      refreshTimer = global.setTimeout(refreshVisibleLists, AUTO_REFRESH_MS);
    }
    function refreshVisibleLists() {
      refreshTimer = null;
      if (card.isConnected === false) return;
      if (document.visibilityState === 'hidden' ||
          pending.busy || pending.loading || state.loading || state.listing) {
        scheduleRefresh();
        return;
      }
      Promise.all([loadPending(false, false, true), loadDevices(false, true)])
        .then(scheduleRefresh, scheduleRefresh);
    }
    // Listing identities must remain useful if backend discovery itself is
    // unavailable; the grant button remains disabled without exact scopes.
    return Promise.resolve().then(function() {
      return api.listBackends();
    }).then(function(items) {
      state.backends = Array.isArray(items) ? items : [];
    }).catch(function() {
      state.backends = [];
    }).then(function() {
      return loadDevices(false);
    }).then(function() {
      scheduleRefresh();
      return card;
    });
  }
  global.VdrSuiteDeviceAdminSettings = Object.freeze({render: render});
})(window);
