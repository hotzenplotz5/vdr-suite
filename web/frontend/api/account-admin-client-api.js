(function(global) {
  'use strict';

  let client = null;

  function publicClient() {
    if (client) return client;
    const factory = global.VdrSuitePublicV1Client;
    if (!factory || typeof factory.createClient !== 'function') {
      throw new Error('Public-v1 client is not available');
    }
    const baseUrl = global.location && global.location.origin
      ? global.location.origin : '';
    client = factory.createClient({
      baseUrl: baseUrl,
      credentials: 'same-origin'
    });
    return client;
  }

  function readOptions(extra) {
    return Object.assign({
      credentials: 'same-origin',
      cache: 'no-store'
    }, extra || {});
  }

  function listAccounts(options) {
    const normalized = options && typeof options === 'object' ? options : {};
    const query = {
      limit: normalized.limit || 25,
      sort: 'accountId',
      order: 'asc'
    };
    if (normalized.cursor) query.cursor = normalized.cursor;
    return publicClient().getAccounts(readOptions({query: query}));
  }

  function loadAccount(accountId) {
    if (typeof accountId !== 'string' || accountId === '') {
      return Promise.reject(new Error('accountId is required'));
    }
    const api = publicClient();
    const options = readOptions({accountId: accountId});
    return Promise.all([
      api.getAccount(options),
      api.getAccountGrants(options),
      api.getAccountCredentials(options),
      api.getAccountSessions(options)
    ]).then(function(results) {
      const accountResult = results[0] || {};
      const grantsResult = results[1] || {};
      const credentials = results[2] || {};
      const sessions = results[3] || {};
      return {
        account: accountResult.data || null,
        accountEtag: accountResult.etag || '',
        grants: grantsResult.data && Array.isArray(grantsResult.data.items)
          ? grantsResult.data.items : [],
        grantsEtag: grantsResult.etag || '',
        credentials: Array.isArray(credentials.items) ? credentials.items : [],
        sessions: Array.isArray(sessions.items) ? sessions.items : []
      };
    });
  }

  function activeSessionCsrfHeaders() {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.csrfHeaders !== 'function') return {};
    const headers = session.csrfHeaders();
    return headers && typeof headers === 'object'
      ? Object.assign({}, headers) : {};
  }

  function afterBrowserSessionRestore(action) {
    const session = global.VdrSuiteBrowserSession;
    if (!session || typeof session.restore !== 'function') {
      return Promise.resolve().then(action);
    }
    return Promise.resolve(session.restore()).then(action);
  }

  function mutationOptions(accountId, ifMatch) {
    if (typeof accountId !== 'string' || accountId === '') {
      throw new Error('accountId is required');
    }
    if (typeof ifMatch !== 'string' || ifMatch === '') {
      throw new Error('ifMatch is required');
    }
    return {
      accountId: accountId,
      ifMatch: ifMatch,
      headers: activeSessionCsrfHeaders(),
      credentials: 'same-origin',
      cache: 'no-store'
    };
  }

  function updateAccountDisplayName(accountId, ifMatch, displayName) {
    return afterBrowserSessionRestore(function() {
      const options = mutationOptions(accountId, ifMatch);
      options.displayName = displayName;
      return publicClient().updateAccountDisplayName(options);
    });
  }

  function activateAccount(accountId, ifMatch) {
    return afterBrowserSessionRestore(function() {
      return publicClient().activateAccount(mutationOptions(accountId, ifMatch));
    });
  }

  function deactivateAccount(accountId, ifMatch) {
    return afterBrowserSessionRestore(function() {
      return publicClient().deactivateAccount(mutationOptions(accountId, ifMatch));
    });
  }

  function resetForTests() {
    client = null;
  }

  global.VdrSuiteAccountAdminClientApi = Object.freeze({
    listAccounts: listAccounts,
    loadAccount: loadAccount,
    updateAccountDisplayName: updateAccountDisplayName,
    activateAccount: activateAccount,
    deactivateAccount: deactivateAccount,
    resetForTests: resetForTests
  });
})(window);
