(function(global) {
  'use strict';
  // Human Browser Session only. Paired Device credentials are never read,
  // exposed to the admin view or used for administrative mutations.
  let client = null;
  function publicClient() {
    if (!client) {
      const factory = global.VdrSuitePublicV1Client;
      if (!factory || typeof factory.createClient !== 'function')
        throw new Error('Public-v1 client is unavailable');
      client = factory.createClient({
        baseUrl: global.location && global.location.origin || '',
        credentials: 'same-origin'
      });
    }
    return client;
  }
  function sessionHeaders() {
    const owner = global.VdrSuiteBrowserSession;
    const source = owner && typeof owner.csrfHeaders === 'function'
      ? owner.csrfHeaders() : null;
    return source && typeof source === 'object'
      ? Object.assign({}, source) : {};
  }
  function restore(action) {
    const owner = global.VdrSuiteBrowserSession;
    const ready = owner && typeof owner.restore === 'function'
      ? owner.restore() : null;
    return Promise.resolve(ready).then(action);
  }
  // Browser Human-Admin only; never a TV bootstrap token.
  function listPendingPairings(cursor) {
    return publicClient().listDevicePairingsForAdmin({
      query: Object.assign({limit: 25, sort: 'pairingRequestId', order: 'asc'},
        cursor ? {cursor: cursor} : {}),
      credentials: 'same-origin', cache: 'no-store'
    });
  }
  function loadPendingPairing(pairingRequestId) {
    if (typeof pairingRequestId !== 'string' ||
        !/^dpr_[0-9a-f]{32}$/.test(pairingRequestId))
      return Promise.reject(new Error('Invalid pairing request ID'));
    return restore(function() {
      return publicClient().getDevicePairingForAdmin({
        pairingRequestId: pairingRequestId,
        credentials: 'same-origin', cache: 'no-store'
      });
    });
  }
  function decidePairing(pairingRequestId, etag, decision) {
    if (typeof pairingRequestId !== 'string' ||
        !/^dpr_[0-9a-f]{32}$/.test(pairingRequestId) ||
        typeof etag !== 'string' || !/^"[^"]{1,256}"$/.test(etag) ||
        (decision !== 'approve' && decision !== 'reject'))
      return Promise.reject(new Error('Invalid pairing decision or ETag'));
    return restore(function() {
      return publicClient().decideDevicePairingForAdmin({
        pairingRequestId: pairingRequestId, ifMatch: etag,
        decision: decision, headers: sessionHeaders(),
        credentials: 'same-origin', cache: 'no-store'
      });
    });
  }
  function listDevices(cursor) {
    return publicClient().getDevices({
      query: Object.assign({limit: 25},
        cursor ? {cursor: cursor} : {}),
      credentials: 'same-origin', cache: 'no-store'
    });
  }
  function loadDevice(deviceId) {
    return publicClient().getDeviceGrants({
      deviceId: deviceId, credentials: 'same-origin', cache: 'no-store'
    });
  }
  function setGrant(deviceId, etag, permission, backendId, active) {
    // Server Device-grant admission requires a concrete backend for every
    // Timer and SearchTimer mutation permission, not only for CREATE.
    const concreteBackendRequired = [
      'timers.create', 'timers.modify', 'timers.delete',
      'searchtimers.create', 'searchtimers.modify',
      'searchtimers.execute', 'searchtimers.delete'
    ].includes(permission);
    if (typeof etag !== 'string' || !etag ||
        !/^[A-Za-z0-9_-]{1,128}$/.test(deviceId) ||
        typeof permission !== 'string' ||
        typeof backendId !== 'string' ||
        typeof active !== 'boolean' ||
        (concreteBackendRequired &&
          (backendId === '*' || backendId === '.' || backendId === '..' ||
            !/^[A-Za-z0-9_.-]{1,128}$/.test(backendId)))) {
      return Promise.reject(new Error('Unsafe Device Grant mutation'));
    }
    return restore(function() {
      return publicClient().setDeviceGrant({
        deviceId: deviceId, ifMatch: etag,
        permission: permission, backendId: backendId, active: active,
        headers: sessionHeaders(), credentials: 'same-origin', cache: 'no-store'
      });
    });
  }
  function listBackends() {
    return publicClient().getBackends({
      query: {limit: 100, sort: 'backendId', order: 'asc'},
      credentials: 'same-origin', cache: 'no-store'
    }).then(function(result) {
      return result && Array.isArray(result.items) ? result.items : [];
    });
  }
  global.VdrSuiteDeviceAdminClientApi = Object.freeze({
    listPendingPairings: listPendingPairings,
    loadPendingPairing: loadPendingPairing,
    decidePairing: decidePairing,
    listDevices: listDevices,
    loadDevice: loadDevice,
    listBackends: listBackends,
    setGrant: setGrant,
    resetForTests: function() { client = null; }
  });
})(window);
