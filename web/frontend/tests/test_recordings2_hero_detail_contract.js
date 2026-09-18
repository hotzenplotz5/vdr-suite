'use strict';

const assert = require('assert');
const fs = require('fs');

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
assert(source.includes('if (!root || !recording) return root;'),
  'Hero availability must depend on the Recording detail, not on optional extended metadata');
assert(!source.includes('metadata.available !== true'),
  'missing TVScraper metadata must not suppress the Recording Hero');

assert(source.includes('root.__vdrSuiteRecordingPlaybackOwner'),
  'Hero playback must reuse the canonical recording playback owner');
assert(source.includes('owner.startAtAbsolute(position)'),
  'Hero resume must reuse the canonical absolute-start path');

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
assert(visibility.includes('detail || playbackSurface'),
  'playback must keep the metadata-owned recording panel available for technical facts');
assert(visibility.includes('recordings2-hero-recording-panel'),
  'nested recording panel must retain its explicit layout hook');
assert(visibility.includes('--recordings2-technical-max-width:80rem'),
  'playback and marks must share a bounded technical desktop rail');
assert(visibility.includes('max-width:var(--recordings2-technical-max-width)'),
  'technical playback, facts and marks must stay within the shared maximum width');
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

console.log('recordings2 hero/playback placement contract ok');
