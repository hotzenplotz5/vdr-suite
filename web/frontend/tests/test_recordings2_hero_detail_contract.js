'use strict';

const assert = require('assert');
const fs = require('fs');
const vm = require('vm');

const source = fs.readFileSync('web/frontend/recordings2-hero-detail.js', 'utf8');
const visibility = fs.readFileSync('web/frontend/recordings2-hero-visibility.js', 'utf8');
const metadataDetail = fs.readFileSync('web/frontend/recordings2-metadata-detail.js', 'utf8');
const packaging = fs.readFileSync('mk/recordings2.mk', 'utf8');

assert(source.includes('global.VdrSuiteRecordings2HeroDetail = Object.freeze'),
  'hero detail runtime must export a stable owner');
assert(source.includes('position:fixed;inset:0;z-index:1200'),
  'hero detail must render as its own full-page surface');
assert(source.includes("root.dataset.recordings2HeroMode = 'detail'"),
  'recording detail must open in hero detail mode first');

assert(source.includes('recordings2-hero-facts'),
  'rating/release/genre/duration/size must be integrated into the Hero styling');
['Bewertung', 'Veröffentlichung', 'Genre', 'Dauer', 'Größe'].forEach(function (label) {
  assert(source.includes("'" + label + "'"), 'Hero fact missing: ' + label);
});
assert(!source.includes("'Metadatenquelle'"),
  'metadata source must not be rendered as a Hero fact');

assert(source.includes("'▶ Abspielen'"),
  'Hero must expose primary playback');
assert(source.includes("'▶ Wiedergabe fortsetzen'"),
  'Hero must support resume labelling when a resume position exists');
const enhanceStart = source.indexOf(
  'function enhance(root, recording, backendId, metadata) {'
);
const enhanceEnd = source.indexOf(
  '\n  global.VdrSuiteRecordings2HeroDetail = Object.freeze',
  enhanceStart
);
assert(enhanceStart >= 0 && enhanceEnd > enhanceStart,
  'Hero enhance owner must remain discoverable');
const enhanceSource = source.slice(enhanceStart, enhanceEnd);
assert(enhanceSource.includes('if (!root || !recording) return root;'),
  'Hero availability must depend on the Recording detail, not on optional extended metadata');
assert(!enhanceSource.includes('metadata.available !== true'),
  'missing TVScraper metadata must not suppress the Recording Hero');
assert(source.includes('if (!metadata || metadata.available !== true) return null;'),
  'Trailer availability may depend on optional extended metadata without suppressing the Hero');

assert(source.includes('root.__vdrSuiteRecordingPlaybackOwner'),
  'Hero playback must reuse the canonical recording playback owner');
assert(source.includes('owner.startAtAbsolute(position)'),
  'Hero resume must reuse the canonical absolute-start path');

assert(source.includes("makeButton('▶ Trailer'"),
  'Hero must expose Trailer only through the existing action owner');
assert(source.includes('api.fetchClientRecordingTrailer({'),
  'Trailer lookup must use the Web Client API owner');
assert(source.includes('https://www.youtube-nocookie.com/embed/'),
  'YouTube trailers must use the privacy-enhanced embed origin');
assert(source.includes('?autoplay=0&rel=0'),
  'Trailer iframe must explicitly disable autoplay');
assert(source.includes('.recordings2-hero-trailer-section{position:relative;z-index:2;display:grid'),
  'Trailer must use the normal Hero document flow below related recordings');
assert(!source.includes('enablejsapi'),
  'Trailer embed must not attach YouTube to the Recording MediaSession owner');
assert(!source.includes('api.themoviedb.org'),
  'Hero must never call TMDB directly');
assert(!source.includes('VDR_SUITE_TMDB_READ_ACCESS_TOKEN'),
  'provider credentials must never enter the browser runtime');
assert(source.includes("iframe.allow = 'encrypted-media; picture-in-picture; fullscreen'"),
  'Trailer embed permissions must omit autoplay');
assert(source.includes("section.remove()"),
  'closing the inline Trailer must remove the iframe and terminate its media');
assert(source.includes("'.recordings2-hero-trailer-section'"),
  'Trailer must render as an inline Hero section');
assert(!source.includes("recordings2-hero-trailer-overlay"),
  'Trailer must not render as a modal overlay');
assert(!source.includes("aria-modal"),
  'inline Trailer must not claim modal semantics');
assert(source.includes('placeTrailerAfterRelated(root);'),
  'Trailer placement must be repaired after asynchronous related-rail updates');
assert(source.includes("root.insertBefore(section, related.nextSibling)"),
  'inline Trailer must be placed directly after the related-film section');
assert(source.includes("scrollIntoView({behavior: 'smooth', block: 'start'})"),
  'Trailer action must scroll the inline section into view');

assert(!source.includes("makeButton('Schnittmarken'"),
  'cut marks must not be a Hero action');
assert(!source.includes("makeButton('Schneiden'"),
  'cutting must not be a Hero action');
assert(source.includes("makeButton('Aufnahmeaktionen'"),
  'Hero must expose the existing Recording actions panel');
assert(source.includes("showMode(root, 'actions', '.recordings2-actions')"),
  'Recording actions must open through the Hero mode owner');
assert(source.includes("['detail', 'playback', 'marks', 'metadata', 'actions']"),
  'Hero mode contract must include Recording actions');

assert(visibility.includes("'.recordings2-volume-owner-shell', !playbackSurface"),
  'detail mode must hide the complete decorated playback shell');
assert(visibility.includes("'.recordings2-detail-grid', !(playbackSurface || metadata)"),
  'legacy technical detail cards must not appear in the Hero');
assert(visibility.includes("'.recordings2-marks-detail', !playbackSurface"),
  'cut/marks tooling must belong to the playback surface');
assert(visibility.includes("const actions = mode === 'actions';"),
  'Hero visibility owner must model Recording actions explicitly');
assert(visibility.includes("setHidden(root, '.recordings2-actions', !actions, 'block')"),
  'Recording actions must be visible only in the dedicated Hero action mode');
assert(visibility.includes("setHidden(root, '.recordings2-metadata-tabs', !metadata)"),
  'metadata tabs must stay hidden in the normal Hero and appear only in metadata mode');
assert(source.includes("showMode(root, 'metadata', '.recordings2-metadata-tabs')"),
  'the Hero Metadata action must open the canonical metadata mode');
assert(visibility.includes('data-recordings2-hero-mode="metadata"]>.recordings2-metadata-tabs{position:relative'),
  'metadata tabs must render in normal flow inside metadata mode');
assert(visibility.includes('max-width:80rem;margin:1rem auto 3rem!important'),
  'metadata tabs must live at the bottom of the bounded metadata content rail');
assert(visibility.includes('recordings2-metadata-panel{position:relative;z-index:2;width:calc(100% - clamp(2rem,6vw,6rem));max-width:80rem;margin:5.5rem auto 0!important'),
  'metadata content must begin below the fixed Details action while tabs remain below the content');
assert(!visibility.includes('selectRecordingMetadataTab(root)'),
  'Hero visibility must not take ownership of metadata tab selection');
assert(!visibility.includes('__vdrSuiteHeroModeBound'),
  'Hero visibility must not add a second metadata-tab click owner');
assert(visibility.includes('detail || playbackSurface'),
  'playback must keep the metadata-owned recording panel available for technical facts');
assert(visibility.includes('recordings2-hero-recording-panel'),
  'nested recording panel must retain its explicit layout hook');
assert(visibility.includes('--recordings2-technical-max-width:80rem'),
  'playback and marks must share a bounded technical desktop rail');
assert(visibility.includes('max-width:var(--recordings2-technical-max-width)'),
  'technical playback, facts and marks must stay within the shared maximum width');
assert(visibility.includes('display:grid;grid-template-columns:minmax(0,1fr);justify-items:stretch;width:calc(100% - var(--recordings2-technical-inline-space))'),
  'the outer playback owner shell must stretch the complete technical rail');
assert(visibility.includes('.recordings2-hero-page .recordings2-track-owner-shell'),
  'the real nested track owner must participate in the full-width playback contract');
assert(visibility.includes('.recordings2-hero-page .recordings2-recording-fallback-shell'),
  'fallback Recording playback must share the full-width playback contract');
assert(visibility.includes('.recordings2-hero-page .recordings2-playback-controls'),
  'canonical Recording controls must explicitly inherit the full technical width');
assert(visibility.includes('grid-template-columns:repeat(auto-fit,minmax(6.5rem,1fr))!important'),
  'transport controls must distribute across the available width instead of clustering left');
assert(visibility.includes('.recordings2-hero-page .recordings2-playback-transport>button{width:100%;min-width:0}'),
  'every Recording transport button must fill its responsive grid cell');
assert(!visibility.includes('>.recordings2-volume-owner-shell>.recordings2-playback'),
  'Hero playback layout must not assume that playback is a direct child of the volume shell');
assert(visibility.includes('.recordings2-volume-owner-shell .recordings2-marks-detail{margin:1rem 0 0}'),
  'marks moved beside the canonical playback timeline must not keep a second viewport gutter');
assert(visibility.includes('overflow:hidden'),
  'Hero backdrop must be clipped to prevent an image seam below the Hero');
assert(visibility.includes('.recordings2-detail-copy>.recordings2-hero-actions{display:contents}'),
  'Hero action wrapper must allow primary and secondary actions to be positioned independently');
assert(visibility.includes('.recordings2-detail-copy>.recordings2-hero-actions>button.primary{order:4}'),
  'primary playback must stay immediately after the compact film facts');
assert(visibility.includes('.recordings2-detail-copy>.recordings2-detail-description{order:5}'),
  'long description must follow primary playback');
assert(visibility.includes('.recordings2-detail-copy>.recordings2-hero-cast{order:6}'),
  'cast must remain below the description');
assert(visibility.includes('.recordings2-detail-copy>.recordings2-hero-actions>button:not(.primary){order:7'),
  'secondary metadata action must move below description and cast');
assert(visibility.includes('syncHeroFactsFromDetail(root)'),
  'Hero must recover missing compact facts from the canonical recording detail fields');
assert(visibility.includes("elements(root, '.recordings2-detail-field')"),
  'Hero fallback must reuse the already-rendered recording detail truth');
assert(visibility.includes("['Bewertung', 'Veröffentlichung', 'Genre', 'Dauer', 'Größe']"),
  'Hero fallback must preserve all requested compact facts');
assert(visibility.includes('new global.MutationObserver'),
  'visibility wiring must react to async DOM changes');

assert(source.includes("entry.orientation === 'landscape' || entry.orientation === 'banner'"),
  'hero backdrop must prefer landscape/banner metadata artwork');
assert(source.includes('personOwner.findRecordings(actor, backendId, 20)'),
  'related rail must reuse the canonical Person Search owner first');
assert(source.includes('fetchClientGenres'),
  'related rail must be able to discover recording genres as fallback');
assert(source.includes('fetchClientGenreRecordings'),
  'related rail must load same-genre recordings as fallback');
assert(source.includes("scope: 'recordings'"),
  'genre fallback must stay scoped to local recordings');
assert(source.includes("'Mehr aus ' + text(genre.label || genre.id)"),
  'genre fallback rail must identify the matched genre');
assert(source.includes("shared.node('button', 'recordings2-hero-person')"),
  'Hero cast entries must be keyboard-accessible buttons');
assert(source.includes('.recordings2-hero-page button.recordings2-hero-person{'),
  'Hero cast buttons must override the generic blue Recordings2 button skin');
assert(source.includes('.recordings2-hero-person-role{color:#b8bec8!important}'),
  'Hero cast role label must stay neutral instead of blue');
assert(source.includes('.recordings2-hero-page button.recordings2-hero-related-card{'),
  'Hero related cards must override the generic blue Recordings2 button skin');
assert(source.includes('.recordings2-hero-page button.recordings2-hero-related-back{'),
  'Hero related back action must use the transparent Hero button skin');
assert(source.includes('border-color:rgba(255,255,255,.28)'),
  'Hero related hover treatment must remain neutral rather than blue');
assert(source.includes("person.name + ' in vorhandenen Aufnahmen suchen'"),
  'Hero cast buttons must expose person-search intent');
assert(source.includes('owner.roleLabel(person && person.role)'),
  'Hero cast must reuse the canonical translated person role');
assert(source.includes('personOwner.findRecordings(person, backendId, 20)'),
  'Hero cast search must reuse the canonical Person Search owner');
assert(!source.includes('api.fetchClientRecordingPersons'),
  'Hero must not implement a second direct person-recording API query');
assert(source.includes("'Keine weitere vorhandene Aufnahme mit dieser Person gefunden.'"),
  'Hero person search must have an explicit empty state');
assert(source.includes("'← Zurück'"),
  'Hero person search must provide local back navigation');
assert(source.includes('__vdrSuiteHeroRelatedGeneration'),
  'Hero person search must guard stale async related-result updates');
assert(source.includes('!sameRecording(candidate, recording)'),
  'current recording must be excluded from related recordings');
assert(source.includes('runtime.openRecording(recording'),
  'related cards must reuse Recordings 2 navigation');

assert(metadataDetail.includes('heroDetail.enhance(root, recording, backendId, presented)'),
  'canonical metadata detail must activate the hero enhancer');
assert(packaging.includes('web/frontend/recordings2-hero-detail.js'),
  'hero detail runtime must be bundled');
assert(packaging.includes('web/frontend/recordings2-hero-visibility.js'),
  'hero visibility runtime must be bundled');
assert(packaging.includes('node --check web/frontend/recordings2-hero-visibility.js'),
  'hero visibility runtime must be syntax checked');

const trailerContext = vm.createContext({
  window: {
    VdrSuiteRecordings2Shared: {
      text(value) {
        return value === undefined || value === null ? '' : String(value);
      }
    }
  },
  console
});
vm.runInContext(source, trailerContext, {filename: 'recordings2-hero-detail.js'});
const trailerRuntime = trailerContext.window.VdrSuiteRecordings2HeroDetail;
assert(trailerRuntime && trailerRuntime.__test);
const trailerIdentity = trailerRuntime.__test.trailerIdentity;

assert.deepStrictEqual(
  JSON.parse(JSON.stringify(trailerIdentity({
    available: true,
    manualAssignment: {
      active: true,
      providerId: 'tmdb',
      externalNamespace: 'movie',
      externalId: '11120'
    }
  }))),
  {mediaType: 'movie', externalId: '11120'}
);
assert.deepStrictEqual(
  JSON.parse(JSON.stringify(trailerIdentity({
    available: true,
    manualAssignment: {
      active: true,
      providerId: 'tmdb',
      externalNamespace: 'tv',
      externalId: '19885'
    }
  }))),
  {mediaType: 'series', externalId: '19885'}
);
assert.strictEqual(trailerIdentity({
  available: true,
  manualAssignment: {
    active: true,
    providerId: 'tmdb',
    externalNamespace: 'tv-episode',
    externalId: '123'
  }
}), null, 'episode assignments must not guess a parent TMDB series identity');
assert.deepStrictEqual(
  JSON.parse(JSON.stringify(trailerIdentity({
    available: true,
    provider: 'tvscraper',
    mediaType: 'movie',
    providerId: 11120
  }))),
  {mediaType: 'movie', externalId: '11120'}
);
assert.deepStrictEqual(
  JSON.parse(JSON.stringify(trailerIdentity({
    available: true,
    provider: 'tvscraper',
    mediaType: 'episode',
    providerId: 19885
  }))),
  {mediaType: 'series', externalId: '19885'}
);
assert.strictEqual(trailerIdentity({
  available: true,
  provider: 'tvscraper',
  mediaType: 'series',
  providerId: -74205
}), null, 'negative TVDB identity must not be sent to TMDB');
assert.strictEqual(trailerRuntime.__test.validYoutubeVideoId('abcdefghijk'), true);
assert.strictEqual(trailerRuntime.__test.validYoutubeVideoId('bad'), false);

console.log('recordings2 hero/playback placement contract ok');
