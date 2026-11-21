#!/usr/bin/env node
'use strict';

// API.V1.F1: read-only repository parity inventory.
// This report never changes routes, CI gates, deployments or runtime behavior.
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');

const root = path.resolve(__dirname, '..');
const policyPath = 'docs/development/api-v1-f1-parity-matrix.json';
const read = (relative) => fs.readFileSync(path.join(root, relative), 'utf8');
const parse = (relative) => JSON.parse(read(relative));
const hash = (text) => crypto.createHash('sha256').update(text).digest('hex');

function matches(pathname, prefix) {
  return pathname === prefix ||
    (prefix !== '/api/v1' && pathname.startsWith(prefix + '/'));
}

function ownerFor(pathname, domains) {
  const matchesAll = [];
  for (const domain of domains) {
    for (const prefix of domain.prefixes) {
      if (matches(pathname, prefix)) {
        matchesAll.push({ domain: domain.id, prefix });
      }
    }
  }
  matchesAll.sort((a, b) => b.prefix.length - a.prefix.length ||
    a.domain.localeCompare(b.domain));
  return matchesAll.length ? matchesAll[0].domain : null;
}

function browserOperations(source, filename) {
  const names = new Set();
  const regex = /function\s+((?:fetchClient|fetchPublicSearchTimer|mutatePublicSearchTimer)[A-Za-z0-9_]*)\s*\(/g;
  for (const match of source.matchAll(regex)) names.add(match[1]);
  if (filename.endsWith('/live-remote-client-api.js')) {
    for (const name of ['createClientLiveUpdateSource', 'createClientOperationUpdateSource']) {
      if (source.includes('function ' + name + '(')) names.add(name);
    }
  }
  return names;
}

function quotedApiLiterals(source) {
  const values = new Set();
  for (const match of source.matchAll(/['"](\/api\/[^'"]*)['"]/g)) {
    values.add(match[1]);
  }
  return [...values].sort();
}

function report() {
  const policy = parse(policyPath);
  const specText = read(policy.sources.openapi);
  const spec = JSON.parse(specText);
  const old = parse(policy.sources.historicalBrowserContract);
  const files = policy.sources.browserFiles.map((filename) => ({
    filename, source: read(filename)
  }));
  const browserFound = new Set();
  const browserPaths = new Set();
  for (const file of files) {
    for (const name of browserOperations(file.source, file.filename)) {
      browserFound.add(name);
    }
    for (const route of quotedApiLiterals(file.source)) browserPaths.add(route);
  }
  const classified = new Map();
  for (const group of old.browserClientGroups || []) {
    for (const name of group.exports || []) {
      if (!classified.has(name)) classified.set(name, []);
      classified.get(name).push(group.domain);
    }
  }
  const documented = [];
  const httpMethods = new Set(['get', 'post', 'put', 'patch', 'delete', 'head', 'options']);
  for (const [pathname, shape] of Object.entries(spec.paths || {})) {
    for (const method of Object.keys(shape)) {
      if (!httpMethods.has(method.toLowerCase())) continue;
      documented.push({ method: method.toUpperCase(), path: pathname,
        domain: ownerFor(pathname, policy.domains) });
    }
  }
  documented.sort((a, b) => a.path.localeCompare(b.path) ||
    a.method.localeCompare(b.method));
  const historicalGroups = new Set((old.browserClientGroups || []).map((g) => g.domain));
  const mappedGroups = new Map();
  for (const domain of policy.domains) {
    for (const group of domain.browserGroups) {
      if (!mappedGroups.has(group)) mappedGroups.set(group, []);
      mappedGroups.get(group).push(domain.id);
    }
  }
  const domains = policy.domains.map((domain) => {
    const methods = documented.filter((op) => op.domain === domain.id);
    const legacyEvidence = (domain.legacyExamples || []).map((route) => ({
      route, foundInBrowserSource: browserPaths.has(route)
    }));
    const groups = (old.browserClientGroups || [])
      .filter((group) => domain.browserGroups.includes(group.domain));
    return {
      id: domain.id, parity: domain.parity, nextSlice: domain.nextSlice,
      note: domain.note, documentedPublicOperations: methods,
      browserOperationsByHistoricalGroup: groups.map((group) => ({
        group: group.domain, historicalClassification: group.classification,
        operations: group.exports
      })),
      legacyLiteralEvidence: legacyEvidence
    };
  });
  const sourceLiterals = [];
  const sourceDirectory = path.join(root, 'api/rest/src');
  if (fs.existsSync(sourceDirectory)) {
    for (const filename of fs.readdirSync(sourceDirectory).filter((f) => f.endsWith('.cpp'))) {
      const contents = fs.readFileSync(path.join(sourceDirectory, filename), 'utf8');
      for (const literal of quotedApiLiterals(contents)) {
        if (literal.startsWith('/api/v1')) {
          sourceLiterals.push({ filename: 'api/rest/src/' + filename, literal });
        }
      }
    }
  }
  const missingFromHistorical = [...browserFound].filter((name) => !classified.has(name)).sort();
  const staleHistorical = [...classified.keys()].filter((name) => !browserFound.has(name)).sort();
  const duplicatedClassification = [...classified.entries()]
    .filter((entry) => entry[1].length !== 1).map((entry) => entry[0]).sort();
  const unmappedHistoricalGroups = [...historicalGroups]
    .filter((group) => !mappedGroups.has(group)).sort();
  const multiplyMappedGroups = [...mappedGroups.entries()]
    .filter((entry) => entry[1].length !== 1).map((entry) => entry[0]).sort();
  return {
    schemaVersion: 1, workPackage: policy.workPackage,
    auditBaselineCommit: policy.baselineCommit,
    releaseStatus: policy.releaseStatus, changePolicy: policy.changePolicy,
    provenance: {
      openapiSha256: hash(specText),
      historicalBrowserMatrixSha256: hash(read(policy.sources.historicalBrowserContract)),
      browserSourceSha256: Object.fromEntries(files.map(
        (file) => [file.filename, hash(file.source)]))
    },
    publicOpenApi: {
      pathTemplates: Object.keys(spec.paths || {}).length,
      methods: documented.length,
      undocumentedByF1Domain: documented.filter((op) => op.domain === null)
    },
    browser: {
      inspectedSources: policy.sources.browserFiles,
      discoveredNamedOperations: browserFound.size,
      historicallyClassifiedOperations: classified.size,
      discoveredLiteralRoutes: [...browserPaths].sort(),
      unclassifiedOperations: missingFromHistorical,
      staleHistoricalOperations: staleHistorical,
      multiplyClassifiedOperations: duplicatedClassification,
      unmappedHistoricalGroups, multiplyMappedGroups
    },
    sourceObservations: {
      scope: 'Quoted v1 route literals in api/rest/src/*.cpp only; neither exhaustive route registration nor method/security parity',
      publicRouteLiterals: sourceLiterals
    },
    domains
  };
}

const result = report();
if (process.argv.includes('--json')) {
  process.stdout.write(JSON.stringify(result, null, 2) + '\n');
} else {
  process.stdout.write('API.V1.F1 source parity report (read-only; no release approval)\n');
  process.stdout.write('OpenAPI: ' + result.publicOpenApi.pathTemplates +
    ' path templates, ' + result.publicOpenApi.methods + ' method/path pairs\n');
  process.stdout.write('Browser wrappers: ' + result.browser.discoveredNamedOperations +
    ' discovered, ' + result.browser.historicallyClassifiedOperations +
    ' historically classified\n');
  for (const item of result.domains) {
    process.stdout.write(item.id + ': ' + item.documentedPublicOperations.length +
      ' documented method/path pairs; ' + item.parity +
      (item.nextSlice ? '; ' + item.nextSlice : '') + '\n');
  }
  const drift = result.publicOpenApi.undocumentedByF1Domain.length +
    result.browser.unclassifiedOperations.length +
    result.browser.staleHistoricalOperations.length +
    result.browser.multiplyClassifiedOperations.length +
    result.browser.unmappedHistoricalGroups.length +
    result.browser.multiplyMappedGroups.length;
  process.stdout.write('Classification drift observations: ' + drift +
    ' (report only; no new CI or runtime gate)\n');
}
