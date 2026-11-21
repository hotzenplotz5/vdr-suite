'use strict';
const assert = require('node:assert/strict');
const {createClient, PublicClientError} = require('../public-v1-client.js');
async function main() {
  const requests=[];
  const fetch=async(url,options)=>{
    requests.push({url,...options});
    return {ok:true,status:url.endsWith('/api/v1/device-pairings')?201:200,
      headers:{get(){return null;}},
      text:async()=>JSON.stringify({items:[]})};
  };
  const origin='https://suite.example';
  const browser=createClient({baseUrl:origin,fetch,credentials:'include',
    headers:{'X-CSRF-Token':'test-csrf-from-caller'}});
  await browser.getAccounts();
  assert.equal(requests[0].url,origin+'/api/v1/accounts');
  assert.equal(requests[0].credentials,'include');
  assert.equal(requests[0].headers['X-CSRF-Token'],'test-csrf-from-caller');
  assert.equal(requests[0].headers.Authorization,undefined);
  const proof='VDR-Suite-Device dv_test.secret';
  const device=createClient({baseUrl:origin,fetch,headers:{Authorization:proof}});
  await device.getCapabilities();
  assert.equal(requests[1].headers.Authorization,proof);
  assert.equal(requests[1].headers['X-CSRF-Token'],undefined);
  const unpaired=createClient({baseUrl:origin,fetch});
  await unpaired.createDevicePairing({displayName:'Test TV',clientKind:'vidaa'});
  assert.equal(requests[2].url,origin+'/api/v1/device-pairings');
  assert.equal(requests[2].method,'POST');
  assert.equal(requests[2].headers.Authorization,undefined);
  assert.equal(JSON.parse(requests[2].body).displayName,'Test TV');
  await unpaired.issueDeviceCredential({pairingRequestId:'pr_test',pairingToken:'opaque-test-token'});
  assert.equal(requests[3].method,'POST');
  assert.equal(requests[3].headers['X-VDR-Suite-Pairing-Token'],'opaque-test-token');
  assert.equal(requests[3].credentials,'omit');
  assert.equal(requests[3].headers.Authorization,undefined);
  let attempts=0;
  const rejecting=createClient({baseUrl:origin,fetch:async()=>{
    attempts++;
    return {ok:false,status:401,headers:{get(){return null;}},
      text:async()=>JSON.stringify({code:'unauthorized',detail:'Login required'})};
  }});
  await assert.rejects(rejecting.getAccounts(),error=>
    error instanceof PublicClientError && error.status===401 &&
    error.code==='unauthorized');
  assert.equal(attempts,1);
  console.log('API_V1_F2_AUTH_TRANSPORT=PASS');
}
main().catch(error=>{console.error(error);process.exitCode=1;});
