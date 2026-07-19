// Channels 2 platform module: responsive channel/day programme with safe actions.
(function (global) {
  'use strict';

  const state = {active:false, channels:[], visible:[], channel:null, day:new Date(), events:[], event:null, filter:'', sequence:0};
  const platform = () => global.VdrSuitePlatform || null;
  const clientApi = () => platform() && platform().getClientApi ? platform().getClientApi() : global.VdrSuiteClientApi;
  const mount = () => platform() && platform().getMountTarget ? (platform().getMountTarget('channels2') || platform().getMountTarget('detail')) : document.getElementById('detail-data');
  const backendId = () => String(platform() && platform().getSelectedBackendId ? platform().getSelectedBackendId() : 'default') || 'default';
  const value = (o, keys, fallback='') => { for (const k of keys) if (o && o[k] !== undefined && o[k] !== null && o[k] !== '') return o[k]; return fallback; };
  const text = v => String(v === undefined || v === null ? '' : v).trim();
  const list = (data, key) => Array.isArray(data) ? data : data && Array.isArray(data[key]) ? data[key] : data && Array.isArray(data.items) ? data.items : data && Array.isArray(data.results) ? data.results : [];
  const epoch = v => { const n=Number(v); if (Number.isFinite(n)&&n>0) return n>1e11?Math.floor(n/1000):Math.floor(n); const p=Date.parse(String(v||'')); return Number.isFinite(p)?Math.floor(p/1000):0; };
  const channelId = c => text(value(c,['id','channelId','nativeId']));
  const channelName = c => text(value(c,['name','channelName','title','displayName'],'Kanal'));
  const channelNumber = c => Number(value(c,['number','channelNumber','position'],0))||0;
  const channelGroup = c => text(value(c,['group','groupName','channelGroup','bouquet','provider']));
  const eventTitle = e => text(value(e,['title','name','eventTitle'],'Sendung'));
  const eventSubtitle = e => text(value(e,['subtitle','shortText','short_text']));
  const eventDescription = e => text(value(e,['description','longText','long_text','summary']));
  const eventChannelId = e => text(value(e,['channelId','channel','channel_id']));
  const eventStart = e => epoch(value(e,['startTime','start','beginTime'],0));
  const eventEnd = e => { const s=eventStart(e), x=epoch(value(e,['endTime','end','stopTime'],0)), d=Number(value(e,['durationSeconds','duration'],0)); return x>s?x:s+(Number.isFinite(d)&&d>0?d:0); };
  const addText = (el,v) => { el.textContent=String(v); return el; };
  const dayOnly = v => { const d=v instanceof Date?v:new Date(v||Date.now()); return new Date(d.getFullYear(),d.getMonth(),d.getDate()); };
  const moveDay = (v,n) => { const d=dayOnly(v); d.setDate(d.getDate()+n); return d; };
  const dateValue = v => { const d=dayOnly(v); return `${d.getFullYear()}-${String(d.getMonth()+1).padStart(2,'0')}-${String(d.getDate()).padStart(2,'0')}`; };
  const clock = v => new Date(Number(v)*1000).toLocaleTimeString('de-DE',{hour:'2-digit',minute:'2-digit'});
  const hhmm = v => { const d=new Date(Number(v)*1000); return d.getHours()*100+d.getMinutes(); };

  function ensureStylesheet() {
    if (document.querySelector('link[data-channels2-style]')) return;
    const link=document.createElement('link'); link.rel='stylesheet'; link.href='/frontend/channels2.css'; link.dataset.channels2Style='true'; document.head.appendChild(link);
  }

  function timerPayload(event, channel) {
    const start=eventStart(event), end=eventEnd(event), eventId=text(value(event,['eventId','id','nativeId']));
    return {backendId:backendId(),channelId:eventChannelId(event)||channelId(channel),title:eventTitle(event),directory:'',day:dateValue(new Date(start*1000)),weekdays:'-------',start:hhmm(start),stop:hhmm(end),priority:50,lifetime:99,active:true,vps:false,aux:eventId?'eventId='+eventId:''};
  }

  function createTimer(event, channel, feedback, button) {
    const api=clientApi();
    if (!api || typeof api.fetchClientTimerCreateAction!=='function') { feedback.className='channels2-feedback error'; feedback.textContent='Timer-API ist nicht verfügbar.'; return; }
    button.disabled=true; feedback.className='channels2-feedback'; feedback.textContent='Timer wird erstellt …';
    api.fetchClientTimerCreateAction({payload:timerPayload(event,channel),cache:'no-store',credentials:'same-origin'})
      .then(result=>{ if(result&&result.success===false) throw new Error(result.message||result.error||'Aktion wurde abgelehnt.'); button.textContent='Timer erstellt'; feedback.className='channels2-feedback success'; feedback.textContent='Timer für „'+eventTitle(event)+'“ wurde erstellt.'; })
      .catch(error=>{ button.disabled=false; feedback.className='channels2-feedback error'; feedback.textContent='Timer konnte nicht erstellt werden: '+error.message; });
  }

  function ensureEditorApi() {
    if (global.VdrSuiteEpgSearchTimerActions && typeof global.VdrSuiteEpgSearchTimerActions.openSearchTimerEditor==='function') return Promise.resolve(global.VdrSuiteEpgSearchTimerActions);
    const existing=document.querySelector('script[data-channels2-searchtimer-runtime]');
    if (existing) return new Promise((resolve,reject)=>{ existing.addEventListener('load',()=>resolve(global.VdrSuiteEpgSearchTimerActions),{once:true}); existing.addEventListener('error',reject,{once:true}); });
    return new Promise((resolve,reject)=>{ const script=document.createElement('script'); script.src='/frontend/epg-searchtimer-actions.js'; script.async=false; script.dataset.channels2SearchtimerRuntime='true'; script.onload=()=>resolve(global.VdrSuiteEpgSearchTimerActions); script.onerror=reject; document.head.appendChild(script); });
  }

  function prepareSearchTimer(event, channel, detail) {
    const feedback=detail.querySelector('.channels2-feedback'); feedback.className='channels2-feedback'; feedback.textContent='SearchTimer-Editor wird geöffnet …';
    ensureEditorApi().then(editor=>{
      if (!editor || typeof editor.openSearchTimerEditor!=='function') throw new Error('SearchTimer-Editor ist nicht verfügbar.');
      return editor.openSearchTimerEditor({title:eventTitle(event),channelId:eventChannelId(event)||channelId(channel),channelGroup:channelGroup(channel),statusTarget:detail});
    }).catch(error=>{ feedback.className='channels2-feedback error'; feedback.textContent=error.message; });
  }

  function filterChannels() {
    const q=state.filter.toLocaleLowerCase('de-DE');
    state.visible=state.channels.filter(c=>!q||channelName(c).toLocaleLowerCase('de-DE').includes(q)||channelGroup(c).toLocaleLowerCase('de-DE').includes(q)||String(channelNumber(c)).includes(q));
  }

  function renderDetail(parent) {
    if (!state.event || !state.channel) return;
    const detail=document.createElement('article'); detail.className='channels2-detail';
    const header=document.createElement('header'); header.className='channels2-detail-header';
    header.appendChild(addText(document.createElement('h3'),eventTitle(state.event)));
    if(eventSubtitle(state.event)){ const p=addText(document.createElement('p'),eventSubtitle(state.event)); p.className='channels2-detail-subtitle'; header.appendChild(p); }
    const meta=addText(document.createElement('p'),clock(eventStart(state.event))+'–'+clock(eventEnd(state.event))+' · '+channelName(state.channel)); meta.className='channels2-detail-time'; header.appendChild(meta); detail.appendChild(header);
    const description=addText(document.createElement('p'),eventDescription(state.event)||'Keine Beschreibung vorhanden.'); description.className='channels2-description'; detail.appendChild(description);
    const actions=document.createElement('div'); actions.className='channels2-actions';
    const timer=addText(document.createElement('button'),'Timer erstellen'); timer.type='button'; timer.className='channels2-primary-action';
    const series=addText(document.createElement('button'),'Serientimer vorbereiten'); series.type='button'; series.className='channels2-secondary-action';
    const feedback=document.createElement('p'); feedback.className='channels2-feedback'; feedback.setAttribute('role','status'); feedback.setAttribute('aria-live','polite');
    timer.addEventListener('click',()=>createTimer(state.event,state.channel,feedback,timer)); series.addEventListener('click',()=>prepareSearchTimer(state.event,state.channel,detail));
    actions.append(timer,series); detail.append(actions,feedback); parent.appendChild(detail);
  }

  function render() {
    ensureStylesheet(); const target=mount(); if(!target) return; target.replaceChildren();
    const root=document.createElement('section'); root.className='channels2';
    const toolbar=document.createElement('header'); toolbar.className='channels2-toolbar';
    const intro=document.createElement('div'); intro.append(addText(document.createElement('h3'),'Channels 2'),addText(document.createElement('p'),'Kanal auswählen, Tagesprogramm prüfen und Timer sicher vorbereiten.'));
    const tools=document.createElement('div'); tools.className='channels2-tools'; const search=document.createElement('input'); search.type='search'; search.className='channels2-search'; search.placeholder='Kanal, Nummer oder Gruppe suchen'; search.value=state.filter; const reload=addText(document.createElement('button'),'Neu laden'); reload.type='button'; tools.append(search,reload); toolbar.append(intro,tools); root.appendChild(toolbar);
    const status=addText(document.createElement('p'),state.channels.length+' Kanäle geladen.'); status.className='channels2-status'; root.appendChild(status);
    const grid=document.createElement('div'); grid.className='channels2-grid'; const channelList=document.createElement('section'); channelList.className='channels2-list'; const program=document.createElement('section'); program.className='channels2-program'; grid.append(channelList,program); root.appendChild(grid); target.appendChild(root);

    const renderChannels=()=>{ channelList.replaceChildren(); state.visible.forEach(channel=>{ const button=document.createElement('button'); button.type='button'; button.className='channels2-channel'; if(state.channel&&channelId(state.channel)===channelId(channel)) button.classList.add('active'); const logo=document.createElement('span'); logo.className='channels2-logo'; if(typeof global.createChannelLogoElement==='function') logo.appendChild(global.createChannelLogoElement(channelName(channel),channelId(channel))); else logo.textContent=String(channelNumber(channel)||'•'); const copy=document.createElement('span'); const title=addText(document.createElement('span'),channelName(channel)); title.className='channels2-title'; const meta=addText(document.createElement('span'),'Nr. '+(channelNumber(channel)||'-')+(channelGroup(channel)?' · '+channelGroup(channel):'')); meta.className='channels2-meta'; copy.append(title,meta); button.append(logo,copy); button.addEventListener('click',()=>{state.channel=channel;state.event=null;render();loadEvents();}); channelList.appendChild(button); }); };
    const renderProgram=()=>{ program.replaceChildren(); if(!state.channel){const empty=addText(document.createElement('div'),'Einen Kanal auswählen.');empty.className='channels2-empty';program.appendChild(empty);return;} const head=document.createElement('article');head.className='channels2-head';const copy=document.createElement('div');copy.append(addText(document.createElement('h3'),channelName(state.channel)),addText(document.createElement('p'),'Kanal '+(channelNumber(state.channel)||'-')+(channelGroup(state.channel)?' · '+channelGroup(state.channel):'')));const controls=document.createElement('div');controls.className='channels2-date';const prev=addText(document.createElement('button'),'←'),today=addText(document.createElement('button'),'Heute'),next=addText(document.createElement('button'),'→');prev.type=today.type=next.type='button';const input=document.createElement('input');input.type='date';input.value=dateValue(state.day);prev.onclick=()=>{state.day=moveDay(state.day,-1);loadEvents();};today.onclick=()=>{state.day=dayOnly(new Date());loadEvents();};next.onclick=()=>{state.day=moveDay(state.day,1);loadEvents();};input.onchange=()=>{state.day=dayOnly(input.value);loadEvents();};controls.append(prev,today,next,input);head.append(copy,controls);program.appendChild(head);const heading=addText(document.createElement('h3'),dayOnly(state.day).toLocaleDateString('de-DE',{weekday:'long',day:'2-digit',month:'long',year:'numeric'}));heading.className='channels2-day-heading';program.appendChild(heading);const events=document.createElement('section');events.className='channels2-events';const now=Math.floor(Date.now()/1000);if(!state.events.length){const empty=addText(document.createElement('div'),'Für diesen Tag wurden keine EPG-Einträge gefunden.');empty.className='channels2-empty';events.appendChild(empty);}state.events.forEach(event=>{const button=document.createElement('button');button.type='button';button.className='channels2-event';if(eventStart(event)<=now&&eventEnd(event)>now)button.classList.add('current');if(state.event===event)button.classList.add('active');const time=addText(document.createElement('span'),clock(eventStart(event))+'–'+clock(eventEnd(event)));time.className='channels2-time';const copy=document.createElement('span');const title=addText(document.createElement('span'),eventTitle(event));title.className='channels2-title';copy.appendChild(title);if(eventSubtitle(event)){const sub=addText(document.createElement('span'),eventSubtitle(event));sub.className='channels2-meta';copy.appendChild(sub);}const badge=addText(document.createElement('span'),eventStart(event)<=now&&eventEnd(event)>now?'Jetzt':'EPG');badge.className='channels2-badge';button.append(time,copy,badge);button.onclick=()=>{state.event=event;render();};events.appendChild(button);});program.appendChild(events);renderDetail(program);};
    search.oninput=()=>{state.filter=search.value.trim();filterChannels();renderChannels();status.textContent=state.visible.length+' von '+state.channels.length+' Kanälen sichtbar.';}; reload.onclick=loadChannels; renderChannels(); renderProgram();
  }

  function loadEvents() {
    const api=clientApi(); if(!api||!state.channel){render();return;} const start=dayOnly(state.day),end=moveDay(start,1),range={start:Math.floor(start.getTime()/1000),end:Math.floor(end.getTime()/1000)},sequence=++state.sequence;
    const cached=typeof api.fetchClientEpgCacheWindow==='function'?api.fetchClientEpgCacheWindow({query:{backend:backendId(),channelId:channelId(state.channel),fromTime:String(range.start),untilTime:String(range.end),limit:'0',_:String(Date.now())},cache:'no-store',credentials:'same-origin'}).catch(()=>({events:[]})):Promise.resolve({events:[]});
    cached.then(data=>list(data,'events').length||typeof api.fetchClientEpgChannelWindow!=='function'?data:api.fetchClientEpgChannelWindow({query:{channelId:channelId(state.channel),from:String(range.start),timespan:String(range.end-range.start),limit:'192',_:String(Date.now())},cache:'no-store',credentials:'same-origin'})).then(data=>{if(!state.active||sequence!==state.sequence)return;state.events=list(data,'events').filter(event=>{const id=eventChannelId(event);return(!id||id===channelId(state.channel))&&eventStart(event)<range.end&&eventEnd(event)>range.start;}).sort((a,b)=>eventStart(a)-eventStart(b));state.event=null;render();}).catch(error=>{const target=mount();if(target&&state.active){const message=addText(document.createElement('p'),'Tagesprogramm konnte nicht geladen werden: '+error.message);message.className='channels2-status error';target.prepend(message);}});
  }

  function loadChannels() {
    const api=clientApi(),target=mount(); if(!target)return; target.replaceChildren(); const loading=addText(document.createElement('p'),'Lade Channels 2 …');loading.className='channels2-status';target.appendChild(loading);if(!api||typeof api.fetchClientChannels!=='function'){loading.className='channels2-status error';loading.textContent='Kanäle konnten nicht geladen werden: Client API ist nicht verfügbar.';return;}
    api.fetchClientChannels({query:{backend:backendId(),_:String(Date.now())},cache:'no-store',credentials:'same-origin'}).then(data=>{if(!state.active)return;state.channels=list(data,'channels').map(c=>Object.assign({},c,{id:channelId(c),name:channelName(c),number:channelNumber(c),group:channelGroup(c)})).sort((a,b)=>channelNumber(a)-channelNumber(b)||channelName(a).localeCompare(channelName(b),'de-DE'));filterChannels();if(!state.channel&&state.channels.length)state.channel=state.channels[0];render();if(state.channel)loadEvents();}).catch(error=>{if(state.active){loading.className='channels2-status error';loading.textContent='Kanäle konnten nicht geladen werden: '+error.message;}});
  }

  const moduleApi=Object.freeze({activate(){state.active=true;loadChannels();},deactivate(){state.active=false;state.sequence+=1;},refresh(){if(state.active)loadChannels();}});
  const runtime=platform(); if(runtime&&typeof runtime.registerModule==='function'&&(!runtime.hasModule||!runtime.hasModule('channels2')))runtime.registerModule('channels2',moduleApi); global.VdrSuiteChannels2=moduleApi;
}(window));
