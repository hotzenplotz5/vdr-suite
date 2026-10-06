(function(global) {
  'use strict';

  function t(key, fallback) {
    const i18n = global.VdrSuiteI18n;
    return i18n && typeof i18n.t === 'function'
      ? i18n.t(key, {}, fallback) : fallback;
  }

  function addText(element, value) {
    element.textContent = value === undefined || value === null ? '' : String(value);
    return element;
  }

  function statusText(item) {
    if (item && item.revoked) return t('settings.accountAdminRevoked', 'widerrufen');
    if (item && item.expired) return t('settings.accountAdminExpired', 'abgelaufen');
    return item && item.active
      ? t('settings.accountAdminActive', 'aktiv')
      : t('settings.accountAdminInactive', 'inaktiv');
  }

  function permissionLabel(permission, options) {
    const items = Array.isArray(options) ? options : [];
    const match = items.find(function(option) {
      return option && option.permission === permission;
    });
    const presentationKey = match &&
      typeof match.presentationKey === 'string' &&
      match.presentationKey ? match.presentationKey : permission;
    if (!presentationKey) return permission || '-';
    return t(
      'settings.accountAdminPermissionLabel.' + presentationKey,
      permission || '-');
  }

  function scopeLabel(backendId, backends) {
    if (backendId === '*') {
      return t('settings.accountAdminScopeGlobal', 'Alle Backends / serverweit');
    }
    const items = Array.isArray(backends) ? backends : [];
    const match = items.find(function(backend) {
      return backend && backend.backendId === backendId;
    });
    if (match && match.name) return match.name + ' (' + backendId + ')';
    return backendId || '-';
  }

  function appendHint(parent, text) {
    const hint = addText(document.createElement('p'), text);
    hint.className = 'settings-account-admin-meta settings-account-admin-hint';
    parent.appendChild(hint);
    return hint;
  }

  function appendTechnicalDetails(parent, rows) {
    const details = document.createElement('details');
    details.className = 'settings-account-admin-technical';
    const summary = addText(document.createElement('summary'),
      t('settings.accountAdminTechnicalDetails', 'Technische Details'));
    details.appendChild(summary);
    rows.forEach(function(row) {
      appendMeta(details, row[0], row[1]);
    });
    parent.appendChild(details);
    return details;
  }

  function appendGuideStep(parent, title, detail, complete) {
    const item = document.createElement('div');
    item.className = 'settings-account-admin-guide-step' +
      (complete ? ' complete' : ' pending');
    const marker = addText(document.createElement('span'), complete ? '✓' : '!');
    marker.className = 'settings-account-admin-guide-marker';
    const copy = document.createElement('div');
    copy.appendChild(addText(document.createElement('strong'), title));
    copy.appendChild(addText(document.createElement('span'), detail));
    item.appendChild(marker);
    item.appendChild(copy);
    parent.appendChild(item);
    return item;
  }

  function appendOption(select, value, label, disabled) {
    const option = addText(document.createElement('option'), label);
    option.value = value;
    option.disabled = Boolean(disabled);
    select.appendChild(option);
    return option;
  }

  function errorText(error) {
    if (error && error.status === 401) {
      return t('settings.accountAdminUnauthenticated',
        'Für die Benutzerverwaltung ist eine Browser-Anmeldung erforderlich.');
    }
    if (error && error.status === 403) {
      return t('settings.accountAdminForbidden',
        'Für die Benutzerverwaltung fehlt die globale Administrationsberechtigung.');
    }
    return t('settings.accountAdminUnavailable',
      'Benutzerverwaltung ist derzeit nicht verfügbar.');
  }

  function createAccountErrorText(error) {
    const code = error && typeof error.code === 'string' ? error.code : '';
    if (error && error.status === 409 && code === 'idempotency_conflict') {
      return t('settings.accountAdminCreateIdempotencyConflict',
        'Die Kontoanlage wurde mit widersprüchlichen Daten wiederholt. Bitte erneut auslösen.');
    }
    if (error && error.status === 409) {
      return t('settings.accountAdminCreateOperationConflict',
        'Der Loginname ist bereits vergeben oder die Kontoanlage steht im Konflikt.');
    }
    if (error && (error.status === 400 || error.status === 415)) {
      return t('settings.accountAdminCreateInvalidRequest',
        'Die Kontoanlage-Anfrage ist ungültig.');
    }
    if (error && error.status === 422) {
      return t('settings.accountAdminCreateInvalidFields',
        'Loginname, Anzeigename oder initiales Passwort sind ungültig.');
    }
    if (error && error.status === 503) {
      return t('settings.accountAdminCreateUnavailable',
        'Kontoanlage ist wegen einer vorübergehend nicht verfügbaren Sicherheitskomponente nicht möglich.');
    }
    return errorText(error);
  }

  function createAccountIdempotencyKey() {
    const cryptoApi = global.crypto;
    if (!cryptoApi || typeof cryptoApi.randomUUID !== 'function') return '';
    return 'mu9f-' + cryptoApi.randomUUID();
  }

  function mutationErrorText(error) {
    if (error && error.status === 409) {
      return t('settings.accountAdminFinalAdministrator',
        'Der letzte nutzbare Administrator kann nicht deaktiviert werden.');
    }
    if (error && error.status === 412) {
      return t('settings.accountAdminRevisionConflict',
        'Das Konto wurde zwischenzeitlich geändert. Die aktuellen Daten wurden neu geladen.');
    }
    if (error && error.status === 428) {
      return t('settings.accountAdminRevisionRequired',
        'Für diese Änderung fehlt ein aktueller Konto-Stand.');
    }
    return errorText(error);
  }

  function grantMutationErrorText(error) {
    if (error && error.status === 409) {
      return t('settings.accountAdminGrantFinalAdministrator',
        'Die Administratorberechtigung kann nicht entzogen werden, weil sonst kein nutzbarer Administrator übrig bliebe.');
    }
    if (error && error.status === 412) {
      return t('settings.accountAdminGrantRevisionConflict',
        'Die Berechtigungen wurden zwischenzeitlich geändert. Die aktuellen Daten wurden neu geladen.');
    }
    if (error && error.status === 422) {
      return t('settings.accountAdminGrantUnsupported',
        'Diese Berechtigungs-/Scope-Kombination wird vom Server nicht unterstützt.');
    }
    if (error && error.status === 428) {
      return t('settings.accountAdminGrantRevisionRequired',
        'Für diese Änderung fehlt ein aktueller Berechtigungsstand.');
    }
    return errorText(error);
  }

  function credentialMutationErrorText(error) {
    if (error && error.status === 409) {
      return t('settings.accountAdminCredentialFinalAdministrator',
        'Diese Anmeldedaten können nicht widerrufen werden, weil sonst kein nutzbarer Administrator übrig bliebe.');
    }
    if (error && error.status === 412) {
      return t('settings.accountAdminCredentialRevisionConflict',
        'Die Anmeldedaten wurden zwischenzeitlich geändert. Die aktuellen Daten wurden neu geladen.');
    }
    if (error && error.status === 428) {
      return t('settings.accountAdminCredentialRevisionRequired',
        'Für diesen Widerruf fehlt ein aktueller Stand der Anmeldedaten.');
    }
    return errorText(error);
  }

  function sessionMutationErrorText(error) {
    if (error && error.status === 412) {
      return t('settings.accountAdminSessionRevisionConflict',
        'Die Sitzung wurde zwischenzeitlich geändert. Die aktuellen Daten wurden neu geladen.');
    }
    if (error && error.status === 428) {
      return t('settings.accountAdminSessionRevisionRequired',
        'Für diesen Widerruf fehlt ein aktueller Sitzungsstand.');
    }
    return errorText(error);
  }

  function appendMeta(parent, label, value) {
    const row = document.createElement('div');
    row.className = 'settings-line';
    const name = addText(document.createElement('span'), label);
    name.className = 'settings-label';
    const content = addText(document.createElement('span'), value || '-');
    content.className = 'settings-value';
    row.appendChild(name);
    row.appendChild(content);
    parent.appendChild(row);
  }

  function section(title) {
    const container = document.createElement('section');
    container.className = 'settings-account-admin-section';
    container.appendChild(addText(document.createElement('h4'), title));
    return container;
  }

  function renderItems(parent, items, renderItem) {
    const list = document.createElement('div');
    list.className = 'settings-account-admin-collection';
    if (!Array.isArray(items) || items.length === 0) {
      const empty = addText(document.createElement('p'),
        t('settings.accountAdminEmpty', 'Keine Einträge.'));
      empty.className = 'settings-account-admin-meta';
      list.appendChild(empty);
      parent.appendChild(list);
      return;
    }
    items.forEach(function(item) {
      const entry = document.createElement('article');
      entry.className = 'settings-account-admin-item';
      renderItem(entry, item || {});
      list.appendChild(entry);
    });
    parent.appendChild(list);
  }

  function renderDetail(parent, overview, actions) {
    parent.replaceChildren();
    if (!overview || !overview.account) {
      parent.appendChild(addText(document.createElement('p'),
        t('settings.accountAdminSelect', 'Konto auswählen')));
      return;
    }

    const account = overview.account;
    const backends = actions && Array.isArray(actions.backends)
      ? actions.backends : [];
    const activeCredentials = Array.isArray(overview.credentials)
      ? overview.credentials.filter(function(item) {
          return item && item.active === true && !item.revoked && !item.expired;
        }) : [];
    const activeSessions = Array.isArray(overview.sessions)
      ? overview.sessions.filter(function(item) {
          return item && item.active === true && !item.revoked && !item.expired;
        }) : [];
    const activeGrants = Array.isArray(overview.grants) ? overview.grants : [];
    const permissionOptions =
      Array.isArray(overview.supportedPermissionOptions) &&
      overview.supportedPermissionOptions.length
        ? overview.supportedPermissionOptions
        : (Array.isArray(overview.supportedPermissions)
            ? overview.supportedPermissions.map(function(permission) {
                return {
                  permission: permission,
                  presentationKey: permission,
                  category: 'permission'
                };
              })
            : []);

    const guide = section(t('settings.accountAdminSetupTitle', 'Einrichtung auf einen Blick'));
    guide.className += ' settings-account-admin-guide';
    appendGuideStep(
      guide,
      t('settings.accountAdminSetupAccount', '1. Konto'),
      account.active
        ? t('settings.accountAdminSetupAccountReady', 'Aktiv und für Anmeldungen freigegeben.')
        : t('settings.accountAdminSetupAccountPending', 'Noch inaktiv. Aktiviere das Konto für die Anmeldung.'),
      account.active === true);
    appendGuideStep(
      guide,
      t('settings.accountAdminSetupAccess', '2. Zugriff'),
      activeGrants.length
        ? t('settings.accountAdminSetupAccessReady', 'Mindestens ein Zugriff ist zugewiesen.')
        : t('settings.accountAdminSetupAccessPending', 'Noch keine Rolle oder Backend-Berechtigung zugewiesen.'),
      activeGrants.length > 0);
    appendGuideStep(
      guide,
      t('settings.accountAdminSetupLogin', '3. Anmeldung'),
      activeCredentials.length
        ? t('settings.accountAdminSetupLoginReady', 'Aktive Anmeldedaten vorhanden.')
        : t('settings.accountAdminSetupLoginPending', 'Keine aktiven Anmeldedaten vorhanden.'),
      activeCredentials.length > 0);
    appendGuideStep(
      guide,
      t('settings.accountAdminSetupDevices', '4. Geräte'),
      activeSessions.length
        ? t('settings.accountAdminSetupDevicesReady', 'Mindestens ein Browser/Gerät ist angemeldet.')
        : t('settings.accountAdminSetupDevicesPending', 'Noch kein Browser/Gerät angemeldet.'),
      activeSessions.length > 0);
    parent.appendChild(guide);

    const identity = section(t('settings.accountAdminIdentity', 'Konto & Status'));
    identity.appendChild(addText(document.createElement('h3'),
      account.displayName || account.accountId));
    appendMeta(identity, t('settings.accountAdminStatus', 'Status'), statusText(account));
    appendHint(identity, t('settings.accountAdminIdentityHint',
      'Hier änderst du den Anzeigenamen und aktivierst oder deaktivierst die Anmeldung für dieses Konto.'));
    appendTechnicalDetails(identity, [
      ['Account-ID', account.accountId],
      ['Actor-ID', account.actorId]
    ]);

    if (actions) {
      const controls = document.createElement('div');
      controls.className = 'settings-account-admin-lifecycle';

      const displayLabel = addText(document.createElement('label'),
        t('settings.accountAdminDisplayName', 'Anzeigename'));
      displayLabel.className = 'settings-account-admin-field';

      const displayInput = document.createElement('input');
      displayInput.type = 'text';
      displayInput.className = 'settings-account-admin-display-name-input';
      displayInput.value = account.displayName || '';
      displayInput.disabled = Boolean(actions.busy);
      displayLabel.appendChild(displayInput);
      controls.appendChild(displayLabel);

      const save = addText(document.createElement('button'),
        t('settings.accountAdminSaveDisplayName', 'Anzeigename speichern'));
      save.type = 'button';
      save.className = 'settings-account-admin-save-name';
      save.disabled = Boolean(actions.busy);
      save.addEventListener('click', function() {
        return actions.rename(displayInput.value);
      });
      controls.appendChild(save);

      const toggle = addText(document.createElement('button'), account.active
        ? t('settings.accountAdminDeactivate', 'Konto deaktivieren')
        : t('settings.accountAdminActivate', 'Konto aktivieren'));
      toggle.type = 'button';
      toggle.className = 'settings-account-admin-toggle-active';
      toggle.disabled = Boolean(actions.busy);
      toggle.addEventListener('click', function() {
        if (account.active && typeof global.confirm === 'function') {
          const confirmed = global.confirm(t('settings.accountAdminDeactivateConfirm',
            'Konto deaktivieren? Aktive Browsersitzungen dieses Kontos werden widerrufen.'));
          if (!confirmed) return Promise.resolve(false);
        }
        return account.active ? actions.deactivate() : actions.activate();
      });
      controls.appendChild(toggle);

      identity.appendChild(controls);
    }
    parent.appendChild(identity);

    const grants = section(t('settings.accountAdminGrants', 'Zugriff & Rolle'));
    appendHint(grants, t('settings.accountAdminGrantServerPolicy',
      'Wähle aus, was dieser Benutzer darf und wo es gilt. Die verfügbaren Optionen kommen direkt vom Server.'));

    if (!activeGrants.length) {
      const warning = addText(document.createElement('p'),
        t('settings.accountAdminNoAccessWarning',
          'Dieser Benutzer hat noch keinen Zugriff. Weise mindestens eine Rolle oder Berechtigung zu.'));
      warning.className = 'settings-account-admin-callout';
      grants.appendChild(warning);
    }

    if (actions) {
      const scopeKinds = Array.isArray(overview.supportedScopeKinds)
        ? overview.supportedScopeKinds : [];
      const grantControls = document.createElement('div');
      grantControls.className = 'settings-account-admin-lifecycle settings-account-admin-grant-picker';

      if (permissionOptions.length && scopeKinds.length) {
        const permissionField = addText(document.createElement('label'),
          t('settings.accountAdminGrantPermission', 'Rolle / Berechtigung'));
        permissionField.className = 'settings-account-admin-field';
        const permissionInput = document.createElement('select');
        permissionInput.className =
          'settings-account-admin-display-name-input settings-account-admin-grant-permission-input';
        permissionInput.disabled = Boolean(actions.busy);
        appendOption(permissionInput, '',
          t('settings.accountAdminChoosePermission', 'Bitte auswählen …'), true);
        permissionOptions.forEach(function(option) {
          if (!option || !option.permission) return;
          const label = permissionLabel(option.permission, permissionOptions);
          appendOption(
            permissionInput,
            option.permission,
            label === option.permission
              ? label
              : label + ' — ' + option.permission,
            false);
        });
        permissionInput.value = '';
        permissionField.appendChild(permissionInput);
        grantControls.appendChild(permissionField);

        const backendField = addText(document.createElement('label'),
          t('settings.accountAdminGrantBackend', 'Gültig für'));
        backendField.className = 'settings-account-admin-field';
        const backendInput = document.createElement('select');
        backendInput.className =
          'settings-account-admin-display-name-input settings-account-admin-grant-backend-input';
        backendInput.disabled = Boolean(actions.busy);

        if (scopeKinds.indexOf('global') !== -1) {
          appendOption(
            backendInput,
            '*',
            t('settings.accountAdminScopeGlobal', 'Alle Backends / serverweit'),
            false);
        }
        if (scopeKinds.indexOf('backend') !== -1) {
          backends.forEach(function(backend) {
            if (!backend || !backend.backendId) return;
            appendOption(
              backendInput,
              backend.backendId,
              backend.name
                ? backend.name + ' (' + backend.backendId + ')'
                : backend.backendId,
              false);
          });
        }

        const ensure = addText(document.createElement('button'),
          t('settings.accountAdminEnsureGrant', 'Zugriff hinzufügen'));
        ensure.type = 'button';
        ensure.className = 'settings-account-admin-grant-ensure';
        ensure.disabled = Boolean(actions.busy);
        ensure.addEventListener('click', function() {
          return actions.ensureGrant(permissionInput.value, backendInput.value);
        });
        grantControls.appendChild(backendField);
        backendField.appendChild(backendInput);
        grantControls.appendChild(ensure);
      } else {
        appendHint(grantControls, t('settings.accountAdminGrantOptionsUnavailable',
          'Der Server liefert derzeit keine auswählbaren Zugriffstypen. Bestehende Zugriffe können weiterhin angezeigt oder entzogen werden.'));
      }
      grants.appendChild(grantControls);
    }

    renderItems(grants, overview.grants, function(entry, grant) {
      entry.appendChild(addText(document.createElement('strong'),
        permissionLabel(grant.permission, permissionOptions)));
      appendMeta(entry, t('settings.accountAdminGrantBackend', 'Gültig für'),
        scopeLabel(grant.backendId, backends));
      appendTechnicalDetails(entry, [
        [t('settings.accountAdminGrantPermissionCode', 'Permission-Code'), grant.permission],
        ['Scope', grant.backendId]
      ]);

      if (actions && grant.permission && grant.backendId) {
        const revoke = addText(document.createElement('button'),
          t('settings.accountAdminRevokeGrant', 'Zugriff entziehen'));
        revoke.type = 'button';
        revoke.className = 'settings-account-admin-grant-revoke';
        revoke.disabled = Boolean(actions.busy);
        revoke.addEventListener('click', function() {
          if (typeof global.confirm === 'function') {
            const confirmed = global.confirm(t('settings.accountAdminRevokeGrantConfirm',
              'Diesen Zugriff entziehen?'));
            if (!confirmed) return Promise.resolve(false);
          }
          return actions.revokeGrant(grant.permission, grant.backendId);
        });
        entry.appendChild(revoke);
      }
    });
    parent.appendChild(grants);

    const credentials = section(t('settings.accountAdminCredentials', 'Anmeldung & Passwort'));
    appendHint(credentials, t('settings.accountAdminCredentialsHint',
      'Hier siehst du die aktiven Anmeldedaten. Ein Widerruf beendet auch damit ausgestellte Browser-Sitzungen.'));
    renderItems(credentials, overview.credentials, function(entry, credential) {
      entry.appendChild(addText(document.createElement('strong'),
        credential.credentialType === 'human-password'
          ? t('settings.accountAdminPasswordCredential', 'Passwort-Anmeldung')
          : (credential.credentialType || '-')));
      appendMeta(entry, t('settings.accountAdminStatus', 'Status'), statusText(credential));
      appendMeta(entry, t('settings.accountAdminValidUntil', 'Gültig bis'), credential.expiresAt || '-');
      appendMeta(entry, t('settings.accountAdminCreatedAt', 'Erstellt'), credential.createdAt || '-');
      appendTechnicalDetails(entry, [
        ['Credential-ID', credential.credentialId]
      ]);

      if (actions && credential.credentialId &&
          credential.credentialType === 'human-password' &&
          credential.active === true &&
          !credential.revoked && !credential.expired) {
        const revoke = addText(document.createElement('button'),
          t('settings.accountAdminRevokeCredential', 'Passwort-Anmeldung widerrufen'));
        revoke.type = 'button';
        revoke.className = 'settings-account-admin-credential-revoke';
        revoke.disabled = Boolean(actions.busy);
        revoke.addEventListener('click', function() {
          if (typeof global.confirm === 'function') {
            const confirmed = global.confirm(t('settings.accountAdminRevokeCredentialConfirm',
              'Passwort-Anmeldung widerrufen? Damit ausgestellte Browser-Sitzungen werden ebenfalls beendet.'));
            if (!confirmed) return Promise.resolve(false);
          }
          return actions.revokeCredential(credential.credentialId);
        });
        entry.appendChild(revoke);
      }
    });
    parent.appendChild(credentials);

    const sessions = section(t('settings.accountAdminSessions', 'Angemeldete Geräte'));
    appendHint(sessions, t('settings.accountAdminSessionsHint',
      'Hier erscheinen aktive Browser-Sitzungen. Du kannst einzelne Geräte abmelden, ohne das Konto zu deaktivieren.'));
    renderItems(sessions, overview.sessions, function(entry, session) {
      entry.appendChild(addText(document.createElement('strong'),
        session.deviceId || t('settings.accountAdminUnknownDevice', 'Unbekanntes Gerät')));
      appendMeta(entry, t('settings.accountAdminStatus', 'Status'), statusText(session));
      appendMeta(entry, t('settings.accountAdminLastActive', 'Zuletzt aktiv'), session.lastSeenAt || '-');
      appendMeta(entry, t('settings.accountAdminValidUntil', 'Gültig bis'), session.expiresAt || '-');
      appendTechnicalDetails(entry, [
        ['Session-ID', session.sessionId],
        ['Credential-ID', session.issuedFromCredentialId || '-']
      ]);

      if (actions && session.sessionId && session.active === true &&
          !session.revoked && !session.expired) {
        const revoke = addText(document.createElement('button'),
          t('settings.accountAdminRevokeSession', 'Gerät abmelden'));
        revoke.type = 'button';
        revoke.className = 'settings-account-admin-session-revoke';
        revoke.disabled = Boolean(actions.busy);
        revoke.addEventListener('click', function() {
          if (typeof global.confirm === 'function') {
            const confirmed = global.confirm(t('settings.accountAdminRevokeSessionConfirm',
              'Dieses Gerät abmelden? Es muss sich anschließend erneut anmelden.'));
            if (!confirmed) return Promise.resolve(false);
          }
          return actions.revokeSession(session.sessionId);
        });
        entry.appendChild(revoke);
      }
    });
    parent.appendChild(sessions);
  }

  function render(parent) {
    const api = global.VdrSuiteAccountAdminClientApi;
    if (!parent || !api ||
        typeof api.listAccounts !== 'function' ||
        typeof api.loadAccount !== 'function' ||
        typeof api.createAccount !== 'function') {
      return Promise.resolve(null);
    }

    const card = document.createElement('article');
    card.className = 'module-placeholder settings-card settings-account-admin';
    card.appendChild(addText(document.createElement('h3'),
      t('settings.accountAdminTitle', 'Benutzer & Zugriffe')));
    card.appendChild(addText(document.createElement('p'),
      t('settings.accountAdminDescription',
        'Benutzer anlegen, Zugriff zuweisen und angemeldete Geräte verwalten.')));

    const intro = document.createElement('div');
    intro.className = 'settings-account-admin-intro';
    intro.appendChild(addText(document.createElement('strong'),
      t('settings.accountAdminHowToTitle', 'So funktioniert die Einrichtung')));
    intro.appendChild(addText(document.createElement('p'),
      t('settings.accountAdminHowTo',
        '1. Benutzer anlegen. 2. Benutzer auswählen. 3. Unter „Zugriff & Rolle“ festlegen, was er auf welchen Backends darf. 4. Danach kann sich der Benutzer anmelden.')));
    card.appendChild(intro);

    const status = document.createElement('p');
    status.className = 'settings-account-admin-status';
    status.setAttribute('role', 'status');
    status.setAttribute('aria-live', 'polite');
    card.appendChild(status);

    const createPanel = document.createElement('section');
    createPanel.className = 'settings-account-admin-section settings-account-admin-create';

    createPanel.appendChild(addText(document.createElement('h4'),
      t('settings.accountAdminCreateTitle', 'Benutzer hinzufügen')));

    const createLoginLabel = addText(document.createElement('label'),
      t('settings.accountAdminCreateLoginName', 'Loginname'));
    createLoginLabel.className = 'settings-account-admin-field';
    const createLoginInput = document.createElement('input');
    createLoginInput.type = 'text';
    createLoginInput.className = 'settings-account-admin-create-login';
    createLoginInput.setAttribute('autocomplete', 'username');
    createLoginLabel.appendChild(createLoginInput);
    createPanel.appendChild(createLoginLabel);

    const createDisplayLabel = addText(document.createElement('label'),
      t('settings.accountAdminCreateDisplayName', 'Anzeigename'));
    createDisplayLabel.className = 'settings-account-admin-field';
    const createDisplayInput = document.createElement('input');
    createDisplayInput.type = 'text';
    createDisplayInput.className = 'settings-account-admin-create-display-name';
    createDisplayLabel.appendChild(createDisplayInput);
    createPanel.appendChild(createDisplayLabel);

    const createPasswordLabel = addText(document.createElement('label'),
      t('settings.accountAdminCreatePassword', 'Initiales Passwort'));
    createPasswordLabel.className = 'settings-account-admin-field';
    const createPasswordInput = document.createElement('input');
    createPasswordInput.type = 'password';
    createPasswordInput.className = 'settings-account-admin-create-password';
    createPasswordInput.setAttribute('autocomplete', 'new-password');
    createPasswordLabel.appendChild(createPasswordInput);
    createPanel.appendChild(createPasswordLabel);

    createPanel.appendChild(addText(document.createElement('p'),
      t('settings.accountAdminCreateNoAutomaticAccess',
        'Nach dem Anlegen ist das Konto noch ohne Zugriff. Es wird automatisch ausgewählt; weise anschließend unter „Zugriff & Rolle“ die gewünschte Rolle oder Berechtigung zu.')));

    const createButton = addText(document.createElement('button'),
      t('settings.accountAdminCreateSubmit', 'Benutzer anlegen'));
    createButton.type = 'button';
    createButton.className = 'settings-account-admin-create-submit';
    createPanel.appendChild(createButton);
    card.appendChild(createPanel);

    const layout = document.createElement('div');
    layout.className = 'settings-account-admin-layout';
    const list = document.createElement('div');
    list.className = 'settings-account-admin-list';
    const detail = document.createElement('div');
    detail.className = 'settings-account-admin-detail';
    layout.appendChild(list);
    layout.appendChild(detail);
    card.appendChild(layout);
    parent.appendChild(card);

    const state = {
      items: [],
      nextCursor: '',
      selectedAccountId: '',
      selectedOverview: null,
      backends: [],
      mutating: false,
      creating: false
    };

    function setCreateBusy(busy) {
      state.creating = Boolean(busy);
      createLoginInput.disabled = state.creating;
      createDisplayInput.disabled = state.creating;
      createPasswordInput.disabled = state.creating;
      createButton.disabled = state.creating;
    }

    function createAccountFromForm() {
      if (state.creating) return Promise.resolve(false);

      const loginName = typeof createLoginInput.value === 'string'
        ? createLoginInput.value.trim() : '';
      const displayName = typeof createDisplayInput.value === 'string'
        ? createDisplayInput.value.trim() : '';
      const password = typeof createPasswordInput.value === 'string'
        ? createPasswordInput.value : '';

      if (!loginName || !displayName || !password) {
        status.textContent = t('settings.accountAdminCreateFieldsRequired',
          'Loginname, Anzeigename und initiales Passwort dürfen nicht leer sein.');
        return Promise.resolve(false);
      }

      const idempotencyKey = createAccountIdempotencyKey();
      if (!idempotencyKey) {
        status.textContent = t('settings.accountAdminCreateIdempotencyUnavailable',
          'Kontoanlage ist in diesem Browser nicht sicher verfügbar.');
        return Promise.resolve(false);
      }

      createPasswordInput.value = '';
      setCreateBusy(true);
      status.textContent = t('settings.accountAdminCreateSaving',
        'Benutzer wird angelegt …');

      return api.createAccount(loginName, displayName, password, idempotencyKey)
        .then(function(result) {
          const createdAccountId = result && result.data &&
            typeof result.data.accountId === 'string' ? result.data.accountId : '';

          createLoginInput.value = '';
          createDisplayInput.value = '';

          if (createdAccountId) {
            state.selectedAccountId = createdAccountId;
            state.selectedOverview = null;
          }

          return loadAccounts(false).then(function() {
            if (createdAccountId) return selectAccount(createdAccountId);
            return null;
          });
        }).then(function() {
          status.textContent = t('settings.accountAdminCreateSaved',
            'Benutzer wurde angelegt. Weise jetzt unter „Zugriff & Rolle“ den gewünschten Zugriff zu.');
          return true;
        }).catch(function(error) {
          status.textContent = createAccountErrorText(error);
          return false;
        }).finally(function() {
          setCreateBusy(false);
        });
    }

    createButton.addEventListener('click', createAccountFromForm);

    function markSelected() {
      list.querySelectorAll('button[data-account-id]').forEach(function(button) {
        button.classList.toggle('active',
          button.dataset.accountId === state.selectedAccountId);
      });
    }

    function replaceListAccount(account) {
      if (!account || !account.accountId) return;
      state.items = state.items.map(function(item) {
        return item.accountId === account.accountId
          ? Object.assign({}, item, account) : item;
      });
      redrawList();
    }

    function lifecycleActions() {
      return {
        busy: state.mutating,
        backends: state.backends,
        rename: function(displayName) {
          const normalized = typeof displayName === 'string' ? displayName.trim() : '';
          if (!normalized) {
            status.textContent = t('settings.accountAdminDisplayNameRequired',
              'Der Anzeigename darf nicht leer sein.');
            return Promise.resolve(false);
          }
          return mutateSelected(function(overview) {
            return api.updateAccountDisplayName(
              overview.account.accountId, overview.accountEtag, normalized);
          });
        },
        activate: function() {
          return mutateSelected(function(overview) {
            return api.activateAccount(overview.account.accountId, overview.accountEtag);
          });
        },
        deactivate: function() {
          return mutateSelected(function(overview) {
            return api.deactivateAccount(overview.account.accountId, overview.accountEtag);
          });
        },
        ensureGrant: function(permission, backendId) {
          const normalizedPermission =
            typeof permission === 'string' ? permission.trim() : '';
          const normalizedBackendId =
            typeof backendId === 'string' ? backendId.trim() : '';
          if (!normalizedPermission || !normalizedBackendId) {
            status.textContent = t('settings.accountAdminGrantFieldsRequired',
              'Berechtigung und Backend/Scope dürfen nicht leer sein.');
            return Promise.resolve(false);
          }
          return mutateSelected(function(overview) {
            return api.setAccountGrant(
              overview.account.accountId,
              overview.grantsEtag,
              normalizedPermission,
              normalizedBackendId,
              true);
          }, grantMutationErrorText);
        },
        revokeGrant: function(permission, backendId) {
          return mutateSelected(function(overview) {
            return api.setAccountGrant(
              overview.account.accountId,
              overview.grantsEtag,
              permission,
              backendId,
              false);
          }, grantMutationErrorText);
        },
        revokeCredential: function(credentialId) {
          return mutateSelected(function(overview) {
            return api.revokeAccountCredential(overview.account.accountId, credentialId);
          }, credentialMutationErrorText);
        },
        revokeSession: function(sessionId) {
          return mutateSelected(function(overview) {
            return api.revokeAccountSession(overview.account.accountId, sessionId);
          }, sessionMutationErrorText);
        }
      };
    }

    function showSelected() {
      renderDetail(detail, state.selectedOverview, lifecycleActions());
    }

    function refreshSelected(accountId) {
      return api.loadAccount(accountId).then(function(overview) {
        if (state.selectedAccountId !== accountId) return null;
        state.selectedOverview = overview;
        replaceListAccount(overview.account);
        showSelected();
        return overview;
      });
    }

    function mutateSelected(operation, errorFormatter) {
      if (state.mutating || !state.selectedOverview ||
          !state.selectedOverview.account || !state.selectedOverview.accountEtag) {
        return Promise.resolve(false);
      }

      const accountId = state.selectedOverview.account.accountId;
      state.mutating = true;
      showSelected();
      status.textContent = t('settings.accountAdminSaving', 'Änderung wird gespeichert …');

      return Promise.resolve().then(function() {
        return operation(state.selectedOverview);
      }).then(function() {
        return refreshSelected(accountId);
      }).then(function() {
        status.textContent = t('settings.accountAdminSaved', 'Änderung gespeichert.');
        return true;
      }).catch(function(error) {
        return api.loadAccount(accountId).then(function(overview) {
          if (state.selectedAccountId === accountId) {
            state.selectedOverview = overview;
            replaceListAccount(overview.account);
          }
        }).catch(function() {
          return null;
        }).then(function() {
          const formatError = typeof errorFormatter === 'function'
            ? errorFormatter : mutationErrorText;
          status.textContent = formatError(error);
          return false;
        });
      }).finally(function() {
        state.mutating = false;
        if (state.selectedAccountId === accountId && state.selectedOverview) {
          showSelected();
        }
      });
    }

    function selectAccount(accountId) {
      state.selectedAccountId = accountId;
      state.selectedOverview = null;
      markSelected();
      status.textContent = t('settings.accountAdminLoading', 'Benutzer werden geladen …');
      return api.loadAccount(accountId).then(function(overview) {
        if (state.selectedAccountId !== accountId) return;
        state.selectedOverview = overview;
        showSelected();
        status.textContent = '';
      }).catch(function(error) {
        if (state.selectedAccountId === accountId) {
          state.selectedOverview = null;
          detail.replaceChildren();
          status.textContent = errorText(error);
        }
      });
    }

    function accountButton(account) {
      const button = document.createElement('button');
      button.type = 'button';
      button.className = 'settings-account-admin-account';
      button.dataset.accountId = account.accountId;
      button.appendChild(addText(document.createElement('strong'),
        account.displayName || account.accountId));
      const meta = addText(document.createElement('span'),
        statusText(account));
      meta.className = 'settings-account-admin-meta';
      button.appendChild(meta);
      button.addEventListener('click', function() {
        selectAccount(account.accountId);
      });
      return button;
    }

    function redrawList() {
      list.replaceChildren();
      state.items.forEach(function(account) {
        list.appendChild(accountButton(account));
      });
      if (state.nextCursor) {
        const more = addText(document.createElement('button'),
          t('settings.accountAdminLoadMore', 'Weitere Konten laden'));
        more.type = 'button';
        more.addEventListener('click', function() {
          more.disabled = true;
          loadAccounts(true).finally(function() { more.disabled = false; });
        });
        list.appendChild(more);
      }
      markSelected();
    }

    function loadAccounts(append) {
      status.textContent = t('settings.accountAdminLoading', 'Benutzer werden geladen …');
      return api.listAccounts({
        limit: 25,
        cursor: append ? state.nextCursor : ''
      }).then(function(result) {
        const incoming = result && Array.isArray(result.items) ? result.items : [];
        state.items = append ? state.items.concat(incoming) : incoming;
        state.nextCursor = result && result.page && result.page.hasMore
          ? (result.page.nextCursor || '') : '';
        redrawList();
        if (!state.items.length) {
          detail.replaceChildren();
          status.textContent = t('settings.accountAdminNoAccounts',
            'Keine Human Accounts sichtbar.');
          return;
        }
        status.textContent = '';
        if (!state.selectedAccountId) {
          return selectAccount(state.items[0].accountId);
        }
      }).catch(function(error) {
        list.replaceChildren();
        detail.replaceChildren();
        status.textContent = errorText(error);
      });
    }

    const loadBackends = typeof api.listBackends === 'function'
      ? Promise.resolve().then(function() {
          return api.listBackends();
        }).then(function(items) {
          state.backends = Array.isArray(items) ? items : [];
        }).catch(function() {
          state.backends = [];
        })
      : Promise.resolve();

    return loadBackends
      .then(function() { return loadAccounts(false); })
      .then(function() { return card; });
  }

  global.VdrSuiteAccountAdminSettings = Object.freeze({render: render});
})(window);
