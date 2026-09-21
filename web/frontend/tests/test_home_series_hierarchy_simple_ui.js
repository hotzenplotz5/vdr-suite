'use strict';

const assert = require('assert');
const fs = require('fs');
const path = require('path');

const source = fs.readFileSync(
  path.join(
    __dirname,
    '..',
    'home-recording-discovery.js'
  ),
  'utf8'
);

function functionSource(name) {
  const marker = 'function ' + name + '(';
  const start = source.indexOf(marker);

  assert(
    start >= 0,
    name + ' must exist'
  );

  const opening = source.indexOf(
    '{',
    start
  );

  let depth = 0;
  let quote = null;
  let escaped = false;

  for (
    let index = opening;
    index < source.length;
    index += 1
  ) {
    const character = source[index];

    if (quote !== null) {
      if (escaped) {
        escaped = false;
      } else if (character === '\\') {
        escaped = true;
      } else if (character === quote) {
        quote = null;
      }
      continue;
    }

    if (
      character === '"' ||
      character === "'" ||
      character === '`'
    ) {
      quote = character;
      continue;
    }

    if (character === '{') {
      depth += 1;
    } else if (character === '}') {
      depth -= 1;

      if (depth === 0) {
        return source.slice(
          start,
          index + 1
        );
      }
    }
  }

  throw new Error(
    name + ' is unterminated'
  );
}

const requestSource =
  functionSource(
    'requestSeriesHierarchyOverride'
  );

assert(
  requestSource.includes(
    'seriesHierarchyCsrfHeaders()'
  ),
  'hierarchy mutation must use the existing browser CSRF helper'
);

assert(
  source.includes(
    'media-home-series-hierarchy-modal'
  )
);

assert(
  source.includes(
    'min-height:44px'
  ),
  'assignment action must have a real touch/click target'
);

assert(
  source.includes(
    "label === 'Staffel zuordnen'"
  )
);

assert(
  source.includes(
    "label === 'Eigene Gruppe übernehmen'"
  )
);

assert(
  source.includes(
    "heading.textContent =\n          'Aufnahme zuordnen'"
  ) ||
  source.includes(
    "heading.textContent = 'Aufnahme zuordnen'"
  )
);

assert(
  source.includes(
    'Wohin gehört diese Aufnahme?'
  )
);

assert(
  source.includes(
    'simplifySeriesHierarchyEditor();'
  )
);

assert(
  source.includes(
    "field.value = ''"
  ),
  'hidden episode range fields must not silently persist old numbers'
);

console.log(
  'Home Series hierarchy simple-choice UI and CSRF contract ok'
);
