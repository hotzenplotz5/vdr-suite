'use strict';

const assert = require('assert').strict;
const {createClient} = require('../public-v1-client.js');

const requests = [];
const timerId = 'timer:abc-12';
const etag = '"native-binding-revision-7"';
const client = createClient({
  baseUrl: 'https://suite.example',
  headers: {Authorization: 'VDR-Suite-Device fixture.credential'},
  fetch: async (url, options) => {
    requests.push({url, options});
    assert.strictEqual(options.method, 'GET');
    assert.strictEqual(options.headers.Authorization,
      'VDR-Suite-Device fixture.credential');
    assert(!url.includes('/api/vdr/'));
    if (url === 'https://suite.example/api/v1/native-timers?backendId=default&offset=0&limit=30') {
      return {ok:true, status:200, text:async()=>JSON.stringify({
        backendId:'default', items:[{timerId, title:'Sample recording',
          directory:'Movies~SciFi',priority:55,lifetime:25,vps:true}],
        page:{offset:0,limit:30,hasMore:false,nextOffset:null},
        meta:{partial:false}
      })};
    }
    if (url === 'https://suite.example/api/v1/native-timers/'+timerId+'?backendId=default') {
      assert.strictEqual(options.headers['If-None-Match'] === undefined ||
        options.headers['If-None-Match'] === etag, true);
      const notModified = options.headers['If-None-Match'] === etag;
      return {ok:!notModified, status:notModified?304:200,
        headers:{get(name) {return name === 'ETag' ? etag : null;}},
        text:async()=>notModified?'':JSON.stringify({
          backendId:'default',timerId,title:'Sample recording',
          directory:'Movies~SciFi',priority:55,lifetime:25,vps:true,
          ownership:'managed',bindingState:'verified',
          links:{self:'/api/v1/native-timers/'+timerId}
        })};
    }
    throw Error('Unexpected GET '+url);
  }
});

(async()=>{
  const list = await client.getNativeTimers({
    query:{backendId:'default',offset:0,limit:30}
  });
  assert.strictEqual(list.items[0].timerId,timerId);
  assert.strictEqual(list.items[0].directory,'Movies~SciFi');
  assert.strictEqual(list.items[0].priority,55);
  assert.strictEqual(list.items[0].lifetime,25);
  assert.strictEqual(list.items[0].vps,true);
  assert.strictEqual(Object.prototype.hasOwnProperty.call(list.items[0],'aux'),false);
  assert.strictEqual(list.page.hasMore,false);
  const first = await client.getNativeTimer({backendId:'default',timerId});
  assert.strictEqual(first.status,200);
  assert.strictEqual(first.etag,etag);
  assert.strictEqual(first.data.bindingState,'verified');
  for(const field of ['directory','priority','lifetime','vps'])
    assert.strictEqual(first.data[field],list.items[0][field]);
  assert.strictEqual(Object.prototype.hasOwnProperty.call(first.data,'aux'),false);
  const cached = await client.getNativeTimer({
    backendId:'default',timerId,ifNoneMatch:first.etag
  });
  assert.strictEqual(cached.status,304);
  assert.strictEqual(cached.data,null);
  assert.strictEqual(requests.length,3);

  assert.throws(()=>client.getNativeTimers({query:{backendId:'default',limit:31}}));
  assert.throws(()=>client.getNativeTimers({query:{backendId:'default',offset:-1}}));
  assert.throws(()=>client.getNativeTimers({query:{backendId:'default',cursor:'wrong'}}));
  assert.throws(()=>client.getNativeTimer({backendId:'default',timerId:'../other'}));
  assert.throws(()=>client.getNativeTimer({backendId:'default',timerId,
    ifNoneMatch:''}));
  assert.strictEqual(requests.length,3,'invalid reads must not dispatch');
  assert(requests.every(r=>r.url.startsWith('https://suite.example/api/v1/native-timers')));
  console.log('Public-v1 F5 Native Timer list/detail conditional client PASS');
})().catch(error=>{console.error(error);process.exitCode=1;});
