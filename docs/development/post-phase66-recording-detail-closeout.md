# Post-Phase-66 Recording Detail Closeout

Status: **COMPLETED / REAL-SYSTEM ACCEPTED / MERGED TO `main`.**

This document records the non-numbered Recordings 2 detail/presentation work that followed the completed Phase-66 Home/Browse phase. It does not reopen Phase 66 and does not start a new numbered phase.

## Scope

The accepted work modernizes the existing Recording detail surface while preserving the established Recording identity, metadata, marks and playback owners.

It covers:

- the cinematic/full-page Recordings 2 detail presentation;
- compact facts and prominent playback entry;
- restored Recording actions through the existing action owner;
- canonical Recording-cache metadata fallback for detail presentation where richer TVScraper metadata is unavailable;
- cast/person presentation with clickable local person search;
- related Recording rails that reuse normal Recording navigation;
- existing metadata/marks/playback modes inside the same canonical detail owner;
- a usable metadata tab bar inside Metadata mode, including mobile 2 x 2 layout and bottom placement;
- canonical-owner playback prewarm without autoplay or duplicate MediaSessions;
- the follow-up Recording poster-selection correction used by related-Genre cards;
- explicit, on-demand Trailer lookup through the existing Suite/TMDB metadata provider path;
- privacy-enhanced YouTube playback rendered inline below related Recording cards.

It does **not** create a second Recording domain, a second playback owner, a new metadata authority, a parallel History/Continue-Watching path or a browser-side TMDB credential path.

## Accepted runtime identity

The primary Recording-detail work was merged through PR #287:

```text
recording_detail_branch=work/post-phase66-recording-hero-detail-v2
recording_detail_head=72357090f7ebdbe4032d0188e952cee6e33759a2
recording_detail_pr=287
recording_detail_merge=44c73ed44d4a48c0a76e7886bb5100493fb978aa
```

The portrait-cover follow-up was merged through PR #288:

```text
recording_poster_fix_branch=fix/recording-genre-rail-portrait-covers
recording_poster_fix_head=325bb7b4889f8899a43fd7ecd2695d356c7f9710
recording_poster_fix_pr=288
recording_poster_fix_merge=d242f8fa8b583df5b86c1d39fc045abd039bc972
```

The later accepted Recording-detail hardening chain is:

```text
PR #294
branch=work/recording-detail-actions-prewarm-fix
head=d3a79deca14d70423d6481853e38f91acf917fc8
merge=4c75c687677374ec98a6c4ef382e308155219240

PR #295
branch=work/recording-detail-ux-layout
head=17631492bd0737853144bb556150bdf29303b2e0
merge=d5c8a1e8cd4b9dfa4a831b56c228ca8770b49596

PR #296
branch=work/recording-detail-clickable-cast
head=d4c7c0bce6ab518032ea28a895767a0fb48d05d9
merge=5fca6c31f62f72a6457b467a2b4ced8047ac3f51

PR #297
branch=work/recording-detail-metadata-tabs-reachability
head=e9f3f4a2bcf50228d2c6a537dd2c913f342dc61f
merge=dc1639f1783fad9574574545d92c1e4d33a79b7e

PR #298
branch=work/recording-detail-trailer-youtube
head=2ff103796bd493cb7576f69a707b3524525994b5
merge=2b0d0990974244eac90e9be711843678c77e301d
```

PR #298 is the latest accepted Recording-detail runtime checkpoint captured by this closeout. Live `main` must still be queried before repository-state actions.

## Product result

The accepted Recording detail now provides:

- a full-page cinematic Hero presentation for the selected Recording;
- retained canonical Recording title, subtitle, description, poster/backdrop and metadata identity;
- compact factual metadata in the Hero instead of pushing all information into a separate technical panel;
- a prominent playback action that enters the existing canonical Recording playback owner;
- the existing Recording actions workflow from the Hero without introducing a second mutation path;
- cast/person presentation using existing person metadata;
- clickable cast entries that search existing local recordings through the canonical person-search owner;
- related Recording cards that open normal Recording detail routes rather than creating a parallel detail implementation;
- metadata, marks and playback submodes that remain attached to the same Recordings 2 detail owner;
- the existing Metadata view with `Aufnahme | Scraper | Schauspieler | Bilder` presented as usable bottom navigation, including a mobile 2 x 2 layout;
- Recording-cache metadata/artwork fallback for detail presentation when richer metadata is unavailable;
- conditional `Trailer` action when a stable TMDB identity is available;
- server-side TMDB trailer resolution only after explicit Trailer selection;
- privacy-enhanced YouTube embedding through `youtube-nocookie.com`;
- an inline Trailer section below the related-film rail rather than a modal overlay;
- desktop and mobile presentation behavior validated against the real installed frontend.

The implementation remains modularized through the installed Recordings 2 runtime (`recordings2-hero-detail.js`, `recordings2-hero-prewarm.js` and `recordings2-hero-visibility.js`) plus existing metadata, Client API and playback modules. It does not replace the established Recordings 2 controller or playback runtime.

## Playback prewarm boundary

The Hero may prepare the existing canonical playback owner before the user presses Play so startup can be effectively immediate.

The prewarm contract is deliberately narrow:

```text
Recording detail Hero visible
  -> prepare canonical Recording playback owner/session path
  -> preserve autoPlay=false through compatibility/fallback decorators
  -> buffer/prepare only
  -> no audible or advancing playback
  -> no second playback owner
  -> no duplicate MediaSession
  -> no Continue Watching/history truth from prewarm alone
  -> explicit user Play enters normal playback lifecycle
```

PR #294 restored the intended paused-prewarm behavior after a compatibility path had lost the explicit non-autoplay option. PR #297 hardened the same contract across the fallback-controls and restart/seek decorators so `autoPlay:false` reaches the inner HLS owner unchanged.

The Continue Watching synchronization still excludes the `starting` state, so preparing the playback path does not falsely publish viewing progress before real playback begins.

## Metadata access and fallback boundary

PR #295 restored the accepted Recording-cache metadata fallback for the detail path and kept cached artwork behind the existing opaque/authenticated Recording artwork route.

This means:

- the Hero remains available when optional extended TVScraper metadata is missing;
- persisted Recording-cache metadata may enrich the selected Recording detail;
- cached provider identity can be retained where already persisted;
- no local artwork path becomes public;
- detail enrichment remains scoped to the selected Recording and does not restore Home-wide metadata fan-out.

The technical playback/marks surface was also re-centered without changing playback ownership.

## Cast and local person-search boundary

PR #296 made cast entries real accessible buttons and reused the existing person-search owner.

The accepted behavior is:

- cast clicks query existing local recordings only;
- the existing `VdrSuiteRecordings2PersonSearchView` remains the search owner;
- the current Recording is filtered from related results;
- related Recording navigation uses the existing Recordings 2 open route;
- no second person-search API or content graph is introduced;
- cast and related-film buttons use neutral/transparent Hero styling rather than inheriting the generic blue Recordings 2 button skin.

## Metadata navigation boundary

PR #297 keeps the metadata tabs inside Metadata mode instead of exposing them permanently over the cinematic Hero.

Accepted behavior:

- normal Hero detail does not show the metadata tab bar;
- explicit `Metadaten` opens the existing Metadata mode;
- `Aufnahme | Scraper | Schauspieler | Bilder` remains owned by the existing metadata view;
- the tab bar is placed after metadata/assignment content;
- desktop uses a four-button row;
- mobile reflows to a usable 2 x 2 grid;
- choosing a tab brings its selected panel into view;
- no second Hero-owned metadata-tab click controller exists.

## Trailer boundary

PR #298 adds Trailer support as an explicit user action, not as automatic metadata loading.

The accepted flow is:

```text
Recording Hero
  -> stable TMDB movie/series identity already present in Suite metadata
  -> user presses Trailer
  -> Web Client API owner calls backend-scoped Suite route
  -> server-side TMDB provider queries /movie|tv/{id}/videos
  -> Suite returns bounded provider-neutral Trailer descriptor
  -> browser embeds youtube-nocookie.com
  -> autoplay remains disabled
  -> Trailer renders inline below related Recording cards
```

Important boundaries:

- normal Hero/metadata rendering performs no TMDB Trailer request;
- TMDB credentials remain daemon/server-side;
- the browser never calls `api.themoviedb.org` directly;
- only validated YouTube Trailer descriptors are exposed to the frontend;
- `youtube-nocookie.com` is used for the embed;
- `autoplay=0` is explicit and YouTube is not attached to the Recording MediaSession owner;
- closing the inline Trailer removes the iframe and stops its media;
- asynchronous rerendering of related Recording cards repositions an open Trailer directly below that rail instead of losing the intended layout;
- Movie and Series identities are supported; episode-specific Trailer identity is not guessed from an unrelated ID.

## Recording poster selection follow-up

The related-Genre rail initially could select a generic preferred artwork/landscape frame even when richer portrait Recording metadata existed.

PR #288 adds a canonical `kind=poster` Recording metadata image selection rule:

1. a locked manual poster remains authoritative when present;
2. otherwise an available native portrait metadata image is preferred;
3. the previous `preferredArtwork` remains a fallback;
4. the public response still serves through the existing authenticated Recording metadata image route and does not expose local artwork paths.

Genre Recording cards request `kind=poster&index=0`, which keeps portrait-card presentation aligned with the canonical Recording metadata/artwork model.

## Real yaVDR/browser acceptance

The PR #287 Recording-detail candidate was manually exercised on the real yaVDR/browser path. Accepted observations included:

```text
RECORDING_HERO_DETAIL=PASS
MOBILE_HERO_PRESENTATION=PASS
PROMINENT_PLAYBACK_ACTION=PASS
PLAYBACK_PREWARM_STARTUP=PASS
AUTOPLAY_FROM_PREWARM=absent
DUPLICATE_MEDIASESSION=absent
```

The PR #288 follow-up was also validated on the real yaVDR/browser path:

```text
RELATED_GENRE_PORTRAIT_COVERS=PASS
NATIVE_PORTRAIT_WINS_OVER_LANDSCAPE_PREFERRED=PASS
```

The PR #294-#298 hardening chain was additionally exercised on the real yaVDR/browser/mobile path. Accepted observations include:

```text
RECORDING_ACTIONS_FROM_HERO=PASS
TECHNICAL_PLAYBACK_AND_MARKS_SURFACE=PASS
RECORDING_CACHE_METADATA_FALLBACK=PASS
CLICKABLE_CAST_LOCAL_SEARCH=PASS
TRANSPARENT_CAST_AND_RELATED_CARDS=PASS
METADATA_TABS_BOTTOM_NAVIGATION=PASS
MOBILE_METADATA_TABS_2X2=PASS
PREWARM_BUFFERS_WITHOUT_PLAYBACK=PASS
YOUTUBE_TRAILER_LOOKUP=PASS
YOUTUBE_TRAILER_PLAYBACK=PASS
INLINE_TRAILER_BELOW_RELATED_FILMS=PASS
```

## Ownership and performance boundaries retained

The accepted implementation preserves the existing architecture:

- Recording identity and detail navigation remain Recordings 2 owned;
- VDR remains canonical for native Recording and marks/cutting state;
- persisted/manual/TVScraper metadata remains behind the existing Suite metadata model;
- TMDB remains an external provider behind the Suite metadata adapter rather than a browser-side authority;
- browser Trailer requests go through the canonical Web Client API owner;
- playback remains Phase-65 MediaSession / canonical client-owner based;
- Hero prewarm does not create progress/history truth until actual playback;
- YouTube Trailer playback is separate from the Recording playback owner and does not create/replace a Recording MediaSession;
- related Recording cards reuse canonical Recording routes rather than introducing a second content graph;
- the post-Phase-66 Home no-global-metadata-fan-out boundary remains intact;
- Phase 67 Teletext/HbbTV work remains separately gated and is not implemented by this Recording-detail hardening.

## Acceptance conclusion

The current Recordings 2 detail/presentation state represented by PR #287/#288 and the accepted hardening chain PR #294-#298 is merged and real-system accepted. The latest accepted runtime checkpoint recorded here is PR #298 merge `2b0d0990974244eac90e9be711843678c77e301d`.

These changes are non-numbered post-Phase-66 product hardening. They do not reopen Phase 66 and do not replace the separately governed Recording playback, native editing or Phase-67 companion-service architecture.

Later Recording-detail work requires its own accepted evidence and must not be inferred from this closeout.

## Related documents

- [Current State](../CURRENT.md)
- [Current Project Status](current-status.md)
- [Current Architecture State](current-architecture-state.md)
- [Post-Phase-66 Home Rebuild Closeout](post-phase66-home-rebuild-closeout.md)
- [Post-Phase-66 Native Recording Editing Closeout](post-phase66-recording-editing-closeout.md)
- [Phase 66 Closeout](phase-66-closeout.md)
- [Phase 65 Closeout](phase-65-closeout.md)
- [ADR-0058 Media Home](../adr/ADR-0058-media-home-responsive-browse-preview.md)
- [ADR-0059 VDR-Native Recording Editing](../adr/ADR-0059-vdr-native-recording-editing-marks-cutting-authority.md)
