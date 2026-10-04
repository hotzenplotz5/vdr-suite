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

  function renderDetail(parent, overview) {
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
        'Read-only Übersicht der Human Accounts sowie ihrer Berechtigungen, Anmeldedaten und Sitzungen.')));

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

    const state = {items: [], nextCursor: '', selectedAccountId: ''};

    function markSelected() {
      list.querySelectorAll('button[data-account-id]').forEach(function(button) {
        button.classList.toggle('active',
          button.dataset.accountId === state.selectedAccountId);
      });
    }

    function selectAccount(accountId) {
      state.selectedAccountId = accountId;
      markSelected();
      status.textContent = t('settings.accountAdminLoading', 'Benutzer werden geladen …');
      return api.loadAccount(accountId).then(function(overview) {
        if (state.selectedAccountId !== accountId) return;
        renderDetail(detail, overview);
        status.textContent = '';
      }).catch(function(error) {
        if (state.selectedAccountId === accountId) {
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
