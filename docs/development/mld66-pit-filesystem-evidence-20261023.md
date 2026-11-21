# MLD 6.6 — Pit's VDR directory and recording-layout evidence (2026-10-23)

**Status:** MAINTAINER-REPORTED, sourced from Pit's VDR-Portal reply on
2026-10-23. No remote access to the MLD host, no screenshot/attachment bytes,
and no direct inspection of Pit's MLD BitBake recipes.

## Confirmed by Pit's report

- VDR configuration directory: **`/etc/vdr/`**.
- **`channels.conf` is a symlink to a user-selected channels list**. The
  literal target path and individual list selection are not supplied.
- Recording/video directory: **`/data/tv`**, configured in MLD as a
  user-selectable share/mount. Depending on a user's configuration, it may
  point to a different partition, drive or a NAS; there is no invariant
  underlying block device or remote share path.
- A sample recording directory is:
  `/data/tv/Notruf_Hafenkante/Überrollt/2026-10-22.19.24.2-0.rec`.
- Pit's `ls -l` of that recording shows `00001.ts`, `index`, `info`,
  `marks`, `markad.vps`, `tvscraper.json`, `poster.jpg`,
  `fanart.jpg`, and `ZDF_HD-A16_9-P0.pgm`. This is an example,
  **not** evidence every recording has every such file.
- TVScraper metadata/artwork can therefore be placed **inside an individual
  `.rec` directory**, not only under a global plugin cache.
- Pit mentions a screenshot/attachment listing plugin configuration, cache
  and resource directory paths, naming `epgsearch` and `tvscraper` as
  examples. **The attachment is not available here**: exact plugin config,
  cache and resource roots are still **OPEN**, not inferred as `/etc/vdr`
  or any Ubuntu path.

## Architectural implications

1. **VDR is authoritative for its active video directory.** VDR 2.8.3
   provides `cVideoDirectory::Name()` to code running inside a VDR plugin;
   the independent Suite daemon cannot call this directly. SuiteBridge
   should eventually expose a **private, read-only, versioned discovery
   field** backed by VDR's current value. Absence/unsupported discovery
   must not silently enable recording mutations against guessed paths.
2. **Do not prescribe `/data/tv` to every MLD installation.** It is the
   observed mountpoint on Pit's instance, and MLD permits users to select
   other storage. Explicit per-backend video roots remain a safe interim
   packaging/operator override, not the preferred long-term detection
   mechanism. `/srv/vdr/video` remains the Ubuntu/yaVDR default without
   new configuration changes.
3. A recording move, rename, copy, trash, export or cut must preserve
   **recording sidecars** according to VDR's semantics. When possible
   perform recording mutations via native VDR/plugin APIs rather than
   composing physical paths in the daemon. Mount boundaries, permissions,
   symlinks and cross-filesystem operations need hardware acceptance; a
   successful `ls` does not establish write/rename capability.
4. The optional Backend Agent `channels-conf` reader previously rejected
   symlinked `channels.conf` paths because it used `lstat()` and
   `S_ISREG`. The MLD branch now follows a link with `stat()` and still
   requires a regular file with bounded size/content. Regression tests
   verify a selected-list link and a dangling link. The ordinary
   VDR/RESTfulAPI channel reader is not subject to this direct-file path.
5. Treat `tvscraper.json`, images and metadata **inside the recording**
   separately from central EPG artwork root allowlists. A configurable
   `VDR_SUITE_EPG_ARTWORK_ROOTS` does not by itself solve embedded
   recording-sidecar discovery. Never broaden a generic file-serving
   allowlist to all of `/data/tv` just to make artwork work.
6. Pit's previous **`/recordings.json` timeout** is not resolved merely
   by selecting `/data/tv` or changing `channels.conf` handling.

## Integration guardrails

- Ubuntu/yaVDR remains zero-config; new variables are opt-in and must
  preserve existing defaults when unset.
- Do not alter Pit's mounted storage, `/etc/vdr/`, VDR unit, symlink,
  file ownership or permissions during source development.
- Do not assume `root` for the standalone Suite Agent or Daemon simply
  because Pit's VDR process runs as root.
- Pending: screenshot's actual plugin directory values; authentic VDR
  video-directory discovery contract; bounded native MLD build and
  runtime/recording acceptance; package SRCREV update when agreed.
