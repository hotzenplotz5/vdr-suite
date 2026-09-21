'use strict';

// Read-only, loopback-only fixture server. Never connects to a VDR backend.
const http = require('http');
const fs = require('fs');
const path = require('path');
const {execFileSync} = require('child_process');
const root = path.resolve(__dirname, '..');
const baseline = execFileSync('git', [
  'show', '8bdf508908454a43c3e00466d93037bb95f7dc17:web/frontend/home-recording-discovery.js'
], {cwd: root});
const files = new Map([
  ['/web/frontend/tests/browser-performance.html', 'web/frontend/tests/browser-performance.html'],
  ['/web/frontend/home-recording-discovery.js', 'web/frontend/home-recording-discovery.js']
]);
http.createServer((request, response) => {
  const url = new URL(request.url, 'http://127.0.0.1');
  const file = files.get(url.pathname);
  if (request.method !== 'GET' || !file) {
    response.writeHead(404);
    response.end();
    return;
  }
  response.setHeader('Content-Type', file.endsWith('.html')
    ? 'text/html; charset=utf-8' : 'text/javascript; charset=utf-8');
  response.setHeader('Cache-Control', 'no-store');
  response.end(url.searchParams.has('baseline') && file.endsWith('.js')
    ? baseline : fs.readFileSync(path.join(root, file)));
}).listen(18765, '127.0.0.1', () => {
  console.log('Open http://127.0.0.1:18765/web/frontend/tests/browser-performance.html');
  console.log('Append ?baseline=1 for the verified main baseline; stop with Ctrl+C.');
});
