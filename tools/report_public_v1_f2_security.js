#!/usr/bin/env node
'use strict';
// Read-only API.V1.F2 source metadata report; no CI, runtime or deployment gate.
const fs = require('node:fs');
const path = require('node:path');
const root = path.resolve(__dirname, '..');
const read = (name) => fs.readFileSync(path.join(root, name), 'utf8');
const spec = JSON.parse(read('api/openapi.json'));
const schemes = spec.components?.securitySchemes || {};
const profiles = {};
const missing = [], unknown = [];
let operations = 0;
for (const [route, methods] of Object.entries(spec.paths || {})) {
  for (const [method, contract] of Object.entries(methods)) {
    if (!['get','post','put','patch','delete'].includes(method)) continue;
    operations += 1;
    const name = method.toUpperCase() + ' ' + route;
    if (!Array.isArray(contract.security) || !contract['x-vdr-suite-auth-profile']) {
      missing.push(name);
      continue;
    }
    const profile = contract['x-vdr-suite-auth-profile'];
    profiles[profile] = (profiles[profile] || 0) + 1;
    for (const alternative of contract.security) {
      for (const key of Object.keys(alternative))
        if (!Object.prototype.hasOwnProperty.call(schemes, key))
          unknown.push(name + ': ' + key);
    }
  }
}
const browser = read('core/security/include/BrowserSessionAuthenticator.h');
const evidence = {
  cookie: browser.includes('"vdr_suite_session"') && schemes.browserSession?.name === 'vdr_suite_session',
  csrf: browser.includes('"X-CSRF-Token"') && schemes.browserCsrf?.name === 'X-CSRF-Token',
  device: read('core/security/include/DeviceCredentialAuthenticator.h').includes('VDR-Suite-Device ') &&
    schemes.deviceCredential?.name === 'Authorization',
  pairing: read('core/security/include/SecurityHttpGate.h').includes('X-VDR-Suite-Pairing-Token') &&
    schemes.pairingToken?.name === 'X-VDR-Suite-Pairing-Token',
  basic: schemes.managedBasic?.type === 'http' && schemes.managedBasic?.scheme === 'basic'
};
const report = {slice:'API.V1.F2',releaseAccepted:false,
  pathTemplates:Object.keys(spec.paths || {}).length,operations,
  profiles,schemes:Object.keys(schemes).sort(),sourceEvidence:evidence,
  missingSecurityProfiles:missing,unknownSchemeReferences:unknown};
process.stdout.write(process.argv.includes('--json')
  ? JSON.stringify(report,null,2)+'\n'
  : 'API.V1.F2 '+report.pathTemplates+' paths / '+operations+' operations; '+
    missing.length+' missing profiles; '+unknown.length+' unknown refs\n');
