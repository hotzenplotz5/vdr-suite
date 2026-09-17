# Post-Phase-66 Recording Detail Closeout

Status: **COMPLETED / REAL-SYSTEM ACCEPTED / MERGED TO `main`.**

This document records the non-numbered Recordings 2 detail/presentation work that followed the completed Phase-66 Home/Browse phase. It does not reopen Phase 66 and does not start Phase 67.

## Scope

The accepted work modernizes the existing Recording detail surface while preserving the established Recording identity, metadata, marks and playback owners.

It covers:

- the cinematic/full-page Recordings 2 detail presentation;
- compact facts and prominent playback entry;
- cast/person and related-Genre rails;
- existing metadata/marks/playback modes inside the same canonical detail owner;
- canonical-owner playback prewarm without autoplay or duplicate MediaSessions;
- the follow-up Recording poster-selection correction used by related-Genre cards.

It does **not** create a second Recording domain, a second playback owner, a new metadata authority or a parallel History/Continue-Watching path.

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

## Product result

The accepted Recording detail now provides:

- a full-page cinematic Hero presentation for the selected Recording;
- retained canonical Recording title, subtitle, description, poster/backdrop and metadata identity;
- compact factual metadata in the Hero instead of pushing all information into a separate technical panel;
- a prominent playback action that enters the existing canonical Recording playback owner;
- cast/person presentation using existing person metadata and person-search behavior;
- a related-Genre rail that opens normal Recording detail routes rather than creating a parallel detail implementation;
- metadata, marks and playback submodes that remain attached to the same Recordings 2 detail owner;
- desktop and mobile presentation behavior validated against the real installed frontend.

The implementation is modularized through the installed Recordings 2 runtime (`recordings2-hero-detail.js`, `recordings2-hero-prewarm.js` and `recordings2-hero-visibility.js`) rather than by replacing the established Recordings 2 controller or playback runtime.

## Playback prewarm boundary

The Hero may prepare the existing canonical playback owner before the user presses Play so startup can be effectively immediate.

The prewarm contract is deliberately narrow:

```text
Recording detail Hero visible
  -> prepare canonical Recording playback owner/session path
  -> no autoplay
  -> no second playback owner
  -> no duplicate MediaSession
  -> no Continue Watching/history truth from prewarm alone
  -> explicit user Play enters normal playback lifecycle
```

The Continue Watching synchronization explicitly excludes the `starting` state, so preparing the playback path does not falsely publish viewing progress before real playback begins.

## Recording poster selection follow-up

The related-Genre rail initially could select a generic preferred artwork/landscape frame even when richer portrait Recording metadata existed.

PR #288 adds a canonical `kind=poster` Recording metadata image selection rule:

1. a locked manual poster remains authoritative when present;
2. otherwise an available native portrait metadata image is preferred;
3. the previous `preferredArtwork` remains a fallback;
4. the public response still serves through the existing authenticated Recording metadata image route and does not expose local artwork paths.

Genre Recording cards now request `kind=poster&index=0`, which keeps portrait-card presentation aligned with the canonical Recording metadata/artwork model.

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

## Ownership and performance boundaries retained

The accepted implementation preserves the existing architecture:

- Recording identity and detail navigation remain Recordings 2 owned;
- VDR remains canonical for native Recording and marks/cutting state;
- persisted/manual/TVScraper metadata remains behind the existing Suite metadata model;
- playback remains Phase-65 MediaSession / canonical client-owner based;
- Hero prewarm does not create progress/history truth until actual playback;
- related Recording cards reuse canonical Recording routes rather than introducing a second content graph;
- the post-Phase-66 Home no-global-metadata-fan-out boundary remains intact;
- Phase 67 Teletext/HbbTV runtime remains separately gated and is not started by this work.

## Acceptance conclusion

The current Recordings 2 detail/presentation state represented by PR #287 plus the portrait-selection correction in PR #288 is accepted and merged. These changes are non-numbered post-Phase-66 product hardening and do not reopen Phase 66.

Later Recording editing/marks hardening, if any, requires its own accepted evidence and must not be inferred from this closeout.

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
