'use strict';
const fs = require('node:fs');
const path = require('node:path');
const source = path.resolve(__dirname, '../web/frontend/index.html');
const destination = process.argv[2];
if (!destination) throw new Error('Usage: node tools/build_browser_performance_diagnostics.js OUTPUT');
const html = fs.readFileSync(source, 'utf8');
const marker = '<!-- VDR-Suite browser diagnostics: generated, opt-in entrypoint -->';
if (!html.includes('<head>') || !html.includes('</head>') || html.includes(marker)) throw new Error('Unexpected frontend HTML structure');
// The generated entrypoint is installed alongside index.html, so all ordinary
// relative asset URLs retain their production meaning. No application bootstrap
// or API owner is replaced.
const scripts = marker + '\n  <script src="browser-artwork-probe.js"></script>\n  <script src="browser-performance-bridge.js"></script>\n';
fs.mkdirSync(path.dirname(path.resolve(destination)), { recursive: true });
fs.writeFileSync(destination, html.replace('<head>', '<head>\n' + scripts));
console.log('Generated browser diagnostics entrypoint');
