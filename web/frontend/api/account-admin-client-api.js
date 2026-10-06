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

  function listBackends() {
    return publicClient().getBackends(readOptions({
      query: {
        limit: 100,
        sort: 'backendId',
        order: 'asc'
      }
    })).then(function(result) {
      return result && Array.isArray(result.items) ? result.items : [];
    });
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
        supportedPermissions:
          grantsResult.data && Array.isArray(grantsResult.data.supportedPermissions)
            ? grantsResult.data.supportedPermissions : [],
        supportedScopeKinds:
          grantsResult.data && Array.isArray(grantsResult.data.supportedScopeKinds)
            ? grantsResult.data.supportedScopeKinds : [],
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

  function createAccount(loginName, displayName, password, idempotencyKey) {
    return afterBrowserSessionRestore(function() {
      return publicClient().createAccount({
        loginName: loginName,
        displayName: displayName,
        password: password,
        idempotencyKey: idempotencyKey,
        headers: activeSessionCsrfHeaders(),
        credentials: 'same-origin'
      });
    });
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

  function setAccountGrant(accountId, ifMatch, permission, backendId, active) {
    if (typeof ifMatch !== 'string' || ifMatch === '') {
      const error = new Error('grant ifMatch is required');
      error.status = 428;
      return Promise.reject(error);
    }
    return afterBrowserSessionRestore(function() {
      const options = mutationOptions(accountId, ifMatch);
      options.permission = permission;
      options.backendId = backendId;
      options.active = active;
      return publicClient().setAccountGrant(options);
    });
  }

  function revokeAccountCredential(accountId, credentialId) {
    if (typeof credentialId !== 'string' || credentialId === '') {
      return Promise.reject(new Error('credentialId is required'));
    }
    return afterBrowserSessionRestore(function() {
      const api = publicClient();
      return api.getAccountCredential(readOptions({
        accountId: accountId,
        credentialId: credentialId
      })).then(function(result) {
        const etag = result && typeof result.etag === 'string' ? result.etag : '';
        if (!etag) {
          const error = new Error('credential ifMatch is required');
          error.status = 428;
          throw error;
        }
        const options = mutationOptions(accountId, etag);
        options.credentialId = credentialId;
        return api.revokeAccountCredential(options);
      });
    });
  }

  function revokeAccountSession(accountId, sessionId) {
    if (typeof sessionId !== 'string' || sessionId === '') {
      return Promise.reject(new Error('sessionId is required'));
    }
    return afterBrowserSessionRestore(function() {
      const api = publicClient();
      return api.getAccountSession(readOptions({
        accountId: accountId,
        sessionId: sessionId
      })).then(function(result) {
        const etag = result && typeof result.etag === 'string' ? result.etag : '';
        if (!etag) {
          const error = new Error('session ifMatch is required');
          error.status = 428;
          throw error;
        }
        const options = mutationOptions(accountId, etag);
        options.sessionId = sessionId;
        return api.revokeAccountSession(options);
      });
    });
  }

  function resetForTests() {
    client = null;
  }

  global.VdrSuiteAccountAdminClientApi = Object.freeze({
    listAccounts: listAccounts,
    listBackends: listBackends,
    loadAccount: loadAccount,
    createAccount: createAccount,
    updateAccountDisplayName: updateAccountDisplayName,
    activateAccount: activateAccount,
    deactivateAccount: deactivateAccount,
    setAccountGrant: setAccountGrant,
    revokeAccountCredential: revokeAccountCredential,
    revokeAccountSession: revokeAccountSession,
    resetForTests: resetForTests
  });
})(window);
