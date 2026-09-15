# Post-Phase-66 Home Rebuild Closeout

Status: **COMPLETED / REAL-SYSTEM ACCEPTED / MERGED TO `main`.**

This document closes the non-numbered Home rebuild and the directly related correctness/performance follow-ups that occurred after the numbered Phase-66 closeout. It does not reopen Phase 66 and does not start Phase 67.

## Repository checkpoint

```text
base_main_before_home_rebuild=67c1be4ab2ab0c76b8aa5913849e2c1514830c92
accepted_home_rebuild_head=0cce4d1c9e58abe4d529132e92340ae4cbb7a99c
accepted_home_rebuild_tree=37d49f4a8abce0fefb56b18ac5f6448c63b1b437
main_merge=ea5967b983aee9ccc3f855b685db01abbfb2326a
main_merge_tree=37d49f4a8abce0fefb56b18ac5f6448c63b1b437
```

The merge tree is identical to the accepted branch tree.

## Why this closeout exists

The original Phase-66 closeout correctly captured the accepted numbered Home/Browse phase at that time. Significant bounded work followed: performance hardening, Recording Discovery optimization, Series metadata/artwork completion, artwork-preview caching, EPG recovery, native Recording editing and finally a broader Home rebuild. Those changes remained inside established ownership contracts and did not justify inventing a new numbered phase.

This closeout provides one durable post-phase truth point before Phase 67 begins.

## Home rebuild blocks

The working Home-rebuild acceptance was organized into internal H-blocks. These are **not roadmap phases or Phase-66 slices**.

### H0 — Baseline and ownership

- preserve existing Channel, EPG, Recording, Metadata, Artwork, Genre and playback authorities;
- preserve the installed `overview`/Home navigation owner;
- avoid a second Home-specific database, media lifecycle or metadata owner;
- protect the already accepted Recording metadata performance boundary.

### H1 — Home shell

- retain the responsive first-party Home composition root;
- keep desktop/mobile recomposition and canonical navigation ownership;
- preserve warm lifecycle behavior and stable mounted projections where valid.

### H2 / H2.1 — Now / Next and metadata consistency

- compact Home EPG critical path;
- `Was läuft jetzt` / `Was läuft danach` projection with bounded optional artwork enrichment;
- avoid per-event Metadata/Artwork fan-out;
- recover EPG cache work after transient backend timeout without widening ordinary request timeouts.

### H3 — Newly recorded

- retain progressive Recording discovery under canonical Recording identity;
- keep folder/detail handoff and existing playback owner boundaries.

### H4 — Movies / Genres

- preserve recent-movie full canonical Recording semantics and in-place rail expansion;
- bound random-Genre Home work to visible cards;
- project persisted canonical metadata/artwork rather than weak Recording-path/still data when richer native metadata exists;
- retain Genre/EPG presentation without creating a second provider path.

### H5 — Series

- Series -> seasons -> episodes hierarchy;
- progressive/scoped metadata completion rather than global enrichment;
- canonical TVScraper/native portrait artwork;
- persistent Series cover overrides;
- manual hierarchy overrides;
- pilot/miniseries handling;
- TV-film/special handling;
- reset-to-automatic hierarchy semantics;
- backend/generation/Home-active fencing;
- preservation of complete existing hierarchy without unnecessary loading state.

Real acceptance included the intended multi-season hierarchy behavior for the existing Series inventory, including the previously problematic Battlestar Galactica season structure.

## Performance contract retained

A central acceptance rule of the rebuild is that the old global Recording metadata fan-out must not return.

The completed design therefore keeps:

- representative/scoped metadata work during Home Series discovery;
- richer metadata completion concentrated on the Series/detail being opened when necessary;
- bounded concurrency and generation fencing;
- no browser per-Recording metadata reads for folder cards when persisted native metadata can be embedded in the existing folder response;
- preview/cache variants as delivery optimization only, never as new artwork authority.

## Final folder-poster regression and repair

After the broader H5 acceptance, a real regression remained visible in:

```text
Home -> Aufnahmeordner -> Horror -> The Exorcist
```

The Recording detail already showed the correct red TVScraper portrait, while the Home folder card could still show a weak Recording still. The root cause was not metadata mutation or browser cache invalidation. The folder response serialized the weaker base Recording metadata, whose preferred legacy artwork could be a `Still`, while the already-persisted native TVScraper portrait existed separately.

The final repair:

1. reuses the folder controller's existing server-side native-metadata repository lookup;
2. adds an additive `nativeMetadata` projection to non-manual folder Recording objects when persisted native metadata exists;
3. keeps locked manual metadata authoritative;
4. projects the card through the existing canonical native metadata artwork helper, where portrait gallery artwork wins before weaker fallback artwork;
5. preserves the original Recording object for detail/playback identity;
6. adds no per-Recording `/recordings/metadata` browser fan-out.

Focused backend/frontend regressions prove the embedded native projection and zero additional folder metadata reads. The real yaVDR/browser result showed `The Exorcist` with the correct red TVScraper portrait in the Horror folder. No metadata reassignment/rescrape was required.

## Accepted commit sequence

The Home-rebuild branch contained six commits on top of the prior merged baseline:

```text
9d4d88e1 Fix retained Home and scoped Series metadata completion
e457367e WIP: home rebuild H2 and H2.1 backend foundation
2d37399e Home: finalize H2/H2.1 and metadata consistency
2ce8ea66 Genres: compact EPG artwork cards
f0e8acc4 Home: complete H5 series artwork and hierarchy
0cce4d1c Home: restore canonical recording posters in folders
```

The final commit also registered the already-existing Series hierarchy override repository contract test in the SQLite architecture allowlist; it did not weaken the production SQLite boundary.

## Earlier post-Phase-66 work incorporated by this truth point

The merged main baseline before this final branch already included the post-Phase-66 chain such as Home performance hardening, Recording Discovery performance, Series metadata/artwork completion, native Recording editing, Series revalidation retention, diagnostics, artwork previews/cache, bounded Genre work, EPG cache recovery, Recent Movies rail retention and representative Series metadata scheduling.

Those individual closeouts remain valid historical evidence. This file is the current consolidated Home truth point, not a replacement for their exact accepted evidence.

## Phase boundary

Phase 66 remains completed at its original numbered closeout. The Home rebuild is non-numbered hardening/correctness work.

```text
Phase 66 - Media Home and Browse Experience [COMPLETED]
Phase 67 - Broadcast Companion Services: Teletext and HbbTV [NEXT; NOT STARTED]
```

No Teletext, HbbTV, Legacy OSD, public `/api/v1` hardening or recommendation runtime was started by this work.

## Related documents

- [Current State](../CURRENT.md)
- [Phase 66 Closeout](phase-66-closeout.md)
- [Post-Phase-66 Home Performance Hardening](post-phase-66-home-performance-hardening.md)
- [Post-Phase-66 Native Recording Editing Closeout](post-phase66-recording-editing-closeout.md)
- [Strict Roadmap](../planning/roadmap.md)
- [Phase Map](../planning/phase-map.md)
- [ADR-0058 Media Home](../adr/ADR-0058-media-home-responsive-browse-preview.md)
- [ADR-0054 Broadcast Companion Services](../adr/ADR-0054-broadcast-companion-teletext-hbbtv.md)
