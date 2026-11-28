'use strict';
const assert = require('node:assert/strict');
const api = require('../public-v1-client.js');

const REC='rec_0123456789abcdef0123456789abcdef';
const FLD='fld1_0123456789abcdef0123456789abcdef';
const ART='fedcba9876543210fedcba9876543210';
function reply(status,data) {
  return {ok:status>=200&&status<300,status,
    headers:{get(){return null;}},text:async()=>JSON.stringify(data)};
}
async function run() {
  const calls=[],replies=[];
  const client=api.createClient({baseUrl:'https://suite.example',
    headers:{Authorization:'VDR-Suite-Device test.opaque-secret'},
    fetch:async(url,options)=>{
      calls.push({url,options});
      assert.ok(replies.length,'unexpected fetch');
      const next=replies.shift();
      return typeof next==='function'?next():next;
    }
  });
  replies.push(reply(200,{items:[{recordingId:REC,backendId:'home',
    title:'Der Hobbit',edited:true,durationKnown:true}],
    page:{limit:1,nextCursor:'rc1_aa',hasMore:true},meta:{partial:false}}));
  const flat=await client.getRecordings({
    query:{backendId:'home',limit:1,sort:'recordingId',order:'asc'}
  });
  assert.equal(flat.items[0].title,'Der Hobbit');
  assert.equal(flat.items[0].edited,true);
  assert.equal(calls[0].url,'https://suite.example/api/v1/recordings?backendId=home&limit=1&sort=recordingId&order=asc');

  replies.push(reply(200,{items:[
    {kind:'folder',folderId:FLD,name:'Filme',recordingCount:2},
    {kind:'recording',recordingId:REC,title:'Der Hobbit',edited:true}
  ],page:{limit:2,offset:0,totalCount:3,hasMore:true},meta:{partial:false}}));
  const root=await client.getRecordingFolders({query:{backendId:'home',limit:2,offset:0}});
  assert.equal(root.items[0].folderId,FLD);
  assert.equal(root.items[1].kind,'recording');
  assert.equal(root.page.totalCount,3);
  assert.equal(calls[1].url,'https://suite.example/api/v1/recordings?backendId=home&view=folders&limit=2&offset=0');

  replies.push(reply(200,{items:[{kind:'recording',recordingId:REC,
    title:'Der Hobbit',edited:true}],page:{offset:2,totalCount:3,hasMore:false}}));
  await client.getRecordingFolders({query:{backendId:'home',folderId:FLD,offset:2}});
  assert.equal(calls[2].url,'https://suite.example/api/v1/recordings?backendId=home&view=folders&folderId='+FLD+'&offset=2');

  replies.push(reply(200,{recordingId:REC,backendId:'home',title:'Der Hobbit',
    edited:true,metadata:{availability:'partial'}}));
  const detail=await client.getRecordingDetail({backendId:'home',recordingId:REC});
  assert.equal(detail.title,flat.items[0].title);
  assert.equal(detail.edited,flat.items[0].edited);
  assert.equal(calls[3].url,'https://suite.example/api/v1/recordings/'+REC+'?backendId=home');

  replies.push(reply(200,{items:[{genreId:'fantasy',label:'Fantasy',count:2}],
    page:{offset:0,limit:2,totalCount:2,hasMore:false}}));
  const genres=await client.getRecordingGenres({query:{backendId:'home',locale:'de',limit:2,offset:0}});
  assert.equal(genres.items[0].genreId,'fantasy');
  assert.equal(calls[4].url,'https://suite.example/api/v1/genres?backendId=home&locale=de&limit=2&offset=0');

  replies.push(reply(200,{items:[{recordingId:REC,backendId:'home',
    title:'Der Hobbit',edited:true}],page:{offset:0,totalCount:1,hasMore:false}}));
  const genreRec=await client.getGenreRecordings({query:{backendId:'home',
    genreId:'fantasy',locale:'en',limit:30,offset:0}});
  assert.equal(genreRec.items[0].recordingId,REC);
  assert.equal(genreRec.items[0].title,flat.items[0].title);
  assert.equal(calls[5].url,'https://suite.example/api/v1/genres/recordings?backendId=home&genreId=fantasy&locale=en&limit=30&offset=0');

  replies.push(reply(200,{recordingId:REC,items:[{kind:'poster',artworkId:ART}]}));
  const art=await client.getRecordingArtwork({backendId:'home',recordingId:REC});
  assert.equal(art.items[0].artworkId,ART);
  assert.equal(calls[6].url,'https://suite.example/api/v1/recordings/'+REC+'/artwork?backendId=home');

  replies.push(()=>({ok:true,status:200,headers:{get(k){return k==='Content-Type'?'image/png':null;}},
    arrayBuffer:async()=>Uint8Array.from([137,80,78,71]).buffer}));
  const image=await client.getRecordingArtworkImage({
    backendId:'home',recordingId:REC,artworkId:ART});
  assert.equal(image.contentType,'image/png');
  assert.deepEqual(Array.from(new Uint8Array(image.bytes)),[137,80,78,71]);
  assert.equal(calls[7].url,'https://suite.example/api/v1/recordings/'+REC+'/artwork/'+ART+'?backendId=home');

  replies.push(reply(200,{status:'ambiguous',candidates:[]}));
  const metadata=await client.resolveMetadata({backendId:'home',kind:'series',
    title:'Andor',seasonNumber:2,episodeNumber:4});
  assert.equal(metadata.status,'ambiguous');
  assert.equal(calls[8].options.method,'POST');
  assert.equal(calls[8].url,'https://suite.example/api/v1/backends/home/metadata/resolve');
  assert.equal(JSON.parse(calls[8].options.body).title,'Andor');

  replies.push(reply(200,{items:[]}));
  const noArtwork=await client.getRecordingArtwork({backendId:'home',recordingId:REC});
  assert.deepEqual(noArtwork.items,[]);

  const action={backendId:'home',recordingId:REC,action:'RENAME',newName:'Test',dryRun:false};
  const actionTag='"recording-action-test"';
  replies.push({...reply(200,{valid:true}),headers:{get(name){return name.toLowerCase()==='etag'?actionTag:null;}}});
  const validation=await client.validateRecordingAction({payload:action});
  assert.equal(validation.etag,actionTag);
  assert.equal(calls.at(-1).url,'https://suite.example/api/v1/recordings/actions/validate');
  replies.push(reply(200,{executed:true}));
  const execute=await client.executeRecordingAction({payload:action,ifMatch:validation.etag});
  assert.equal(execute.data.executed,true);
  assert.equal(calls.at(-1).options.headers['If-Match'],actionTag);
  assert.deepEqual(JSON.parse(calls.at(-1).options.body),action);
  for(const [method,operation,payload] of [
    ['searchRecordingMetadata','search',{query:'Hobbit',kind:'movie'}],
    ['getRecordingMetadataSeasons','seasons',{seriesExternalId:'1'}],
    ['getRecordingMetadataEpisodes','episodes',{seriesExternalId:'1',seasonNumber:1}],
    ['assignRecordingMetadata','assign',{recordingId:REC,providerId:'tmdb',externalNamespace:'tmdb',externalId:'1',mediaType:'movie',title:'Hobbit',expectedRevision:0}],
    ['withdrawRecordingMetadata','withdraw',{recordingId:REC,expectedRevision:1}],
    ['setRecordingMetadataGenre','genre',{recordingId:REC,genreId:''}]
  ]) {
    replies.push(reply(200,{ok:true}));
    assert.equal((await client[method]({backendId:'home',payload})).ok,true);
    assert.equal(calls.at(-1).url,'https://suite.example/api/v1/backends/home/recordings/metadata/'+operation);
    assert.deepEqual(JSON.parse(calls.at(-1).options.body),payload);
  }
  for(const [method,operation] of [['getRecordingMetadataTrailer','trailer'],['getRecordingMetadataGenre','genre']]) {
    replies.push(reply(200,{available:false}));
    await client[method]({backendId:'home',recordingId:REC});
    assert.equal(calls.at(-1).url,'https://suite.example/api/v1/backends/home/recordings/metadata/'+operation+'?recordingId='+REC);
  }
  replies.push(reply(200,{items:[],page:{limit:100,offset:0,hasMore:false}}));
  await client.getGenreRecordings({query:{backendId:'home',genreId:'fantasy',limit:100,offset:0}});
  assert.equal(calls.at(-1).url,'https://suite.example/api/v1/genres/recordings?backendId=home&genreId=fantasy&limit=100&offset=0');
  const invalid=[
    ()=>client.getRecordingDetail({backendId:'home',recordingId:'../file'}),
    ()=>client.getRecordingDetail({backendId:'../home',recordingId:REC}),
    ()=>client.getRecordingFolders({query:{backendId:'home',folderId:'../bad'}}),
    ()=>client.getRecordingFolders({query:{backendId:'home',cursor:'rc1_aa'}}),
    ()=>client.getRecordingFolders({query:{backendId:'home',offset:-1}}),
    ()=>client.getRecordingFolders({query:{backendId:'home',limit:101}}),
    ()=>client.getRecordingGenres({query:{backendId:'home',genreId:'fantasy'}}),
    ()=>client.getRecordingGenres({query:{backendId:'home',locale:'es'}}),
    ()=>client.getGenreRecordings({query:{backendId:'home'}}),
    ()=>client.getGenreRecordings({query:{backendId:'home',genreId:'../../x'}}),
    ()=>client.getGenreRecordings({query:{backendId:'home',genreId:'fantasy',limit:101}})
  ];
  const before=calls.length;
  for(const bad of invalid)assert.throws(bad);
  assert.equal(calls.length,before);

  replies.push(reply(404,{status:404,code:'not_found',
    requestId:'req-404',detail:'No artwork'}));
  await assert.rejects(client.getRecordingArtworkImage({
    backendId:'home',recordingId:REC,artworkId:ART}),
    err=>api.isClientError(err)&&err.status===404&&err.code==='not_found'&&
      err.requestId==='req-404');
  assert.equal(calls.length,before+1);
  replies.push(reply(409,{status:409,code:'cursor_expired',detail:'Expired'}));
  await assert.rejects(client.getRecordings({query:{backendId:'home',cursor:'rc1_aabb'}}),
    err=>api.isClientError(err)&&err.status===409&&err.code==='cursor_expired');
  assert.equal(calls.length,before+2);

  assert.equal(replies.length,0);
  for(const call of calls) {
    assert.equal(call.options.headers.Authorization,'VDR-Suite-Device test.opaque-secret');
    assert.ok(call.url.startsWith('https://suite.example/api/v1/'));
    assert.equal(call.url.includes('/api/vdr/'),false);
    assert.equal(call.url.includes('/srv/'),false);
  }
  console.log('API_V1_F4_RECORDING_READS=PASS');
}
run().catch(e=>{console.error(e);process.exitCode=1;});
