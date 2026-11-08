'use strict';
const assert = require('assert');
const {createClient} = require('../public-v1-client.js');

(async () => {
  const calls = [];
  const client = createClient({
    fetch(url, opts) {
      calls.push({url, opts});
      return Promise.resolve({
        ok: true, status: 200,
        headers: {get() { return null; }},
        text() {
          return Promise.resolve(JSON.stringify({
            status: 'matched', mediaType: 'episode',
            series: {kind: 'series', providerId: 'tmdb',
              externalId: '1399', title: 'Star Wars: Andor'},
            episode: {kind: 'episode', providerId: 'tmdb',
              externalId: '911', seasonNumber: 2, episodeNumber: 4}
          }));
        }
      });
    }
  });
  const result = await client.resolveMetadata({
    backendId: 'backend-a', kind: 'series',
    title: 'Star Wars-Andor', seasonNumber: 2, episodeNumber: 4
  });
  assert.strictEqual(result.status, 'matched');
  assert.strictEqual(result.series.title, 'Star Wars: Andor');
  assert.strictEqual(calls.length, 1);
  assert.strictEqual(calls[0].url,
    '/api/v1/backends/backend-a/metadata/resolve');
  assert.strictEqual(calls[0].opts.method, 'POST');
  assert.deepStrictEqual(JSON.parse(calls[0].opts.body), {
    kind: 'series', title: 'Star Wars-Andor',
    seasonNumber: 2, episodeNumber: 4
  });
  assert.throws(() => client.resolveMetadata({
    backendId: 'backend-a', kind: 'series', title: 'Andor',
    seasonNumber: 0, episodeNumber: 4
  }), /season and episode/);
  assert.throws(() => client.resolveMetadata({
    backendId: '../bad', kind: 'movie', title: 'Andor'
  }), /canonical backendId/);
  assert.strictEqual(calls.length, 1,
    'invalid inputs must never hit the network');

  const movie = await client.resolveMetadata({
    backendId: 'backend-a', kind: 'movie', title: 'Forrest Gump', year: 1994
  });
  assert.strictEqual(movie.status, 'matched');
  assert.deepStrictEqual(JSON.parse(calls[1].opts.body), {
    kind: 'movie', title: 'Forrest Gump', year: 1994
  });
  console.log('Public-v1 centralized metadata resolver client: PASS');
})().catch(error => { console.error(error); process.exitCode = 1; });
