'use strict';
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const digest = bytes => crypto.createHash('sha256').update(bytes).digest('hex');

// Use the existing daemon route inventory and mirror its exact two-file bundle
// composition against the installed payload, not the raw frontend source tree.
function generate(frontend, routes, worker, helper) {
  const assets = {};
  function add(name, type, primary, addon) {
    const file = path.join(frontend, primary);
    if (!fs.existsSync(file)) return;
    let bytes = fs.readFileSync(file);
    if (addon && fs.existsSync(path.join(frontend, addon))) {
      bytes = Buffer.concat([bytes, Buffer.from('\n\n'), fs.readFileSync(path.join(frontend, addon))]);
    }
    assets[name] = {type, sha256: digest(bytes)};
  }
  add('index.html', 'text/html', 'index.html');
  for (const entry of routes.matchAll(/\{"\/frontend\/([^"\n]+)", "([^"\n]+)", "(application\/javascript|text\/css); charset=utf-8", (?:"([^"\n]+)"|nullptr)\}/g)) {
    const [, name, primary, type, addon] = entry;
    if (name === 'service-worker.js' || name.startsWith('browser-')) continue;
    add(name, type, primary, addon);
  }
  const id = digest(JSON.stringify(assets) + '\n' + worker + '\n' + helper);
  return worker + '\nconst localShellRelease = ' + JSON.stringify({id, assets}) + ';\n' + helper + '\n';
}

function main() {
  const frontend = path.resolve(process.argv[2]);
  const root = path.resolve(__dirname, '..');
  const read = name => fs.readFileSync(path.join(root, name), 'utf8');
  const output = generate(frontend, read('core/http/src/TestHttpServerPaths.inc'),
    read('web/frontend/service-worker.js'), read('web/frontend/pwa-local-shell.js'));
  const target = path.join(frontend, 'service-worker.js');
  if (process.argv[3] === '--check') {
    if (fs.readFileSync(target, 'utf8') !== output) throw Error('installed PWA shell release differs');
  } else {
    const tmp = target + '.tmp';
    fs.writeFileSync(tmp, output, {mode: 0o644});
    fs.renameSync(tmp, target);
  }
  console.log('PWA_STATIC_SHELL_RELEASE=PASS');
}
if (require.main === module) main();
module.exports = {generate};
