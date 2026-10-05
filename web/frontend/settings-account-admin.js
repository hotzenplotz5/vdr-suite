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
    const identity = section(t('settings.accountAdminIdentity', 'Konto'));
    identity.appendChild(addText(document.createElement('h3'),
      account.displayName || account.accountId));
    appendMeta(identity, 'Account-ID', account.accountId);
    appendMeta(identity, 'Actor-ID', account.actorId);
    appendMeta(identity, 'Status', statusText(account));

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

    const grants = section(t('settings.accountAdminGrants', 'Berechtigungen'));
    renderItems(grants, overview.grants, function(entry, grant) {
      entry.appendChild(addText(document.createElement('strong'), grant.permission || '-'));
      appendMeta(entry, 'Backend', grant.backendId || '-');
    });
    parent.appendChild(grants);

    const credentials = section(t('settings.accountAdminCredentials', 'Anmeldedaten'));
    renderItems(credentials, overview.credentials, function(entry, credential) {
      entry.appendChild(addText(document.createElement('strong'),
        credential.credentialType || '-'));
      appendMeta(entry, 'Credential-ID', credential.credentialId);
      appendMeta(entry, 'Status', statusText(credential));
      appendMeta(entry, 'Gültig bis', credential.expiresAt || '-');
      appendMeta(entry, 'Erstellt', credential.createdAt || '-');

      if (actions && credential.credentialId &&
          credential.credentialType === 'human-password' &&
          credential.active === true &&
          !credential.revoked && !credential.expired) {
        const revoke = addText(document.createElement('button'),
          t('settings.accountAdminRevokeCredential', 'Anmeldedaten widerrufen'));
        revoke.type = 'button';
        revoke.className = 'settings-account-admin-credential-revoke';
        revoke.disabled = Boolean(actions.busy);
        revoke.addEventListener('click', function() {
          if (typeof global.confirm === 'function') {
            const confirmed = global.confirm(t('settings.accountAdminRevokeCredentialConfirm',
              'Anmeldedaten widerrufen? Damit ausgestellte Browser-Sitzungen werden ebenfalls beendet.'));
            if (!confirmed) return Promise.resolve(false);
          }
          return actions.revokeCredential(credential.credentialId);
        });
        entry.appendChild(revoke);
      }
    });
    parent.appendChild(credentials);

    const sessions = section(t('settings.accountAdminSessions', 'Sitzungen'));
    renderItems(sessions, overview.sessions, function(entry, session) {
      entry.appendChild(addText(document.createElement('strong'),
        session.deviceId || session.sessionId || '-'));
      appendMeta(entry, 'Session-ID', session.sessionId);
      appendMeta(entry, 'Credential', session.issuedFromCredentialId || '-');
      appendMeta(entry, 'Status', statusText(session));
      appendMeta(entry, 'Zuletzt aktiv', session.lastSeenAt || '-');
      appendMeta(entry, 'Gültig bis', session.expiresAt || '-');

      if (actions && session.sessionId && session.active === true &&
          !session.revoked && !session.expired) {
        const revoke = addText(document.createElement('button'),
          t('settings.accountAdminRevokeSession', 'Sitzung widerrufen'));
        revoke.type = 'button';
        revoke.className = 'settings-account-admin-session-revoke';
        revoke.disabled = Boolean(actions.busy);
        revoke.addEventListener('click', function() {
          if (typeof global.confirm === 'function') {
            const confirmed = global.confirm(t('settings.accountAdminRevokeSessionConfirm',
              'Sitzung widerrufen? Dieses Gerät muss sich anschließend erneut anmelden.'));
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
        typeof api.loadAccount !== 'function') {
      return Promise.resolve(null);
    }

    const card = document.createElement('article');
    card.className = 'module-placeholder settings-card settings-account-admin';
    card.appendChild(addText(document.createElement('h3'),
      t('settings.accountAdminTitle', 'Benutzer & Zugriffe')));
    card.appendChild(addText(document.createElement('p'),
      t('settings.accountAdminDescription',
        'Human Accounts, Berechtigungen, Anmeldedaten und Sitzungen verwalten.')));

    const status = document.createElement('p');
    status.className = 'settings-account-admin-status';
    status.setAttribute('role', 'status');
    status.setAttribute('aria-live', 'polite');
    card.appendChild(status);

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
      mutating: false
    };

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
        account.accountId + ' · ' + statusText(account));
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

    return loadAccounts(false).then(function() { return card; });
  }

  global.VdrSuiteAccountAdminSettings = Object.freeze({render: render});
})(window);
