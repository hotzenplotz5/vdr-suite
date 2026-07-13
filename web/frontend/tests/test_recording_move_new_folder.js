"use strict";

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const registeredModules = Object.create(null);

global.window = {
  VdrSuitePlatform: {
    hasModule(name) {
      return Object.prototype.hasOwnProperty.call(registeredModules, name);
    },
    registerModule(name, moduleApi) {
      registeredModules[name] = moduleApi;
    }
  }
};

vm.runInThisContext(
  fs.readFileSync('web/frontend/modules/recordings.js', 'utf8'),
  { filename: 'web/frontend/modules/recordings.js' }
);

const api = window.VdrSuiteRecordingBrowser;
assert.ok(api);
assert.strictEqual(typeof api.validateNewMoveFolderName, 'function');
assert.strictEqual(typeof api.joinMoveFolderPath, 'function');

assert.strictEqual(api.validateNewMoveFolderName('').valid, false);
assert.strictEqual(
  api.validateNewMoveFolderName('').messageKey,
  'recordings.move.newFolderRequired'
);
assert.strictEqual(
  api.validateNewMoveFolderName('.').messageKey,
  'recordings.move.newFolderReserved'
);
assert.strictEqual(
  api.validateNewMoveFolderName('..').messageKey,
  'recordings.move.newFolderReserved'
);
['Anime/Neu', 'Anime\\Neu', 'Anime~Neu', 'Anime\u0001Neu'].forEach(name => {
  assert.strictEqual(
    api.validateNewMoveFolderName(name).messageKey,
    'recordings.move.newFolderInvalidCharacters'
  );
});
assert.strictEqual(
  api.validateNewMoveFolderName('x'.repeat(81)).messageKey,
  'recordings.move.newFolderTooLong'
);
assert.deepStrictEqual(
  api.validateNewMoveFolderName('  Anime Filme  '),
  {
    valid: true,
    name: 'Anime Filme',
    messageKey: '',
    fallback: ''
  }
);
assert.strictEqual(api.joinMoveFolderPath('', 'Anime'), 'Anime');
assert.strictEqual(api.joinMoveFolderPath('Ghibli', 'Anime'), 'Ghibli/Anime');
assert.strictEqual(
  api.joinMoveFolderPath('Ghibli/Archiv', 'Anime Filme'),
  'Ghibli/Archiv/Anime Filme'
);

console.log('recording move new-folder helpers ok');
